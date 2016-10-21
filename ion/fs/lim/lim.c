#include "lim.h"
#include "../fat/mbr.h"
#include "../fat/vol.h"

#include <string.h>
#include "../osd/osd.h"

/*-----------------------------------------------------------------------------
 DEFINE DEFINITIONS
-----------------------------------------------------------------------------*/

/* Size of lim-cache hash. */
#if ( 8 >= LIM_CACHE_NUM )
#define LIM_CACHE_HASH 1
#elif ( 16 >= LIM_CACHE_NUM )
#define LIM_CACHE_HASH 4
#else
#define LIM_CACHE_HASH 8
#endif

#if ( (LIM_CACHE_HASH*VOLUME_NUM) > LIM_CACHE_NUM )
#error "LIM_CACHE_NUM must be greater than (LIM_CACHE_HASH*VOLUME_NUM)"
#endif

/* Hash generation function. */
#define LIM_GET_CACHE_HASH(sect, first_sect) ((CACHE_MIN_SECT_NUM(sect, first_sect))&(LIM_CACHE_HASH-1))
#define LIM_DATA_CACHE 1




/*-----------------------------------------------------------------------------
 DEFINE GLOBAL VARIABLES
-----------------------------------------------------------------------------*/

/* free & active list (don't link dirty)  :  ACTIVE--LIST--FREE */
static list_head_t lim_cache_lru;
/* link only dirty */
static list_head_t lim_cache_dirty_lru[VOLUME_NUM];
/* Hash list on ACTIVE. */
static list_head_t lim_cache_hash_lru[VOLUME_NUM][LIM_CACHE_HASH];




/* Entries of lim-cache. */
static lim_cacheent_t lim_cache_table[LIM_CACHE_NUM];
/* Data Entries of lim-cache. */
static lim_cachedat_t lim_cache_data;
/* The buffer is caching to each sector's real-data. */
static uint8_t lim_cache_sect[LIM_CACHE_NUM + LIM_DATA_CACHE][CACHE_BUFFER_SIZE];




/* Flag for cheking initializing.*/
static bool lim_inited;
/* Volume information for LIM. */
lim_volinfo_t lim_vol[VOLUME_NUM];




/*-----------------------------------------------------------------------------
 DEFINE FUNCTIONS
-----------------------------------------------------------------------------*/

/*
 Name: lim_zinit_lim
 Desc: Zero initialize all data.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void lim_zinit_lim(void)
  {
  memset(&lim_cache_lru, 0, sizeof (lim_cache_lru));
  memset(&lim_cache_dirty_lru, 0, sizeof (lim_cache_dirty_lru));
  memset(&lim_cache_hash_lru, 0, sizeof (lim_cache_hash_lru));
  memset(&lim_cache_table, 0, sizeof (lim_cache_table));
  /*memset( &lim_cache_sect, 0, sizeof(lim_cache_sect) );*/
  memset(&lim_inited, 0, sizeof (lim_inited));
  memset(&lim_vol, 0, sizeof (lim_vol));
  }

/*
 Name: lim_init_cache
 Desc: Initialize the cache table & all list.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void lim_init_cache(void)
  {
  list_head_t *list;
  lim_cacheent_t *entry;
  int32_t i, j;


  list = &lim_cache_lru;
  entry = &lim_cache_table[0];

  /* init all list */
  list_init(list);

  /* init cache buffer */
  for(i = 0; i < LIM_CACHE_NUM; i++, entry++)
    {
    list_init(&entry->head);
    list_init(&entry->hash_head);
    entry->flag = 0;
    entry->sect_no = 0;
    entry->buf = lim_cache_sect[i];

    /* Add to LRU. */
    list_add_tail(list, &entry->head);
    }

  lim_cache_data.flag = CACHE_FREE;
  lim_cache_data.sect_no = 0;
  lim_cache_data.ref_cnt = 0;
  lim_cache_data.buf = lim_cache_sect[LIM_CACHE_NUM];

  /* init cache dirty list */
  for(i = 0; i < VOLUME_NUM; i++)
    {
    list = &lim_cache_dirty_lru[i];
    list_init(list);
    }


  /* init cache hash list */
  for(i = 0; i < VOLUME_NUM; i++)
    {
    for(j = 0; j < LIM_CACHE_HASH; j++)
      {
      list = &lim_cache_hash_lru[i][j];
      list_init(list);
      }
    tr_lim_init(i);
    }
  }

/*
 Name: lim_reinit_cache_vol
 Desc: This function re-initializes all caches in the LIM layer.
 Params:
   - vol_id: Volume's ID to be initialized.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

result_t lim_reinit_cache_vol(int32_t vol_id)
  {
  list_head_t *list;
  lim_cacheent_t *pos, *n;
  int32_t i;

  /* init lim cache */
  for(i = 0; i < LIM_CACHE_HASH; i++)
    {
    list = &lim_cache_hash_lru[vol_id][i];

    list_for_each_entry_safe(lim_cacheent_t, pos, n, list, hash_head)
      {
      lim_clean_csector(pos);
      }
    }

  tr_lim_init(vol_id);

  return s_ok;
  }

/*
 Name: lim_init
 Desc: Initialize file system in the LIM layer. Initialize all device & cache table.
 Params: None.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: This function must be called to Initialze PIM Layer & Cache table.
 */

result_t lim_init(void)
  {
  int32_t rtn,
    i;


  if(true == lim_inited)
    return s_ok;

  /* call the pim's init function. Initialize all devices. */
  rtn = pim_init();
  if(0 > rtn) return rtn;

  for(i = 0; i < VOLUME_NUM; i++)
    lim_vol[i].opened = false;

  /* init cache table */
  lim_init_cache();

  lim_inited = true;

  return rtn;
  }

/*
 Name: lim_open
 Desc: Open a specific device on LIM layer.
 Params:
   - vol_id: The ID of volume to be opened.
   - dev_id: The device number to be opened.
   - part_no: The partition number to be opened.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t lim_open(int32_t vol_id, int32_t dev_id, int32_t part_no)
  {
  lim_volinfo_t *lvi = &lim_vol[vol_id];
  int32_t rtn;


  if(false == lim_inited)
    return e_noinit;

  rtn = pim_open(dev_id);
  if(0 > rtn) return rtn;

  rtn = mbr_load_partition(dev_id, part_no, lvi);
  if(0 > rtn)
    {
    if(e_mbr == get_errno())
      return set_errno(e_nofmt);
    else
      return rtn;
    }
  else
    lvi->opened = true;

  return rtn;
  }

/*
 Name: lim_terminate
 Desc: Terminate file system on LIM layer.
 Params: None.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t lim_terminate(void)
  {
  int32_t i;
  lim_volinfo_t *lvi = &lim_vol[0];

  /* Clear the LIM table */
  for(i = 0; i < VOLUME_NUM; i++)
    lvi[i].opened = false;

  lim_inited = false;

  /* Call the PIM's terminate_fs function */
  return pim_terminate();
  }

/*
 Name: __lim_lookup_csector
 Desc:  Search a specific sector from recently used cache entries.
 Params:
   - hash_list: Pointer to list_head_t structure represented the hash list.
   - vol_id: Volume's ID to be opened.
   - sect_no: Number of sector to search.
 Returns:
   lim_cacheent_t *  value on success. The returned value is a pointer to the
                           found cache entry.
                     NULL on fail.
 Caveats: None.
 */

static lim_cacheent_t *__lim_lookup_csector(list_head_t *hash_list,
                                            uint32_t vol_id, uint32_t sect_no)
  {
  lim_cacheent_t *pos;
  uint32_t sect_min;
  fat_volinfo_t *fvi;
  fvi = GET_FAT_VOL(vol_id);

  sect_min = CACHE_MIN_SECT_NUM(sect_no, fvi->br.first_root_sect);

  /* Check the sector number & the volume id */
  list_for_each_entry_rev(lim_cacheent_t, pos, hash_list, hash_head)
    {
    if(pos->sect_no == sect_min && pos->vol_id == vol_id)
      return pos;
    }

  return (lim_cacheent_t *) NULL;
  }

/*
 Name: lim_load_csector
 Desc: Read a sector specified by the 'sect_no' parameter to a cache entry.
 Params:
   - entry: Pointer to the lim_cacheent_t structure.
   - vol_id: The ID of volume.
   - sect_no: The sector number to be read.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t lim_load_csector(lim_cacheent_t *entry, uint32_t vol_id, uint32_t sect_no)
  {
  fat_volinfo_t *fvi;
  fvi = GET_FAT_VOL(vol_id);
  if(entry->flag & CACHE_DIRTY)
    return set_errno(e_fcache);

  /* Sets the cache-entry with the volume and the sector number. */
  entry->vol_id = (uint8_t) vol_id;
  entry->sect_no = CACHE_MIN_SECT_NUM(sect_no, fvi->br.first_root_sect);

  /* read data from LIM's Layer */
  if(0 > lim_read_sector(entry->vol_id, entry->sect_no, entry->buf, CACHE_SECTOR_NUM))
    return get_errno();

  entry->flag = CACHE_VALID;

  return s_ok;
  }

/*
 Name: lim_flush_csector
 Desc: flush a cash entry.
       Write the cache sector's data in a specific device.
 Params:
   - entry: Pointer to the lim_cacheent_t structure.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t lim_flush_csector(lim_cacheent_t *entry)
  {
  fsm_assert1(CACHE_VALID & entry->flag);

  if(!(entry->flag & CACHE_DIRTY))
    return s_ok;

  /* Writes the cache data in the LIM */
  if(0 > lim_write_sector(entry->vol_id, entry->sect_no, entry->buf, CACHE_SECTOR_NUM))
    return get_errno();

  /* Move an element to a tail at a specific list. */
  list_move_tail(&lim_cache_lru, &entry->head);

  entry->flag &= ~(uint8_t) CACHE_DIRTY;

  return s_ok;
  }

/*
 Name: lim_flush_csectors
 Desc: Flush all entry, it is inside the dirty cache list.
 Params:
   - vol_id: The ID of volume.
   - list: Pointer to the linked-list about cache entries.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None
 */

int32_t lim_flush_csectors(int32_t vol_id, list_head_t *list)
  {
  lim_cacheent_t *pos, *n;

  /* search dirty list */
  if(NULL == list)
    list = &lim_cache_dirty_lru[vol_id];

  /* Iterate over list of given type safe against removal of list entry */
  list_for_each_entry_safe(lim_cacheent_t, pos, n, list, head)
    {
    if(!(CACHE_DIRTY & pos->flag))
      continue;

    fsm_assert1(CACHE_VALID & pos->flag);
    /* Writes the cache data in the LIM */
    if(0 > lim_write_sector(pos->vol_id, pos->sect_no, pos->buf, CACHE_SECTOR_NUM))
      return get_errno();

    list_move_tail(&lim_cache_lru, &pos->head);

    pos->flag &= ~(uint8_t) CACHE_DIRTY;
    }

  return s_ok;
  }

/*
 Name: lim_load_cdsector
 Desc: Read a sector specified by the 'sect_no' parameter.
 Params:
   - Pointer to a buffer in which the bytes read are placed.
   - vol_id: The ID of volume.
   - sect_no: The sector number to be read.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t lim_load_cdsector(uint32_t vol_id, uint32_t sect_no, uint8_t *buf)
  {
  int32_t rtn;

#if defined( CDATA )
  lim_cache_data.ref_cnt = 1;
#else
  lim_cache_data.ref_cnt = 0;
#endif

  /* read data from LIM's Layer */
  rtn = lim_read_sector(vol_id, sect_no, buf, 1);

  lim_cache_data.ref_cnt = 0;

  if(0 > rtn) return get_errno();

  return s_ok;
  }

/*
 Name: lim_flush_cdsector
 Desc: flush a cash entry.
       Write the cache sector's data in a specific device.
 Params:
   - entry: Pointer to the lim_cachedat_t structure.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t lim_flush_cdsector(void)
  {
  lim_volinfo_t *lvi = &lim_vol[lim_cache_data.vol_id];

  cache_lock();
  if(lim_cache_data.flag & CACHE_DIRTY)
    {
    if(0 > pim_write_sector(lvi->dev_id, lim_cache_data.sect_no, lim_cache_data.buf, 1))
      return set_errno(e_io);
    lim_cache_data.flag = CACHE_FREE;
    }
  cache_unlock();

  return s_ok;
  }

/*
 Name: lim_clean_csector
 Desc: Delete a sector in a cache. Delete an element at the specific cache
       entry and initialize the element.
 Params:
   - entry: Pointer to the lim_cacheent_t structure.
 Returns: None
 Caveats: None
 */

void lim_clean_csector(lim_cacheent_t *entry)
  {
  if(!((entry->flag & CACHE_VALID) || (entry->flag & CACHE_DIRTY)))
    return;

  list_del_init(&entry->hash_head);
  /* Moves the hash list with the cache list */
  list_move(&lim_cache_lru, &entry->head);

  /* Sets the entry-flag is free */
  entry->flag = CACHE_FREE;

  return;
  }

/*
 Name: lim_clean_csectors
 Desc: Clean all cache sectors in a volume.
 Params:
   - vol_id: Volume's ID to be cleaned
 Returns: None.
 Caveats: None.
 */

void lim_clean_csectors(int32_t vol_id)
  {
  lim_cacheent_t *pos, *n;
  list_head_t *list;


  /* search dirty list */
  list = &lim_cache_dirty_lru[vol_id];

  list_for_each_entry_safe(lim_cacheent_t, pos, n, list, head)
    {
    lim_clean_csector(pos);
    }
  }

/*
 Name: lim_get_sector
 Desc: Gets the sector-entry from the cache list.
       If not exist sector's information in cache, read data from the physical device.
 Params:
   - vol_id: The ID of volume.
   - sect_no: The sector number.
 Returns:
   lim_cacheent_t*  value on success. Return value is the cache-entry pointer.
                    NULL on fail.
 Caveats: None.
 */

lim_cacheent_t *lim_get_sector(int32_t vol_id, uint32_t sect_no)
  {
  lim_cacheent_t *pos, *n;
  list_head_t *list, *hash_list;
  int32_t hash, vol_idx;

  fat_volinfo_t *fvi;
  fvi = GET_FAT_VOL(vol_id);

  tr_lim_inc_cache_refs(vol_id);

  hash = LIM_GET_CACHE_HASH(sect_no, fvi->br.first_root_sect);
  hash_list = &lim_cache_hash_lru[vol_id][hash];

  /* search normal list: Get empty entry & Loading sector from LIM */
  list = &lim_cache_lru;

  list_for_each_entry_safe(lim_cacheent_t, pos, n, list, head)
    {
    if(0 == pos->ref_cnt)
      {
      if(0 > lim_load_csector(pos, vol_id, sect_no))
        return (lim_cacheent_t *) NULL;
      pos->ref_cnt = 1;

      list_move_tail(list, &pos->head);
      list_move_tail(hash_list, &pos->hash_head);
      return pos;
      }
    }

  /* search dirty list: If not exist empty entry,
                        flush the dirty list's entry & re-loading a sector */
  for(vol_idx = 0; vol_idx < VOLUME_NUM; vol_idx++)
    {
    list = &lim_cache_dirty_lru[vol_idx];

    list_for_each_entry_safe(lim_cacheent_t, pos, n, list, head)
      {
      if(0 == pos->ref_cnt)
        {
        if(0 > lim_flush_csector(pos))
          return (lim_cacheent_t *) NULL;

        if(0 > lim_load_csector(pos, vol_id, sect_no))
          return (lim_cacheent_t *) NULL;
        pos->ref_cnt = 1;

        list_move_tail(hash_list, &pos->hash_head);
        return pos;
        }
      }
    }
  set_errno(e_nomem);
  return (lim_cacheent_t *) NULL;
  }

/*
 Name: lim_get_csector
 Desc: Read a sector it belongs to the specific sector number.
 Params:
   - vol_id: The ID of volume.
   - sect_no: The sector number.
 Returns:
   lim_cacheent_t*  value on success. Return value is cache-entry pointer.
                    NULL on fail.
 Caveats: None.
 */

lim_cacheent_t *lim_get_csector(int32_t vol_id, uint32_t sect_no)
  {
  lim_cacheent_t *pos;
  list_head_t *hash_list;
  int32_t hash;

  fat_volinfo_t *fvi;
  fvi = GET_FAT_VOL(vol_id);

  /* Searches the entry from hash list */
  hash = LIM_GET_CACHE_HASH(sect_no, fvi->br.first_root_sect);
  hash_list = &lim_cache_hash_lru[vol_id][hash];

  /* Lookup sector from the hash list */
  pos = __lim_lookup_csector(hash_list, vol_id, sect_no);
  if(pos)
    {
    tr_lim_inc_cache_refs(vol_id);

    list_move_tail(hash_list, &pos->hash_head);
    pos->ref_cnt++;
    return pos;
    }

  return lim_get_sector(vol_id, sect_no);
  }

/*
 Name: lim_rel_csector
 Desc: Release sector. That is, decrease the number of references about cache
       entry.
 Params:
   - entry: Pointer to lim_cacheent_t structure to be marking.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t lim_rel_csector(lim_cacheent_t *entry)
  {
  tr_lim_dec_cache_refs(entry->vol_id);

  /* Discount the reference-count */
  if(0 < entry->ref_cnt)
    entry->ref_cnt--;

  return s_ok;
  }

/*
 Name: lim_mark_dirty_csector
 Desc: Mark the cache-entry's flag with DIRTY status.
 Params:
   - entry: Pointer to lim_cacheent_t structure to be marking.
   - list: Pointer to the linked-list.
 Returns: None.
 Caveats: Dirty means the data which it will update.
 */

void lim_mark_dirty_csector(lim_cacheent_t *entry, list_head_t *list)
  {
  /* search dirty list */
  if(NULL == list)
    list = &lim_cache_dirty_lru[entry->vol_id];
  list_move_tail(list, &entry->head);

  /* Set flag is dirty. */
  entry->flag |= CACHE_DIRTY;
  }

/*
 Name: __lim_ioctl_part_info
 Desc: Setting the value to Requests the partition's information.
 Params:
   - lvi: Pointer to the structure which includes volume information of LIM.
   - param: Pointer to the structure which includes partition information.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None
 */

static int32_t __lim_ioctl_part_info(lim_volinfo_t *lvi, void *param)
  {
  partinfo_t *pi = (partinfo_t *) param;


  pi->start_sect = lvi->start_sect;
  pi->end_sect = lvi->end_sect;
  pi->sectors = lvi->totsect_cnt;
  pi->bytes_per_sect = lvi->bytes_per_sect;
  return s_ok;
  }

/*
 Name: lim_ioctl
 Desc: Execute the ioctl functions.
 Params:
   - vol_id: The ID of volume.
   - func: The requested function. The function is one of the following symbols.
           IO_MS_ATTACH - Requests attach of a mass-storage.
           IO_MS_DETACH - Requests detach of a mass-storage.
           IO_DEV_ATTACH - Requests attach of a device.
           IO_DEV_DETACH - Requests detach of a device.
           IO_PART_INFO - Requests the partition's information.
           IO_OP - Requests the following operation: read, write, erase.
           IO_READ - Requests read function.
           IO_WRITE - Requests write function.
           IO_ERASE - Requests erase function.
   - param: Void pointer to the parameter to be used at the function specified by
            the 'param' parameter.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t lim_ioctl(int32_t vol_id, uint32_t func, void *param)
  {
  lim_volinfo_t *lvi = GET_LIM_VOL(vol_id);
  iorw_t *rw = (iorw_t *) param;


  if(false == lim_inited || false == lvi->opened)
    return set_errno(e_io);

  switch(func)
    {
    case IO_MS_ATTACH:
      lvi->io_flag |= LIM_MS_attached;
      break;
    case IO_MS_DETACH:
      lvi->io_flag &= ~(uint32_t) LIM_MS_attached;
      break;
    case IO_DEV_ATTACH:
      lvi->io_flag |= LIM_DEV_attached;
      break;
    case IO_DEV_DETACH:
      lvi->io_flag &= ~(uint32_t) LIM_DEV_attached;
      break;
    case IO_PART_INFO:
      return __lim_ioctl_part_info(lvi, param);
    case IO_READ:
      return lim_read_sector(vol_id, rw->sect_no, rw->buf, rw->sect_cnt);
    case IO_WRITE:
      return lim_write_sector(vol_id, rw->sect_no, rw->buf, rw->sect_cnt);
    case IO_ERASE:
      return lim_erase_sector(vol_id, rw->sect_no, rw->sect_cnt);
    }

  return s_ok;
  }

/*
 Name: lim_read_sector
 Desc: Request the PIM Layer to read buffer from a specific sector.
 Params:
   - vol_id: The ID of volume.
   - sect_no: The sector number to be read.
   - buf: Pointer to a buffer in which the bytes read are placed.
   - cnt: The number of sectors to be read.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None
 */

int32_t lim_read_sector(int32_t vol_id, uint32_t sect_no, void *buf, uint32_t cnt)
  {
  lim_volinfo_t *lvi = &lim_vol[vol_id];
  lim_cachedat_t *dentry = &lim_cache_data;
  uint32_t start_sect,
    end_sect;

  start_sect = lvi->start_sect + sect_no;
  end_sect = start_sect + cnt - 1;

  if(end_sect > lvi->end_sect)
    return set_errno(e_outof);

  cache_lock();
  if(dentry->sect_no == start_sect &&
     dentry->sect_no == end_sect &&
     dentry->vol_id == vol_id &&
     (dentry->flag & CACHE_VALID))
    {

    memcpy(buf, dentry->buf, lvi->bytes_per_sect);
    cache_unlock();
    return s_ok;
    }

  if(dentry->flag & CACHE_DIRTY)
    {
    if(dentry->ref_cnt || (dentry->sect_no >= start_sect && dentry->sect_no <= end_sect))
      {
      if(0 > pim_write_sector(lim_vol[dentry->vol_id].dev_id, dentry->sect_no, dentry->buf, 1))
        {
        cache_unlock();
        return set_errno(e_io);
        }
      dentry->flag = CACHE_VALID;
      }
    }
  cache_unlock();

  /* Call read function of lower layer. */
  if(0 > pim_read_sector(lvi->dev_id, start_sect, (uint8_t *) buf, cnt))
    return set_errno(e_io);

  cache_lock();
  if(dentry->ref_cnt && !(dentry->flag & CACHE_DIRTY))
    {
    dentry->vol_id = (uint8_t) vol_id;
    dentry->sect_no = end_sect;
    memcpy((void *) dentry->buf, buf, lvi->bytes_per_sect);
    dentry->flag = CACHE_VALID;
    }
  cache_unlock();

  return s_ok;
  }

/*
 Name: lim_read_at_sector
 Desc: Read data from a specific offset of sectors in PIM Layer.
 Params:
   - vol_id: The ID of volume.
   - sect_no: The sector number to be read.
   - offs: The amount byte offset in the sector to be read.
   - len: The number of bytes to be read.
   - buf: Pointer to a buffer in which the bytes read are placed.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t lim_read_at_sector(int32_t vol_id, uint32_t sect_no, uint32_t offs, uint32_t len, void *buf)
  {
  lim_volinfo_t *lvi = &lim_vol[vol_id];
  uint32_t sector;


  sector = lvi->start_sect + sect_no;

  if(sector > lvi->end_sect)
    return set_errno(e_outof);

  /* Read data from a specific offset of sector in a specific device. */
  if(0 > pim_read_at_sector(lvi->dev_id, sector, offs, len, (uint8_t *) buf))
    return set_errno(e_io);
  return s_ok;
  }

/*
 Name: lim_write_sector
 Desc: Request the PIM Layer to write buffer to a specific sector.
 Params:
   - vol_id: The ID of volume.
   - sect_no: The sector number to be written.
   - buf: Pointer to a buffer containing data to be written.
   - cnt: The number of bytes to be written.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None
 */

int32_t lim_write_sector(int32_t vol_id, uint32_t sect_no, void *buf, uint32_t cnt)
  {
  lim_volinfo_t *lvi = &lim_vol[vol_id];
  lim_cachedat_t *dentry = &lim_cache_data;
  uint32_t start_sect,
    end_sect;


  start_sect = lvi->start_sect + sect_no;
  end_sect = start_sect + cnt - 1;

  cache_lock();
  if(dentry->sect_no == start_sect &&
     dentry->sect_no == end_sect &&
     dentry->vol_id == vol_id &&
     (dentry->flag & CACHE_VALID))
    {

    memcpy(dentry->buf, buf, lvi->bytes_per_sect);
    dentry->flag |= CACHE_DIRTY;
    cache_unlock();
    return s_ok;
    }
  else if(!(dentry->flag & CACHE_FREE))
    {
    if(dentry->sect_no >= start_sect && dentry->sect_no <= end_sect)
      dentry->flag = CACHE_FREE;
    }
  cache_unlock();

  if(end_sect > lvi->end_sect)
    return set_errno(e_outof);

  /* Call Write function of lower layer. */
  if(0 > pim_write_sector(lvi->dev_id, start_sect, (uint8_t *) buf, cnt))
    return set_errno(e_io);

  return s_ok;
  }

/*
 Name: lim_erase_sector
 Desc: Request the PIM Layer to erase a specific sector.
 Params:
   - vol_id: The ID of volume.
   - sect_no: The sector number to be erased.
   - cnt: The number of bytes to be erase.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None
 */

int32_t lim_erase_sector(int32_t vol_id, uint32_t sect_no, uint32_t cnt)
  {
  lim_volinfo_t *lvi = &lim_vol[vol_id];
  uint32_t start_sect,
    end_sect;


  start_sect = lvi->start_sect + sect_no;
  end_sect = start_sect + cnt - 1;

  if(end_sect > lvi->end_sect)
    return set_errno(e_outof);

  /* Call Erase function of lower layer. */
  if(0 > pim_erase_sector(lvi->dev_id, start_sect, cnt))
    return set_errno(e_io);
  return s_ok;
  }

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

