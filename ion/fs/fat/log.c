/* FILE: ion_log.c */
/**************************************************************************
 * Copyright (C)2009 Spansion LLC and its licensors. All Rights Reserved. 
 *
 * This software is owned by Spansion or its licensors and published by: 
 * Spansion LLC, 915 DeGuigne Dr. Sunnyvale, CA  94088-3453 ("Spansion").
 *
 * BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND 
 * BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
 *
 * This software constitutes source code for use in programming Spansion's Flash 
 * memory components. This software is licensed by Spansion to be adapted only 
 * for use in systems utilizing Spansion's Flash memories. Spansion is not be 
 * responsible for misuse or illegal use of this software for devices not 
 * supported herein.  Spansion is providing this source code "AS IS" and will 
 * not be responsible for issues arising from incorrect user implementation 
 * of the source code herein.  
 *
 * SPANSION MAKES NO WARRANTY, EXPRESS OR IMPLIED, ARISING BY LAW OR OTHERWISE, 
 * REGARDING THE SOFTWARE, ITS PERFORMANCE OR SUITABILITY FOR YOUR INTENDED 
 * USE, INCLUDING, WITHOUT LIMITATION, NO IMPLIED WARRANTY OF MERCHANTABILITY, 
 * FITNESS FOR A  PARTICULAR PURPOSE OR USE, OR NONINFRINGEMENT.  SPANSION WILL 
 * HAVE NO LIABILITY (WHETHER IN CONTRACT, WARRANTY, TORT, NEGLIGENCE OR 
 * OTHERWISE) FOR ANY DAMAGES ARISING FROM USE OR INABILITY TO USE THE SOFTWARE, 
 * INCLUDING, WITHOUT LIMITATION, ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS OF DATA, SAVINGS OR PROFITS, 
 * EVEN IF SPANSION HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  
 *
 * This software may be replicated in part or whole for the licensed use, 
 * with the restriction that this Copyright notice must be included with 
 * this software, whether used in part or whole, at all times.  
 */


/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "log.h"
#include "chkdsk.h"
#include "fat.h"
#include "vol.h"
#include "dir.h"
#include "file.h"
#include "name.h"

#include <string.h>

#if defined( CHKDISK ) && defined( LOG )
#error "Must define only one!!"
#endif


#define LOG_FILE                       "%logsys%.ion"
#define LOG_MAGIC                      0x4C4B4843
#define LOG_CLUST_PER_LOG              1  /* Number of clusters per chk. */
#define LOG_EMPTY_ENTRY                0xfffffff7ul
#define LOG_MAX_ENTRY_PER_LOG          2
#define LOG_SECT_SIZE                  FAT_ALLOW_MAX_SECT_SIZE
#define LOG_SECT_CNT                   (LOG_SECT_SIZE/FAT_ALLOW_MAX_SECT_SIZE)
#define LOG_MAX_LOG                    16

typedef struct entry_log_s
  {
  uint32_t parent_sect;
  uint8_t parent_ent_idx; /* entry offset in 'parent_sect'. */
  uint32_t own_clust;

  } entry_log_t;

typedef struct log_s
  {
  entry_log_t entry[LOG_MAX_ENTRY_PER_LOG];

  } log_t;

typedef struct log_file_s
  {
  uint8_t parent_ent_idx;
  uint32_t parent_sect;

  } log_file_t;

typedef struct log_mng_s
  {
  uint32_t sect_no;
  log_file_t log_file;
  uint8_t sector[LOG_SECT_SIZE];

  } log_mng_t;




#if defined( DBG )
#if !defined( WIN32 )
#pragma arm section zidata="NoInitData",rwdata="NoInitData"
#endif
log_dbg_t log_dbg;
#if !defined( WIN32 )
#pragma arm section zidata,rwdata
#endif
#endif

static log_mng_t fat_log_file[VOLUME_NUM];

#define log_sect_no(vol_id)                (fat_log_file[vol_id].sect_no)
#define log_get_sect_ptr(vol_id)           ((void *)&fat_log_file[vol_id].sector)
#define log_get_magic_ptr(vol_id)          ((uint32_t *)&fat_log_file[vol_id].sector)
#define log_sizeof_magic                   sizeof(uint32_t)
#define log_get_msk_ptr(vol_id)            ((uint32_t *)((uint8_t *)&fat_log_file[vol_id].sector+log_sizeof_magic))
#define log_sizeof_msk                     sizeof(uint32_t)
#define log_get_log_ptr(vol_id,idx)        ((log_t *)((uint8_t *)&fat_log_file[vol_id].sector+log_sizeof_magic+log_sizeof_msk)+(idx))




/* external function */
extern int32_t fat_lookup_short(fat_volinfo_t *fvi, fat_fileent_t *fe);
extern int32_t fat_lookup_long(fat_volinfo_t *fvi, fat_fileent_t *fe);

/* local function */
static int32_t __fat_chk_log_recover(int32_t vol_id);

/*
 Name: fat_zinit_log
 Desc:
 Params:
 Returns:
 Caveats:
 */

void fat_zinit_log(void)
  {
  memset(&fat_log_file, 0, sizeof (fat_log_file));
  }

/*
 Name: fat_is_log_file
 Desc: Test the given file is the log file.
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

bool fat_is_log_file(fat_fileent_t *fe, bool cmp_name)
  {
  log_mng_t *log_mng = &fat_log_file[fe->vol_id];


  if(cmp_name)
    {
    if(!strcmp((LOG_FILE), fe->name))
      return true;
    else
      return false;
    }
  else
    {
    if((log_mng->log_file.parent_sect == fe->parent_sect) &&
       (log_mng->log_file.parent_ent_idx == fe->parent_ent_idx))
      return true;
    else
      return false;
    }
  }

/*
 Name: __fat_log_lookup
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_log_lookup(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
  int32_t rtn;
  bool tmp, tmp2;


  fvi = GET_FAT_VOL(fe->vol_id);

  /* Root-directory access is different with FAT16 and FAT32. */
  if(eFAT16_SIZE == fvi->br.efat_type)
    SET_OWN_CLUST(&fe->dir, FAT_ROOT_CLUST);
  else /* eFAT32_SIZE : first_data_sect is root-directory */
    SET_OWN_CLUST(&fe->dir, D_SECT_2_CLUST(fvi, fvi->br.first_data_sect));

  /* Chk-file has always been created into root-dir. */
  fe->parent_clust = GET_OWN_CLUST(&fe->dir);

  /* Create the name of chk-file on File-entry. */
  rtn = fat_cp_name(fe, (LOG_FILE), eFAT_FILE);
  if(0 > rtn) return rtn;

  /* Create a short-name on base of File-entry. */
  rtn = fat_make_shortname(fe, &tmp, &tmp2);
  if(0 > rtn) return rtn;

  /* Lookup if there is Check-file. */
  if((uint32_t) eFILE_LONGENTRY & fe->flag)
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
 Name: __fat_log_creat
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_log_creat(fat_fileent_t *fe)
  {
  uint32_t clust_list[1/*last cluster*/ + LOG_CLUST_PER_LOG], /* sector less equal cluster */
    filesize,
    clust_cnt,
    vol_id;
  int32_t rtn;


  vol_id = fe->vol_id;

  /* Calculate need clusters */
  filesize = LOG_SECT_SIZE;
  clust_cnt = LOG_CLUST_PER_LOG;

  /* Allocate clusters. */
  rtn = fat_map_alloc_clusts(vol_id, clust_cnt, &clust_list[1]);
  if(0 > rtn) return -1;

  /* log-file has system-file attribute. */
  fe->dir.attr = eFAT_ATTR_HIDDEN | eFAT_ATTR_SYS | eFAT_ATTR_RO;
  SET_OWN_CLUST(&fe->dir, clust_list[1]);
  fe->dir.filesize = filesize;

  rtn = fat_alloc_entry_pos(fe);
  if(0 > rtn) return -1;

  /* Create entry. */
  rtn = fat_creat_entry(fe, true);
  if(0 > rtn) return -1;

  /* Real allocate */
  clust_list[0] = 0; /*last cluster*/
  rtn = fat_stamp_clusts(vol_id, clust_cnt, clust_list);
  if(0 > rtn) return rtn;

  rtn = fat_sync_table(vol_id, true);
  if(0 > rtn)
    return rtn;
  else
    return clust_list[1];
  }

/*
 Name: __fat_log_load
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_log_load(int32_t vol_id, fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
  int32_t clust_no,
    sect_no;


  fvi = GET_FAT_VOL(vol_id);

  clust_no = GET_OWN_CLUST(&fe->dir);
  sect_no = D_CLUST_2_SECT(fvi, clust_no);
  log_sect_no(vol_id) = sect_no;

  return lim_read_sector(vol_id, sect_no, log_get_sect_ptr(vol_id), LOG_SECT_CNT);
  }

/*
 Name: __fat_log_sync
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_log_sync(int32_t vol_id)
  {
  return lim_write_sector(vol_id, log_sect_no(vol_id), log_get_sect_ptr(vol_id), LOG_SECT_CNT);
  }

/*
 Name: __fat_init_log_data
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_init_log_data(int32_t vol_id)
  {
  uint32_t *p4;
  int32_t i;


  for(i = 0, p4 = log_get_sect_ptr(vol_id); i < (LOG_SECT_SIZE / sizeof (uint32_t *)); i++)
    *p4++ = LOG_EMPTY_ENTRY;

  *log_get_magic_ptr(vol_id) = LOG_MAGIC;
  *log_get_msk_ptr(vol_id) = 0;

  return __fat_log_sync(vol_id);
  }

/*
 Name: fat_log_init
 Desc:
 Params:
 Returns:
 Caveats:
 */

int32_t fat_log_init(int32_t vol_id)
  {
  log_mng_t *log_mng = &fat_log_file[vol_id];
  fat_volinfo_t *fvi;
  fat_fileent_t *fe;
  int32_t max_logs,
    additional_size,
    rtn;


  fe = fat_alloc_file_entry(vol_id);
  if(NULL == fe) return -1;

  /* Lookup if there is check-file. */
  rtn = __fat_log_lookup(fe);
  if(0 > rtn)
    {
    if(e_noent == get_errno())
      {
      /* If not found, create log-file. */
      rtn = __fat_log_creat(fe);
      if(0 > rtn) goto End;

      fvi = GET_FAT_VOL(vol_id);
      log_sect_no(vol_id) = D_CLUST_2_SECT(fvi, rtn);

      rtn = __fat_init_log_data(vol_id);
      if(0 > rtn) goto End;
      }
    else
      goto End;
    }
  else
    {
    rtn = __fat_log_load(vol_id, fe);
    if(0 > rtn) goto End;
    }

  /* Rember position of log-file. */
  log_mng->log_file.parent_ent_idx = fe->parent_ent_idx;
  log_mng->log_file.parent_sect = fe->parent_sect;

  if(LOG_MAGIC == *log_get_magic_ptr(vol_id))
    /* Log file is valid. */
    rtn = __fat_chk_log_recover(vol_id);
  else
    /* Log file is invalid. */
    rtn = __fat_init_log_data(vol_id);

  additional_size = (uint8_t *) log_get_log_ptr(vol_id, 0) - (uint8_t *) log_get_sect_ptr(vol_id);
  max_logs = ((LOG_SECT_SIZE * LOG_SECT_CNT) - additional_size) / sizeof (log_t);

  if(LOG_MAX_LOG > max_logs)
    rtn = set_errno(e_port);

  *log_get_msk_ptr(vol_id) = 0;

End:
  fat_free_file_entry(fe);

  return rtn;
  }

/*
 Name: __fat_log_alloc_el
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_log_alloc_el(int32_t vol_id)
  {
  uint32_t index = 0;


  /* Allocate entry. */
  index = bit_ffz(*log_get_msk_ptr(vol_id));

  if(LOG_MAX_LOG <= index)
    /* Too many allocated. */
    return -1;

  /* Set the mask for free buffer */
  bit_set(log_get_msk_ptr(vol_id), index);

  return index;
  }

/*
 Name: __fat_log_free_el
 Desc:
 Params:
 Returns:
 Caveats:
 */

static void __fat_log_free_el(int32_t vol_id, int32_t index)
  {
  fsm_assert2(LOG_MAX_LOG > index);

  /* Clear the mask for free buffer */
  bit_clear(log_get_msk_ptr(vol_id), index);
  }

/*
 Name: fat_log_on
 Desc:
 Params:
 Returns:
 Caveats:
 */

int32_t fat_log_on(fat_fileent_t *fe, fat_fileent_t *fe2)
  {
  int32_t vol_id;
  entry_log_t *pentry;
  int32_t index,
    rtn;


  fsm_assert3(NULL != fe);

  vol_id = fe->vol_id;

#if 0    /* fat_log_on() is not called while the Log system is initialized. */
  if(0 == log_sect_no(vol_id))
    /* Log system was not initialized. */
    return 0;
#endif

  index = __fat_log_alloc_el(vol_id);
  if(0 > index)
    return set_errno(e_nomem);

  pentry = (entry_log_t *) log_get_log_ptr(vol_id, index);

  pentry->parent_sect = fe->parent_sect;
  pentry->parent_ent_idx = fe->parent_ent_idx;
  pentry->own_clust = GET_OWN_CLUST(&fe->dir);

  pentry++;

  if(NULL == fe2)
    {
    pentry->parent_sect = LOG_EMPTY_ENTRY;
    }
  else
    {
    pentry->parent_sect = fe2->parent_sect;
    pentry->parent_ent_idx = fe2->parent_ent_idx;
    pentry->own_clust = GET_OWN_CLUST(&fe2->dir);
    }

  rtn = __fat_log_sync(vol_id);
  if(0 > rtn)
    return rtn;

  return index;
  }

/*
 Name: fat_log_clust_on
 Desc:
 Params:
 Returns:
 Caveats:
 */

int32_t fat_log_clust_on(int32_t vol_id, uint32_t clust)
  {
  entry_log_t *pentry;
  int32_t index,
    rtn;


  fsm_assert1(0 != clust);

  if(0 == log_sect_no(vol_id))
    /* Log system was not initialized.
       fat_log_clust_on() may be called while the Log file is created. */
    return 0;

  index = __fat_log_alloc_el(vol_id);
  if(0 > index)
    return set_errno(e_nomem);

  pentry = (entry_log_t *) log_get_log_ptr(vol_id, index);

  pentry->parent_sect = 0;
  pentry->parent_ent_idx = 0;
  pentry->own_clust = clust;

  pentry++;
  pentry->parent_sect = LOG_EMPTY_ENTRY;

  rtn = __fat_log_sync(vol_id);
  if(0 > rtn)
    return rtn;

  return index;
  }

/*
 Name: fat_log_off
 Desc:
 Params:
 Returns:
 Caveats:
 */

void fat_log_off(int32_t vol_id, int32_t id)
  {
  __fat_log_free_el(vol_id, id);
  }

/*
 Name: __fat_log_unlink_entry
 Desc: Remove all entries in the File-Entry specified by 'fe' parameter.
 Params:
   - fe: The file entry pointer.
 Returns:
   int32_t  ==0 on success.
            < 0  on fail.
 Caveats: None.
 */

static int32_t __fat_log_unlink_entry(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
  fat_fileent_t *pfe;
  lim_cacheent_t *ce;
  fat_dirent_t *de, *end_de;
  list_head_t list;
  int32_t sect_no,
    ent_idx,
    i,
    rtn = s_ok;
  bool is_dirty;


  if(eFAT_EOF == fe->parent_sect)
    return e_eos;

  list_init(&list);

  pfe = (fat_fileent_t *) fe;
  fvi = GET_FAT_VOL(pfe->vol_id);
  sect_no = fe->parent_sect;
  ent_idx = fe->parent_ent_idx;

  for(i = 0; i < FAT_LONGNAME_SLOTS; i++)
    {
    ce = lim_get_csector(fvi->vol_id, sect_no);
    if(NULL == ce) return get_errno();
    de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
    end_de = de + fvi->br.ents_per_sect - 1/*start index 0*/;
    is_dirty = false;

    for(de += ent_idx; de <= end_de; de++)
      {

      if(IS_EMPTY_ENTRY(de))
        goto End;

      if(!IS_DELETED_ENTRY(de))
        {
        DELETE_ENTRY(de);
        is_dirty = true;
        }

      /* If short, done. */
      if(eFAT_ATTR_LONG_NAME != de->attr)
        goto End;
      }

    if(is_dirty)
      /* Mark as Dirty status to update data. */
      lim_mark_dirty_csector(ce, &list);
    lim_rel_csector(ce);

    ent_idx = 0;
    sect_no = fat_get_next_sectno_16root(fvi, pfe, sect_no);
    if(eFAT_EOF == sect_no)
      break;
    else if(0 > sect_no)
      return -1;
    }

End:
  if(is_dirty)
    lim_mark_dirty_csector(ce, &list);

  lim_rel_csector(ce);

  if(list.next != &list)
    {
    /* If more than one sector were dirtied, we flush list of dirty sector. */
    rtn = lim_flush_csectors(pfe->vol_id, &list);
#if defined( DBG )
    log_dbg.delete_entry_cnt++;
#endif
    }

  return rtn;
  }

/*
 Name: __fat_log_chk_entry
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

static int32_t __fat_log_chk_entry(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(fe->vol_id);
  lim_cacheent_t *ce;
  fat_dirent_t *de, *end_de;
  int32_t sect_no,
    parent_ent_idx,
    prev_lfn_idx = 0,
    rtn = s_ok;
  bool prev_ent_is_long = false,
    found_last_lfn = false;


  sect_no = fe->parent_sect;
  parent_ent_idx = fe->parent_ent_idx;

  while(1)
    {
    ce = lim_get_csector(fvi->vol_id, sect_no);
    if(NULL == ce) return get_errno();
    de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);
    end_de = de + fvi->br.ents_per_sect - 1/*start index 0*/;

    for(de += parent_ent_idx; de <= end_de; de++)
      {
      if(IS_DELETED_ENTRY(de))
        goto del_entry;
      else if(IS_EMPTY_ENTRY(de))
        {
        if(prev_ent_is_long)
          /* The last entry must be LFN. Otherwise, Power loss is occured in creating/deleting entries. */
          goto del_entry;
        rtn = e_plo;
        goto End;
        }
      else
        {
        if(eFAT_ATTR_LONG_NAME == de->attr)
          {
          if(FAT_LFN_LAST_ENTRY & ((fat_lfnent_t *) de)->idx)
            {
            fe->flag |= (uint32_t) eFILE_LONGENTRY;
            found_last_lfn = true;
            prev_lfn_idx = ((fat_lfnent_t *) de)->idx & FAT_LFN_IDX_MASK;
            }
          else if(prev_lfn_idx - 1 != (((fat_lfnent_t *) de)->idx & FAT_LFN_IDX_MASK))
            {
            /* LFN's index was corrupted. Perhaps FTL won't support PLR with sector unit. */
#if defined( DBG )
            break();
#endif
            goto del_entry;
            }

          prev_lfn_idx = ((fat_lfnent_t *) de)->idx & FAT_LFN_IDX_MASK;
          prev_ent_is_long = true;
          continue;
          }
        else
          { /* Short directory. */
          if(prev_ent_is_long)
            {
            if(1 != prev_lfn_idx)
              {
              /* LFN's index was corrupted. Perhaps FTL won't support PLR with sector unit. */
#if defined( DBG )
              break();
#endif
              goto del_entry;
              }
            if(!found_last_lfn)
              /* If power loss occured on deleting entries, All LFNs were deleted.
                 But Short-entry was not deleted. So, it should be deleted. */
              goto del_entry;
            }
          if(!found_last_lfn)
            fe->flag = 0;
          }

        /* If LFN or not, we set up information of short-entry. */
        fe->lfn_short_sect = sect_no;
        fe->lfn_shortent_idx = (uint8_t) (de - (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect));
        memcpy(&fe->dir, de, sizeof (fat_dirent_t));

        rtn = s_ok;
        goto End;
        }
      }

    lim_rel_csector(ce);

    parent_ent_idx = 0;
    sect_no = fat_get_next_sectno_16root(fvi, fe, sect_no);
    if(eFAT_EOF == sect_no)
      {
      if(prev_ent_is_long)
        goto del_entry;
      goto End;
      }
    else if(0 > sect_no)
      {
      rtn = get_errno();
      goto End;
      }
    }

del_entry:
  rtn = __fat_log_unlink_entry(fe);
  if(s_ok == rtn)
    rtn = e_plo;

End:
  lim_rel_csector(ce);

  return rtn;
  }

/*
 Name: __fat_log_update_sentry
 Desc:
 Params:
   - vol_id: The ID of volume which is checked.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_log_update_sentry(fat_fileent_t *fe)
  {
  int32_t rtn;


  rtn = fat_update_sentry(fe, true);

  return rtn;
  }

/*
 Name: __fat_log_chk_clust
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_log_chk_clust(int32_t vol_id, uint32_t own_clust, bool only_chk)
  {
  uint32_t last_clust_no,
    last_clust_fat;
  int32_t real_clust_cnt,
    rtn = s_ok;


  real_clust_cnt = fat_get_clust_cnt(vol_id, own_clust, &last_clust_no, &last_clust_fat);
  if(0 > real_clust_cnt)
    return real_clust_cnt;

  if(eFAT_FREE == last_clust_fat && real_clust_cnt)
    {
    if(!only_chk)
      {
      rtn = fat_set_next_clustno(vol_id, last_clust_no, (uint32_t) eFAT_EOC);
      if(0 > rtn)
        return rtn;

#if defined( DBG )
      log_dbg.total_recovery_cnt++;
#endif
      }
    else
      return e_plo;
    }

  return real_clust_cnt;
  }

/*
 Name: __fat_log_chk_dir
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_log_chk_dir(fat_fileent_t *fe, bool only_chk)
  {
  pim_devinfo_t *pdi = GET_LIM_DEV(fe->vol_id);
  fat_volinfo_t *fvi = GET_FAT_VOL(fe->vol_id);
  lim_cacheent_t *ce;
  fat_dirent_t *de;
  uint32_t own_clust,
    last_clust_no,
    last_clust_fat,
    dir_clust,
    sect_no;
  int32_t rtn = s_ok;
  bool op_is_not_completed = false; /* Operation is not completed. */


  own_clust = GET_OWN_CLUST(&fe->dir);

  if(eFAT_FREE == fat_get_next_clustno(fe->vol_id, own_clust))
    {
    if(!only_chk)
      {
      /*
         This is a case which power-loss occurred when a directory is removed.
         So we will unlink the directory.
       */
      rtn = __fat_log_unlink_entry(fe);
      if(s_ok == rtn)
        rtn = e_plo;
      return rtn;
      }
    else
      return e_plo;
    }
  else
    {
    rtn = fat_get_clust_cnt(fe->vol_id, own_clust, &last_clust_no, &last_clust_fat);

    if(((eFAT16_SIZE == fvi->br.efat_type) && !IS_FAT16_EOC(last_clust_fat)) ||
       ((eFAT32_SIZE == fvi->br.efat_type) && !IS_FAT32_EOC(last_clust_fat)))
      {
      /*
         If the last cluster is not EOC, it must have been under 'remove' operation.
         So we will redo 'remove' operation.
       */
      op_is_not_completed = true;
      goto end;
      }

    }

  /* If onw_clust is 0, this entry must have been deleted by __fat_log_chk_entry(). */
  fsm_assert1(0 != own_clust);

  /*
     If device has property as 'ePIM_NeedErase', directory's cluster might have been deleted.
     So, we check whether the dot and dot-dot entry is valid.
     If it is invalid, the entry is unlinked.
   */
  if(ePIM_NeedErase & pdi->dev_flag)
    {
    sect_no = D_CLUST_2_SECT(fvi, own_clust);

    ce = lim_get_csector(fe->vol_id, sect_no);
    if(NULL == ce) return get_errno();
    de = (fat_dirent_t *) LIM_REAL_BUF_ADDR(ce->buf, sect_no, fvi->br.first_root_sect);

    /* Check dot-dir. */
    dir_clust = GET_OWN_CLUST(de);
    if((dir_clust != own_clust) || (0 != memcmp(de->name, FAT_DOT, FAT_SHORTNAME_SIZE)))
      op_is_not_completed = true;
    else
      {
      /* Check dotdot-dir. */
      de++;
      if(0 != memcmp(de->name, FAT_DOTDOT, FAT_SHORTNAME_SIZE))
        op_is_not_completed = true;
      }

    lim_rel_csector(ce);
    }

end:
  if(op_is_not_completed)
    {
    if(!only_chk)
      {
      rtn = fat_unlink_clusts(fe->vol_id, own_clust, false);
      if(0 > rtn)
        return rtn;

#if defined( DBG )
      log_dbg.total_recovery_cnt++;
#endif

      /* Remove own directory-entry in parent cluster */
      rtn = __fat_log_unlink_entry(fe);
      if(0 > rtn)
        return rtn;
      }

    rtn = e_plo;
    }

  return rtn;
  }

/*
 Name: __fat_log_chk_file
 Desc:
 Params:
 Returns:
 Caveats:
 */

int32_t __fat_log_chk_file(fat_fileent_t *fe, bool only_chk)
  {
  int32_t vol_id = fe->vol_id;
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  int32_t file_clust_cnt,
    real_clust_cnt = 0,
    rtn = s_ok;
  uint32_t clust_list[CLUST_LIST_BUF_CNT + 1],
    own_clust,
    clust_no,
    last_clust_no,
    clust_cnt,
    entry_update = 0,
    got_clust_cnt = 0,
    remand_file_clust_cnt = 0;

#if defined( DBG )
  bool recovered = false;
#endif

  own_clust = GET_OWN_CLUST(&fe->dir);

  /* Calc the number of cluster that a file has occupied. */
  file_clust_cnt = fe->dir.filesize / fvi->br.bits_per_clust;

  if(own_clust)
    {
    real_clust_cnt = rtn = __fat_log_chk_clust(vol_id, own_clust, only_chk);
    if(0 > rtn)
      return e_plo;
    }
  else
    real_clust_cnt = 0;

  if(real_clust_cnt != file_clust_cnt)
    {
    if(!only_chk)
      {
      if(0 == real_clust_cnt)
        {
        SET_OWN_CLUST(&fe->dir, 0);
        /* The number of cluster that a file hs occupied actually is 0, so size of file is also 0. */
        fe->dir.filesize = 0;
        entry_update = 1;
        }
      else
        {
        if(real_clust_cnt < file_clust_cnt)
          {
          fe->dir.filesize = (real_clust_cnt - 1) * (1 << fvi->br.bits_per_clust) + 1/*byte*/;
          entry_update = 1;
          }
        else if(real_clust_cnt > file_clust_cnt)
          {
          clust_no = own_clust;
          clust_cnt = CLUST_LIST_BUF_CNT;
          remand_file_clust_cnt = file_clust_cnt;
          do
            {
            rtn = fat_get_clust_list(vol_id, clust_no, clust_cnt, clust_list);
            if(0 >= rtn)
              return rtn;
            if(rtn == clust_cnt)
              rtn--; /* first cluster number */
            got_clust_cnt += rtn;
            if(got_clust_cnt >= (uint32_t) file_clust_cnt)
              break;

            remand_file_clust_cnt -= rtn;
            clust_no = clust_list[rtn];
            }
          while(1);

          clust_no = clust_list[remand_file_clust_cnt];
          rtn = fat_unlink_clusts(vol_id, clust_no, false);
          if(0 > rtn)
            return rtn;

          if(0 == remand_file_clust_cnt)
            last_clust_no = clust_list[remand_file_clust_cnt];
          else
            last_clust_no = clust_list[remand_file_clust_cnt - 1];

          if(0 != file_clust_cnt)
            {
            rtn = fat_set_next_clustno(vol_id, last_clust_no, (uint32_t) eFAT_EOC);
            if(0 > rtn)
              return rtn;
            }
          else
            {
            SET_OWN_CLUST(&fe->dir, 0);
            entry_update = 1;
            }
          }
        }

      if(entry_update)
        {
        rtn = __fat_log_update_sentry(fe);
        if(0 > rtn)
          return rtn;
        }
#if defined( DBG )
      recovered = true;
#endif
      }

    rtn = e_plo;
    }

#if defined( DBG )
  if(recovered)
    {
    log_dbg.total_recovery_cnt++;
    fsm_assert2(false == only_chk);
    }
#endif

  return rtn;
  }

/*
 Name: __fat_log_rev_log
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_log_rev_log(fat_fileent_t *fe, entry_log_t *pentry)
  {
  int32_t vol_id = fe->vol_id;
  uint32_t own_clust,
    clust_no;
  int32_t rtn;


  fe->parent_sect = pentry->parent_sect;
  fe->parent_ent_idx = pentry->parent_ent_idx;
  fe->dir.attr = 0;
  SET_OWN_CLUST(&fe->dir, pentry->own_clust);
  own_clust = pentry->own_clust;

  if(0 == fe->parent_sect)
    {
    fsm_assert2(0 == fe->parent_ent_idx);
    /* Check only cluster. */
    fsm_assert1(0 != pentry->own_clust);
    rtn = __fat_log_chk_clust(vol_id, pentry->own_clust, false);
    if(0 > rtn)
      return -1;
    else
      return s_ok;
    }

  rtn = __fat_log_chk_entry(fe);

  if(e_plo == rtn && own_clust)
    {
    clust_no = fat_get_next_clustno(vol_id, own_clust);
    if(eFAT_FREE != clust_no)
      {
      /* Delete FAT Clusters. */
      rtn = fat_unlink_clusts(vol_id, own_clust, false);
      if(0 > rtn) return rtn;
#if defined( DBG )
      log_dbg.total_recovery_cnt++;
#endif
      }
    rtn = e_plo;
    }
  else if(s_ok == rtn)
    {
    fsm_assert2(0 != fe->dir.attr);
    /* Check/Adust FAT Clusters. */
    if(eFAT_ATTR_DIR & fe->dir.attr)
      rtn = __fat_log_chk_dir(fe, false);
    else
      {
      if((0 == fe->dir.filesize) && (0 == GET_OWN_CLUST(&fe->dir)))
        SET_OWN_CLUST(&fe->dir, pentry->own_clust);
      rtn = __fat_log_chk_file(fe, false);
      }
    }

  if((0 < rtn) || (e_plo == rtn))
    rtn = s_ok;

  return rtn;
  }

/*
 Name: __fat_log_rev_log2
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_log_rev_log2(fat_fileent_t *fe, entry_log_t *pentry)
  {
  int32_t vol_id = fe->vol_id;
  uint32_t own_clust,
    clust_no;
  entry_log_t *pentry2 = pentry + 1;
  int32_t rtn;


  fsm_assert2(pentry2->own_clust == pentry->own_clust);

  /* Setup to new-entry. */
  fe->parent_sect = pentry2->parent_sect;
  fe->parent_ent_idx = pentry2->parent_ent_idx;
  fe->dir.attr = 0;
  SET_OWN_CLUST(&fe->dir, pentry2->own_clust);
  own_clust = pentry2->own_clust;

  rtn = __fat_log_chk_entry(fe);

  if(pentry->parent_sect == pentry2->parent_sect && pentry->parent_ent_idx == pentry2->parent_ent_idx)
    {
    if(s_ok == rtn)
      rtn = __fat_log_unlink_entry(fe);
    if(own_clust)
      {
      rtn = __fat_log_chk_clust(vol_id, own_clust, false);
      if(0 > rtn)
        return e_plo;

      clust_no = fat_get_next_clustno(vol_id, own_clust);
      if(eFAT_FREE != clust_no)
        {
        /* Delete FAT Clusters. */
        rtn = fat_unlink_clusts(vol_id, own_clust, false);
        if(0 > rtn) return rtn;
#if defined( DBG )
        log_dbg.total_recovery_cnt++;
#endif
        }
      }
    rtn = s_ok;
    }
  else
    {
    /* Setup to old-entry. */
    fe->parent_sect = pentry->parent_sect;
    fe->parent_ent_idx = pentry->parent_ent_idx;
    SET_OWN_CLUST(&fe->dir, pentry->own_clust);

    if(s_ok == rtn)
      {
      /* The old-file is vaild. So delete new-file. */
      rtn = __fat_log_unlink_entry(fe);
      if(0 > rtn)
        return rtn;
      }
    else if(e_plo == rtn)
      {
      /* The new-entry is valid. Check old-file. */
      rtn = __fat_log_chk_entry(fe);
      if(e_plo == rtn)
        return e_cfs;
      else if(0 > rtn)
        return rtn;
      }
    else
      return e_cfs;

    fsm_assert1(0 != fe->dir.attr);

    /* Check/Adust FAT Clusters. */
    if(eFAT_ATTR_DIR & fe->dir.attr)
      rtn = __fat_log_chk_dir(fe, true);
    else
      rtn = __fat_log_chk_file(fe, true);

    if(0 < rtn)
      rtn = s_ok;

    fsm_assert1(e_plo != rtn);
    }

  return rtn;
  }

/*
 Name: __fat_log_chk_log
 Desc:
 Params:
 Returns:
 Caveats:
 */

static int32_t __fat_log_chk_log(int32_t vol_id, int32_t index)
  {
  fat_fileent_t *fe;
  entry_log_t *pentry = (entry_log_t *) log_get_log_ptr(vol_id, index);
  int32_t rtn;


  if(NULL == (fe = fat_alloc_file_entry(vol_id)))
    return -1;

  fsm_assert1(LOG_EMPTY_ENTRY != pentry->parent_sect);

  if(LOG_EMPTY_ENTRY == (pentry + 1)->parent_sect)
    {
    rtn = __fat_log_rev_log(fe, pentry);
    }
  else
    rtn = __fat_log_rev_log2(fe, pentry);

  fat_free_file_entry(fe);

  return rtn;
  }

/*
 Name: __fat_chk_log_recover
 Desc:
 Params:
 Returns:
 Caveats:
 */

int32_t __fat_chk_log_recover(int32_t vol_id)
  {
  uint32_t log_mask;
  int32_t idx,
    rtn = s_ok;


  log_mask = *log_get_msk_ptr(vol_id);

  while(log_mask)
    {
    idx = bit_ffo(log_mask);

    rtn = __fat_log_chk_log(vol_id, idx);
    if(0 > rtn)
      return rtn;

    bit_clear(&log_mask, idx);
    }

#if defined( DBG )
  /* fat_lock() is called in fat_mount(). */
  fat_unlock();
  rtn = fat_chk_fattable(vol_id);
  fat_lock();
  fsm_assert1(s_ok == rtn);
#endif

  return rtn;
  }

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

