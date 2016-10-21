#include "vol.h"
#include "file.h"
#include "dir.h"
#include "name.h"
#include "misc.h"
#include "chkdsk.h"
#include "log.h"
#include "../lim/lim.h"
#include "../osd/osd.h"

#include <string.h>

#ifdef SFILEMODE
extern void __fat_set_file_information(fat_fileent_t *fe, mod_t mode, uint8_t attr);
#define FROM_NTRES(z) (((z&0x20)>>2) | (z&0x07))
#define TO_NTRES(z) (((z&0x08)<<2) | (z&0x07))
#endif

typedef struct odir_s
  {
  list_head_t head;
  dir_t dir;
  int8_t vol_id;
  uint8_t parent_ent_idx;
  uint32_t parent_sect;

  } odir_t;

static list_head_t odir_free_lru;
static list_head_t odir_alloc_lru;

/* Entries of odir. */
static odir_t fat_odir[FAT_MAX_ODIR];

/*
 Name: fat_zinit_dir
 Desc: Zero initialize all data.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void fat_zinit_dir(void)
  {
  memset(&fat_odir, 0, sizeof (fat_odir));
  }

/*
 Name: fat_init_dir
 Desc: Do nothing.
 Params: None.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: This function is reserved for the future.
 */

result_t fat_init_dir(void)
  {
  list_head_t *list;
  odir_t *odir;
  int32_t i;


  /* Initialize free-list. */
  list = &odir_free_lru;
  list_init(list);

  for (i = 0; i < FAT_MAX_ODIR; i++)
    {
    odir = &fat_odir[i];
    list_init(&odir->head);

    /* TODO: Initialize odir. */

    list_add_tail(list, &odir->head);
    }

  /* Initialize alloc-list. */
  list = &odir_alloc_lru;
  list_init(list);

  return s_ok;
  }

/*
 Name: fat_creat_short_info
 Desc: Setup the elements in the Directory-Entry specified by 'de' parameter.
 Params:
   - de: Pointer to the Directory-Entry of the file or directory .
   - attr: Attribute of the file or directory to be set.
   - own_clust: The own cluster number of Directory-Entry.
   - filesize: The size of file or directory to be set.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: This function does not save the created Entry into the cache.
 */

result_t fat_creat_short_info(fat_dirent_t *de, uint32_t attr,
                              uint32_t own_clust, uint32_t filesize)
  {
  de->attr = (uint8_t) attr;
  SET_OWN_CLUST(de, own_clust);
  de->filesize = filesize;

  /* Initialize all time included in Directory-Entry. */
  return fat_set_ent_time(de, TM_CREAT_ENT | TM_WRITE_ENT | TM_ACCESS_ENT);
  }

/*
 Name: fat_recreat_short_info
 Desc: Change the previous entry information to the new entry information.
 Params:
   - dst: Pointer to the Directory-Entry to be changed.
   - src: Pointer to the new Directory-Entry.
 Returns: None.
 Caveats: None.
 */

void fat_recreat_short_info(fat_dirent_t *dst, fat_dirent_t *src)
  {
  dst->attr = src->attr;
#ifdef SFILEMODE 
  dst->char_case = (dst->char_case & 0x18) | TO_NTRES(FROM_NTRES(src->char_case));
#endif
  dst->ctime_tenth = src->ctime_tenth;
  dst->ctime = src->ctime;
  dst->cdate = src->cdate;
  dst->adate = src->adate;
  dst->fst_clust_hi = src->fst_clust_hi;
  dst->wtime = src->wtime;
  dst->wdate = src->wdate;
  dst->fst_clust_lo = src->fst_clust_lo;
  dst->filesize = src->filesize;
  }

/*
 Name: __fat_get_empty_entry
 Desc: Allocate the Entry as much as the number of 'lfnent_cnt' parameter in
       the data area. If it is not enough the memory space to allocate in that
       cluster, bring the new cluster from FAT file system.
 Params:
   - fe:  Pointer to the File-Entry which is same meaning of a directory to
          allocate Entry.
   - lfnent_cnt: The number of entries to be allocated.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static result_t __fat_get_empty_entry(fat_fileent_t *fe, uint32_t lfnent_cnt)
  {
#define MAX_NEED_CLUST 2
  lim_cacheent_t *ce;
  fat_volinfo_t *fvi;
  fat_dirent_t *de, *end_de;
  uint16_t vol_id;
  uint32_t sect_no,
    seq_dir_cnt, /* count of sequential dir */
    need_size,
    need_clust_cnt,
    valid_ent_cnt,
    ent_cnt,
    skip_ent_cnt,
    i, j,
    clust_list[1/*last cluster*/ + MAX_NEED_CLUST],
    *pclust_list;
  int32_t rtn = s_ok;
#if defined( LOG )
  int32_t cl_id = -1;
#endif


  fsm_assert1(eFAT_EOF != fe->parent_sect);

  vol_id = fe->vol_id;
  fvi = GET_FAT_VOL(vol_id);
  seq_dir_cnt = skip_ent_cnt = 0;

  if (FAT_ROOT_CLUST == fe->parent_clust && eFAT16_SIZE == fvi->br.efat_type)
    {
    /* search sequentially (fat16's root-dir does not use FAT table) */
    for (sect_no = fvi->br.first_root_sect; sect_no <= fvi->br.last_root_sect; sect_no++)
      {
      /* A Cluster's Sector load from LIM layer */
      ce = lim_get_csector(vol_id, sect_no);
      if (NULL == ce) return get_errno();
      de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect) + skip_ent_cnt;
      end_de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect) + fvi->br.ents_per_sect - 1/*start index 0*/;
      /* Searching the empty entry from first-entry into directory entry. */
      for (skip_ent_cnt = 0; de <= end_de; de += ent_cnt)
        {
        ent_cnt = 1;

        if (IS_EMPTY_ENTRY(de))
          ent_cnt = (int32_t) (end_de - de + 1/*start index 0*/);
        else if (!IS_DELETED_ENTRY(de))
          {
          if (eFAT_ATTR_LONG_NAME == ((fat_lfnent_t*) de)->attr)
            {
            /* skip entire LFN */
            ent_cnt = (((fat_lfnent_t*) de)->idx & FAT_LFN_IDX_MASK) + FAT_LFN_SHORT_CNT;
            valid_ent_cnt = (int32_t) (end_de - de + 1/*start index 0*/);
            skip_ent_cnt = valid_ent_cnt >= ent_cnt ? 0 : ent_cnt - valid_ent_cnt;
            }
          seq_dir_cnt = 0;
          continue;
          }

        if (0 == seq_dir_cnt)
          {
          fe->parent_sect = sect_no;
          fe->parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
          }
        seq_dir_cnt += ent_cnt;
        /* if the sequential empty-entry count exceeds the requested emtry count,
           ends a search. */
        if (lfnent_cnt <= seq_dir_cnt)
          return lim_rel_csector(ce); /* done */
        }

      lim_rel_csector(ce);
      }
    }
  else
    {
    /* Get the sector number. The sector includes the parrent cluster. */
    sect_no = D_CLUST_2_SECT(fvi, fe->parent_clust);

    while (1)
      {
      ce = lim_get_csector(vol_id, sect_no);
      if (NULL == ce) return get_errno();
      de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect) + skip_ent_cnt;
      end_de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect) + fvi->br.ents_per_sect - 1/*start index 0*/;

      for (skip_ent_cnt = 0; de <= end_de; de += ent_cnt)
        {
        ent_cnt = 1;

        if (IS_EMPTY_ENTRY(de))
          ent_cnt = (int32_t) (end_de - de + 1/*start index 0*/);
        else if (!IS_DELETED_ENTRY(de))
          {
          if (eFAT_ATTR_LONG_NAME == ((fat_lfnent_t*) de)->attr)
            {
            /* skip entire LFN */
            ent_cnt = (((fat_lfnent_t*) de)->idx & FAT_LFN_IDX_MASK) + FAT_LFN_SHORT_CNT;
            valid_ent_cnt = (int32_t) (end_de - de + 1/*start index 0*/);
            skip_ent_cnt = valid_ent_cnt >= ent_cnt ? 0 : ent_cnt - valid_ent_cnt;
            }
          seq_dir_cnt = 0;
          continue;
          }

        if (0 == seq_dir_cnt)
          {
          fe->parent_sect = sect_no;
          fe->parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
          }
        seq_dir_cnt += ent_cnt;
        if (lfnent_cnt <= seq_dir_cnt)
          return lim_rel_csector(ce); /* done */
        }

      lim_rel_csector(ce);
      rtn = fat_get_next_sectno(vol_id, sect_no);
      if (eFAT_EOF == rtn) break;
      else if (0 > rtn) return rtn;
      sect_no = rtn;
      };


    need_size = (lfnent_cnt - seq_dir_cnt) * sizeof (fat_lfnent_t);
    need_clust_cnt = need_size / fvi->br.bits_per_clust;

    /* Get free-clusters from the bit-map */
    rtn = fat_map_alloc_clusts(vol_id, need_clust_cnt, &clust_list[1]);
    if (0 > rtn) return -1;
    else rtn = s_ok;

    /* Setup the last cluster. */
    clust_list[0] = D_SECT_2_CLUST(fvi, sect_no);

    /* initialize new cluster to 0 */
    for (i = 0, pclust_list = &clust_list[1]; i < need_clust_cnt; i++, pclust_list++)
      {
      sect_no = D_CLUST_2_SECT(fvi, *pclust_list);
      for (j = 0; j < fvi->br.sects_per_clust; j++, sect_no++)
        {
        /* Loading a sector from LIM's layer */
        ce = lim_get_csector(fe->vol_id, sect_no);
        if (NULL == ce)
          {
          /* Update the bit-map & return cluster*/
          fat_map_free_clusts(fe->vol_id, 1, &clust_list[1]);
          return -1;
          }

        memset(LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect), 0, fvi->br.bytes_per_sect);

        /* Mark entry flag with CACHE_DIRTY in dirty sector. */
        lim_mark_dirty_csector(ce, NULL);

        /* Write sector's data of dirty LIM-Cache list*/
        rtn = lim_flush_csector(ce);
        if (0 > rtn)
          {
          fat_map_free_clusts(fe->vol_id, 1, &clust_list[1]);
          lim_rel_csector(ce);
          return -1;
          }
        lim_rel_csector(ce);
        }
      }

    if (0 == seq_dir_cnt)
      {
      fe->parent_sect = D_CLUST_2_SECT(fvi, clust_list[1]);
      fe->parent_ent_idx = 0;
      }

#if defined( LOG )
    cl_id = fat_log_clust_on(vol_id, clust_list[0]);
    if (0 > cl_id) return cl_id;
#endif

    /* NOTE: The one cluster may be flushed in fat_stamp_clusts(). */
    if (0 > fat_stamp_clusts(vol_id, need_clust_cnt, clust_list))
      return -1;

#if defined( LOG )
    rtn = fat_sync_table(vol_id, true);
#endif

#if defined( LOG )
    fat_log_off(vol_id, cl_id);
#endif

    return rtn;
    }

  return set_errno(e_nospc);
  }

/*
 Name: fat_alloc_entry_pos
 Desc: Allocate the physical space to save the Entry specified by the 'fe'
       parameter.
 Params:
   - fe: Pointer to the fat_fileent_t structure.
 Returns:
   int32_t  =0 on success. The space of the File-Entry was allocated
               successfully.
            <0 on fail.
 Caveats: Before operating this function, in the File-Entry, the attribute of
          file name should be existed for the length of file name, and if it is
          long file name or short file name.
 */

result_t fat_alloc_entry_pos(fat_fileent_t *fe)
  {
  int32_t need_ent_cnt,
    rtn;


  /* Determine the number of needed entries according to file name length. */
  if ((uint32_t) eFILE_LONGENTRY & fe->flag)
    need_ent_cnt = (fe->name_len / FAT_CHARSPERLFN) + FAT_LFN_SHORT_CNT;
  else
    need_ent_cnt = 1;

  rtn = __fat_get_empty_entry(fe, need_ent_cnt);
  if (0 > rtn)
    fe->ent_cnt = 0;
  else
    fe->ent_cnt = (uint8_t) need_ent_cnt;

  return rtn;
  }

/*
 Name: fat_create_dots_dir
 Desc: Create a dot-directory under the directory specified by the 'fe'
       parameter.
 Params:
   - fe: Pointer to the Directory-Entry which means the upper directory to
         create a dot-directory.
   - own_clust: The cluster number which dot-directory will be created.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_create_dots_dir(fat_fileent_t *fe, uint32_t own_clust)
  {
  lim_cacheent_t *ce;
  fat_volinfo_t *fvi;
  fat_dirent_t dot_dir, dot2_dir, *de;
  int32_t sect_no,
    i,
    rtn = s_ok;


  fvi = GET_FAT_VOL(fe->vol_id);

  /* Fill the information to the Directory-Entry of the dot-directory. */
  memcpy(dot_dir.name, FAT_DOT, FAT_SHORTNAME_SIZE);
  fat_creat_short_info(&dot_dir, eFAT_ATTR_DIR, own_clust, 0);

  /* Fill the information to the Directory-Entry of the doe-dot directory. */
  memcpy(dot2_dir.name, FAT_DOTDOT, FAT_SHORTNAME_SIZE);
  dot2_dir.char_case = 0;

  if (FAT_ROOT_CLUST == fe->parent_clust && eFAT16_SIZE == fvi->br.efat_type)
    fat_creat_short_info(&dot2_dir, eFAT_ATTR_DIR, 0, 0);
  else
    fat_creat_short_info(&dot2_dir, eFAT_ATTR_DIR, fe->parent_clust, 0);

  sect_no = D_CLUST_2_SECT(fvi, own_clust);

  for (i = 0; i < fvi->br.sects_per_clust; i++, sect_no++)
    {
    ce = lim_get_csector(fe->vol_id, sect_no);
    if (NULL == ce) return get_errno();
    de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);

    /* Initialize the created cluster to '0' value. */
    memset(de, 0, fvi->br.bytes_per_sect);

    if (0 == i)
      {
      memcpy(de, &dot_dir, sizeof (fat_dirent_t));
      memcpy(de + 1, &dot2_dir, sizeof (fat_dirent_t));
      }

    /* Mark as Dirty status to update data. */
    lim_mark_dirty_csector(ce, NULL);

    /* Update data to the physical device. */
    rtn = lim_flush_csector(ce);
    lim_rel_csector(ce);
    if (0 > rtn) return -1;
    }

  return rtn;
  }

/*
 Name: fat_change_dotdots_dir
 Desc: Change a dotdot-directory entry under the directory specified by the 'fe'
       parameter.
 Params:
   - fe: Pointer to the Directory-Entry which means the upper directory to
         create a dot-directory.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_change_dotdots_dir(fat_fileent_t *fe)
  {
  lim_cacheent_t *ce;
  fat_volinfo_t *fvi;
  fat_dirent_t *de;
  int32_t sect_no,
    clust_no;
  result_t rtn = s_ok;

  fvi = GET_FAT_VOL(fe->vol_id);
  clust_no = GET_OWN_CLUST(&fe->dir);
  sect_no = D_CLUST_2_SECT(fvi, clust_no);

  ce = lim_get_csector(fe->vol_id, sect_no);
  if (NULL == ce) return get_errno();
  de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);

  de++;
  SET_OWN_CLUST(de, fe->parent_clust);

  /* Mark as Dirty status to update data. */
  lim_mark_dirty_csector(ce, NULL);

  /* Update data to the physical device. */
  rtn = lim_flush_csector(ce);
  lim_rel_csector(ce);
  if (0 > rtn) return -1;

  return rtn;
  }

/*
 Name: __fat_make_long_entry
 Desc: Convert file name specified by 'name' parameter to the LFN format.
 Params:
   - lfn: LFN's information structure pointer. LFN:(Long File Name)
   - name: The file name.
   - idx: The index of LFN entry.
   - chksum: Checksum value about name.
 Returns:
   char*  value on success. The returned value means the address of remained
                  string after converting.
 Caveats: None.
 */

static char *__fat_make_long_entry(fat_lfnent_t *lfn, char *name,
                                   int32_t idx, uint32_t chksum)
  {
  lfn->idx = (uint8_t) idx;
  lfn->attr = eFAT_ATTR_LONG_NAME;
  lfn->nt_rsvd = 0;
  lfn->chksum = (uint8_t) chksum;
  lfn->fst_clust_lo = 0;
  return fat_name_2_lfn(lfn, name);
  }

/*
 Name: __fat_make_last_long_entry
 Desc: Convert file name specified by 'name' parameter to the LFN format when
       given LFN entry is the last.
 Params:
   - lfn: LFN's information structure pointer.
   - idx: The index of LFN entry.
   - name: The null-termimnated string name for last LFN entry.
   - name_len: The length of last LFN entry.
   - chksum: Checksum value of name entry.
 Returns:
   char*  value on success. The returned value means the address of remained
                  string after converting.
 Caveats: None.
 */

static char *__fat_make_last_long_entry(fat_lfnent_t *lfn, int32_t idx,
                                        char *name, uint32_t name_len, uint32_t chksum)
  {
  lfn->idx = (uint8_t) (idx | FAT_LFN_LAST_ENTRY);
  lfn->attr = eFAT_ATTR_LONG_NAME;
  lfn->nt_rsvd = 0;
  lfn->chksum = (uint8_t) chksum;
  lfn->fst_clust_lo = 0;
  return fat_name_2_last_lfn(lfn, name, name_len);
  }

/*
 Name: fat_creat_entry_long
 Desc: Consist LFN Entry for long file name and write to physical device.
 Params:
   - fe: Ponter to the fat_fileent_t structure that LFN is created.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: Information about the name in File-Entry should be set previously.
          The LFN Entry for long file name is compatible with the
          Directory-Entry. That is, FAT file system uses the Directory-Entry
          structure to save LFN Entry.
 */

result_t fat_creat_entry_long(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
#if defined( LOG )
  list_head_t list;
  list_head_t * const plist = &list;
#else
  list_head_t * const plist = NULL;
#endif
  lim_cacheent_t *ce;
  fat_dirent_t *de;
  fat_lfnent_t *lfn, *end_lfn;
  char *name, *p_last_name;
  int32_t rtn,
    lfnent_cnt,
    sect_no,
    len;
  uint8_t chksum;


  fvi = GET_FAT_VOL(fe->vol_id);

#if defined( LOG )
  list_init(&list);
#endif

  /* Make a LFN's short name. */
  rtn = fat_resolve_lfn_shortname(fe);
  if (0 > rtn) return rtn;

  lfnent_cnt = fe->ent_cnt - FAT_LFN_SHORT_CNT;

  /* Calculate the check-sum of a LFN's name.*/
  chksum = fat_checksum_lfn_name(fe->dir.name);
  name = fe->name;
  sect_no = fe->parent_sect;
  ce = lim_get_csector(fe->vol_id, sect_no);
  if (NULL == ce) return get_errno();
  lfn = (fat_lfnent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
  end_lfn = lfn + fvi->br.ents_per_sect - 1/*start index 0*/;
  lfn += fe->parent_ent_idx;

  /* Calculate the length of the last LFN entry. */
  len = fe->name_len % LFN_NAME_CHARS;
  if (0 == len) len = LFN_NAME_CHARS;
  p_last_name = name + fe->name_len - len;
  /* Fill 'lfn' structure with the last name. */
  name = __fat_make_last_long_entry(lfn++, lfnent_cnt--, p_last_name, len, chksum);

  /* Fill 'lfn' structure with the rest names. */
  while (lfnent_cnt)
    {
    for (/*no init*/; (lfn <= end_lfn) && (0 < lfnent_cnt); lfn++, lfnent_cnt--)
      name = __fat_make_long_entry(lfn, name, lfnent_cnt, chksum);

    if (0 == lfnent_cnt) break;

    /* Mark the sector 'ce' including LFN information as Dirty status. */
    lim_mark_dirty_csector(ce, plist);
    lim_rel_csector(ce);

    sect_no = fat_get_next_sectno_16root(fvi, fe, sect_no);
    ce = lim_get_csector(fe->vol_id, sect_no);
    if (NULL == ce) return get_errno();
    lfn = (fat_lfnent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
    end_lfn = lfn + fvi->br.ents_per_sect - 1/*start index 0*/;
    }

  if (lfn > end_lfn)
    {
    lim_mark_dirty_csector(ce, plist);
    lim_rel_csector(ce);

    sect_no = fat_get_next_sectno_16root(fvi, fe, sect_no);
    ce = lim_get_csector(fe->vol_id, sect_no);
    if (NULL == ce) return get_errno();
    de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
    }
  else
    de = (fat_dirent_t *) lfn;

  /* Set infomation about LFN's short entry. */
  fe->lfn_shortent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
  fe->lfn_short_sect = sect_no;
  memcpy(de, &fe->dir, sizeof (fat_dirent_t));

  lim_mark_dirty_csector(ce, plist);
  lim_rel_csector(ce);
  rtn = lim_flush_csectors(ce->vol_id, plist);

  return rtn;
  }

/*
 Name: fat_creat_entry_short
 Desc: Consist LFN Entry for short file name and write to physical device.
 Params:
   - fe: Ponter to the fat_fileent_t structure.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: Information about name in File-Entry should be set previously.
 */

result_t fat_creat_entry_short(fat_fileent_t *fe)
  {
  lim_cacheent_t *ce;
  fat_dirent_t *de;
  int32_t rtn = s_ok;
  fat_volinfo_t *fvi;
  fvi = GET_FAT_VOL(fe->vol_id);


  /* Get the sector where the entry to be create is located. */
  ce = lim_get_csector(fe->vol_id, fe->parent_sect);
  if (NULL == ce) return get_errno();
  de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, fe->parent_sect, fvi->br.first_root_sect);

  de += fe->parent_ent_idx;
  memcpy(de, &fe->dir, sizeof (fat_dirent_t));

  /* Mark the sector 'ce' including the information of a short name as Dirty status. */
  lim_mark_dirty_csector(ce, NULL);
  rtn = lim_flush_csector(ce);
  lim_rel_csector(ce);

  return rtn;
  }

/*
 Name: __fat_sect_find_short
 Desc:  Find entry of specific short name in a specific sector.
 Params:
   - fe: Ponter to the fat_fileent_t structure.
   - sect_no: Sector number which will be searched in.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static result_t __fat_sect_find_short(fat_fileent_t *fe, uint32_t sect_no)
  {
  lim_cacheent_t *ce;
  fat_volinfo_t *fvi;
  fat_dirent_t *de, *end_de;
  int32_t skip_entries,
    rtn = e_noent;

  fvi = GET_FAT_VOL(fe->vol_id);
  ce = lim_get_csector(fe->vol_id, sect_no);
  if (NULL == ce) return get_errno();
  /* 'de' indicates first directory-entry in sector. */
  de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);

  end_de = de + fvi->br.ents_per_sect - 1;

  /* Skip the directory for volume. */
  if (eFAT_ATTR_VOL == de->attr) de++;

  for (/*no init*/; de <= end_de; de++)
    {
    if (IS_DELETED_ENTRY(de))
      {
      if (eFAT_ATTR_LONG_NAME == de->attr)
        de++; /* In case of LFN, we can skip a entry at the least. */
      continue;
      }

    /* Skip as the number of entries for long name. */
    if (eFAT_ATTR_LONG_NAME == de->attr)
      {
      skip_entries = (((fat_lfnent_t*) de)->idx & FAT_LFN_IDX_MASK) - 1/*for short-entry*/;
      fsm_assert1(0 <= skip_entries);
      de += skip_entries;
      continue;
      }

    /* There isn't any more entries. */
    if (IS_EMPTY_ENTRY(de))
      {
      rtn = e_eos;
      goto End;
      }

    /* If entry found, */
    if (!memcmp(de->name, fe->dir.name, FAT_SHORTNAME_SIZE))
      {
      fe->parent_sect = sect_no;
      fe->parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));

      memcpy(&fe->dir, de, sizeof (fat_dirent_t));
      rtn = s_ok;
      goto End;
      }
    }

End:
  lim_rel_csector(ce);
  return rtn;
  }

/*
 Name: fat_creat_entry
 Desc: Create a new Directory-Entry in a File-Entry specified by the 'fe'
       parameter.
 Params:
   - fe: Ponter to the fat_fileent_t structure for File Entry.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_creat_entry(fat_fileent_t *fe, bool is_new)
  {
  uint32_t type;


  if (is_new)
    type = TM_CREAT_ENT | TM_WRITE_ENT | TM_ACCESS_ENT;
  else
    type = TM_ACCESS_ENT;

  /* Initialize all times of Directory-Entry to be created. */
  fat_set_ent_time(&fe->dir, type);

  /* When the name in 'fe' File Entry is a long file name, */
  if ((uint32_t) eFILE_LONGENTRY & fe->flag)
    return fat_creat_entry_long(fe);
    /* When the name in 'fe' File Entry is a short file name, */
  else
    return fat_creat_entry_short(fe);
  }

/*
 Name: fat_hasnt_entry
 Desc: Check whether there is a File-Entry in a directory ort not.
       Skip Dot directory and Dot-Dot directory, and then Check child entries.
 Params:
   - fe: Ponter to the fat_fileent_t structure.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_hasnt_entry(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
  int32_t own_clust,
    parent_clust,
    parent_sect,
    parent_ent_idx,
    rtn;


  fvi = GET_FAT_VOL(fe->vol_id);

  /* Get the culster number from a Directory Entry. */
  own_clust = GET_OWN_CLUST(&fe->dir);

  /* The backup */
  parent_clust = fe->parent_clust;
  parent_sect = fe->parent_sect;
  parent_ent_idx = fe->parent_ent_idx;

  /* Prevent that parent_clust is accessed in fat_get_next_sectno_16root to
     check for Rott-Directory. */
  fe->parent_clust = own_clust;
  fe->parent_sect = D_CLUST_2_SECT(fvi, own_clust);
  fe->parent_ent_idx = 2; /* skip dot & dot dot */

  rtn = fat_get_entry_info(fe, (fat_entattr_t *) NULL);

  /* Determine whether there is a File Entry or not by the returned value. */
  if (s_ok == rtn)
    rtn = e_notempty;
  if (e_eos == rtn)
    rtn = s_ok;

  /* The roll-back */
  fe->parent_clust = parent_clust;
  fe->parent_sect = parent_sect;
  fe->parent_ent_idx = (uint8_t) parent_ent_idx;

  return rtn;
  }

/*
 Name: fat_unlink_entry_long
 Desc: Remove the Directory-Entry for LFN in the File-Entry specified by 'fe'
       parameter.
 Params:
   - fe: Ponter to the fat_fileent_t structure.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_unlink_entry_long(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
  fat_fileent_t *pfe;
  lim_cacheent_t *ce;
  fat_dirent_t *de, *end_de;
#if defined( LOG )
  list_head_t list;
  list_head_t * const plist = &list;
#else
  list_head_t * const plist = NULL;
#endif
  int32_t sect_no,
    ent_idx,
    i,
    rtn = s_ok;


  if (eFAT_EOF == fe->parent_sect)
    return e_eos;

#if defined( LOG )
  list_init(&list);
#endif

  pfe = (fat_fileent_t *) fe;
  fvi = GET_FAT_VOL(pfe->vol_id);
  sect_no = fe->parent_sect;
  ent_idx = fe->parent_ent_idx;

  for (i = 0; i < FAT_LONGNAME_SLOTS; i++)
    {
    ce = lim_get_csector(fvi->vol_id, sect_no);
    if (NULL == ce) return get_errno();
    de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
    end_de = de + fvi->br.ents_per_sect - 1/*start index 0*/;

    for (de += ent_idx; de <= end_de; de++)
      {

      if (IS_EMPTY_ENTRY(de))
        goto End;

      DELETE_ENTRY(de);

      /* If short, done. */
      if (eFAT_ATTR_LONG_NAME != de->attr)
        goto End;
      }

    /* Mark as Dirty status to update data. */
    lim_mark_dirty_csector(ce, plist);
    lim_rel_csector(ce);

    ent_idx = 0;
    sect_no = fat_get_next_sectno_16root(fvi, pfe, sect_no);
    if (eFAT_EOF == sect_no)
      break;
    else if (0 > sect_no)
      return -1;
    }

End:
  lim_mark_dirty_csector(ce, plist);
  lim_rel_csector(ce);
  rtn = lim_flush_csectors(pfe->vol_id, plist);

  return rtn;
  }

/*
 Name: fat_unlink_entry_short
 Desc: Remove the Short Directory -Entry in the File-Entry specified by 'fe'
       parameter.
 Params:
   - fe: Ponter to the fat_fileent_t structure.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_unlink_entry_short(fat_fileent_t *fe)
  {
  lim_cacheent_t *ce;
  fat_dirent_t *de;
  int32_t rtn = s_ok;
  fat_volinfo_t *fvi;
  fvi = GET_FAT_VOL(fe->vol_id);


  /* Read a cache to remove a Short Directory Entry. */
  ce = lim_get_csector(fe->vol_id, fe->parent_sect);
  if (NULL == ce) return get_errno();
  /* Get a Short Directory Entry from the cache. */
  de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, fe->parent_sect, fvi->br.first_root_sect);

  /* Get the index of entry in parent cluster. */
  de += fe->parent_ent_idx;

  /* Make to deleted entry. */
  if (!IS_EMPTY_ENTRY(de))
    { /* check for unlink in log-recover */
    /* Mark to the deleted file */
    DELETE_ENTRY(de);

    /* Mark the entry's flag with DIRTY status. */
    lim_mark_dirty_csector(ce, NULL);
    /* Flush the sector in cache. */
    rtn = lim_flush_csector(ce);
    }

  lim_rel_csector(ce);
  return rtn;
  }

/*
 Name: fat_unlink_entry
 Desc: Remove all entries in the File-Entry specified by 'fe' parameter.
 Params:
   - fe: Ponter to the fat_fileent_t structure.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_unlink_entry(fat_fileent_t *fe)
  {
  int32_t clust_no,
#if defined( LOG )
    cl_id,
#endif
    rtn;


  /* Check the File-Entry should have been becoming the open already.*/
  if (fat_get_opend_file_entry(fe))
    return set_errno(e_busy);

  clust_no = GET_OWN_CLUST(&fe->dir);

  /* Flush the LIM data cache to the physical device. */
  if (0 > lim_flush_cdsector()) return -1;

#if defined( LOG )
  cl_id = fat_log_on(fe, fe);
  if (0 > cl_id) return cl_id;
#endif

  rtn = fat_unlink_clusts(fe->vol_id, clust_no, false);

  /* Remove own directory-entry in parent cluster */
  if ((uint32_t) eFILE_LONGENTRY & fe->flag)
    rtn = fat_unlink_entry_long(fe);
  else
    rtn = fat_unlink_entry_short(fe);

#if defined( LOG )
  fat_log_off(fe->vol_id, cl_id);
#endif

  return rtn;
  }

/*
 Name: fat_get_entry_info
 Desc: Get information of the current File Entry.
 Params:
   - fe: Pointer to the fat_fileent_t structure means a File Entry.
   - next_ep: Pointer to the next searching Entry.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: If 'next_ep' is NULL, then
      (fe->parent_sect & fe->parent_ent_idx) point to the next searching Entry.
   if 'next_ep' is not NULL, then
      (fe->parent_sect & fe->parent_ent_idx) point to the current got Entry.
      'next_ep' point to next searching entry.
 */

result_t fat_get_entry_info(fat_fileent_t *fe, fat_entattr_t *next_ep)
  {
  fat_volinfo_t *fvi;
  lim_cacheent_t *ce;
  fat_dirent_t *de, *end_de;
  char *name;
  int32_t sect_no,
    next_sect_no,
    lfn_idx,
    parent_ent_idx,
    rtn;


  if (eFAT_EOF == fe->parent_sect)
    return e_eos;

  fvi = GET_FAT_VOL(fe->vol_id);
  rtn = e_eos;
  sect_no = fe->parent_sect;
  lfn_idx = 0;
  if (fe->parent_ent_idx == 0xFF)
    parent_ent_idx = 0;
  else
    parent_ent_idx = fe->parent_ent_idx;
  name = &fe->name[FAT_LONGNAME_SIZE - sizeof (fat_dirent_t)];

  while (1)
    {
    /* Get the cluster's number of parents-directory */
    ce = lim_get_csector(fvi->vol_id, sect_no);
    if (NULL == ce) return get_errno();
    de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
    end_de = de + fvi->br.ents_per_sect - 1/*start index 0*/;

    for (de += parent_ent_idx; de <= end_de; de++)
      {
      if (IS_DELETED_ENTRY(de))
        {
        if (eFAT_ATTR_LONG_NAME == de->attr)
          de++; /* In case of LFN, we can skip a entry at the least. */
        lfn_idx = 0;
        continue;
        }
      else if (IS_EMPTY_ENTRY(de))
        {
        rtn = e_eos;
        goto End;
        }
      else
        {
        if (eFAT_ATTR_LONG_NAME == de->attr)
          {
          if (FAT_LFN_LAST_ENTRY & ((fat_lfnent_t *) de)->idx)
            {
            lfn_idx = ((fat_lfnent_t *) de)->idx & FAT_LFN_IDX_MASK;
            name = fe->name + (lfn_idx - 1/*start index 0*/) * FAT_CHARSPERLFN;
            fe->flag |= (uint32_t) eFILE_LONGENTRY;
            if (next_ep)
              {
              /* point to current got entry. */
              fe->parent_sect = sect_no;
              fe->parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
              }
            /* If the file-name is aligned in FAT_CHARSPERLFN, append '\0' */
            *(name + FAT_CHARSPERLFN) = '\0';
            }
          name = fat_lfn_2_name(name, (fat_lfnent_t *) de);
          continue;
          }
        else
          { /* short dir */
          fsm_assert3(!IS_EMPTY_ENTRY(de));

          /*If the file-attribute is volume-label, searching next entry*/
          if (de->attr == eFAT_ATTR_VOL)
            {
            lfn_idx = 0;
            fe->flag &= ~(uint32_t) eFILE_LONGENTRY;
            name = &fe->name[FAT_LONGNAME_SIZE - sizeof (fat_dirent_t)];
            continue;
            }

          memcpy(&fe->dir, de, sizeof (fat_dirent_t));
          if (0 == lfn_idx)
            {
            fat_cp_shortname(fe->name, &fe->dir); /* point to next entry */
            fe->flag &= ~(uint32_t) eFILE_LONGENTRY;
            if (next_ep)
              {
              /* point to current got entry. */
              fe->parent_sect = sect_no;
              fe->parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
              }
            }
          }

        de++;
        rtn = s_ok;
        goto End;
        }
      }

    lim_rel_csector(ce);

    parent_ent_idx = 0;
    sect_no = fat_get_next_sectno_16root(fvi, fe, sect_no);
    if (eFAT_EOF == sect_no) return e_eos;
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

  if (NULL == next_ep)
    {
    /* fe point to next searching entry. */
    fe->parent_sect = sect_no;
    fe->parent_ent_idx = (uint8_t) parent_ent_idx;
    }
  else
    {
    /* next_ep point to next searching entry. */
    next_ep->parent_sect = sect_no;
    next_ep->parent_ent_idx = (uint8_t) parent_ent_idx;
    }

  return rtn;
  }

/*
 Name: __fat_get_entry_sinfo
 Desc: Read the Entry information from the physical device.
 Params:
   - fe: Pointer to the fat_fileent_t structure means a File Entry.
   - ea: Pointer to the fat_entattr_t structure to be filled with the
         information of Entry.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static result_t __fat_get_entry_sinfo(fat_fileent_t *fe, fat_entattr_t *ea)
  {
  fat_volinfo_t *fvi;
  lim_cacheent_t *ce;
  fat_dirent_t *de, *end_de;
  int32_t sect_no,
    next_sect_no,
    lfn_idx,
    parent_ent_idx,
    rtn;


  if (eFAT_EOF == ea->parent_sect)
    return e_eos;

  fvi = GET_FAT_VOL(fe->vol_id);
  rtn = e_eos;
  sect_no = ea->parent_sect;
  lfn_idx = 0;
  parent_ent_idx = ea->parent_ent_idx;

  while (1)
    {
    /* Read data about the entry from memory or cache. */
    ce = lim_get_csector(fvi->vol_id, sect_no);
    if (NULL == ce) return get_errno();
    de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
    end_de = de + fvi->br.ents_per_sect - 1/*start index 0*/;

    /* Read the Directory-Entry. */
    for (de += parent_ent_idx; de <= end_de; de++)
      {
      if (IS_DELETED_ENTRY(de))
        {
        if (eFAT_ATTR_LONG_NAME == de->attr)
          de++; /* In case of LFN, we can skip a entry at the least. */
        continue;
        }
      else if (IS_EMPTY_ENTRY(de))
        {
        rtn = e_eos;
        goto End;
        }
      else
        {
        if (eFAT_ATTR_LONG_NAME == de->attr)
          {
          if (FAT_LFN_LAST_ENTRY & ((fat_lfnent_t *) de)->idx)
            {
            lfn_idx = ((fat_lfnent_t *) de)->idx & FAT_LFN_IDX_MASK;
            fsm_assert1((1 <= lfn_idx) && (lfn_idx < FAT_LONGNAME_SLOTS));
            de += lfn_idx - FAT_LFN_SHORT_CNT;
            }
          continue;
          }
        else
          { /* short name entry. */
          ea->attr = de->attr;
          ea->u.filesize = de->filesize;
          ea->own_clust = GET_OWN_CLUST(de);
          }

        de++;
        rtn = s_ok;
        goto End;
        }
      }

    lim_rel_csector(ce);

    parent_ent_idx = 0;
    sect_no = fat_get_next_sectno_16root(fvi, ea, sect_no);
    if (eFAT_EOF == sect_no) return e_eos;
    else if (0 > sect_no) return get_errno();
    }

End:
  if (s_ok == rtn)
    {
    if (de > end_de)
      {
      next_sect_no = fat_get_next_sectno_16root(fvi, ea, sect_no);
      if (0 > next_sect_no) return get_errno();
      sect_no = next_sect_no;
      parent_ent_idx = 0;
      }
    else
      parent_ent_idx = (int32_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
    }
  lim_rel_csector(ce);

  /* next_ep point to next searching entry. */
  ea->parent_sect = sect_no;
  ea->parent_ent_idx = (uint8_t) parent_ent_idx;

  return rtn;
  }

/*
 Name: __fat_get_entry_pos
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

static result_t __fat_get_entry_pos(fat_fileent_t *fe, fat_entattr_t *cur_ea, fat_entattr_t *next_ea)
  {
  fat_volinfo_t *fvi;
  lim_cacheent_t *ce;
  fat_dirent_t *de, *end_de;
  int32_t sect_no,
    next_sect_no,
    lfn_idx,
    parent_ent_idx,
    rtn;


  if (eFAT_EOF == cur_ea->parent_sect)
    return e_eos;

  fvi = GET_FAT_VOL(fe->vol_id);
  rtn = e_eos;
  sect_no = cur_ea->parent_sect;
  parent_ent_idx = cur_ea->parent_ent_idx;
  lfn_idx = 0;

  while (1)
    {
    ce = lim_get_csector(fvi->vol_id, sect_no);
    if (NULL == ce) return get_errno();
    de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
    end_de = de + fvi->br.ents_per_sect - 1/*start index 0*/;

    for (de += parent_ent_idx; de <= end_de; de++)
      {
      if (IS_DELETED_ENTRY(de))
        {
        if (eFAT_ATTR_LONG_NAME == de->attr)
          de++; /* In case of LFN, we can skip a entry at the least. */
        continue;
        }
      else if (IS_EMPTY_ENTRY(de))
        {
        fsm_assert1(0 == lfn_idx);
        rtn = e_eos;
        goto End;
        }
      else
        {
        if (eFAT_ATTR_LONG_NAME == de->attr)
          {
          if (FAT_LFN_LAST_ENTRY & ((fat_lfnent_t *) de)->idx)
            {
            cur_ea->u.flag = (uint32_t) eFILE_LONGENTRY;
            /* point to current got entry. */
            cur_ea->parent_sect = sect_no;
            cur_ea->parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
            lfn_idx = ((fat_lfnent_t *) de)->idx & FAT_LFN_IDX_MASK;
            de += lfn_idx - FAT_LFN_SHORT_CNT;
            }
          continue;
          }
        else
          { /* Short directory. */
          cur_ea->attr = de->attr;
          cur_ea->own_clust = GET_OWN_CLUST(de);
          if (0 == lfn_idx)
            {
            cur_ea->u.flag = 0;
            cur_ea->parent_sect = sect_no;
            cur_ea->parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
            }
          }

        de++;
        rtn = s_ok;
        goto End;
        }
      }

    lim_rel_csector(ce);

    parent_ent_idx = 0;
    sect_no = fat_get_next_sectno_16root(fvi, fe, sect_no);
    if (eFAT_EOF == sect_no) return e_eos;
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
 Name: fat_resolve_lfn_shortname
 Desc: Determine the LFN's short-entry name.
 Params:
   - fe: Pointer to the fat_fileent_t structure.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_resolve_lfn_shortname(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
  lim_cacheent_t *ce;
  fat_dirent_t *de, *end_de;
  int32_t sect_no,
    ori_sect_no,
    valid_idx,
    num;
#define MAX_INDEX_NUM (256)
  ionfs_local uint32_t exist_nums[MAX_INDEX_NUM]; /* The bit number that set to 0 is the lowest number. */
  uint32_t lowest,
    max_idxs = (MAX_INDEX_NUM * BITS_PER_UINT32),
    i, j;


  fvi = GET_FAT_VOL(fe->vol_id);
  if (FAT_ROOT_CLUST == fe->parent_clust && eFAT16_SIZE == fvi->br.efat_type)
    sect_no = ori_sect_no = fvi->br.first_root_sect;
  else
    sect_no = ori_sect_no = D_CLUST_2_SECT(fvi, fe->parent_clust);

  valid_idx = lowest = 0;

Search:
  memset(exist_nums, 0, sizeof (exist_nums));

  /* Get the cluster's number of parents-directory */
  ce = lim_get_csector(fvi->vol_id, sect_no);
  if (NULL == ce) return get_errno();
  de = ((fat_dirent_t*) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect)) + 2/* dot and dot-dot directory*/;
  end_de = ((fat_dirent_t*) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect)) + fvi->br.ents_per_sect - 1/*start index 0*/;

  while (1)
    {
    for (/* no init */; de <= end_de; de++)
      {
      if (IS_DELETED_ENTRY(de))
        {
        if (eFAT_ATTR_LONG_NAME == de->attr)
          de++; /* In case of LFN, we can skip a entry at the least. */
        continue;
        }
      else if (IS_EMPTY_ENTRY(de))
        {
SetIdx:
        for (i = 0; i < MAX_INDEX_NUM; i++)
          {
          if (BITS_UINT32_ALL_ONE != exist_nums[i])
            {
            j = bit_ffz(exist_nums[i]);
            valid_idx = lowest + (i * BITS_PER_UINT32) + j;
            goto End;
            }
          }

        lowest += max_idxs;

        sect_no = ori_sect_no;
        lim_rel_csector(ce);
        goto Search; /* Re-search to get the lowest number. */
        }
      else
        {
        if (eFAT_ATTR_LONG_NAME == de->attr)
          de += ((fat_lfnent_t *) de)->idx & FAT_LFN_IDX_MASK;
        if (de > end_de) break;

        if (0 != (num = fat_get_short_index(de->name, fe->dir.name)))
          {
          num--; /*start index 0*/
          if ((num >= (int32_t) lowest) && num < (int32_t) (lowest + max_idxs))
            {
            i = (num - lowest) / BITS_PER_UINT32;
            exist_nums[i] |= 1 << ((num - lowest) & BITS_UINT32_MASK);
            }
          }
        }
      }

    lim_rel_csector(ce);

    sect_no = fat_get_next_sectno_16root(fvi, fe, sect_no);
    if (eFAT_EOF == sect_no) goto SetIdx;
    else if (0 > sect_no) return get_errno();

    ce = lim_get_csector(fvi->vol_id, sect_no);
    if (NULL == ce) return get_errno();
    de = (fat_dirent_t*) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
    end_de = ((fat_dirent_t*) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect)) + fvi->br.ents_per_sect - 1/*start index 0*/;
    }

End:
  lim_rel_csector(ce);

  fat_set_short_index(fe->dir.name, valid_idx + 1/*start index 0*/);

  return s_ok;
  }

/*
 Name: __fat_sect_find_long
 Desc:  Find entry of a specific long name in a specific sector.
 Params:
   - fe: Pointer to file-entry.
   - ne: Point to lfn_nameent_t structure.
   - idx_name_ent: Index of name entry to be searched.
   - sect_no: Sector number which will be searched in.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static result_t __fat_sect_find_long(fat_fileent_t *fe, lfn_nameent_t *ne,
                                    uint32_t idx_name_ent, uint32_t sect_no)
  {
  fat_volinfo_t *fvi;
  lim_cacheent_t *ce;
  fat_lfnent_t *lfn, *end_lfn;
  fat_dirent_t *de;
  uint16_t entries, ne_idx;
  int32_t rtn = e_noent;

  fvi = GET_FAT_VOL(fe->vol_id);

  ce = lim_get_csector(fe->vol_id, sect_no);
  if (NULL == ce) return get_errno();
  lfn = (fat_lfnent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);

  end_lfn = lfn + fvi->br.ents_per_sect - 1/*start index 0*/;
  entries = ne->entries;
  ne_idx = (uint16_t) idx_name_ent;
  lfn += ne->skip_ents;

  for (/*no init*/; lfn <= end_lfn; lfn++)
    {
    if (IS_EMPTY_ENTRY(lfn))
      {
      rtn = e_eos;
      goto End;
      }
    else if (IS_DELETED_ENTRY(lfn) || !(eFAT_ATTR_LONG_NAME == lfn->attr))
      {
#if 0 /* After using ne->skip_ents this code make a bug. */
      if (eFAT_ATTR_LONG_NAME == lfn->attr)
        lfn++; /* In case of LFN, we can skip a entry at the least. */
#endif

      ne_idx = 0;
      continue;
      }

    /* Compare a entry's name to find 'ne' in 'lfn' */
    if (!fat_cmp_lfn_entry(lfn, ne, ne_idx))
      {
      lfn += lfn->idx & FAT_LFN_IDX_MASK;
      ne_idx = 0;
      continue;
      }

    if (0 == ne_idx)
      {
      fe->parent_sect = sect_no;
      fe->parent_ent_idx = (uint8_t) (lfn - (fat_lfnent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
      }

    if (++ne_idx == entries)
      {
      /* setup LFN's short-dir-entry */
      if (lfn == end_lfn)
        {
        lim_rel_csector(ce);

        /* LFN's short-entry exists the next sector */
        sect_no = fat_get_next_sectno_16root(fvi, fe, sect_no);
        if (eFAT_EOF == sect_no)
          return e_eos;

        ce = lim_get_csector(fe->vol_id, sect_no);
        if (NULL == ce) return get_errno();
        de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
        }
      else
        de = (fat_dirent_t *) (lfn + 1);

      if (IS_DELETED_ENTRY(de))
        {
#if defined( DBG )
        break();
#endif
        /* It was deleted by LFN's short-entry */
        ne_idx = 0;
        continue;
        }

      fe->lfn_shortent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
      fe->lfn_short_sect = sect_no;
      memcpy(&fe->dir, de, sizeof (fat_dirent_t));
      lim_rel_csector(ce);
      return s_ok;
      }
    }

End:
  ne->skip_ents = (uint16_t) (lfn - end_lfn) - 1;
  lim_rel_csector(ce);
  /* Return index of the accumulated LFN which is corresponded. */
  if (ne_idx) return ne_idx;
  return rtn;
  }

/*
 Name: __fat_sect_find_name
 Desc:  Find appreciate name from both of long and short entries in a specific sector.
 Params:
   - fe: Pointer to file-entry.
   - ne: Point to lfn_nameent_t structure.
   - sect_no: Sector number which will be searched in.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static result_t __fat_sect_find_name(fat_fileent_t *fe, lfn_nameent_t *ne,
                                    uint32_t sect_no)
  {
  fat_volinfo_t *fvi;
  lim_cacheent_t *ce;
  fat_lfnent_t *lfn, *end_lfn;
  fat_dirent_t *de;
  int32_t rtn = e_noent;

  fvi = GET_FAT_VOL(fe->vol_id);

  ce = lim_get_csector(fe->vol_id, sect_no);
  if (NULL == ce) return get_errno();
  lfn = (fat_lfnent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);

  end_lfn = lfn + fvi->br.ents_per_sect - 1/*start index 0*/;
#if defined( DBG )
  if (1 != ne->entries)
    break();
#endif

  lfn += ne->skip_ents;

  /* 'de' indicates first directory-entry in sector. */
  for (/*no init*/; lfn <= end_lfn; lfn++)
    {
    if (IS_EMPTY_ENTRY(lfn))
      {
      rtn = e_eos;
      goto End;
      }
    else if (IS_DELETED_ENTRY(lfn) /* || !(eFAT_ATTR_LONG_NAME == lfn->attr) */)
      {

      continue;
      }
    else if (!(eFAT_ATTR_LONG_NAME == lfn->attr))
      {
      de = (fat_dirent_t *) lfn;

      /* If entry found, */
      if (!memcmp(de->name, fe->dir.name, FAT_SHORTNAME_SIZE))
        {
        fe->parent_sect = sect_no;
        fe->parent_ent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));

        memcpy(&fe->dir, de, sizeof (fat_dirent_t));
        fe->flag &= (~eFILE_LONGENTRY);
        lim_rel_csector(ce);
        return s_ok;
        }
      continue;
      }

    /* Compare a entry's name to find 'ne' in 'lfn' */
    if (!fat_cmp_lfn_entry(lfn, ne, 0))
      {
      lfn += lfn->idx & FAT_LFN_IDX_MASK;
      continue;
      }

    fe->parent_sect = sect_no;
    fe->parent_ent_idx = (uint8_t) (lfn - (fat_lfnent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));

    /* setup LFN's short-dir-entry */
    if (lfn == end_lfn)
      {
      lim_rel_csector(ce);

      /* LFN's short-entry exists the next sector */
      sect_no = fat_get_next_sectno_16root(fvi, fe, sect_no);
      if (eFAT_EOF == sect_no)
        return e_eos;

      ce = lim_get_csector(fe->vol_id, sect_no);
      if (NULL == ce) return get_errno();
      de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
      }
    else
      de = (fat_dirent_t *) (lfn + 1);

    if (IS_DELETED_ENTRY(de))
      {
#if defined( DBG )
      break();
#endif
      continue;
      }

    fe->lfn_shortent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
    fe->lfn_short_sect = sect_no;
    memcpy(&fe->dir, de, sizeof (fat_dirent_t));
    fe->flag |= (eFILE_LONGENTRY);
    lim_rel_csector(ce);
    return s_ok;
    }


End:
  ne->skip_ents = (uint16_t) (lfn - end_lfn) - 1;
  lim_rel_csector(ce);
  /* Return index of the accumulated LFN which is corresponded. */
  return rtn;
  }

/*
 Name: fat_lookup_long
 Desc: Find long name entry in volume.
 Params:
   - fvi: Pointer to fat_volinfo_t structure has information of volume.
   - fe: Pointer to the File-Entry to be searched.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_lookup_long(fat_volinfo_t *fvi, fat_fileent_t *fe)
  {
  lfn_nameent_t lfn_ne;
  uint32_t parent_clust,
    sect_no,
    ne_idx; /* name-entry's index */
  int32_t rtn;


  /* Make LFN from long-name */
  fat_make_lfn_name_entry(&lfn_ne, fe->name);

  parent_clust = fe->parent_clust;
  ne_idx = 0;

  if (FAT_ROOT_CLUST == parent_clust && eFAT16_SIZE == fvi->br.efat_type)
    {
    /* Search sequentially. ( FAT16's Root-Directory doesn't use FAT table.) */
    for (sect_no = fvi->br.first_root_sect; sect_no <= fvi->br.last_root_sect; sect_no++)
      {
      /* Find long name entry in sector. */
      rtn = __fat_sect_find_long(fe, &lfn_ne, ne_idx, sect_no);

      if (s_ok == rtn) return s_ok;
      else if (e_noent == rtn)
        {
        ne_idx = 0;
        continue;
        }
      else if (e_eos == rtn)
        return set_errno(e_noent);
      else if (0 < rtn) ne_idx = rtn;
      else return set_errno(rtn);
      }
    }
  else
    {
    sect_no = D_CLUST_2_SECT(fvi, parent_clust);

    while (sect_no <= fvi->br.last_data_sect)
      {
      /* Find long name entry in sector. */
      rtn = __fat_sect_find_long(fe, &lfn_ne, ne_idx, sect_no);

      if (s_ok == rtn) return s_ok;
      else if (e_noent == rtn)
        {
        ne_idx = 0;
        rtn = fat_get_next_sectno(fe->vol_id, sect_no);
        if (0 > rtn) return rtn;
        sect_no = rtn;
        if (eFAT_EOF == sect_no)
          return set_errno(e_noent);
        continue;
        }
      else if (e_eos == rtn)
        return set_errno(e_noent);
      else if (0 < rtn)
        {
        ne_idx = rtn;
        rtn = fat_get_next_sectno(fe->vol_id, sect_no);
        if (0 > rtn) return rtn;
        sect_no = rtn;
        if (eFAT_EOF == sect_no)
          return set_errno(e_noent);
        }
      else return set_errno(rtn);
      }
    }

  return set_errno(e_noent);
  }

/*
 Name: fat_lookup_short
 Desc: Find short name entry in volume.
 Params:
   - fvi: Pointer to fat_volinfo_t structure has information of volume.
   - fe: pointer to the File-Entry to be searched.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_lookup_short(fat_volinfo_t *fvi, fat_fileent_t *fe)
  {
  uint32_t vol_id,
    parent_clust,
    sect_no;
  int32_t rtn;


  parent_clust = fe->parent_clust;

  if (FAT_ROOT_CLUST == parent_clust && eFAT16_SIZE == fvi->br.efat_type)
    {
    /* Search sequentially. ( FAT16's Root-Directory doesn't use FAT table.) */
    for (sect_no = fvi->br.first_root_sect; sect_no <= fvi->br.last_root_sect; sect_no++)
      {
      /* Find short name entry in sector. */
      rtn = __fat_sect_find_short(fe, sect_no);
      if (s_ok == rtn) return s_ok;
      if (e_eos == rtn)
        return set_errno(e_noent);
      else if (e_noent != rtn)
        return rtn;
      }
    }
  else
    {
    vol_id = fvi->vol_id;
    /* Get sector number from cluster number. */
    sect_no = D_CLUST_2_SECT(fvi, parent_clust);

    while (sect_no <= fvi->br.last_data_sect)
      {
      /* Find short name entry in sector. */
      rtn = __fat_sect_find_short(fe, sect_no);
      if (s_ok == rtn) return s_ok;
      if (e_eos == rtn)
        return set_errno(e_noent);
      else if (e_noent != rtn)
        return rtn;

      rtn = fat_get_next_sectno(vol_id, sect_no);
      if (0 > rtn) return rtn;
      sect_no = rtn;
      if (eFAT_EOF == sect_no) return set_errno(e_noent);
      }
    }

  return set_errno(e_noent);
  }

/*
 Name: fat_lookup_name
 Desc: Find appreciate name from both of long and short entries in volume.
 Params:
   - fvi: Pointer to fat_volinfo_t structure has information of volume.
   - fe: Pointer to the File-Entry to be searched.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_lookup_name(fat_volinfo_t *fvi, fat_fileent_t *fe)
  {
  lfn_nameent_t lfn_ne;
  uint32_t parent_clust,
    sect_no;
  int32_t rtn;


  /* Make LFN from long-name */
  fat_make_lfn_name_entry(&lfn_ne, fe->name);

  parent_clust = fe->parent_clust;

  if (FAT_ROOT_CLUST == parent_clust && eFAT16_SIZE == fvi->br.efat_type)
    {
    /* Search sequentially. ( FAT16's Root-Directory doesn't use FAT table.) */
    for (sect_no = fvi->br.first_root_sect; sect_no <= fvi->br.last_root_sect; sect_no++)
      {
      /* Find long or short name entry in sector. */
      rtn = __fat_sect_find_name(fe, &lfn_ne, sect_no);

      if (s_ok == rtn) return s_ok;
      else if (e_noent == rtn)
        {
        continue;
        }
      else if (e_eos == rtn)
        return set_errno(e_noent);
      else return set_errno(rtn);
      }
    }
  else
    {
    sect_no = D_CLUST_2_SECT(fvi, parent_clust);

    while (sect_no <= fvi->br.last_data_sect)
      {
      /* Find long or short name entry in sector. */
      rtn = __fat_sect_find_name(fe, &lfn_ne, sect_no);

      if (s_ok == rtn) return s_ok;
      else if (e_noent == rtn)
        {
        rtn = fat_get_next_sectno(fe->vol_id, sect_no);
        if (0 > rtn) return rtn;
        sect_no = rtn;
        if (eFAT_EOF == sect_no)
          return set_errno(e_noent);
        continue;
        }
      else if (e_eos == rtn)
        return set_errno(e_noent);
      else return set_errno(rtn);
      }
    }

  return set_errno(e_noent);
  }

/*
 Name: fat_lookup_entry
 Desc: Find a entry Search from the Root-Directory.
 Params:
   - arg: Pointer to fat_arg_t structure represents elements in path.
   - fe: Pointer to the File-Entry.
   - ent_type: Type of file-entry. One of the following symbols.
               eFAT_FILE - indicates a file type.
               eFAT_DIR - indicates a directory type.
               eFAT_ALL - indicates a file or directory type.
 Returns:
   int32_t  >=0 on success. The returned value is number of elements in path.
            < 0 on fail.
 Caveats: None.
 */

result_t fat_lookup_entry(fat_arg_t *arg, fat_fileent_t *fe, int32_t ent_type)
  {
  fat_volinfo_t *fvi;
  int32_t rtn;
  uint32_t argc, i;
  bool is_8_3, is_rplc;


  fvi = GET_FAT_VOL(fe->vol_id);
  argc = arg->argc;

  /* search from root-dir */
  if (eFAT16_SIZE == fvi->br.efat_type)
    SET_OWN_CLUST(&fe->dir, FAT_ROOT_CLUST);
  else /* eFAT32_SIZE : first_data_sect is root-directory */
    SET_OWN_CLUST(&fe->dir, D_SECT_2_CLUST(fvi, fvi->br.first_data_sect));

  for (i = 0; i < argc; i++)
    {
    fe->parent_clust = GET_OWN_CLUST(&fe->dir);

    /* Get a name of directory or file. */
    rtn = fat_cp_name(fe, arg->argv[i], ent_type);
    if (0 > rtn) return rtn;

#if defined( CPATH )
    /* get path-cache */
    if (i != (argc - 1)/*path-entries*/ || eFAT_DIR == ent_type)
      {
      rtn = path_get_centry(fe);
      if (s_ok == rtn) continue;
      }
#endif

    /* Make short name and change the short name to DOS format. */
    rtn = fat_make_shortname(fe, &is_8_3, &is_rplc);
    if (0 > rtn) return rtn;

#if defined( LOG )
    if ((1 == i)/*It is in root?*/ && !(fe->dir.attr & eFAT_ATTR_DIR)/*It is file?*/)
      {
      if (fat_is_system_file(fe, true))
        return set_errno(e_perm);
      }
#endif

    /* Searching the discriminated entry according to file name's type */
    if (is_8_3 && !is_rplc)
      {
      rtn = fat_lookup_name(fvi, fe);
      }
    else if ((uint32_t) eFILE_LONGENTRY & fe->flag)
      {
      rtn = fat_lookup_long(fvi, fe);
      }
    else
      {
      rtn = fat_lookup_short(fvi, fe);
      }
    if (0 > rtn) break;

    if (!(fe->dir.attr & eFAT_ATTR_DIR) && i != (argc - 1))
      return set_errno(e_notdir);

#if defined( CPATH )
    if ((fe->dir.attr & eFAT_ATTR_DIR) &&
        (i != (argc - 1)/*path-entries*/ || eFAT_DIR == ent_type))
#if 1
      /* Copy file entry to path-cache entry & It adds in cache list. */
      path_store_centry(fe);
#else
      path_update_store_centry(fe); /* dir that has tild character */
#endif
#endif
    }

  /* The entry was searched? */
  if (i == argc)
    {
    /* If the attribute of searched entry is not matched with one
       specified by the 'ent_type' parameter. */
    if ((eFAT_DIR == ent_type) && !(fe->dir.attr & eFAT_ATTR_DIR))
      return set_errno(e_notdir);
    else if ((eFAT_FILE == ent_type) && (fe->dir.attr & eFAT_ATTR_DIR))
      return set_errno(e_isdir);
    }

  return i; /* found count of path's elements */
  }

/*
 Name: fat_update_sentry
 Desc: Calculate position of Directory-Entry and then update it physically.
 Params:
   - fe: A pointer to the File-Entry.
   - flush: Flag to determine whether Lim Cache flushes physically or not.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_update_sentry(fat_fileent_t *fe, bool flush)
  {
  lim_cacheent_t *ce;
  fat_dirent_t *de;
  int32_t sect_no,
    ent_idx,
    rtn = s_ok;
  fat_volinfo_t *fvi;
  fvi = GET_FAT_VOL(fe->vol_id);


  /* Calculate position of Directory-Entry from File-Entry. */
  if ((uint32_t) eFILE_LONGENTRY & fe->flag)
    {
    sect_no = fe->lfn_short_sect;
    ent_idx = fe->lfn_shortent_idx;
    }
  else
    {
    sect_no = fe->parent_sect;
    ent_idx = fe->parent_ent_idx;
    }

  /* Read buffer which the Directory-Entry is in. */
  ce = lim_get_csector(fe->vol_id, sect_no);
  if (NULL == ce) return get_errno();
  de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
  de += ent_idx;

  /* Copy Directory-Entry to buffer. */
  memcpy(de, &fe->dir, sizeof (fat_dirent_t));

  /* Update buffer to the cache. According to 'flush', update physically. */
  lim_mark_dirty_csector(ce, NULL);
  if (flush)
    rtn = lim_flush_csector(ce);
  lim_rel_csector(ce);

  return rtn;
  }

/*
 Name: __fat_alloc_odir
 Desc: Allocate memory for a opening directory.
 Params:
   - fe: A pointer to the File-Entry.
 Returns:
   DIR_t*  value on success. The returned value is a pointer to
                       allocated DIR_t structure.
                 NULL on fail.
 Caveats: When it tried to open a directory, this function is called.
 */

static dir_t* __fat_alloc_odir(fat_fileent_t *fe)
  {
  list_head_t *list;
  odir_t *odir;


  list = &odir_free_lru;
  if (list->next == list)
    /* Too many allocated. */
    return NULL;

  /* Allocate free odir entry. */
  odir = list_entry(list->next, odir_t, head);

  /* Move to alloc-list. */
  list = &odir_alloc_lru;
  list_move_tail(list, &odir->head);

  odir->vol_id = fe->vol_id;
  odir->parent_sect = fe->parent_sect;
  odir->parent_ent_idx = fe->parent_ent_idx;
  odir->dir.dirent.d_name[0] = '\0';

  return &odir->dir;
  }

/*
 Name: __fat_lookup_odir
 Desc:
 Params:
   - fe: A pointer to the File-Entry.
 Returns:
   odir_t*  value on success. The returned value is a pointer to allocated odir_t structure.
            NULL on fail.
 Caveats:
 */

static odir_t *__fat_lookup_odir(fat_fileent_t *fe)
  {
  list_head_t *list;
  odir_t *pos;


  list = &odir_alloc_lru;

  /* Check the the file/directory should have been opened already. */

  /* If it is being opened already, this returns the opened file entry.*/
  list_for_each_entry_rev(odir_t, pos, list, head)
    {
    if ((pos->parent_sect == fe->parent_sect) &&
        (pos->parent_ent_idx == fe->parent_ent_idx) &&
        (pos->vol_id == fe->vol_id))
      {
      return pos;
      }
    }

  return (odir_t *) NULL;
  }

/*
 Name: __fat_free_odir
 Desc:  Free allocated memory for a opened Directory Stream.
 Params:
   - dir: Pointer to allocated DIR_t structure.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: When it tried to close a opened directory, this function is called.
 */

static result_t __fat_free_odir(dir_t *dir)
  {
  list_head_t *list;
  odir_t *odir;


  fsm_assert3(NULL != dir);

  odir = list_entry(dir, odir_t, dir);

  fsm_assert2((uint8_t *) odir >= (uint8_t *) & fat_odir[0]);
  fsm_assert2((uint8_t *) odir < (uint8_t *) & fat_odir[FAT_MAX_ODIR]);

  /* Move to free-list. */
  list = &odir_free_lru;
  list_move(list, &odir->head);

  return s_ok;
  }

/*
 Name: fat_mkdir
 Desc: Make a specitic directory.
 Params:
   - vol_id: The ID of volume.
   - path: Pointer to the null-terminated path name of the directory to be
           made.
   - mode: Reserved for the future.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_mkdir(int32_t vol_id, const char *path, mod_t mode)
  {
  fat_fileent_t *fe;
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  int32_t rtn;
  uint32_t own_clust;
  uint32_t clust_list[1/*last cluster*/ + 1/*dir's cluster*/];
#if defined( LOG )
  int32_t cl_id = -1;
#endif


  rtn = fat_parse_path(path_buf, &arg, path);

  if (!arg.argc) //SSC_TEST
    {
    set_errno(e_path);
    return -1;
    }

  if (s_ok != rtn) return rtn;

  fat_lock();

  if (NULL == (fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return -1;
    }

  /* Search if the allocated entry already exists. */
  rtn = fat_lookup_entry(&arg, fe, eFAT_DIR);
  if (0 > rtn) goto End2;

  /* If the path already existes, the number of arguments is equal to the
     number of the path's entries. */
  if (arg.argc == rtn)
    {
    rtn = set_errno(e_exist);
    goto End2;
    }
    /* The number of path's entries should be less than the number of arguments
       Because, the high position folder must exist and there is a possibility
       which it will make.
     */
  else if (arg.argc - 1 != rtn)
    {
    rtn = set_errno(e_notdir); /* The path doesn't exist. */
    goto End2;
    }

  fe->parent_clust = GET_OWN_CLUST(&fe->dir);

  /* If 'clust_list[0]' is 0, it means first allocating. */
  clust_list[0] = 0;

  /* Flush the LIM data cache to the physical device. */
  rtn = lim_flush_cdsector();
  if (0 > rtn) goto End;

  /* Get free clusters */
  rtn = fat_map_alloc_clusts(vol_id, 1, &clust_list[1]);
  if (0 > rtn) goto End;

  own_clust = clust_list[1];

  /* Allocate own cluster */
  fe->dir.attr = eFAT_ATTR_DIR | eFAT_ATTR_ARCH;
  SET_OWN_CLUST(&fe->dir, own_clust);
  fe->dir.filesize = 0;
#ifdef SFILEMODE
  /* Assigne file-mode */
  __fat_set_file_information(fe, mode, fe->dir.attr);
#endif
  /* Set position and count of the entry which it will adds */
  rtn = fat_alloc_entry_pos(fe);
  if (0 > rtn)
    {
    fat_map_free_clusts(vol_id, 1, &clust_list[1]);
    goto End;
    }

#if defined( LOG )
  rtn = fat_log_on(fe, NULL);
  if (0 > rtn) goto End;
  cl_id = rtn;
#endif

  /* Renewed one FAT entry */
  rtn = fat_stamp_clusts(vol_id, 1, clust_list);
  if (0 > rtn) goto End;

  /* sync fat table (flushing) */
  rtn = fat_sync_table(vol_id, true);
  if (0 > rtn) goto End;

  /* create own "." & ".." dir */
  rtn = fat_create_dots_dir(fe, own_clust);

  /* Write entry */
  rtn |= fat_creat_entry(fe, true);
  if (0 > rtn)
    {
    fat_map_free_link_clusts(vol_id, own_clust, 0);
    goto End;
    }

End:
#if defined( LOG )
  if (0 <= cl_id)
    fat_log_off(vol_id, cl_id);
#endif

#if defined( CPATH )
  if (s_ok == rtn)
    /* The path-cache entry copies file entry and it adds in the path-cache's list. */
    path_store_centry(fe);
#endif

  tr_fat_inc_unlink_cnt(vol_id);

End2:
  /* It returns the file entry which is allocated. */
  fat_free_file_entry(fe);

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_rmdir
 Desc: Removes the specified directory, checking if it's empty.
 Params:
   - vol_id: Volume's ID to be removed the directory.
   - path: Directory entry path.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_rmdir(int32_t vol_id, const char *path)
  {
  fat_fileent_t *fe;
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  int32_t rtn;


  rtn = fat_parse_path(path_buf, &arg, path);
  if (s_ok != rtn) return rtn;

  fat_lock();

  if (NULL == (fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return -1;
    }

  rtn = fat_lookup_entry(&arg, fe, eFAT_DIR);
  if (0 > rtn) goto End;

  if (arg.argc != rtn)
    {
    if (arg.argc - 1 != rtn)
      set_errno(e_notdir);
    rtn = -1;
    goto End;
    }

  if (NULL != __fat_lookup_odir(fe))
    {
    rtn = set_errno(e_busy);
    goto End;
    }

  /* check a child entry */
  rtn = fat_hasnt_entry(fe);
  if (0 > rtn)
    {
    rtn = set_errno(rtn); /* have one more entries */
    goto End;
    }

  /* The file entry unlink.*/
  rtn = fat_unlink_entry(fe);

#if defined( CPATH )
  if (s_ok == rtn)
    path_del_centry(fe);
#endif

End:
  fat_free_file_entry(fe);

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_opendir
 Desc: Open a direectory stream.
 Params:
   - vol_id: The ID of volume which the directory will be opened.
   - path: Pointer to the null-terminated path name of the directory to be
           opened.
 Returns:
   DIR_t*  value on success. The value returned is a pointer to a
                       DIR_t. This DIR_t describes a directory.
                 NULL on fail.
 Caveats: None.
 */

dir_t* fat_opendir(int32_t vol_id, const char *path)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_fileent_t *fe;
  dir_t *debuf = NULL;
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  int32_t own_clust,
    own_sect,
    rtn;


  rtn = fat_parse_path(path_buf, &arg, path);
  if (s_ok != rtn) return NULL;

  fat_lock();

  if (NULL == (fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return NULL;
    }

  /* Check the directory should have existed actually */
  if (arg.argc)
    { /*isn't the Root-Directory. */
    rtn = fat_lookup_entry(&arg, fe, eFAT_DIR);
    if (0 > rtn) goto Error;

    if (arg.argc != rtn)
      {
      if (arg.argc - 1 != rtn)
        set_errno(e_notdir);
      goto Error;
      }

    if (NULL == (debuf = __fat_alloc_odir(fe)))
      {
      set_errno(e_ndir);
      goto Error;
      }

    own_clust = GET_OWN_CLUST(&fe->dir);
    own_sect = D_CLUST_2_SECT(fvi, own_clust);
    fe->parent_ent_idx = 0; /* Search from the first entry */
    }
  else
    {/* Root-Directory */
    /* Search from the Root-Directory */
    if (eFAT16_SIZE == fvi->br.efat_type)
      own_sect = fvi->br.first_root_sect;
    else
      /* At FAT32 file system, 'first_data_sect' is the Root-Directory. */
      own_sect = fvi->br.first_data_sect;

    own_clust = FAT_ROOT_CLUST;
    /* search from second entry (first entry is volume entry) */
    fe->parent_ent_idx = 0xFF;
    fe->dir.attr = eFAT_ATTR_DIR;
    fe->dir.filesize = 0;
    fe->dir.ctime = 0;
    fe->dir.cdate = 0;
    fe->dir.adate = 0;
    fe->dir.wdate = 0;
    fe->dir.wtime = 0;
    fe->name[0] = '\0';
    }

  fe->parent_clust = own_clust;
  fe->parent_sect = own_sect;

  if (NULL == debuf)
    {
    if (NULL == (debuf = __fat_alloc_odir(fe)))
      {
      set_errno(e_ndir);
      goto Error;
      }
    }
  debuf->fd = fe->idx;
  debuf->attr = fe->dir.attr;
  debuf->filesize = fe->dir.filesize;
  fat_ftime_2_gtime(&debuf->ctime, &debuf->atime, &debuf->wtime, &fe->dir);
  strcpy(debuf->dirent.d_name, fe->name);

  fat_unlock();
  return debuf;

Error:
  fat_free_file_entry(fe);

  fat_unlock();
  return NULL;
  }

/*
 Name: fat_readdir
 Desc: Read a directory entry information from a given directory stream.
 Params:
   - debuf: Pointer to a DIR_t that refers to the open directory stream to
         be read. It used to offer information about directory stream.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

dirent_t* fat_readdir(dir_t *debuf)
  {
  fat_fileent_t *fe;
  dirent_t* dirent;
  int32_t rtn;


  /* Gets the entry's information from the file descriptor of the opened
     Directory-Entry */
  fe = fat_get_file_entry_fromid(GET_FD(debuf->fd));
  if (NULL == fe)
    {
    set_errno(e_badf);
    return NULL;
    }

  fat_lock();

  rtn = fat_get_entry_info(fe, (fat_entattr_t *) NULL);
  if (s_ok == rtn)
    {
    debuf->attr = fe->dir.attr;
#ifdef SFILEMODE
    debuf->filemode = FROM_NTRES(fe->dir.char_case);
#endif
    debuf->filesize = fe->dir.filesize;
    fat_ftime_2_gtime(&debuf->ctime, &debuf->atime, &debuf->wtime, &fe->dir);
    /* Set the directory entry. */
    dirent = &(debuf->dirent);
    strcpy(dirent->d_name, fe->name);
    }
  else
    {
    if (e_eos != rtn)
      set_errno(rtn);
    dirent = NULL;
    }

  fat_unlock();
  return dirent;
  }

/*
 Name: fat_rewinddir
 Desc: Set the directory position to the beginning of the directory entries in
       directory stream.
 Params:
   - debuf: Directory entry buffer.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_rewinddir(dir_t *debuf)
  {
  fat_volinfo_t *fvi;
  fat_fileent_t *fe;
  int32_t parent_sect;

  /* Gets the entry's information from the file descriptor of the opened
     Directory-Entry */
  fe = fat_get_file_entry_fromid(GET_FD(debuf->fd));
  if (NULL == fe)
    return set_errno(e_inval);

  fat_lock();

  fvi = GET_FAT_VOL(fe->vol_id);

  /* Reset entry position */
  if (FAT_ROOT_CLUST == fe->parent_clust)
    {
    /* Search from the second entry. (first entry is volume entry.) */
    fe->parent_ent_idx = 1;

    if (eFAT16_SIZE == fvi->br.efat_type)
      parent_sect = fvi->br.first_root_sect;
    else
      /* At FAT32 file system, 'first_data_sect' is the Root-Directory. */
      parent_sect = fvi->br.first_data_sect;
    }
  else
    {
    /* Search from the first entry. */
    fe->parent_ent_idx = 0;
    parent_sect = D_CLUST_2_SECT(fvi, fe->parent_clust);
    }

  fe->parent_sect = parent_sect;

  fat_unlock();
  return s_ok;
  }

/*
 Name: fat_closedir
 Desc: Close a specific directory stream.
 Params:
   - debuf: Pointer to a DIR_t that refers to the opened directory
            stream to be close.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_closedir(dir_t *debuf)
  {
  fat_fileent_t *fe;


  /* Gets the entry's information from file ID of the opened directory-entry */
  fe = fat_get_file_entry_fromid(GET_FD(debuf->fd));
  if (NULL == fe)
    return set_errno(e_inval);

  fat_lock();

  /* Free to allocated entry */
  fat_free_file_entry(fe);

  /* It converts the buffer for the directory in Free condition. */
  if (0 > __fat_free_odir(debuf))
    return set_errno(e_inval);

  fat_unlock();
  return s_ok;
  }




#if 0 /* delete all files in directory. (directory don't delete) */

/*
 Name: __fat_cleandir
 Desc: delete all files in directory. (directory don't delete)
 Params:
   - fe : File entry pointer.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_cleandir(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
  fat_entattr_t next_ep;
  int32_t own_clust,
    rtn;


  fvi = GET_FAT_VOL(fe->vol_id);

  own_clust = GET_OWN_CLUST(&fe->dir);
  /* prevent that parent_clust is accessed in fat_get_next_sectno_16root to check for root-dir */
  fe->parent_clust = own_clust;
  fe->parent_sect = D_CLUST_2_SECT(fvi, own_clust);
  fe->parent_ent_idx = 2; /* skip dot & dot dot */

  while (1)
    {
    /* get entry info */
    rtn = fat_get_entry_info(fe, &next_ep);

    if (s_ok == rtn)
      {
      if (!(eFAT_ATTR_DIR & fe->dir.attr))
        {
        if (!fat_is_system_file(fe, false))
          {
          /* Delete entry. */
          rtn = fat_unlink_entry(fe);
          if (0 > rtn) return rtn;
          }

        fe->parent_sect = next_ep.parent_sect;
        fe->parent_ent_idx = next_ep.parent_ent_idx;
        }
      }
    else if (e_eos == rtn)
      return s_ok;
    else
      return -1;
    }
  }
#else /* delete all entries ( file & dir ) */

/*
 Name: __fat_cleandir
 Desc: Remove all lower entries included in the entry specified by the 'fe'
       parameter.
 Params:
   - fe: Pointer to the fat_fileent_t structure means entry.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static result_t __fat_cleandir(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(fe->vol_id);
  fat_entattr_t ea[2];
  int32_t ea_idx,
    rtn;
#define cur_ea (&ea[ea_idx])
#define next_ea (&ea[ea_idx^1])
#define INIT_EA() (ea_idx=0)
#define EXCG_EA() (ea_idx^=1) /* Exchange cur_ea & next_ea. */


  /*
  Check if file-access is requested from higher priority task.
  if then, we enter suspend-state.
  NOTE : this leads to priority inversion.
   */
  fat_unlock();
  fat_lock();

  INIT_EA();
  cur_ea->parent_sect = fe->parent_sect;
  cur_ea->parent_ent_idx = fe->parent_ent_idx;

  do
    {
    /* Get information about current entry and next entry. */
    rtn = __fat_get_entry_pos(fe, cur_ea, next_ea);
    if (e_eos == rtn) break;
    else if (0 > rtn) break;

    if (eFAT_ATTR_DIR & cur_ea->attr)
      {
      /* Check the dir entry should have been becoming the open already.*/
      fe->parent_ent_idx = cur_ea->parent_ent_idx;
      fe->parent_sect = cur_ea->parent_sect;
      if (__fat_lookup_odir(fe))
        return set_errno(e_busy);

      /* Set the own cluster number to the parent cluster to search the
         lower enties. */
      fe->parent_sect = D_CLUST_2_SECT(fvi, cur_ea->own_clust);
      fe->parent_ent_idx = 2; /* skip dot & dot-dot directory */

      rtn = __fat_cleandir(fe);
      if (0 > rtn) return rtn;
      }

    /* Setup information of entry for deletion. */
    fe->flag = cur_ea->u.flag;
    fe->dir.attr = cur_ea->attr;
    SET_OWN_CLUST(&fe->dir, cur_ea->own_clust);
    fe->parent_ent_idx = cur_ea->parent_ent_idx;
    fe->parent_sect = cur_ea->parent_sect;

    if (!fat_is_system_file(fe, false))
      {
      /* Delete entry. */
      rtn = fat_unlink_entry(fe);
      if (0 > rtn) return rtn;
      }

    EXCG_EA();
    }
  while (e_eos != rtn);

  if (e_eos == rtn)
    return s_ok;
  else
    return rtn;
  }
#endif

/*
 Name: fat_cleandir
 Desc: Remove lower entries included in the directory specified by the 'path'
       parameter.
 Params:
   - vol_id: The ID of volume included in the removed directory.
   - path: Pointer to the null-terminated path name of the directory to be
           removed.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_cleandir(int32_t vol_id, const char *path)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_fileent_t *fe;
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  int32_t own_clust,
    flag,
    rtn;


  rtn = fat_parse_path(path_buf, &arg, path);
  if (s_ok != rtn) return rtn;

  fat_lock();

  if (NULL == (fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return -1;
    }

  if (arg.argc)
    {
    rtn = fat_lookup_entry(&arg, fe, eFAT_DIR);
    if (0 > rtn) goto End;

    if (arg.argc != rtn)
      {
      if (arg.argc - 1 != rtn)
        set_errno(e_notdir); /* Path not exist. */
      rtn = -1;
      goto End;
      }

    own_clust = GET_OWN_CLUST(&fe->dir);
    fe->parent_sect = D_CLUST_2_SECT(fvi, own_clust);
    /* Skip dot('.') direcotry and dot-dot('..') directory. */
    fe->parent_ent_idx = 2;
    }
    /* In case of the Root-directory. */
  else
    {
    own_clust = FAT_ROOT_CLUST;
    if (eFAT16_SIZE == fvi->br.efat_type)
      fe->parent_sect = fvi->br.first_root_sect;
    else
      /* At FAT32, the first data sector is for the Root-Directory. */
      fe->parent_sect = fvi->br.first_data_sect;
    /* Search from second entry. The first entry is for volume. */
    fe->parent_ent_idx = 1;
    }

  flag = fe->flag;
  rtn = __fat_cleandir(fe);
  fe->flag = flag;

#if defined( CPATH )
  /* The directory is removed, so we should initialize the cache in the volume
     again. */
  path_reinit_cache_vol(vol_id);
#endif

End:
  fat_free_file_entry(fe);

  fat_unlock();
  return rtn;
  }

/*
 Name: __fat_get_dir_own_size
 Desc: Obtain the size of a cluster.
 Params:
   - vol_id: The ID of volume includes the cluster which is the 'own_clust'
             parameter.
   - own_clust: The cluster number to get the size.
 Returns:
   int32_t  >=0 on success. The returned value is the size of cluster.
            < 0 on fail.
 Caveats: None.
 */

static result_t __fat_get_dir_own_size(uint32_t vol_id, uint32_t own_clust)
  {
  fat_volinfo_t *fvi;
  uint32_t clust_no,
    clust_cnt,
    alloc_size;
  int32_t rtn;


  fvi = GET_FAT_VOL(vol_id);
  clust_no = own_clust;

  /* At FAT file system, the cluster '0' isn't used. */
  if (0 == clust_no) return 0;

  clust_cnt = 1;

  /* Search all clusters included in 'own_clust' cluster. */
  while (1)
    {
    rtn = fat_get_next_clustno(vol_id, clust_no);
    if (eFAT_EOF == rtn)
      break;
    fsm_assert2(fvi->fat_mounted ? (eFAT_FREE != rtn) : true);
    clust_no = rtn;
    clust_cnt++;
    }

  /* Two to the power of (fvi->br.bits_per_clust) is the number of bytes per
     cluaster. */
  alloc_size = clust_cnt << fvi->br.bits_per_clust;
  return alloc_size;
  }

/*
 Name: __fat_get_dir_stat
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


static result_t __fat_get_dir_stat(fat_fileent_t *fe, fat_statdir_t *statbuf)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(fe->vol_id);
  fat_entattr_t ent_attr;
  int32_t clusts,
    rtn;


  /*
  Check whether file-access is requested from higher priority task or not.
  If so, we enter suspend-state.
  NOTE : this leads to priority inversion.
   */
  fat_unlock();
  fat_lock();

  ent_attr.parent_ent_idx = fe->parent_ent_idx;
  ent_attr.parent_sect = fe->parent_sect;

  do
    {
    /* Read the information of 'fe' directory from the physical device and
       save to the 'ent_attr'. */
    rtn = __fat_get_entry_sinfo(fe, &ent_attr);
    if (e_eos == rtn) break;
    else if (0 > rtn) break;

    /* In the case of a found Entry is directory. */
    if (eFAT_ATTR_DIR & ent_attr.attr)
      {
      /* Skip dot('.') directory and dot-dot('..') directory */
      fe->parent_ent_idx = 2;
      fe->parent_sect = D_CLUST_2_SECT(fvi, ent_attr.own_clust);
      /* Read the information of directories under the 'fe' directory. */
      rtn = __fat_get_dir_stat(fe, statbuf);
      if (0 > rtn) return rtn;
      statbuf->alloc_size += __fat_get_dir_own_size(fe->vol_id, ent_attr.own_clust);
      }
      /* In the case of a found entry is file. */
    else
      {
      statbuf->entries++;
      statbuf->size += ent_attr.u.filesize;
      clusts = ent_attr.u.filesize / fvi->br.bits_per_clust;
      statbuf->alloc_size += clusts << fvi->br.bits_per_clust;
      }
    }
  while (e_eos != rtn);

  if (e_eos == rtn)
    return s_ok;
  else
    return rtn;
  }

/*
 Name: fat_statdir
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

result_t fat_statdir(int32_t vol_id, const char *path, statdir_t *statbuf)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_fileent_t *fe;
  fat_statdir_t stat_dir = {0, 0, 0};
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  int32_t own_clust,
    own_sect,
    rtn;


  rtn = fat_parse_path(path_buf, &arg, path);
  if (s_ok != rtn) return rtn;

  fat_lock();

  if (NULL == (fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return -1;
    }

  /* In this case Root-Directory is not. */
  if (arg.argc)
    {
    rtn = fat_lookup_entry(&arg, fe, eFAT_DIR);
    if (0 > rtn) goto End;

    if (arg.argc != rtn)
      {
      if (arg.argc - 1 != rtn)
        /* It means that path doesn't exist. */
        set_errno(e_notdir);
      rtn = -1;
      goto End;
      }

    own_clust = GET_OWN_CLUST(&fe->dir);
    own_sect = D_CLUST_2_SECT(fvi, own_clust);
    /* Skip dot('.') directory and dot-dot('..') directory */
    fe->parent_ent_idx = 2;
    }
    /* In this case Root-Directory, search from Root-Directory. */
  else
    {
    if (eFAT16_SIZE == fvi->br.efat_type)
      own_sect = fvi->br.first_root_sect;
    else
      /* In FAT32, the first data sector is Root-Directory. */
      own_sect = fvi->br.first_data_sect;

    own_clust = FAT_ROOT_CLUST;
    /* Search from second entry (First entry is volume entry.). */
    fe->parent_ent_idx = 1;
    }

  fe->parent_sect = own_sect;

  /* Really, get the information of fount directory. */
  rtn = __fat_get_dir_stat(fe, &stat_dir);
  if (s_ok == rtn)
    {
    statbuf->files = stat_dir.entries;
    statbuf->size = stat_dir.size;
    statbuf->alloc_size = stat_dir.alloc_size;
    }
  else
    {
    /* On fail. */
    statbuf->files = 0;
    statbuf->size = 0;
    statbuf->alloc_size = 0;
    }

  fat_ftime_2_gtime(&statbuf->ctime, &statbuf->atime, &statbuf->mtime, &fe->dir);

End:
  fat_free_file_entry(fe);

  fat_unlock();
  return rtn;
  }

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

