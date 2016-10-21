#include "chkdsk.h"
#include "fat.h"
#include "vol.h"
#include "dir.h"
#include "file.h"
#include "name.h"

#if defined( CHKDISK ) && defined( LOG )
#error "Must define only one!!"
#endif

/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES & DEFINITIONS
-----------------------------------------------------------------------------*/

typedef struct
  {
  uint32_t sig;

  } chkdata_t;

typedef struct
  {
  int32_t vol_id;
  uint32_t *chk_fat_map;
  fat_fileent_t *fe;

  } chkdisk_vol_t;

typedef struct chk_entattr_t
  {
  uint32_t own_clust,
  parent_sect,
  del_parent_sect;

  union
    {
    uint32_t flag,
    filesize;
      } u;
  uint8_t parent_ent_idx,
  del_parent_ent_idx,
  attr;

  } chk_entattr_t;

typedef struct chk_file_s
  {
  uint32_t parent_sect; /* sector number of own entry which is located in parent cluster */
  uint8_t parent_ent_idx; /* entry offset in 'parent_sect'. */

  } chk_file_t;

typedef struct chk_volinfo_s
  {
  bool inited;
  uint32_t vol_id;
  chk_file_t chk_file;

  } chk_volinfo_t;

typedef struct
  {
  uint32_t *fatmap;
  bool is_used;

  } chk_bitmap_cb_t;




#if defined( CHKDISK )
#if defined( DBG )
#if !defined( WIN32 )
#pragma arm section zidata="NoInitData",rwdata="NoInitData"
#endif
chk_dbg_t chk_dbg;
#if !defined( WIN32 )
#pragma arm section zidata,rwdata
#endif
#endif

static chk_volinfo_t chk_vol[VOLUME_NUM];

/* FAT bitmap pool. */
static uint32_t chk_bitmap[FAT_HEAP_SIZE / sizeof (uint32_t) + VOLUME_NUM];
/* Volume's bitmap pointer. */
static chk_bitmap_cb_t chk_vol_bitmap[VOLUME_NUM];




/* external function */
extern int32_t fat_lookup_short(fat_volinfo_t *fvi, fat_fileent_t *fe);




/*-----------------------------------------------------------------------------
 DEFINE FUNCTIONS
-----------------------------------------------------------------------------*/

/*
 Name: chk_init_fatmap_pool
 Desc: Initialize chk-fatmap pool.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void chk_init_fatmap_pool(void)
  {
  int32_t i;


  for (i = 0; i < VOLUME_NUM; i++)
    {
    chk_vol_bitmap[i].fatmap = chk_bitmap;
    chk_vol_bitmap[i].is_used = false;
    }
  }

/*
 Name: __chk_get_fatmap
 Desc: Get buffer of volume's fatmap.
 Params:
   - vol_id: Volume number.
   - uint32_size: Size to be allocated (NOTE:1 is 4bytes).
 Returns:
   uint32_t *  Pointer of allocated fatmap.
               NULL  on fail.
 Caveats:
 */

static uint32_t *__chk_get_fatmap(int32_t vol_id, uint32_t uint32_size)
  {
  uint32_t *fatmap_end = &chk_bitmap[sizeof (chk_bitmap) / sizeof (uint32_t) - 1];
  uint32_t *fatmap;
  int32_t i;


  fatmap = chk_vol_bitmap[vol_id].fatmap;
  chk_vol_bitmap[vol_id].is_used = true;

  if ((fatmap_end <= fatmap) || (fatmap_end < (fatmap + uint32_size)))
    return NULL;

  for (i = 0; i < VOLUME_NUM; i++)
    {
    if (false == chk_vol_bitmap[i].is_used)
      chk_vol_bitmap[i].fatmap = fatmap + uint32_size;
    }

  return fatmap;
  }

/*
 Name: __fat_chk_lookup
 Desc: Test a log file existence.
 Params:
   - fe: File-entry of Check-file
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: If there is no log file when fist format of file system or file system is formatted by external memory,
          it needs to test for log file existence.
 */

static int32_t __fat_chk_lookup(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
  int32_t rtn;
  bool tmp, tmp2;


  fvi = GET_FAT_VOL(fe->vol_id);

  /* Root-directory access is different with FAT16 and FAT32. */
  if (eFAT16_SIZE == fvi->br.efat_type)
    SET_OWN_CLUST(&fe->dir, FAT_ROOT_CLUST);
  else /* eFAT32_SIZE : first_data_sect is root-directory */
    SET_OWN_CLUST(&fe->dir, D_SECT_2_CLUST(fvi, fvi->br.first_data_sect));

  /* Chk-file has always been created into root-dir. */
  fe->parent_clust = GET_OWN_CLUST(&fe->dir);

  /* Create the name of chk-file on File-entry. */
  rtn = fat_cp_name(fe, (FAT_CHK_FILE), eFAT_FILE);
  if (0 > rtn) return rtn;

  /* Create a short-name on base of File-entry. */
  rtn = fat_make_shortname(fe, &tmp, &tmp2);
  if (0 > rtn) return rtn;

  /* Lookup if there is Check-file. */
  if ((uint32_t) eFILE_LONGENTRY & fe->flag)
    {
    rtn = fat_lookup_long(fvi, fe);
    }
  else
    {
    rtn = fat_lookup_short(fvi, fe);
    }

  return rtn;
  }

/*
 Name: __fat_chk_creat
 Desc: Create a check file
 Params:
   - fe: File-entry for check-file
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_chk_creat(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
  uint32_t clust_list[1/*last cluster*/ + CLUST_PER_CHK], /* sector less equal cluster */
    filesize,
    clust_cnt;
  int32_t rtn;


  fvi = GET_FAT_VOL(fe->vol_id);

  /* Calculate need clusters */
  filesize = CLUST_PER_CHK * (1 << fvi->br.bits_per_clust);
  clust_cnt = CLUST_PER_CHK;

  /* Allocate clusters. */
  rtn = fat_map_alloc_clusts(fe->vol_id, clust_cnt, &clust_list[1]);
  if (0 > rtn) return -1;

  /* log-file has system-file attribute. */
  fe->dir.attr = eFAT_ATTR_HIDDEN | eFAT_ATTR_SYS | eFAT_ATTR_RO;
  SET_OWN_CLUST(&fe->dir, clust_list[1]);
  fe->dir.filesize = filesize;

  rtn = fat_alloc_entry_pos(fe);
  if (0 > rtn) return -1;

  /* Create entry. */
  rtn = fat_creat_entry(fe, true);
  if (0 > rtn) return -1;

  /* Real allocate */
  clust_list[0] = 0; /*last cluster*/
  rtn = fat_stamp_clusts(fe->vol_id, clust_cnt, clust_list);
  if (0 > rtn) return rtn;

  rtn = fat_sync_table(fe->vol_id, true);
  return rtn;
  }

/*
 Name: __fat_chk_read_sig
 Desc: Obtain the signal from the File-Entry for the check-file.
 Params:
   - vol_id: The ID of volume with File-Entry.
   - chk_fe: The pointer to the FIle-Entry.
   - sig: The pointer to the signal buffer for check-file.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_chk_read_sig(int32_t vol_id, fat_fileent_t *chk_fe, uint32_t *sig)
  {
  fat_volinfo_t *fvi;
  int32_t buf32[FAT_ALLOW_MAX_SECT_SIZE / sizeof (int32_t)];
  int32_t clust_no,
    sect_no,
    rtn;


  fvi = GET_FAT_VOL(vol_id);

  clust_no = GET_OWN_CLUST(&chk_fe->dir);
  sect_no = D_CLUST_2_SECT(fvi, clust_no);

  rtn = lim_read_sector(vol_id, sect_no, buf32, 1);

  *sig = buf32[0];

  return rtn;
  }

/*
 Name: __fat_chk_write_sig
 Desc: Write the signal to the File-Entry for the check-file .
 Params:
   - vol_id: The ID of volume with File-Entry.
   - chk_fe: The pointer to the FIle-Entry.
   - sig: The pointer to the signal buffer for check-file.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_chk_write_sig(int32_t vol_id, fat_fileent_t *chk_fe, uint32_t sig)
  {
  fat_volinfo_t *fvi;
  uint32_t buf32[FAT_ALLOW_MAX_SECT_SIZE / sizeof (uint32_t)];
  int32_t clust_no,
    sect_no,
    rtn;


  fvi = GET_FAT_VOL(vol_id);

  clust_no = GET_OWN_CLUST(&chk_fe->dir);
  sect_no = D_CLUST_2_SECT(fvi, clust_no);

  buf32[0] = sig;

  rtn = lim_write_sector(vol_id, sect_no, buf32, 1);
  return rtn;
  }

/*
 Name: fat_is_chk_file
 Desc: Test the given file is the check file.
 Params:
   - fe: Ponter to the fat_fileent_t structure.
   - cmp_name: Resolve type of test.
               true: Test a file name.
               false: Test a position of entry.
 Returns:
   bool  true on success.
           false on fail.
 Caveats: None.
 */

bool fat_is_chk_file(fat_fileent_t *fe, bool cmp_name)
  {
  chk_volinfo_t *chk = &chk_vol[fe->vol_id];


  if (cmp_name)
    {
    if (!strcmp((FAT_CHK_FILE), fe->name))
      return true;
    else
      return false;
    }
  else
    {
    if ((chk->chk_file.parent_sect == fe->parent_sect) &&
        (chk->chk_file.parent_ent_idx == fe->parent_ent_idx))
      return true;
    else
      return false;
    }
  }

/*
 Name: __init_chk_info
 Desc: Initialize information to use for Log control.
 Params:
   - vol_id: ID of volume
   - log_fe: File entry of log-file.
 Returns:
   log_volinfo_t*  value. This value is a Log information pointer which is in a pertinent volume.
 Caveats: None.
 */

static chk_volinfo_t * __init_chk_info(int32_t vol_id, fat_fileent_t *chk_fe)
  {
  chk_volinfo_t *chk = &chk_vol[vol_id];


  chk->vol_id = vol_id;

  /* Remember position of check file. */
  chk->chk_file.parent_sect = chk_fe->parent_sect;
  chk->chk_file.parent_ent_idx = chk_fe->parent_ent_idx;

  chk->inited = true;

  return chk;
  }

/*
 Name: fat_chk_init
 Desc: Initialize volume's check manager.
 Params:
   - vol_id: Volume of ID
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_chk_init(int32_t vol_id)
  {
  fat_fileent_t *fe;
  uint32_t sig;
  bool need_chkdisk = false;
  int32_t rtn;


  chk_init_fatmap_pool();

  fe = fat_alloc_file_entry(vol_id);
  if (NULL == fe) return -1;

  /* Lookup if there is check-file. */
  rtn = __fat_chk_lookup(fe);
  if (0 > rtn)
    {
    if (e_noent == get_errno())
      {
      /* If not found, create log-file. */
      rtn = __fat_chk_creat(fe);
      if (0 > rtn) goto End;
      need_chkdisk = true;
      }
    else
      goto End;
    }
  else
    {
    rtn = __fat_chk_read_sig(vol_id, fe, &sig);
    if (0 > rtn) goto End;

    if (FAT_CHK_UMNT_SIG != sig)
      need_chkdisk = true;
    }

  __init_chk_info(vol_id, fe);

  if (need_chkdisk)
    {
    rtn = fat_chkdisk_recover(vol_id);
    if (0 > rtn) goto End;
    }

  rtn = __fat_chk_write_sig(vol_id, fe, FAT_CHK_BOOT_SIG);

End:
  fat_free_file_entry(fe);

  return rtn;
  }

/*
 Name: fat_chk_deinit
 Desc: Deinitialize volume's check manager.
 Params:
   - vol_id: Volume of ID
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_chk_deinit(int32_t vol_id)
  {
  fat_fileent_t *fe;
  int32_t rtn;


  fe = fat_alloc_file_entry(vol_id);
  if (NULL == fe) return -1;

  /* Lookup if there is check-file. */
  rtn = __fat_chk_lookup(fe);
  if (0 > rtn)
    {
    if (e_noent == get_errno())
      {
      /* If not found, create log-file. */
      rtn = __fat_chk_creat(fe);
      if (0 > rtn) goto End;
      }
    else
      goto End;
    }

  rtn = __fat_chk_write_sig(vol_id, fe, FAT_CHK_UMNT_SIG);

End:
  fat_free_file_entry(fe);

  return rtn;
  }

/*
 Name: fat_chkdsk_dir
 Desc: Do nothing.
 Params:
   - vol_id: The ID of the volume.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: This function isn't supported in current version.
 */

int32_t fat_chkdsk_dir(int32_t vol_id)
  {
  return s_ok;
  }

/*
 Name: fat_chk_unlink_entry
 Desc:
 Params:
 Returns:
 Caveats:
 */

int32_t fat_chk_unlink_entry(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
  fat_fileent_t *pfe;
  lim_cacheent_t *ce;
  fat_dirent_t *de, *end_de;
  list_head_t list;
  int32_t sect_no,
    ent_idx,
    i,
    rtn;
  bool is_dirty;


  if (eFAT_EOF == fe->parent_sect)
    return e_eos;

  list_init(&list);

  pfe = (fat_fileent_t *) fe;
  fvi = GET_FAT_VOL(pfe->vol_id);
  sect_no = fe->parent_sect;
  ent_idx = fe->parent_ent_idx;

  for (i = 0; i < FAT_LONGNAME_SLOTS; i++)
    {
    ce = lim_get_csector(fvi->vol_id, sect_no);
    if (NULL == ce) return get_errno();
    de = (fat_dirent_t*) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
    end_de = de + fvi->br.ents_per_sect - 1/*start index 0*/;
    is_dirty = false;

    for (de += ent_idx; de <= end_de; de++)
      {

      if (IS_EMPTY_ENTRY(de))
        goto End;

      if (!IS_DELETED_ENTRY(de))
        {
        DELETE_ENTRY(de);
        is_dirty = true;
        }

      /* If short, done. */
      if (eFAT_ATTR_LONG_NAME != de->attr)
        goto End;
      }

    if (is_dirty)
      /* Mark as Dirty status to update data. */
      lim_mark_dirty_csector(ce, &list);
    lim_rel_csector(ce);

    ent_idx = 0;
    sect_no = fat_get_next_sectno_16root(fvi, pfe, sect_no);
    if (eFAT_EOF == sect_no)
      break;
    else if (0 > sect_no)
      return -1;
    }

End:
  if (is_dirty)
    lim_mark_dirty_csector(ce, &list);

  lim_rel_csector(ce);

  if (list.next != &list)
    /* If more than one sector were dirtied, we flush list of dirty sector. */
    rtn = lim_flush_csectors(pfe->vol_id, &list);
  else
    rtn = e_noent;

  return rtn;
  }

#endif

/*
 Name: __fat_calc_fat32_rootdir
 Desc: Calculate the size of the Root-Directory.
 Params:
   - vol_id: The ID of volume which includes the Root-Directory.
 Returns:
   int32_t  >=0 on success. The returned value is the size of Root-Directory.
            < 0 on fail.
 Caveats: None.
 */

static int32_t __fat_calc_fat32_rootdir(int32_t vol_id)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  int32_t root_clust_cnt;


  /* Calculate the number of clusters of the Root-Directory. */
  root_clust_cnt = fat_get_clust_cnt(vol_id, D_SECT_2_CLUST(fvi, fvi->br.first_root_sect), NULL, NULL);
  if (0 > root_clust_cnt)
    return root_clust_cnt;

  /* Returns the number of bytes. 'fvi->br.bits_per_clust' means the bit number
     which operates Shift to change the cluster number to byte unit */
  return root_clust_cnt << fvi->br.bits_per_clust;
  }

/*
 Name: fat_get_fs_dir_size
 Desc: Check whether the FAT table is corrupted or not.
 Params:
   - vol_id: The ID of volume which is checked.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None
 */

int32_t fat_get_fs_dir_size(int32_t vol_id, uint32_t *fs_size_ptr, uint32_t *dir_size_ptr)
  {
#define FAT_RSVD_CLUSTERS 2
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  statfs_t statfs;
  statdir_t statdir;
  int32_t fs_size,
    dir_size,
    root_dir_size,
    rtn;


  /* Get information of the volume */
  rtn = fat_statfs(vol_id, &statfs);
  if (0 > rtn) return rtn;

  /* Get information of the Root-Directory */
  rtn = fat_statdir(vol_id, ("/"), &statdir);
  if (0 > rtn) return rtn;

  /* The size of all free blocks in the volume. */
  fs_size = statfs.free_blks * statfs.blk_size;
  /* The size which subtracts the allocated size from the Root-Directory. */
  dir_size = ((statfs.blocks - FAT_RSVD_CLUSTERS) * statfs.blk_size) - statdir.alloc_size;

  if (eFAT32_SIZE == fvi->br.efat_type)
    {
    root_dir_size = __fat_calc_fat32_rootdir(vol_id);
    if (0 > root_dir_size)
      return root_dir_size;
    /* Subtracts the size of Root-Directory itself. */
    dir_size -= root_dir_size;
    }

  *fs_size_ptr = fs_size;
  *dir_size_ptr = dir_size;

  if (fs_size == dir_size)
    return s_ok;
  else
    return set_errno(e_cfat);
  }

/*
 Name: fat_chk_fattable
 Desc: Check whether the FAT table is corrupted or not.
 Params:
   - vol_id: The ID of volume which is checked.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None
 */

int32_t fat_chk_fattable(int32_t vol_id)
  {
  uint32_t fs_size;
  uint32_t dir_size;


  return fat_get_fs_dir_size(vol_id, &fs_size, &dir_size);
  }




#if defined( CHKDISK )

/*
 Name: __fat_fatmap_recover
 Desc: Recover FAT-Bit map.
 Params:
   - chk_vol: The pointer to the chkdisk_vol_t structure.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_fatmap_recover(chkdisk_vol_t *chk_vol)
  {
  int32_t vol_id = chk_vol->vol_id;
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce;
  uint32_t clust_cnt_in_fat,
    calced_clsut_cnt,
    fat_cnt_in_fatsect,
    clust_no,
    fat_sectno,
    diff_bits,
    *fatmap,
    *chkmap,
    chk_bit,
    fat_offs,
    free_cnt = 0,
    i, j, k;
  int32_t rtn;


  fatmap = fvi->br.fat_map;
  chkmap = chk_vol->chk_fat_map;
  clust_cnt_in_fat = fvi->br.last_data_clust + 1;
  calced_clsut_cnt = 0;

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    uint16_t *buf16;

    fat_cnt_in_fatsect = fvi->br.bytes_per_sect / eFAT16_SIZE;

    for (i = 0; i < clust_cnt_in_fat; i += fat_cnt_in_fatsect)
      {
      clust_no = i;
      fat_sectno = F16T_CLUST_2_SECT(fvi, clust_no);
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf16 = (uint16_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      for (j = 0; j < fat_cnt_in_fatsect; j += BITS_PER_UINT32)
        {
        diff_bits = *fatmap++ ^ *chkmap++;
        if (diff_bits)
          {
          chk_bit = (uint32_t) (1 << 0);
          for (k = 0; k < BITS_PER_UINT32; k++)
            {
            if (chk_bit & diff_bits)
              {
              clust_no = i + j + k;
              fat_offs = F16T_CLUST_2_OFFS_INSECT(fvi, clust_no);
              buf16[fat_offs] = eFAT_FREE;
              free_cnt++;

              /* Mark the dirty list with the changed cache-entry. */
              fat_mark_dirty_csector(ce);
              }

            if (++calced_clsut_cnt == clust_cnt_in_fat)
              {
              /* Set break condition of the outer loops */
              i = clust_cnt_in_fat;
              j = fat_cnt_in_fatsect;
              break;
              }

            chk_bit <<= 1;
            }
          }
        }

      fat_rel_csector(ce);
      }
    }
  else
    { /*eFAT32_SIZE*/
    uint32_t *buf32;

    fat_cnt_in_fatsect = fvi->br.bytes_per_sect / eFAT32_SIZE;

    for (i = 0; i < clust_cnt_in_fat; i += fat_cnt_in_fatsect)
      {
      clust_no = i;
      fat_sectno = F32T_CLUST_2_SECT(fvi, clust_no);
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf32 = (uint32_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      for (j = 0; j < fat_cnt_in_fatsect; j += BITS_PER_UINT32)
        {
        diff_bits = *fatmap++ ^ *chkmap++;
        if (diff_bits)
          {
          chk_bit = (uint32_t) (1 << 0);
          for (k = 0; k < BITS_PER_UINT32; k++)
            {
            if (chk_bit & diff_bits)
              {
              clust_no = i + j + k;
              fat_offs = F32T_CLUST_2_OFFS_INSECT(fvi, clust_no);
              buf32[fat_offs] = eFAT_FREE;
              free_cnt++;

              /* Mark the dirty list with the changed cache-entry. */
              fat_mark_dirty_csector(ce);
              }

            if (++calced_clsut_cnt == clust_cnt_in_fat)
              {
              /* Set break condition of the outer loops */
              i = clust_cnt_in_fat;
              j = fat_cnt_in_fatsect;
              break;
              }

            chk_bit <<= 1;
            }
          }
        }

      fat_rel_csector(ce);
      }
    }


  fvi->br.free_clust_cnt += free_cnt;
  rtn = fat_sync_table(vol_id, true);

#if defined( DBG )
  if (free_cnt)
    chk_dbg.total_recovery_cnt++;
  chk_dbg.total_free_clust_cnt += free_cnt;
#endif

  return rtn;
  }

/*
 Name: __fat_set_entry_fatmap
 Desc:
 Params:
   - chk_vol: The ID of volume which is checked.
   - first_clust_no:
   - total_clust_cnt:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static uint32_t __fat_set_entry_fatmap(chkdisk_vol_t *chk_vol,
                                       uint32_t first_clust_no, uint32_t total_clust_cnt)
  {
#define CHK_LOCAL_CLUST_CNT 512
  int32_t vol_id = chk_vol->vol_id;
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  uint32_t clust_no,
    got_cnt,
    srch_cnt,
    real_clust_cnt = 0,
    last_clust_no = 0,
    grp32,
    offs32,
    *map,
    i;
  uint32_t clust_list[CHK_LOCAL_CLUST_CNT + 1];


  map = chk_vol->chk_fat_map;
  clust_no = first_clust_no;

  /* if it has own cluster, then delete */
  if (clust_no && (eFAT_FREE != fat_get_next_clustno(vol_id, clust_no)))
    {
    grp32 = clust_no / BITS_PER_UINT32;
    offs32 = clust_no % BITS_PER_UINT32;
    if (1 == bit_get(*(map + grp32), offs32))
      return 0xFFFFFFFF;

    do
      {
      /* Get the cluster's list to be removed. */
      got_cnt = fat_get_clust_list(vol_id, clust_no, CHK_LOCAL_CLUST_CNT, clust_list);
      if (0 >= got_cnt)
        break;

      if (CHK_LOCAL_CLUST_CNT > got_cnt)
        srch_cnt = got_cnt;
      else
        srch_cnt = got_cnt - 1/*next free cluster*/;

      for (i = 0; i < srch_cnt; i++)
        {
        grp32 = clust_list[i] / BITS_PER_UINT32;
        offs32 = clust_list[i] % BITS_PER_UINT32;

        if (++real_clust_cnt <= total_clust_cnt)
          {
          /* Mark used. */
          bit_set((map + grp32), offs32);
          last_clust_no = clust_list[i];
          }
        else
          {
          fsm_assert2(0 != last_clust_no);
          /* Truncate clusters, so the last cluster is set with eFAT_EOC. */
          fat_set_next_clustno(vol_id, last_clust_no, (uint32_t) eFAT_EOC);
          return real_clust_cnt;
          }
        }

      clust_no = clust_list[srch_cnt];
      }
    while (CHK_LOCAL_CLUST_CNT == got_cnt);

    last_clust_no = clust_list[got_cnt - 1];
    clust_no = fat_get_next_clustno(vol_id, last_clust_no);

    if (!((eFAT16_SIZE == fvi->br.efat_type && IS_FAT16_EOC(clust_no)) ||
          eFAT32_SIZE == fvi->br.efat_type && IS_FAT32_EOC(clust_no)))
      {
      grp32 = last_clust_no / BITS_PER_UINT32;
      offs32 = last_clust_no % BITS_PER_UINT32;
      /* Mark used. */
      bit_clear((map + grp32), offs32);
      fat_set_next_clustno(vol_id, last_clust_no, (uint32_t) eFAT_EOC);
      }
    }

  return real_clust_cnt;
  }

/*
 Name: __fat_chk_update_sentry
 Desc:
 Params:
   - vol_id: The ID of volume which is checked.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_chk_update_sentry(fat_fileent_t *fe, chk_entattr_t *ea)
  {
  uint8_t *p_parent_ent_idx,
    ori_parent_ent_idx;
  uint32_t *p_parent_sect,
    ori_parent_sect;
  int32_t rtn;


  if ((uint32_t) eFILE_LONGENTRY & fe->flag)
    {
    p_parent_ent_idx = &fe->lfn_shortent_idx;
    p_parent_sect = &fe->lfn_short_sect;

    }
  else
    {
    p_parent_ent_idx = &fe->parent_ent_idx;
    p_parent_sect = &fe->parent_sect;
    }

  /* Backup position. */
  ori_parent_ent_idx = *p_parent_ent_idx;
  ori_parent_sect = *p_parent_sect;

  /* Setup position to be updated.*/
  *p_parent_ent_idx = ea->parent_ent_idx;
  *p_parent_sect = ea->parent_sect;

  rtn = fat_update_sentry(fe, true);

  /* Rolback position. */
  *p_parent_ent_idx = ori_parent_ent_idx;
  *p_parent_sect = ori_parent_sect;

#if defined( DBG )
  chk_dbg.update_entry_cnt++;
#endif

  return rtn;
  }

/*
 Name: __fat_chk_del_entry
 Desc:
 Params:
   - vol_id: The ID of volume which is checked.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_chk_del_entry(void *fe, uint32_t parent_sect, uint8_t parent_ent_idx)
  {
  fat_fileent_t *ofe = (fat_fileent_t *) fe;
  uint32_t ori_parent_sect;
  uint8_t ori_parent_ent_idx;
  int32_t rtn;


  /* Backup position. */
  ori_parent_ent_idx = ofe->parent_ent_idx;
  ori_parent_sect = ofe->parent_sect;

  /* Setup position to be updated.*/
  ofe->parent_ent_idx = parent_ent_idx;
  ofe->parent_sect = parent_sect;

  rtn = fat_chk_unlink_entry(fe);
  if (e_noent == rtn)
    rtn = s_ok;

  /* Rolback position. */
  ofe->parent_ent_idx = ori_parent_ent_idx;
  ofe->parent_sect = ori_parent_sect;

  return rtn;
  }

/*
 Name: __fat_chk_get_sentry
 Desc: Get position of entry that is either Short or LFN.
 Params:
   - fe: Pointer to the fat_fileent_t structure means entry.
   - cur_ea: Current entry's attribute pointer.
   - next_ea: Next entry's attribute pointer.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_chk_get_sentry(fat_fileent_t *fe, chk_entattr_t *cur_ea, chk_entattr_t *next_ea)
  {
  fat_volinfo_t *fvi;
  lim_cacheent_t *ce;
  fat_dirent_t *de, *end_de;
  int32_t sect_no,
    next_sect_no,
    parent_ent_idx,
    rtn;
  bool prev_ent_is_long = false,
    found_last_lfn = false;
  uint32_t del_parent_sect = 0;
  uint8_t del_parent_ent_idx = 0;


  if (eFAT_EOF == cur_ea->parent_sect)
    return e_eos;

  fvi = GET_FAT_VOL(fe->vol_id);
  rtn = e_eos;
  sect_no = cur_ea->parent_sect;
  parent_ent_idx = cur_ea->parent_ent_idx;

  while (1)
    {
    ce = lim_get_csector(fvi->vol_id, sect_no);
    if (NULL == ce) return get_errno();
    de = (fat_dirent_t*) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
    end_de = de + fvi->br.ents_per_sect - 1/*start index 0*/;

    for (de += parent_ent_idx; de <= end_de; de++)
      {
      if (IS_DELETED_ENTRY(de))
        {
        if (del_parent_sect)
          {
          rtn = __fat_chk_del_entry(fe, del_parent_sect, del_parent_ent_idx);
          if (0 > rtn) goto End;
          del_parent_sect = 0;
          }

        if (eFAT_ATTR_LONG_NAME == de->attr)
          prev_ent_is_long = true;
        else
          prev_ent_is_long = false;
        continue;
        }
      else if (IS_EMPTY_ENTRY(de))
        {
        if (prev_ent_is_long)
          {
          /* The last entry must be LFN. Otherwise, Power loss is occured in creating/deleting entries. */
          rtn = __fat_chk_del_entry(fe, del_parent_sect, del_parent_ent_idx);
          if (0 > rtn) goto End;
          del_parent_sect = 0;
          }
        rtn = e_eos;
        goto End;
        }
      else if (IS_NOTINIT_ENTRY(de))
        {
        rtn = e_plo;
        goto End;
        }
      else
        {
        if (eFAT_ATTR_LONG_NAME == de->attr)
          {
          if (FAT_LFN_LAST_ENTRY & ((fat_lfnent_t *) de)->idx)
            {
            cur_ea->u.flag = (uint32_t) eFILE_LONGENTRY;
            found_last_lfn = true;
            }
          prev_ent_is_long = true;
          if (0 == del_parent_sect)
            {
            del_parent_sect = sect_no;
            del_parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
            }
          continue;
          }
        else
          { /* Short directory. */
          if (prev_ent_is_long && !found_last_lfn)
            {
            /* If power loss occured on deleting entries, All LFNs were deleted.
               But Short-entry was not deleted. So, it should be deleted. */
            if (0 == del_parent_sect)
              {
              del_parent_sect = sect_no;
              del_parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
              }
            rtn = __fat_chk_del_entry(fe, del_parent_sect, del_parent_ent_idx);
            if (0 > rtn) goto End;
            del_parent_sect = 0;
            continue;
            }
          cur_ea->attr = de->attr;
          cur_ea->own_clust = GET_OWN_CLUST(de);
          if (!found_last_lfn)
            cur_ea->u.flag = 0;
          }

        /* point to current got entry. */
        if (del_parent_sect)
          {
          /* It is long-entry(lfn). */
          cur_ea->del_parent_sect = del_parent_sect;
          cur_ea->del_parent_ent_idx = del_parent_ent_idx;
          }
        else
          {
          /* It is short-entry. */
          cur_ea->del_parent_sect = sect_no;
          cur_ea->del_parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
          }

        cur_ea->parent_sect = sect_no;
        cur_ea->parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
        memcpy(&fe->dir, de, sizeof (fat_dirent_t));

        de++;
        rtn = s_ok;
        goto End;
        }
      }

    lim_rel_csector(ce);

    parent_ent_idx = 0;
    sect_no = fat_get_next_sectno_16root(fvi, fe, sect_no);
    if (eFAT_EOF == sect_no)
      {
      if (del_parent_sect)
        {
        rtn = __fat_chk_del_entry(fe, del_parent_sect, del_parent_ent_idx);
        if (0 > rtn) return rtn;
        }
      return e_eos;
      }
    else if (0 > sect_no) return get_errno();
    }

End:
  if (s_ok == rtn)
    {
    if (de > end_de)
      {
      next_sect_no = fat_get_next_sectno_16root(fvi, fe, sect_no);
      if (0 > next_sect_no) return get_errno();
      sect_no = next_sect_no;
      parent_ent_idx = 0;
      }
    else
      parent_ent_idx = (int32_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
    }
  lim_rel_csector(ce);

  next_ea->parent_sect = sect_no;
  next_ea->parent_ent_idx = (uint8_t) parent_ent_idx;

  return rtn;
  }

/*
 Name: __fat_chk_check_clust
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_chk_check_clust(int32_t vol_id, uint32_t own_clust, bool only_chk)
  {
  uint32_t last_clust_no,
    last_clust_fat;
  int32_t real_clust_cnt,
    rtn = s_ok;


  real_clust_cnt = fat_get_clust_cnt(vol_id, own_clust, &last_clust_no, &last_clust_fat);
  if (0 > real_clust_cnt)
    return real_clust_cnt;

  if (eFAT_FREE == last_clust_fat && real_clust_cnt)
    {
    if (!only_chk)
      {
      rtn = fat_set_next_clustno(vol_id, last_clust_no, (uint32_t) eFAT_EOC);
      if (0 > rtn)
        return rtn;

#if defined( DBG )
      chk_dbg.total_recovery_cnt++;
#endif
      }
    else
      return e_plo;
    }

  return real_clust_cnt;
  }

/*
 Name: __fat_chk_adjust_dir_fat
 Desc: Save the information of all files and directories under the Entry
       specified by the 'fe' parameter to fat_statdir_t structure .
 Params:
   - fe: Pointer to the fat_fileent_t structure.
   - statbuf: Pointer to the fat_statdir_t structure where the information
              will be written.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_chk_adjust_dir_fat(chkdisk_vol_t *chk_vol)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(chk_vol->vol_id);
  fat_fileent_t *fe = chk_vol->fe;
  chk_entattr_t ea[2];
  uint32_t file_clust_cnt,
    real_clust_cnt;
  int32_t ea_idx,
    rtn;
#define cur_ea (&ea[ea_idx])
#define next_ea (&ea[ea_idx^1])
#define INIT_EA() (ea_idx=0)
#define EXCG_EA() (ea_idx^=1) /* Exchange cur_ea & next_ea. */


  INIT_EA();
  cur_ea->parent_sect = fe->parent_sect;
  cur_ea->parent_ent_idx = fe->parent_ent_idx;

  do
    {
    /* Get information about current entry and next entry. */
    rtn = __fat_chk_get_sentry(fe, cur_ea, next_ea);
    if (0 > rtn) break;

    if (eFAT_ATTR_DIR & cur_ea->attr)
      {
      /* Set the own cluster number to the parent cluster to search the
         lower enties. */
      fe->parent_sect = D_CLUST_2_SECT(fvi, cur_ea->own_clust);
      fe->parent_ent_idx = 2; /* skip dot & dot-dot directory */

      rtn = __fat_chk_adjust_dir_fat(chk_vol);
      if (0 > rtn)
        {
        if (rtn == e_plo)
          {
          if (!cur_ea->own_clust && (eFAT_FREE != fat_get_next_clustno(fe->vol_id, cur_ea->own_clust)))
            {
            rtn = __fat_chk_check_clust(fe->vol_id, cur_ea->own_clust, false);
            if (0 > rtn) return rtn;
            rtn = fat_unlink_clusts(fe->vol_id, cur_ea->own_clust, false);
            if (0 > rtn) return rtn;
            }
          rtn = __fat_chk_del_entry(fe, cur_ea->del_parent_sect, cur_ea->del_parent_ent_idx);
          if (0 > rtn) return rtn;
          }
        else
          return rtn;
        }
      else
        {
        real_clust_cnt = __fat_set_entry_fatmap(chk_vol, cur_ea->own_clust, 0xFFFFFFFF);
        if (0 == real_clust_cnt || 0xFFFFFFFF == real_clust_cnt)
          {
          rtn = __fat_chk_del_entry(fe, cur_ea->del_parent_sect, cur_ea->del_parent_ent_idx);
          if (0 > rtn) return rtn;
          }
        }
      }
    else
      {
      /* Setup information of entry for deletion. */
      fe->flag = cur_ea->u.flag;
      fe->parent_ent_idx = cur_ea->parent_ent_idx;
      fe->parent_sect = cur_ea->parent_sect;

      /* Calc the number of cluster that a file has occupied. */
      file_clust_cnt = sh_cdiv(fe->dir.filesize, fvi->br.bits_per_clust);
      real_clust_cnt = __fat_set_entry_fatmap(chk_vol, cur_ea->own_clust, file_clust_cnt);
      if (0xFFFFFFFF == real_clust_cnt)
        {
        rtn = __fat_chk_del_entry(fe, cur_ea->del_parent_sect, cur_ea->del_parent_ent_idx);
        if (0 > rtn) return rtn;
        }

      else if (real_clust_cnt < file_clust_cnt)
        {
        /* Resize file size as the number of cluster that  a file has occupied actually. */
        fe->dir.filesize = real_clust_cnt * (1 << fvi->br.bits_per_clust);
        if (0 == real_clust_cnt)
          SET_OWN_CLUST(&fe->dir, 0);

        rtn = __fat_chk_update_sentry(fe, cur_ea);
        if (0 > rtn) return rtn;
        }
      }

    EXCG_EA();
    }
  while (e_eos != rtn);

  if (e_eos == rtn)
    return s_ok;
  else
    return rtn;
  }

/*
 Name: fat_chk_adjust_dir_fat
 Desc: Obtain the information of a directory.
 Params:
   - vol_id: The ID of volume including the directory.
   - path: Pointer to the null-terminated path name of the directory.
   - statbuf: Pointer to an object of type struct statdir_t where the
              file information will be written.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_chk_adjust_dir_fat(chkdisk_vol_t *chk_vol)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(chk_vol->vol_id);
  fat_fileent_t *fe;
  int32_t own_sect,
    rtn;


  if (NULL == (fe = fat_alloc_file_entry(chk_vol->vol_id)))
    return -1;
  chk_vol->fe = fe;

  if (eFAT16_SIZE == fvi->br.efat_type)
    own_sect = fvi->br.first_root_sect;
  else
    /* In FAT32, the first data sector is Root-Directory. */
    own_sect = fvi->br.first_data_sect;

  /* Search from second entry (First entry is volume entry.). */
  fe->parent_ent_idx = 1;
  fe->parent_sect = own_sect;

  /* Set Reserved Cluster to used status. */
  bit_set(chk_vol->chk_fat_map, 0);
  bit_set(chk_vol->chk_fat_map, 1);

  /* Really, get the information of fount directory. */
  rtn = __fat_chk_adjust_dir_fat(chk_vol);

  fat_free_file_entry(fe);

  return rtn;
  }

/*
 Name: fat_chkdisk_recover
 Desc:
 Params:
   - vol_id: The ID of volume which is checked.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None
 */

int32_t fat_chkdisk_recover(int32_t vol_id)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  chkdisk_vol_t chk_vol;
  uint32_t uint32_fat_map,
    clust_cnt_in_fat;
  int32_t rtn;


  chk_vol.vol_id = vol_id;

  clust_cnt_in_fat = fvi->br.last_data_clust + 1;
  uint32_fat_map = cdiv(clust_cnt_in_fat, BITS_PER_UINT32);

  /* Allocate bit-map area. */
  chk_vol.chk_fat_map = __chk_get_fatmap(vol_id, uint32_fat_map);
  if (NULL == chk_vol.chk_fat_map)
    return set_errno(e_port);
  fsm_assert1(0 == ((uint32_t) chk_vol.chk_fat_map & 0x3));

  memset(chk_vol.chk_fat_map, 0, uint32_fat_map * sizeof (uint32_t));

  rtn = fat_chk_adjust_dir_fat(&chk_vol);
  if (0 > rtn)
    return -1;

  rtn = __fat_fatmap_recover(&chk_vol);
  if (0 > rtn)
    return -1;

  rtn = fat_map_reinit(vol_id);
  if (0 > rtn)
    return -1;

#if defined( DBG )
  /* fat_lock() is called in fat_mount(). */
  fat_unlock();
  rtn = fat_chk_fattable(vol_id);
  fat_lock();
  fsm_assert1(s_ok == rtn);
#endif

  return rtn;
  }
#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

