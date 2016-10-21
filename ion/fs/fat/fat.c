#include "fat.h"
#include "vol.h"
#include "misc.h"
#include "../lim/lim.h"
#include <string.h>

typedef struct
  {
  uint32_t *fatmap;
  bool is_used;

  } fat_bitmap_cb_t;




/* Size of fat-cache hash. */
#if ( 8 >= FAT_CACHE_NUM )
#define FAT_CACHE_HASH 1
#elif ( 16 >= FAT_CACHE_NUM )
#define FAT_CACHE_HASH 4
#else
#define FAT_CACHE_HASH 8
#endif

#if ( 2 > FAT_CACHE_NUM )
#error "FAT_CACHE_NUM must be greater than 2. Bcs in fat_stamp_clusts() two FAT cache is used "
#endif

#if ( (FAT_CACHE_HASH*VOLUME_NUM) > FAT_CACHE_NUM )
#error "FAT_CACHE_NUM must be greater than (FAT_CACHE_HASH*VOLUME_NUM)"
#endif

/* Hash generation function. */
#define FAT_GET_CACHE_HASH(sect, first_sect) ((CACHE_MIN_SECT_NUM(sect, first_sect))&(FAT_CACHE_HASH-1))




#if 0
#define use_write_fat_unlock()         fat_unlock()
#define use_write_fat_lock()           fat_lock()
#else
#define use_unlink_fat_unlock()
#define use_unlink_fat_lock()
#endif




/* Free & active list (don't link dirty)  :  ACTIVE--LIST--FREE */
static list_head_t fat_cache_lru;
/* Link only dirty. */
static list_head_t fat_cache_dirty_lru[VOLUME_NUM];
/* Hash list on ACTIVE. */
static list_head_t fat_cache_hash_lru[VOLUME_NUM][FAT_CACHE_HASH];
/* Entries of fat-cache. */
static fat_cache_entry_t fat_cache_table[FAT_CACHE_NUM];
/* This buffer is used to cache the actual data of each sector. */
static uint8_t fat_cache_sector[FAT_CACHE_NUM][CACHE_BUFFER_SIZE];

/* FAT bitmap pool. */
static uint32_t fat_bitmap[FAT_HEAP_SIZE / sizeof (uint32_t) + VOLUME_NUM];
/* Volume's bitmap pointer. */
static fat_bitmap_cb_t fat_vol_bitmap[VOLUME_NUM];

/*
 Name: fat_zinit_fat
 Desc: Zero initialize all data.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void fat_zinit_fat(void)
  {
  memset(&fat_cache_lru, 0, sizeof (fat_cache_lru));
  memset(&fat_cache_dirty_lru, 0, sizeof (fat_cache_dirty_lru));
  memset(&fat_cache_hash_lru, 0, sizeof (fat_cache_hash_lru));
  memset(&fat_cache_table, 0, sizeof (fat_cache_table));
  /*memset( &fat_cache_sector, 0, sizeof(fat_cache_sector) );*/
  memset(&fat_vol_bitmap, 0, sizeof (fat_vol_bitmap));
  /* memset( &fat_bitmap, 0, sizeof(fat_bitmap) ); */
  }

/*
 Name: fat_init_fatmap_pool
 Desc: Initialize fatmap pool.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void fat_init_fatmap_pool(void)
  {
  int32_t i;


  for (i = 0; i < VOLUME_NUM; i++)
    {
    fat_vol_bitmap[i].fatmap = fat_bitmap;
    fat_vol_bitmap[i].is_used = false;
    }
  }

/*
 Name: __fat_get_fatmap
 Desc: Get buffer of volume's fatmap.
 Params:
   - vol_id: Volume number.
   - uint32_size: Size to be allocated (NOTE:1 is 4bytes).
 Returns:
   uint32_t *  Pointer of allocated fatmap.
               NULL  on fail.
 Caveats:
 */

static uint32_t *__fat_get_fatmap(int32_t vol_id, uint32_t uint32_size)
  {
  uint32_t *fatmap_end = &fat_bitmap[sizeof (fat_bitmap) / sizeof (uint32_t) - 1];
  uint32_t *fatmap;
  int32_t i;


  fatmap = fat_vol_bitmap[vol_id].fatmap;
  fat_vol_bitmap[vol_id].is_used = true;

  if ((fatmap_end <= fatmap) || (fatmap_end < (fatmap + uint32_size)))
    return NULL;

  for (i = 0; i < VOLUME_NUM; i++)
    {
    if (false == fat_vol_bitmap[i].is_used)
      fat_vol_bitmap[i].fatmap = fatmap + uint32_size;
    }

  return fatmap;
  }

/*
 Name: fat_init_cache
 Desc: Initialize Cache-Buffer for caching FAT Meta-Data.
 Params: None.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: This function only once calling at mount time.
          Only once calling even in the case which will use a multi volume.
 */

int32_t fat_init_cache(void)
  {
  list_head_t *list;
  fat_cache_entry_t *entry;
  int32_t i, j;

  /* Check the state that the cache-buffer's start address is aligned. */
  if (0x3 & (uint32_t) & fat_cache_sector[0][0])
    return set_errno(e_port);

  list = &fat_cache_lru;
  entry = &fat_cache_table[0];

  /* Initialize FAT cache. */
  list_init(list);

  for (i = 0; i < FAT_CACHE_NUM; i++, entry++)
    {
    list_init(&entry->head);
    list_init(&entry->hash_head);
    /* In the first time, all cache is free. */
    entry->flag = CACHE_FREE;
    entry->ref_cnt = 0;
    entry->sect_no = 0;
    entry->buf = fat_cache_sector[i];

    /* Entry is added to LRU. */
    list_add_tail(list, &entry->head);
    }


  /* Initialize dirty list of all volumes. */
  for (i = 0; i < VOLUME_NUM; i++)
    {
    list = &fat_cache_dirty_lru[i];
    list_init(list);
    }


  /* Initialize hash list of all volumes. */
  for (i = 0; i < VOLUME_NUM; i++)
    {
    for (j = 0; j < FAT_CACHE_HASH; j++)
      {
      list = &fat_cache_hash_lru[i][j];
      list_init(list);
      }
    }

  fat_init_fatmap_pool();

  return s_ok;
  }

/*
 Name: fat_reinit_cache_vol
 Desc: Initialize all entries of cache again.
 Params:
   - vol_id: Volume number.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: This function is called by __fat_reinit_vol_cache() in vol.c.
          Normally, it is called to working  of Mount or Synchronous.
 */

result_t fat_reinit_cache_vol(uint32_t vol_id)
  {
  list_head_t *list;
  fat_cache_entry_t *pos, *n;
  int32_t i;


  for (i = 0; i < FAT_CACHE_HASH; i++)
    {
    list = &fat_cache_hash_lru[vol_id][i];

    list_for_each_entry_safe(fat_cache_entry_t, pos, n, list, hash_head)
      {
      /* The 'pos' entry is deleted from the hash list by fat_clean_csector(). */
      fat_clean_csector(pos);
      }
    }

  tr_fat_reset_cache_refs(vol_id);

  return s_ok;
  }

/*
 Name: __fat_lookup_csector
 Desc: Inspects the corresponding sector should have existed inside the fat-cache.
       And If it exists, it returns a cache-entry.
 Params:
   - hash_list: Head of hash-list.
   - vol_id: id of volume.
   - sect_no: number of sector.
 Returns:
   fat_cache_entry_t*  Value on success. cache's entry.
                         NULL on fail. The requested sector is not found in cache.
 Caveats: This function is only called by fat_get_csector() in fat.c.
 */

static fat_cache_entry_t *__fat_lookup_csector(list_head_t *hash_list, uint32_t vol_id, uint32_t sect_no)
  {
  fat_cache_entry_t *pos;

  uint32_t sect_min;
  fat_volinfo_t *fvi;
  fvi = GET_FAT_VOL(vol_id);

  sect_min = CACHE_MIN_SECT_NUM(sect_no, fvi->br.first_fat_sect);

  /* Search for MRU in hash list. */
  list_for_each_entry_rev(fat_cache_entry_t, pos, hash_list, hash_head)
    {
    if (pos->sect_no == sect_min && pos->vol_id == vol_id)
      return pos;
    }

  return (fat_cache_entry_t *) NULL;
  }

/*
 Name: fat_load_csector
 Desc: It reads the sector from LIM layer, it composes the cache-entry.
 Params:
   - entry: Pointer of cache-entry.
   - vol_id: ID of volume.
   - sect_no: Number of sector.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: This function is only called by fat_get_csector() in fat.c.
 */

int32_t fat_load_csector(fat_cache_entry_t *entry, uint32_t vol_id, uint32_t sect_no)
  {
  fat_volinfo_t *fvi;
  fvi = GET_FAT_VOL(vol_id);
  if (entry->flag & CACHE_DIRTY)
    return set_errno(e_fcache);

  entry->vol_id = (uint8_t) vol_id;
  entry->sect_no = CACHE_MIN_SECT_NUM(sect_no, fvi->br.first_fat_sect);

  /* Read entry's data from LIM layer. */
  if (0 > lim_read_sector(entry->vol_id, entry->sect_no, entry->buf, CACHE_SECTOR_NUM))
    return get_errno();

  entry->flag = CACHE_VALID;

  return s_ok;
  }

/*
 Name: fat_flush_csector
 Desc: Write the cache-entry through the LIM layer and it empties the entry.
 Params:
   - entry: Pointer of cache-entry.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: This function flushing only the cache-entry of CACHE_DIRTY condition.
 */

int32_t fat_flush_csector(fat_cache_entry_t *entry)
  {
  uint32_t sect_align;
  fat_volinfo_t *fvi;
  fvi = GET_FAT_VOL(entry->vol_id);
  if (!(entry->flag & CACHE_VALID))
    return set_errno(e_fcache);

  sect_align = fvi->br.fat_sect_cnt % CACHE_SECTOR_NUM;
  /* Write entry's data to LIM layer. */
  if (sect_align != 0 && entry->sect_no == CACHE_MIN_SECT_NUM(fvi->br.first_root_sect, fvi->br.first_fat_sect))
    {
    if (0 > lim_write_sector(entry->vol_id, entry->sect_no, entry->buf, sect_align))
      return get_errno();
    }
  else
    {
    if (0 > lim_write_sector(entry->vol_id, entry->sect_no, entry->buf, CACHE_SECTOR_NUM))
      return get_errno();
    }

  /* Move an cache-entry to the end of the list. (LRU) */
  list_move_tail(&fat_cache_lru, &entry->head);

  entry->flag &= ~(uint8_t) CACHE_DIRTY;

  return s_ok;
  }

/*
 Name: fat_sync_table
 Desc: This function flushs all cache-entry of the volume inside.
 Params:
   - vol_id: ID of volume.
   - isfifo: Resolve order of flushing.
             true: FIFO
             false: LIFO
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: This function it flushing only the entry which is a dirty condition
          from the fat_flush_csector() calling it is doing inside.
 */

int32_t fat_sync_table(uint32_t vol_id, bool isfifo)
  {
  fat_cache_entry_t *pos, *n;
  list_head_t *list;
  int32_t rtn;


  /* Get dirty list to store the cache-entry. */
  list = &fat_cache_dirty_lru[vol_id];

  if (isfifo)
    {

    /* Flush FIFO(from LRU). */
    list_for_each_entry_safe(fat_cache_entry_t, pos, n, list, head)
      {
      rtn = fat_flush_csector(pos);
      if (s_ok != rtn) return rtn;
      }
    }
  else
    {

    /* Flush LIFO(from MRU). */
    list_for_each_entry_safe_rev(fat_cache_entry_t, pos, n, list, head)
      {
      rtn = fat_flush_csector(pos);
      if (s_ok != rtn) return rtn;
      }
    }

  return s_ok;
  }

/*
 Name: fat_clean_csector
 Desc: The entry is deleted from the hash list.
 Params:
   - entry: Pointer of cache-entry.
 Returns: None.
 Caveats: None.
 */

void fat_clean_csector(fat_cache_entry_t *entry)
  {
  if (!((entry->flag & CACHE_VALID) || (entry->flag & CACHE_DIRTY)))
    return;

  list_del_init(&entry->hash_head);
  list_move(&fat_cache_lru, &entry->head);
  entry->flag = CACHE_FREE;

  return;
  }

/*
 Name: fat_clean_csectors
 Desc: clean all sectors in cache
 Params:
   - vol_id: Volume's ID to be cleaned.
 Returns: None.
 Caveats: None.
 */

void fat_clean_csectors(uint32_t vol_id)
  {
  fat_cache_entry_t *pos, *n;
  list_head_t *list;


  /* Search dirty list to clean the cache-entrys. */
  list = &fat_cache_dirty_lru[vol_id];

  list_for_each_entry_safe(fat_cache_entry_t, pos, n, list, head)
    {
    fat_clean_csector(pos);
    }
  }

/*
 Name: fat_get_csector
 Desc: Get sector pointer form hash list, normal list or dirty list.
 Params:
   - vol_id: Voulume's ID to be got sector.
   - sect_no: Sector number.
 Returns:
   fat_cache_entry_t *  entry pointer  on success.
                        NULL  on fail.
 Caveats: None.
 */

fat_cache_entry_t *fat_get_csector(uint32_t vol_id, uint32_t sect_no)
  {
  fat_volinfo_t *fvi;
  fat_cache_entry_t *pos, *n;
  list_head_t *list, *hash_list;
  int32_t hash, vol_idx;

  fvi = GET_FAT_VOL(vol_id);
  sect_no += fvi->br.first_fat_sect;

  tr_fat_inc_cache_refs(vol_id);

  /* Search hash list. */
  hash = FAT_GET_CACHE_HASH(sect_no, fvi->br.first_fat_sect);
  hash_list = &fat_cache_hash_lru[vol_id][hash];

  /* Lookup sector from the hash list. */
  pos = __fat_lookup_csector(hash_list, vol_id, sect_no);
  if (pos)
    {
    list_move_tail(hash_list, &pos->hash_head);
    pos->ref_cnt++;
    return pos;
    }

  /* Search normal list. */
  list = &fat_cache_lru;

  list_for_each_entry_safe(fat_cache_entry_t, pos, n, list, head)
    {
    if (0 == pos->ref_cnt)
      {
      /* It reads the Cache-entry from LIM layer. */
      if (0 > fat_load_csector(pos, vol_id, sect_no))
        return (fat_cache_entry_t *) NULL;
      pos->ref_cnt = 1;

      /* Move an cache-entry to the end of the normal list. (LRU) */
      list_move_tail(list, &pos->head);

      /* When mount occurs, bitmap is composed after FAT Table is read.   */
      /* At that time, hash list is composed and list_move is used not to */
      /* update the front part of FAT Table which is used very often on   */
      /* the hash list. After mount, list_move_tail is used to update to  */
      /* the thing using recently.                                        */
      if ((uint32_t) eFAT_INVALID_FREECOUNT == fvi->br.free_clust_cnt)
        /* If The file-system was not mounted, add to FIFO. */
        list_move(hash_list, &pos->hash_head);
      else
        /* If it was mounted, add to LIFO. */
        list_move_tail(hash_list, &pos->hash_head);
      return pos;
      }
    }

  /* Search dirty list. */
  for (vol_idx = 0; vol_idx < VOLUME_NUM; vol_idx++)
    {
    list = &fat_cache_dirty_lru[vol_idx];

    list_for_each_entry_safe(fat_cache_entry_t, pos, n, list, head)
      {
      if (0 == pos->ref_cnt)
        {
        /* It stores the Entry in the LIM and it empties the entry. */
        if (0 > fat_flush_csector(pos))
          return (fat_cache_entry_t *) NULL;
        /* It reads the Cache-entry from LIM layer. */
        if (0 > fat_load_csector(pos, vol_id, sect_no))
          return (fat_cache_entry_t *) NULL;
        pos->ref_cnt = 1;
        /* Move an cache-entry to the end of the hash list. (LRU) */
        list_move_tail(hash_list, &pos->hash_head);
        return pos;
        }
      }
    }
  set_errno(e_nomem);
  return (fat_cache_entry_t *) NULL;
  }

/*
 Name: fat_rel_csector
 Desc: Release cache-entry.
 Params:
   - entry: Pointer of cache-entry.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t fat_rel_csector(fat_cache_entry_t *entry)
  {
  tr_fat_dec_cache_refs(entry->vol_id);

  if (0 < entry->ref_cnt)
    entry->ref_cnt--;

  return s_ok;
  }

/*
 Name: fat_mark_dirty_csector
 Desc: Mark dirty sector. Set flag is CACHE_DIRTY & List head move.
 Params:
   - entry: Pointer of cache-entry.
 Returns: None.
 Caveats: None.
 */

void fat_mark_dirty_csector(fat_cache_entry_t *entry)
  {
  list_head_t *list;


  /* Search dirty list. */
  list = &fat_cache_dirty_lru[entry->vol_id];
  list_move_tail(list, &entry->head);

  entry->flag |= CACHE_DIRTY;
  }

/*
 Name: fat_map_init
 Desc: Initialize the FAT bit-map.
 Params:
   - vol_id: Volume's ID to be initialized the bit-map.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_map_init(uint32_t vol_id)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce;
  uint32_t fat_sect_cnt,
    fats_per_sect,
    first_free_clust,
    used_clust_cnt,
    *curmap,
    bit,
    uint32_fat_map,
    clust_cnt_in_fat,
    calced_clsut_cnt,
    i, j;


  fat_sect_cnt = fvi->br.fat_sect_cnt;
  fats_per_sect = fvi->br.bytes_per_sect >> ((uint32_t) fvi->br.efat_type >> 1);
  used_clust_cnt = first_free_clust = 0;

  clust_cnt_in_fat = fvi->br.last_data_clust + 1;
  uint32_fat_map = clust_cnt_in_fat / 32;

  /* Allocate bit-map area. */
  curmap = fvi->br.fat_map = __fat_get_fatmap(vol_id, uint32_fat_map);
  if (NULL == fvi->br.fat_map)
    return set_errno(e_port);

  fsm_assert1(0 == ((uint32_t) fvi->br.fat_map & 0x3));

  memset(fvi->br.fat_map, 0, uint32_fat_map * sizeof (uint32_t));

  calced_clsut_cnt = 0;

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    uint16_t *buf16;

    /* Update bit-map. */
    for (i = 0; i < fat_sect_cnt; i++)
      {
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, i);
      if (NULL == ce) return get_errno();
      buf16 = (uint16_t *) FAT_REAL_BUF_ADDR(ce->buf, i);

      for (bit = j = 0; j < fats_per_sect; j++, buf16++)
        {
        if (eFAT_FREE == *buf16)
          {
          if (0 == first_free_clust)
            first_free_clust = (i * fats_per_sect) + j;
          }
        else
          {
          used_clust_cnt++;
          bit_set(curmap, bit);
          }

        if (++calced_clsut_cnt == clust_cnt_in_fat)
          {
          /* Set break condition of the outer loop */
          i = fat_sect_cnt;
          break;
          }

        if (0 == (++bit & BITS_UINT32_MASK))
          {
          curmap++;
          bit = 0;
          }
        }

      fat_rel_csector(ce);
      }
    }
  else
    { /*eFAT32_SIZE*/
    uint32_t *buf32;

    /* Bit-map set up. */
    for (i = 0; i < fat_sect_cnt; i++)
      {
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, i);
      if (NULL == ce) return get_errno();
      buf32 = (uint32_t *) FAT_REAL_BUF_ADDR(ce->buf, i);

      for (bit = j = 0; j < fats_per_sect; j++, buf32++)
        {
        if (IS_FAT32_FREE(*buf32))
          {
          if (0 == first_free_clust)
            first_free_clust = (i * fats_per_sect) + j;
          }
        else
          {
          used_clust_cnt++;
          bit_set(curmap, bit);
          }

        if (++calced_clsut_cnt == clust_cnt_in_fat)
          {
          /* Set break condition of the outer loop */
          i = fat_sect_cnt;
          break;
          }

        if (0 == (++bit & BITS_UINT32_MASK))
          {
          curmap++;
          bit = 0;
          }
        }

      fat_rel_csector(ce);
      }
    }

  fvi->br.srch_free_clust = first_free_clust;
  fvi->br.free_clust_cnt = fvi->br.data_clust_cnt - used_clust_cnt;
  return s_ok;
  }

/*
 Name: fat_map_deinit
 Desc: De-initialize the FAT bit-map.
 Params:
   - vol_id: Volume's ID to be initialized.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t fat_map_deinit(uint32_t vol_id)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);

  if (fvi->br.fat_map)
    fvi->br.fat_map = (uint32_t *) NULL;

  return s_ok;
  }

/*
 Name: fat_map_reinit
 Desc: Re-initialize the FAT bit-map.
 Params:
   - vol_id: Volume's ID to be initialized the bit-map.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_map_reinit(uint32_t vol_id)
  {
  fat_map_deinit(vol_id);
  return fat_map_init(vol_id);
  }

/*
 Name: fat_map_read_free_clusts
 Desc: scan free-clusters, return free clusters.
 Params:
   - vol_id: Volume's ID to be got free cluster.
   - clust_cnt: Need cluster count.
   - clust_list: Cluster number list pointer.
 Returns:
   int32_t  >=0 on success. The returned value is the number of accumulated free clusters.
            < 0  on fail.
 Caveats: None.
 */

int32_t fat_map_read_free_clusts(uint32_t vol_id, uint32_t clust_cnt, uint32_t *clust_list)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  uint32_t clust_acc, /* Number of accumulated cluster. */
    first_clust,
    map_offs,
    *p_clust,
    *fatmap,
    map,
    cnt,
    i, j;


  if (0 == clust_cnt || NULL == clust_list)
    return set_errno(e_inval);

  p_clust = clust_list;
  clust_acc = 0;
  first_clust = fvi->br.srch_free_clust & ~BITS_UINT32_MASK;
  /* This is the Offset of the BITS_PER_UINT32 unit regarding the cluster number. */
  map_offs = fvi->br.srch_free_clust / BITS_PER_UINT32;
  fatmap = fvi->br.fat_map + map_offs;

  /* Scan free-clusters from bit-map. */
  for (i = first_clust; i < fvi->br.last_data_clust; i += BITS_PER_UINT32, fatmap++)
    {
    map = *fatmap;
    if (BITS_ALL_ZERO == map)
      {
      /* Calculate remain clusters. */
      cnt = clust_cnt - clust_acc;

      if (BITS_PER_UINT32 <= cnt)
        {
        for (j = 0; j < BITS_PER_UINT32; j++)
          *p_clust++ = i + j;
        clust_acc += BITS_PER_UINT32;
        }
      else
        {
        for (j = 0; j < cnt; j++)
          *p_clust++ = i + j;
        clust_acc += cnt;
        }

      if (BITS_PER_UINT32 >= cnt)
        goto End;
      }
    else if (BITS_UINT32_ALL_ONE != map)
      {
      do
        {
        /* Find first zero. */
        j = bit_ffz(map);
        bit_set(&map, j);
        *p_clust++ = i + j;
        if (++clust_acc == clust_cnt)
          goto End;
        }
      while (BITS_UINT32_ALL_ONE != map);
      }
    }

  return set_errno(e_cfat);

End:
  return clust_acc;
  }

/*
 Name: fat_map_alloc_clusts
 Desc: Searches a free cluster from the Bit-map and allocates it on the list.
 Params:
   - vol_id: Volume's ID to allocate a cluster.
   - clust_cnt: The number of needed clusters.
   - clust_list: Poiniter to the list of clusters to allocate.
 Returns:
   int32_t  >=0 on success. The returned value is the number of allocated clusters.
            < 0  on fail.
 Caveats: None.
 */

int32_t fat_map_alloc_clusts(uint32_t vol_id, uint32_t clust_cnt, uint32_t *clust_list)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  uint32_t clust_acc, /* Number of accumulated cluster. */
    first_clust,
    map_offs,
    *p_clust,
    *fatmap,
    cnt,
    i, j;


  if (0 == clust_cnt || NULL == clust_list)
    return set_errno(e_inval);

  if (fvi->br.free_clust_cnt < clust_cnt)
    return set_errno(e_nospc);

  p_clust = clust_list;
  clust_acc = 0;
  first_clust = fvi->br.srch_free_clust & ~BITS_UINT32_MASK;
  /* The Offset of the BITS_PER_UINT32 unit regarding the cluster number. */
  map_offs = fvi->br.srch_free_clust / BITS_PER_UINT32;
  fatmap = fvi->br.fat_map + map_offs;

  /* Scan free-clusters from bit-map */
  for (i = first_clust; i < fvi->br.last_data_clust; i += BITS_PER_UINT32, fatmap++)
    {
    if (BITS_ALL_ZERO == *fatmap)
      {
      /* calculate remain clusters */
      cnt = clust_cnt - clust_acc;

      if (BITS_PER_UINT32 <= cnt)
        {
        for (j = 0; j < BITS_PER_UINT32; j++)
          *p_clust++ = i + j;
        *fatmap = BITS_UINT32_ALL_ONE;
        }
      else
        {
        for (j = 0; j < cnt; j++)
          *p_clust++ = i + j;
        *fatmap = (1 << cnt) - 1;
        }

      if (BITS_PER_UINT32 >= cnt)
        goto End;

      clust_acc += BITS_PER_UINT32;
      }
    else if (BITS_UINT32_ALL_ONE != *fatmap)
      {
      do
        {
        /* Look for first 0 bit at current fatmap. */
        j = bit_ffz(*fatmap);
        fsm_assert3(0 == bit_get(*fatmap, j));
        bit_set(fatmap, j);
        *p_clust++ = i + j;
        if (++clust_acc == clust_cnt)
          goto End;
        }
      while (BITS_UINT32_ALL_ONE != *fatmap);
      }
    }

  return set_errno(e_cfat);

End:
  fsm_assert1(fvi->br.srch_free_clust < *(p_clust - 1) + 1);
  fvi->br.srch_free_clust = *(p_clust - 1) + 1/*searching from next cluster*/;
  fvi->br.free_clust_cnt -= clust_cnt;
  return clust_cnt;
  }

/*
 Name: fat_map_free_clusts
 Desc: Make cluster in free state in the Bit-Map.
 Params:
   - vol_id: Volume's ID to be got free cluster.
   - clust_cnt: Need cluster count.
   - clust_list: Cluster number list pointer.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_map_free_clusts(uint32_t vol_id, uint32_t clust_cnt, uint32_t *clust_list)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  uint32_t *p_clust,
    *fatmap,
    map_offs,
    bit, /*cluster bit*/
    i;


  if (0 == clust_cnt || NULL == clust_list)
    return set_errno(e_inval);

  p_clust = clust_list;

  for (i = 0; i < clust_cnt; i++, p_clust++)
    {
    /* The Offset of the BITS_PER_UINT32 unit regarding the cluster number. */
    map_offs = *p_clust / BITS_PER_UINT32;
    bit = *p_clust & BITS_UINT32_MASK;
    fatmap = fvi->br.fat_map + map_offs;

    fsm_assert3(1 == bit_get(*fatmap, bit));
    bit_clear(fatmap, bit);

    /* Update the first free cluster if new one is smaller than. */
    if (fvi->br.srch_free_clust > *p_clust)
      fvi->br.srch_free_clust = *p_clust;
    }

  fvi->br.free_clust_cnt += clust_cnt;
  return s_ok;
  }

/*
 Name: fat_map_free_link_clusts
 Desc: free clusters from first cluster. freeing use fat-link
 Params:
   - vol_id: Volume's ID to be got free cluster.
   - first_clust_no: first cluster number.
   - skip_cnt: cluster count for skip.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_map_free_link_clusts(uint32_t vol_id, uint32_t first_clust_no, uint32_t skip_cnt)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce = (fat_cache_entry_t *) NULL;
  uint32_t clust_no,
    fat_sectno,
    next_fat_sectno,
    free_cnt,
    fat_offs;


  clust_no = first_clust_no;
  free_cnt = 0;

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    uint16_t *buf16;
    while (1)
      {
      fat_sectno = F16T_CLUST_2_SECT(fvi, clust_no);

      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf16 = (uint16_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      /* Search FAT table */
      while (1)
        {
        fsm_assert1(0 != clust_no);

        fat_offs = F16T_CLUST_2_OFFS_INSECT(fvi, clust_no);
        clust_no = buf16[fat_offs];

        free_cnt++;
        if (free_cnt == skip_cnt)
          buf16[fat_offs] = (uint16_t) eFAT16_EOC;
        else if (free_cnt > skip_cnt)
          {
          buf16[fat_offs] = (uint16_t) eFAT_FREE;

          /* Update the first free cluster if new one is smaller than. */
          if (fvi->br.srch_free_clust > clust_no)
            fvi->br.srch_free_clust = clust_no;
          }

        if (IS_FAT16_EOC(clust_no)) goto End;
        next_fat_sectno = F16T_CLUST_2_SECT(fvi, clust_no);
        /* End loop if next cluster does not exist in the same cache-entry. */
        if (next_fat_sectno != fat_sectno) break;
        }

      /* Mark the dirty list with the changed cache-entry. */
      fat_mark_dirty_csector(ce);
      fat_rel_csector(ce);
      }
    }
  else
    { /* eFAT32_SIZE */
    uint32_t *buf32;
    while (1)
      {
      fat_sectno = F32T_CLUST_2_SECT(fvi, clust_no);

      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf32 = (uint32_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      /* Search FAT table */
      while (1)
        {
        fat_offs = F32T_CLUST_2_OFFS_INSECT(fvi, clust_no);
        clust_no = buf32[fat_offs];

        free_cnt++;
        if (free_cnt == skip_cnt)
          buf32[fat_offs] = (uint32_t) eFAT32_EOC;
        else if (free_cnt > skip_cnt)
          {
          buf32[fat_offs] = (uint32_t) eFAT_FREE;

          /* Update the first free cluster if new one is smaller than. */
          if (fvi->br.srch_free_clust > clust_no)
            fvi->br.srch_free_clust = clust_no;
          }

        if (IS_FAT32_EOC(clust_no)) goto End;
        next_fat_sectno = F32T_CLUST_2_SECT(fvi, clust_no);
        /* End loop if next cluster does not exist in the same cache-entry. */
        if (next_fat_sectno != fat_sectno) break;
        }

      /* Mark the dirty list with the changed cache-entry. */
      fat_mark_dirty_csector(ce);
      fat_rel_csector(ce);
      }
    }

End:

  if (ce)
    {
    /* Mark the dirty list with the changed cache-entry. */
    fat_mark_dirty_csector(ce);
    fat_rel_csector(ce);
    }

  fvi->br.free_clust_cnt += free_cnt;
#if defined( LOG )
  fat_sync_table(vol_id, false);
#endif


  /* Re-initialize fat-map for sync. */
  return fat_map_reinit(vol_id);
  }

/*
 Name: fat_map_sync_clusts
 Desc: synchronize fat_map with physical fat-chain
 Params:
   - vol_id: Volume's ID to be synchronized cluster.
   - clust_cnt: Cluster count to be synchronized.
   - clust_list: Cluster number list pointer.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_map_sync_clusts(uint32_t vol_id, uint32_t clust_cnt, uint32_t *clust_list)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  uint32_t *p_clust,
    *fatmap,
    map_offs,
    bit,
    i;
  int32_t rtn;


  if (0 == clust_cnt || NULL == clust_list)
    return set_errno(e_inval);

  p_clust = clust_list;

  for (i = 0; i < clust_cnt; i++, p_clust++)
    {
    /* The Offset of the BITS_PER_UINT32 unit regarding the cluster number. */
    map_offs = (*p_clust / BITS_PER_UINT32);
    fatmap = fvi->br.fat_map + map_offs;
    bit = *p_clust & BITS_UINT32_MASK;

    rtn = fat_get_next_clustno(vol_id, *p_clust);
    if (0 > rtn) return rtn;

    if (rtn != bit_get(*fatmap, bit))
      {
      if (rtn /* Is alloc */)
        {
        bit_set(fatmap, bit);
        fvi->br.free_clust_cnt--;
        }
      else
        { /* Is free */
        /* Update the first free cluster if new one is smaller than. */
        if (fvi->br.srch_free_clust > *p_clust)
          fvi->br.srch_free_clust = *p_clust;
        bit_clear(fatmap, bit);
        fvi->br.free_clust_cnt++;
        }
      }
    }

  return s_ok;
  }

/*
 Name: fat_stamp_clusts
 Desc: Stamp the cluster of the cluster's list in the fat-cache and
       composes a link.
 Params:
   - vol_id: Volume's ID to be synchronized cluster.
   - clust_cnt: Cluster count to be allocated.
   - clust_list: Cluster list pointer.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: NOTE! clust_list[alloc_clust_cnt] must point to the next free cluster.
 */

int32_t fat_stamp_clusts(uint32_t vol_id, uint32_t alloc_clust_cnt, uint32_t *clust_list)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce = (fat_cache_entry_t *) NULL;
  uint32_t fat_sectno,
    next_fat_sectno,
    fat_offs,
    clust_acc, /* Number of accumulated cluster. */
    last_clustno;


  if (0 == alloc_clust_cnt || NULL == clust_list)
    return e_inval;

  clust_acc = 0;
  last_clustno = *clust_list++;

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    uint16_t *buf16;

    fat_sectno = F16T_CLUST_2_SECT(fvi, *clust_list);

    while (1)
      {
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf16 = (uint16_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      do
        {
        fat_offs = F16T_CLUST_2_OFFS_INSECT(fvi, *clust_list);

        /* It adds the Cluster to the linked cluster. */
        if (0 == clust_acc && last_clustno)
          {
          fsm_assert2(last_clustno != *clust_list);

          if (last_clustno > *clust_list)
            {
            /* Append cluster and flush. */
            if (0 > fat_set_next_clustno(vol_id, last_clustno, *clust_list))
              return -1;
            }
          else
            fat_append_clustno(vol_id, last_clustno, *clust_list);
          }

        buf16[fat_offs] = (uint16_t) *++clust_list; /* Link next cluster. */

        if (alloc_clust_cnt == ++clust_acc)
          {
          buf16[fat_offs] = (uint16_t) eFAT16_EOC;
          goto End;
          }
        next_fat_sectno = F16T_CLUST_2_SECT(fvi, *clust_list);
        /* End loop if next cluster does not exist in the same cache-entry. */
        }
      while (fat_sectno == next_fat_sectno);

      fat_sectno = next_fat_sectno;

      /* Mark the dirty list with the changed cache-entry. */
      fat_mark_dirty_csector(ce);
      fat_rel_csector(ce);
      }
    }
  else
    { /* eFAT32_SIZE */
    uint32_t *buf32;

    fat_sectno = F32T_CLUST_2_SECT(fvi, *clust_list);

    while (1)
      {
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf32 = (uint32_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      do
        {
        fat_offs = F32T_CLUST_2_OFFS_INSECT(fvi, *clust_list);

        if (0 == clust_acc && last_clustno)
          fat_append_clustno(vol_id, last_clustno, *clust_list);

        buf32[fat_offs] = *++clust_list; /* Link next cluster. */

        if (alloc_clust_cnt == ++clust_acc)
          {
          buf32[fat_offs] = (uint32_t) eFAT32_EOC;
          goto End;
          }
        next_fat_sectno = F32T_CLUST_2_SECT(fvi, *clust_list);
        /* End loop if next cluster does not exist in the same cache-entry. */
        }
      while (fat_sectno == next_fat_sectno);

      fat_sectno = next_fat_sectno;

      /* Mark the dirty list with the changed cache-entry. */
      fat_mark_dirty_csector(ce);
      fat_rel_csector(ce);
      }
    }

End:
  /* Mark the dirty list with the changed cache-entry. */
  fat_mark_dirty_csector(ce);
  fat_rel_csector(ce);

  return s_ok;
  }

/*
 Name: __fat_unlink_clust_list
 Desc: Unlink all cluster of cluster list and change them in free state.
       Update Bitmap.
 Params:
   - vol_id: The ID of volume that clusters is unlinked.
   - clust_list: Pointer to the cluster list.
   - free_cnt: The number of clusters to be free.
   - skip_cnt: The number of clustera to be skip.
   - mark_eoc: If this value is true, 'clust_list[0]' should be marked to eFAT_EOC.
               Otherwise we will unlink from the first cluster.
 Returns:
   int32_t  >=0 on success. The returned value is the number of free clusters.
            < 0  on fail.
 Caveats: We free from the last cluster to the first cluster reversly.
 */

static int32_t __fat_unlink_clust_list(uint32_t vol_id, uint32_t *clust_list,
                                       uint32_t free_cnt, bool mark_eoc)
  {
  pim_devinfo_t *pdi = GET_LIM_DEV(vol_id);
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce = (fat_cache_entry_t *) NULL;
  uint32_t clust_no,
    last_clust_no,
    *free_list,
    fat_sectno,
    next_fat_sectno,
    acc_srch_cnt = 0,
#if ( FSM_ASSERT >= 1 )
    real_free_cnt,
#endif
    fat_offs,
    start_sect_no = 0,
    next_clust_no,
    sect_cnt,
    acc_sect_cnt = 0,
    clust_cnt,
    *fatmap,
    *curmap,
    map_offs,
    bit;
  int32_t rtn;
  bool eoc_is_one_for_dbg = false;


  fatmap = fvi->br.fat_map;
  sect_cnt = fvi->br.sects_per_clust;
#if ( FSM_ASSERT >= 1 )
  real_free_cnt = 0;
#endif

  if (mark_eoc)
    {
    clust_cnt = free_cnt + 1/*EOC cluster*/;
    last_clust_no = clust_list[0];
    }
  else
    {
    clust_cnt = free_cnt;
    last_clust_no = 0;
    }
  free_list = &clust_list[clust_cnt - 1/*array base*/];
  clust_no = *free_list;
  next_clust_no = clust_no + 1;

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    uint16_t *buf16;
    while (1)
      {
      fat_sectno = F16T_CLUST_2_SECT(fvi, clust_no);

      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf16 = (uint16_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      /* Search FAT table */
      while (1)
        {
        if (ePIM_NeedErase & pdi->dev_flag)
          {
          /* If previously cluster and cluster is not sequential, */
          /* erase sectors from start number of sector as count   */
          /* and then setup the new start number of sector.       */
          if (clust_no + 1 == next_clust_no)
            acc_sect_cnt += sect_cnt;
          else
            {
            use_unlink_fat_unlock();
            start_sect_no = D_CLUST_2_SECT(fvi, next_clust_no);
            rtn = lim_erase_sector(vol_id, start_sect_no, acc_sect_cnt);
            use_unlink_fat_lock();
            if (0 > rtn) goto End;
            acc_sect_cnt = sect_cnt;
            }
          next_clust_no = clust_no;
          }

        /* Update the first free cluster if new one is smaller than. */
        if (fvi->br.srch_free_clust > clust_no)
          fvi->br.srch_free_clust = clust_no;
        fat_offs = F16T_CLUST_2_OFFS_INSECT(fvi, clust_no);

        acc_srch_cnt++;
        if (last_clust_no == clust_no)
          {
          fsm_assert1(false == eoc_is_one_for_dbg);
          eoc_is_one_for_dbg = true;
          buf16[fat_offs] = (uint16_t) eFAT16_EOC;
          }
        else
          {
          fsm_assert2(acc_srch_cnt <= free_cnt + (mark_eoc ? 1 : 0));
          fsm_assert1((uint16_t) eFAT_FREE != buf16[fat_offs]);
#if ( FSM_ASSERT >= 1 )
          if ((uint16_t) eFAT_FREE != buf16[fat_offs])
            real_free_cnt++;
#endif
          buf16[fat_offs] = (uint16_t) eFAT_FREE;

          /* Update fat-map to free */
          map_offs = (clust_no / BITS_PER_UINT32);
          curmap = fatmap + map_offs;
          bit = clust_no & BITS_UINT32_MASK;
          bit_clear(curmap, bit);
          }

        if ((acc_srch_cnt == clust_cnt) || (0 == clust_no)) goto End;

        /* It brings the cluster it will erase from list.*/
        clust_no = *--free_list;
        next_fat_sectno = F16T_CLUST_2_SECT(fvi, clust_no);
        /* End loop if next cluster does not exist in the same cache-entry. */
        if (next_fat_sectno != fat_sectno)
          break;
        }

      /* Mark the dirty list with the changed cache-entry. */
#if defined( LOG )
      fat_flush_csector(ce);
#else
      fat_mark_dirty_csector(ce);
#endif
      fat_rel_csector(ce);
      }
    }
  else
    { /* eFAT32_SIZE */
    uint32_t *buf32;
    while (1)
      {
      fat_sectno = F32T_CLUST_2_SECT(fvi, clust_no);

      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf32 = (uint32_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      /* Search FAT table */
      while (1)
        {
        if (ePIM_NeedErase & pdi->dev_flag)
          {
          /* If previously cluster and cluster is not sequential, */
          /* erase sectors from start number of sector as count   */
          /* and then setup the new start number of sector.       */
          if (clust_no + 1 == next_clust_no)
            acc_sect_cnt += sect_cnt;
          else
            {
            use_unlink_fat_unlock();
            start_sect_no = D_CLUST_2_SECT(fvi, next_clust_no);
            rtn = lim_erase_sector(vol_id, start_sect_no, acc_sect_cnt);
            use_unlink_fat_lock();
            if (0 > rtn) goto End;
            acc_sect_cnt = sect_cnt;
            }
          next_clust_no = clust_no;
          }

        /* Update the first free cluster if new one is smaller than. */
        if (fvi->br.srch_free_clust > clust_no)
          fvi->br.srch_free_clust = clust_no;
        fat_offs = F32T_CLUST_2_OFFS_INSECT(fvi, clust_no);

        acc_srch_cnt++;
        if (last_clust_no == clust_no)
          {
          fsm_assert1(false == eoc_is_one_for_dbg);
          eoc_is_one_for_dbg = true;
          buf32[fat_offs] = (uint32_t) eFAT32_EOC;
          }
        else
          {
          fsm_assert2(acc_srch_cnt <= free_cnt + (mark_eoc ? 1 : 0));
          fsm_assert1((uint32_t) eFAT_FREE != buf32[fat_offs]);
#if ( FSM_ASSERT >= 1 )
          if ((uint32_t) eFAT_FREE != buf32[fat_offs])
            real_free_cnt++;
#endif
          buf32[fat_offs] = (uint32_t) eFAT_FREE;

          /* Update fat-map to free */
          /* The Offset of the BITS_PER_UINT32 unit regarding the cluster number. */
          map_offs = (clust_no / BITS_PER_UINT32);
          curmap = fatmap + map_offs;
          bit = clust_no & BITS_UINT32_MASK;
          bit_clear(curmap, bit);
          }

        if ((acc_srch_cnt == clust_cnt) || (0 == clust_no)) goto End;

        /* It brings the cluster it will erase from list.*/
        clust_no = *--free_list;
        next_fat_sectno = F32T_CLUST_2_SECT(fvi, clust_no);
        /* End loop if next cluster does not exist in the same cache-entry. */
        if (next_fat_sectno != fat_sectno)
          break;
        }

      /* Mark the dirty list with the changed cache-entry. */
#if defined( LOG )
      fat_flush_csector(ce);
#else
      fat_mark_dirty_csector(ce);
#endif
      fat_rel_csector(ce);
      }
    }

End:
  if (ePIM_NeedErase & pdi->dev_flag)
    {
    if (mark_eoc && acc_sect_cnt)
      {
      next_clust_no++;
      acc_sect_cnt -= sect_cnt;
      }

    if (acc_sect_cnt)
      {
      use_unlink_fat_unlock();
      fsm_assert3(0 != next_clust_no);
      start_sect_no = D_CLUST_2_SECT(fvi, next_clust_no);
      rtn = lim_erase_sector(vol_id, start_sect_no, acc_sect_cnt);
      use_unlink_fat_lock();
      }
    }
  if (ce)
    {
    /* Mark the dirty list with the changed cache-entry. */
#if defined( LOG )
    fat_flush_csector(ce);
#else
    fat_mark_dirty_csector(ce);
#endif
    fat_rel_csector(ce);
    }

  fvi->br.free_clust_cnt += free_cnt;
#if !defined( LOG )
  fat_sync_table(vol_id, true);
#endif

  fsm_assert1(real_free_cnt == free_cnt);
  fsm_assert3(0 != clust_no);
  fsm_assert2(free_cnt == acc_srch_cnt - (mark_eoc ? 1 : 0));

  return free_cnt;
  }

/*
 Name: fat_unlink_clusts
 Desc: Unlink all clusters from a given cluster to the last cluster.
 Params:
   - vol_id: The ID of volume that clusters is unlinked.
   - first_clust: The first cluster of clusters that will be erased.
   - mark_eoc: If this value is true, the first cluster should be marked to eFAT_EOC.
               Otherwise we will unlink from the first cluster.
 Returns:
   int32_t  ==0 on success.
            < 0  on fail.
 Caveats: We free from the last cluster to the first cluster reversly.
 */

int32_t fat_unlink_clusts(int32_t vol_id, uint32_t first_clust, bool mark_eoc)
  {
#define REMEMBER_LIST_CNT 8
#define REMEMBER_LIST_MSK (REMEMBER_LIST_CNT-1)

  uint32_t rem_list[REMEMBER_LIST_CNT],
    rem_idx,
    rem_cnt,
    clust_list[CLUST_LIST_BUF_CNT + 1],
    clust_no,
    free_cnt,
    got_cnt = 0;
  const uint32_t rem_max_cnt = REMEMBER_LIST_CNT,
    rem_max_mask = REMEMBER_LIST_MSK,
    clust_cnt = CLUST_LIST_BUF_CNT;
  bool need_mark_eoc = false;
  int32_t rtn;


  if (0 == first_clust) return 0;

  do
    {
    clust_no = first_clust;
    rem_idx = 0;

    do
      {
      /* Get the cluster's list to be removed. */
      rtn = fat_get_clust_list(vol_id, clust_no, clust_cnt, clust_list);
      if (0 >= rtn) return rtn;

      if (rtn != clust_cnt)
        break;

      rem_list[rem_idx++&rem_max_mask] = clust_no;
      /*
         NOTE(GRP1453): The last cluster of rem_list[]'s entry will be used for next searching.
                        So each entry of ream_list has such as 'clust_cnt-1'.
       */
      clust_no = clust_list[rtn - 1/*array base*/];
      }
    while (1);

    if (0 == got_cnt)
      got_cnt = rtn;
    else
      {
      /*
         NOTE(GRP1453): This code is looping one more.
                        So the last cluster of rem_list[]'s entry was unlinked.
       */
      got_cnt = rtn - 1;
      }
    rem_cnt = rem_idx < rem_max_cnt ? rem_idx : rem_max_cnt;

    do
      {
#if 1
      if (mark_eoc && 0 == rem_idx)
        {
        fsm_assert1(0 == rem_cnt);
        need_mark_eoc = true;
        free_cnt = got_cnt - 1;
        }
#else
      if (mark_eoc && 0 == rem_cnt && 0 == rem_idx)
        {
        need_mark_eoc = true;
        free_cnt = got_cnt - 1;
        }
#endif
      else
        free_cnt = got_cnt;

      /*
         NOTE(GRP1453): It is essential to check whether free_cnt is 0.
       */
      if (free_cnt)
        {
        rtn = __fat_unlink_clust_list(vol_id, clust_list, free_cnt, need_mark_eoc);
        if (0 > rtn) break;
        rtn = s_ok;
        }
      need_mark_eoc = false;

      if (rem_cnt--)
        {
        /*
           NOTE(GRP1453): The last cluster of rem_list[]'s entry was used for the next rem_list[]'s entry.
                          So each entry of ream_list has such as 'clust_cnt-1'.
         */
        rtn = fat_get_clust_list(vol_id, rem_list[--rem_idx & rem_max_mask], clust_cnt - 1, clust_list);
        if (0 >= rtn)
          {
#if defined( DBG )
          break();
#endif
          return rtn;
          }
        got_cnt = rtn;
        }
      else
        break;
      }
    while (1);
    }
  while (rem_idx);

  return rtn;
  }

/*
 Name: fat_get_free_space
 Desc: calculate free clusters and lookup first free-cluster.
 Params:
   - vol_id: Volume's ID to be got free space.
   - free_clust_ptr: Free cluter pointer.
   - free_clust_cnt_ptr: Free cluster number list pointer.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_get_free_space(uint32_t vol_id, uint32_t *free_clust_ptr, uint32_t *free_clust_cnt_ptr)
  {
  fat_cache_entry_t *ce;
  fat_volinfo_t *fvi;
  uint32_t fat_sect_cnt,
    fats_per_sect,
    first_free_clust,
    used_clust_cnt,
    i, j;


  fvi = GET_FAT_VOL(vol_id);
  fat_sect_cnt = fvi->br.fat_sect_cnt;
  fats_per_sect = fvi->br.bytes_per_sect >> ((uint32_t) fvi->br.efat_type >> 1);
  used_clust_cnt = first_free_clust = 0;

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    uint16_t *buf16;

    for (i = 0; i < fat_sect_cnt; i++)
      {
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, i);
      if (NULL == ce) return get_errno();
      buf16 = (uint16_t *) FAT_REAL_BUF_ADDR(ce->buf, i);

      for (j = 0; j < fats_per_sect; j++, buf16++)
        {
        if (eFAT_FREE == *buf16)
          {
          if (0 == first_free_clust)
            first_free_clust = (i * fats_per_sect) + j;
          continue;
          }
        else
          used_clust_cnt++;
        }

      fat_rel_csector(ce);
      }
    }
  else
    { /*eFAT32_SIZE*/
    uint32_t *buf32;

    for (i = 0; i < fat_sect_cnt; i++)
      {
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, i);
      if (NULL == ce) return get_errno();
      buf32 = (uint32_t *) FAT_REAL_BUF_ADDR(ce->buf, i);

      for (j = 0; j < fats_per_sect; j++, buf32++)
        {
        if (IS_FAT32_FREE(*buf32))
          {
          if (0 == first_free_clust)
            first_free_clust = (i * fats_per_sect) + j;
          continue;
          }
        else
          used_clust_cnt++;
        }

      fat_rel_csector(ce);
      }
    }

  *free_clust_ptr = first_free_clust;
  *free_clust_cnt_ptr = fvi->br.data_clust_cnt - used_clust_cnt;
  return s_ok;
  }

/*
 Name: fat_get_clust_list
 Desc: Get clusters as the needed number.
 Params:
   - vol_id: The ID of volume to get clusters.
   - clust_no: the cluster number
   - clust_cnt: Needed number of clusters
   - clust_list: Pointer to the cluster list for linking gotten clusters.
 Returns:
   int32_t  >=0 on success. The returned value is the number of clusters.
            < 0 on fail.
 Caveats: None.
 */

int32_t fat_get_clust_list(uint32_t vol_id, uint32_t clust_no,
                           uint32_t clust_cnt, uint32_t *clust_list)
  {
#define MAX_CLUST_CNT 0x0FFFFFFF
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce;
  uint32_t fat_sectno,
    next_fat_sectno,
    fat_offs,
    acc_clust_cnt; /* Accumulated cluster count. */


  if (0 == clust_no)
    return set_errno(e_inval);

  if (0 == clust_cnt) clust_cnt = MAX_CLUST_CNT;
  acc_clust_cnt = 0;

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    uint16_t *buf16;
    while (1)
      {
      fat_sectno = F16T_CLUST_2_SECT(fvi, clust_no);
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf16 = (uint16_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      /* Get all cluster which is linked.*/
      while (1)
        {
        *clust_list++ = clust_no;
        if (++acc_clust_cnt == clust_cnt) goto Result;

        fat_offs = F16T_CLUST_2_OFFS_INSECT(fvi, clust_no);
        clust_no = buf16[fat_offs];

        if (eFAT_FREE == clust_no) goto Result;

        if (IS_FAT16_EOC(clust_no))
          {
          *clust_list++ = 0; /* This controls the linked last cluster. */
          goto Result;
          }
        next_fat_sectno = F16T_CLUST_2_SECT(fvi, clust_no);
        /* End loop if next cluster does not exist in the same cache-entry. */
        if (next_fat_sectno != fat_sectno)
          break;
        }

      fat_rel_csector(ce);
      }
    }
  else
    { /* eFAT32_SIZE */
    uint32_t *buf32;
    while (1)
      {
      fat_sectno = F32T_CLUST_2_SECT(fvi, clust_no);
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf32 = (uint32_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      /* Get all cluster which is linked.*/
      while (1)
        {
        *clust_list++ = clust_no;
        if (++acc_clust_cnt == clust_cnt) goto Result;

        fat_offs = F32T_CLUST_2_OFFS_INSECT(fvi, clust_no);
        clust_no = buf32[fat_offs];

        if (eFAT_FREE == clust_no) goto Result;

        if (IS_FAT32_EOC(clust_no))
          {
          *clust_list++ = 0; /* This controls the linked last cluster. */
          goto Result;
          }
        next_fat_sectno = F32T_CLUST_2_SECT(fvi, clust_no);
        /* End loop if next cluster does not exist in the same cache-entry. */
        if (next_fat_sectno != fat_sectno)
          break;
        }

      fat_rel_csector(ce);
      }
    }

Result:
  fat_rel_csector(ce);

  return acc_clust_cnt;
  }

/*
 Name: fat_get_clust_cnt
 Desc: Calculate the number of clusters of the root-directory.
 Params:
   - vol_id: Volume's ID to get the number of clusters.
   - clust_no: cluster number
 Returns:
   int32_t  >=0 on success. The returned value is the number of accumulated clusters.
            < 0 on fail.
 Caveats: None.
 */

int32_t fat_get_clust_cnt(uint32_t vol_id, uint32_t clust_no, uint32_t *last_clust_no, uint32_t *last_clust_fat)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce;
  uint32_t fat_sectno,
    next_fat_sectno,
    fat_offs,
    last_valid_clust,
    prev_clust,
    acc_clust_cnt; /* Accumulated cluster count. */


  if (0 == clust_no)
    return set_errno(e_inval);

  prev_clust = last_valid_clust = clust_no;
  acc_clust_cnt = 0;

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    uint16_t *buf16;
    while (1)
      {
      fat_sectno = F16T_CLUST_2_SECT(fvi, clust_no);
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf16 = (uint16_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      while (1)
        {
        fat_offs = F16T_CLUST_2_OFFS_INSECT(fvi, clust_no);
        clust_no = buf16[fat_offs];

#if defined( LOG )
        if (eFAT_FREE == clust_no && false == fvi->fat_mounted)
          goto Result;
#endif

        acc_clust_cnt++;

        fsm_assert3(eFAT_FREE != clust_no);

        if (IS_FAT16_EOC(clust_no))
          goto Result;

        last_valid_clust = prev_clust;
        prev_clust = clust_no;
        next_fat_sectno = F16T_CLUST_2_SECT(fvi, clust_no);
        /* End loop if next cluster does not exist in the same cache-entry. */
        if (next_fat_sectno != fat_sectno) break;
        }

      fat_rel_csector(ce);
      }
    }
  else
    { /* eFAT32_SIZE */
    uint32_t *buf32;
    while (1)
      {
      fat_sectno = F32T_CLUST_2_SECT(fvi, clust_no);
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf32 = (uint32_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      while (1)
        {
        fat_offs = F32T_CLUST_2_OFFS_INSECT(fvi, clust_no);
        clust_no = buf32[fat_offs];

#if defined( LOG )
        if (eFAT_FREE == clust_no && false == fvi->fat_mounted)
          goto Result;
#endif

        acc_clust_cnt++;

        fsm_assert3(eFAT_FREE != clust_no);

        if (IS_FAT32_EOC(clust_no))
          goto Result;

        last_valid_clust = prev_clust;
        prev_clust = clust_no;
        next_fat_sectno = F32T_CLUST_2_SECT(fvi, clust_no);
        /* End loop if next cluster does not exist in the same cache-entry. */
        if (next_fat_sectno != fat_sectno) break;
        }

      fat_rel_csector(ce);
      }
    }

Result:
  fat_rel_csector(ce);

  if (eFAT_FREE == clust_no)
    {
    if (last_clust_no)
      *last_clust_no = last_valid_clust;
    if (last_clust_fat)
      *last_clust_fat = eFAT_FREE;
    }
  else if ((eFAT16_SIZE == fvi->br.efat_type && IS_FAT16_EOC(clust_no)) ||
           (eFAT32_SIZE == fvi->br.efat_type && IS_FAT32_EOC(clust_no)))
    {
    if (last_clust_no)
      *last_clust_no = prev_clust;
    if (last_clust_fat)
      *last_clust_fat = clust_no;
    }
  else
    {
    fsm_assert1(false);
    }

  return acc_clust_cnt;
  }

/*
 Name: fat_append_clustno
 Desc: to be appendixed
 Params:
   - vol_id: Volume's ID to be appendixed.
   - clust_no: Cluster number.
   - next_clust: Next cluster number.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_append_clustno(uint32_t vol_id, uint32_t clust_no, uint32_t next_clust)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce;
  uint32_t fat_sectno,
    fat_offs;
  uint8_t *buf;


  fat_sectno = FT_CLUST_2_SECT(fvi, clust_no);
  fat_offs = FT_CLUST_2_OFFS_INSECT(fvi, clust_no);

  /* Get the cache-entry which corresponds to the sector number. */
  ce = fat_get_csector(vol_id, fat_sectno);
  if (NULL == ce) return get_errno();
  buf = FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    fsm_assert1(IS_FAT16_EOC(*(uint16_t *) & buf[fat_offs]));
    *(uint16_t *)&buf[fat_offs] = (uint16_t) next_clust;
    }
  else
    {
    fsm_assert1(IS_FAT32_EOC(*(uint32_t *) & buf[fat_offs]));
    *(uint32_t *)&buf[fat_offs] = next_clust;
    }

  /* Mark the dirty list with the changed cache-entry. */
  fat_mark_dirty_csector(ce);
  /* Return fat_flush_csector( ce ); */
  return fat_rel_csector(ce);
  }

/*
 Name: fat_get_next_clustno
 Desc: Get the next cluster number to a specific cluster.
 Params:
   - vol_id: ID of volume.
   - clust_no: The cluster number.
 Returns:
   int32_t  >=0 on success. The returned value is the next cluster number.
            < 0 on fail.
 Caveats: None.
 */

int32_t fat_get_next_clustno(uint32_t vol_id, uint32_t clust_no)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce;
  uint32_t clustno,
    fat_sectno,
    fat_offs;
  uint8_t *buf;


  fat_sectno = FT_CLUST_2_SECT(fvi, clust_no);
  fat_offs = FT_CLUST_2_OFFS_INSECT(fvi, clust_no);

  /* Get the cache-entry which corresponds to the sector number. */
  ce = fat_get_csector(vol_id, fat_sectno);
  if (NULL == ce) return get_errno();
  buf = FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    clustno = *(uint16_t *) & buf[fat_offs];
    if (IS_FAT16_EOC(clustno)) clustno = eFAT_EOF;
    }
  else
    {/* eFAT32_SIZE */
    clustno = *(uint32_t *) & buf[fat_offs];
    if (IS_FAT32_EOC(clustno)) clustno = eFAT_EOF;
    }

  fat_rel_csector(ce);
  return clustno;
  }

/*
 Name: fat_set_next_clustno
 Desc: Set a cluster to the next one to a specific cluster.
   - vol_id: ID of volume.
   - clust_no: The cluster number to append the next.
   - next_clust: The next cluster number.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_set_next_clustno(uint32_t vol_id, uint32_t clust_no, uint32_t next_clust)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce;
  uint32_t fat_sectno,
    fat_offs;
  uint8_t *buf;


  fat_sectno = FT_CLUST_2_SECT(fvi, clust_no);
  fat_offs = FT_CLUST_2_OFFS_INSECT(fvi, clust_no);

  /* Get the cache-entry which corresponds to the sector number. */
  ce = fat_get_csector(vol_id, fat_sectno);
  if (NULL == ce) return get_errno();
  buf = FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

  if (eFAT16_SIZE == fvi->br.efat_type)
    *(uint16_t *) & buf[fat_offs] = (uint16_t) next_clust;
  else /* eFAT32_SIZE */
    *(uint32_t *) & buf[fat_offs] = next_clust;

  /* Mark the dirty list with the changed cache-entry */
  fat_mark_dirty_csector(ce);
  /* It stores the entry to use the LIM layer. */
  fat_flush_csector(ce);
  fat_rel_csector(ce);
  return s_ok;
  }

/*
 Name: fat_get_next_sectno
 Desc: Get next sector number in volume.
 Params:
   - vol_id: ID of volume to get a sector number.
   - sect_no: Base sector number to get next sector in a specific volume.
 Returns:
   int32_t  >=0 on success. The returned value is the next sector number.
            < 0 on fail.
 Caveats: None.
 */

int32_t fat_get_next_sectno(uint32_t vol_id, uint32_t sect_no)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce;
  uint32_t clust_no,
    fat_sectno,
    fat_offs;
  uint8_t *buf;


  if (GET_SECT_IDX(fvi, sect_no) < GET_SECT_IDX(fvi, sect_no + 1))
    return sect_no + 1;

  fsm_assert1(fvi->br.first_data_sect <= sect_no);

  /* This sector is the last sector in the cluster.
     Therefore the next sector exist the next cluster. */

  clust_no = D_SECT_2_CLUST(fvi, sect_no);

  fat_sectno = FT_CLUST_2_SECT(fvi, clust_no);
  fat_offs = FT_CLUST_2_OFFS_INSECT(fvi, clust_no);
  /* Get the cache-entry which corresponds to the sector number. */
  ce = fat_get_csector(vol_id, fat_sectno);
  if (NULL == ce) return get_errno();
  buf = FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    clust_no = *(uint16_t *) & buf[fat_offs];
    if (IS_FAT16_EOC(clust_no))
      {
      fat_rel_csector(ce);
      return eFAT_EOF;
      }
    }
  else
    { /* eFAT32_SIZE */
    clust_no = *(uint32_t *) & buf[fat_offs];
    if (IS_FAT32_EOC(clust_no))
      {
      fat_rel_csector(ce);
      return eFAT_EOF;
      }
    }

  fat_rel_csector(ce);

  fsm_assert2(0 != clust_no);
  return D_CLUST_2_SECT(fvi, clust_no);
  }

/*
 Name: fat_get_clustno
 Desc: Get number of cluster in offset of the 'cur_clust'
 Params:
   - vol_id: Volume's ID to be got cluster number.
   - cur_clust: current cluster.
   - clust_offs: cluster offset.
 Returns:
   int32_t  >=0 on success. The returned value is the cluster number.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_get_clustno(uint32_t vol_id, uint32_t cur_clust, uint32_t clust_offs)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  fat_cache_entry_t *ce;
  uint32_t clust_no,
    clust_cnt,
    acc_clust_cnt,
    fat_sectno,
    next_fat_sectno,
    fat_offs;


  if (0 == cur_clust)
    return set_errno(e_cfs);

  if (0 == clust_offs)
    return cur_clust;

  clust_no = cur_clust;

  clust_cnt = clust_offs + 1/*Start index 0*/;
  acc_clust_cnt = 0;

  if (eFAT16_SIZE == fvi->br.efat_type)
    {
    uint16_t *buf16;
    while (1)
      {
      fat_sectno = F16T_CLUST_2_SECT(fvi, clust_no);
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf16 = (uint16_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      while (1)
        {
        if (++acc_clust_cnt == clust_cnt) goto Result;

        fat_offs = F16T_CLUST_2_OFFS_INSECT(fvi, clust_no);
        clust_no = buf16[fat_offs];

        if (IS_FAT16_EOC(clust_no))
          {
          fat_rel_csector(ce);
          return set_errno(e_afat);
          }
        next_fat_sectno = F16T_CLUST_2_SECT(fvi, clust_no);
        /* End loop if next cluster does not exist in the same cache-entry. */
        if (next_fat_sectno != fat_sectno)
          break;
        }

      fat_rel_csector(ce);
      }
    }
  else
    { /* eFAT32_SIZE */
    uint32_t *buf32;
    while (1)
      {
      fat_sectno = F32T_CLUST_2_SECT(fvi, clust_no);
      /* Get the cache-entry which corresponds to the sector number. */
      ce = fat_get_csector(vol_id, fat_sectno);
      if (NULL == ce) return get_errno();
      buf32 = (uint32_t *) FAT_REAL_BUF_ADDR(ce->buf, fat_sectno);

      while (1)
        {
        if (++acc_clust_cnt == clust_cnt) goto Result;

        fat_offs = F32T_CLUST_2_OFFS_INSECT(fvi, clust_no);
        clust_no = buf32[fat_offs];

        if (IS_FAT32_EOC(clust_no))
          {
          fat_rel_csector(ce);
          return set_errno(e_afat);
          }
        next_fat_sectno = F32T_CLUST_2_SECT(fvi, clust_no);
        /* End loop if next cluster does not exist in the same cache-entry. */
        if (next_fat_sectno != fat_sectno)
          break;
        }

      fat_rel_csector(ce);
      }
    }

Result:
  fat_rel_csector(ce);

  return clust_no;
  }

/*
 Name: fat_get_sectno
 Desc: Get the sector number in offset of the 'cur_sect'
 Params:
   - vol_id: Volume's ID to be got sector number.
   - cur_sect: The current sector number.
   - sect_offs: The offset of sector in a cluster.
 Returns:
   int32_t  >=0 on success. The returned value is gotten sector number.
            < 0 on fail.
 Caveats: None.
 */

int32_t fat_get_sectno(uint32_t vol_id, uint32_t cur_sect, uint32_t sect_offs)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  uint32_t clust_no,
    clust_offs,
    sect_offs_inclust,
    virtual_sect,
    virtual_clust;


  if (0 == sect_offs)
    return cur_sect;

  if (0 == cur_sect)
    return set_errno(e_cfs);

  /* Calculate count of searching cluster and offset searching cluster. */
  virtual_sect = cur_sect + sect_offs;
  clust_no = D_SECT_2_CLUST(fvi, cur_sect);
  virtual_clust = D_SECT_2_CLUST(fvi, virtual_sect);
  sect_offs_inclust = virtual_sect - D_CLUST_2_SECT(fvi, virtual_clust);
  clust_offs = virtual_clust - clust_no;

  clust_no = fat_get_clustno(vol_id, clust_no, clust_offs);
  if (0 > (int32_t) clust_no)
    return -1;

  return D_CLUST_2_SECT(fvi, clust_no) + sect_offs_inclust;
  }

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

