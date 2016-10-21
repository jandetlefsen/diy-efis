/* FILE: ion_path.c */
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

#include "path.h"
#include "fat.h"
#include "file.h"
#include "dir.h"
#include "vol.h"

#include <string.h>

#include "../osd/osd.h"

#if defined( CPATH )

/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES
-----------------------------------------------------------------------------*/

/**/
typedef struct path_cacheent_s
  {
  list_head_t head, /* the linked-list to the free & active cache entries */
  hash_head; /* only the linked-list to the active cache entries */
  uint32_t flag; /* refer to the cache_flag_t enumeration. */
  fat_fileent_t *pfe; /* pointer to the file entry in the cache. */

  } path_cache_entry_t;




/*-----------------------------------------------------------------------------
 DEFINE DEFINITIONS
-----------------------------------------------------------------------------*/

/* Define hash information */
#define PATH_CACHE_HASH 16
#define PATH_GET_CACHE_HASH(fe) \
   ((fe->vol_id + fe->parent_clust + fe->name[0] + fe->name_len) \
    & (PATH_CACHE_HASH-1))
#define PATH_STR_CP_SIZE  (sizeof(fat_fileent_t))
#define PATH_UPD_CP_SIZE  PATH_STR_CP_SIZE




/*-----------------------------------------------------------------------------
 DEFINE GLOVAL VARIABLES
-----------------------------------------------------------------------------*/

/* Cache information. */
static list_head_t path_cache_lru;
static list_head_t path_cache_hash_lru[VOLUME_NUM][PATH_CACHE_HASH];
/* Entries of pach-cache. */
static path_cache_entry_t path_cache_table[PATH_CACHE_NUM];
/* This buffer is used to cache the data of each enrty for path_cache_table. */
static fat_fileent_t path_entry_table[PATH_CACHE_NUM];




/*-----------------------------------------------------------------------------
 DEFINE FUNCTIONS
-----------------------------------------------------------------------------*/

/*
 Name: path_zinit_path
 Desc: Zero initialize all data.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void path_zinit_path(void)
  {
  memset(&path_cache_lru, 0, sizeof (path_cache_lru));
  memset(&path_cache_hash_lru, 0, sizeof (path_cache_hash_lru));
  memset(&path_cache_table, 0, sizeof (path_cache_table));
  memset(&path_entry_table, 0, sizeof (path_entry_table));
  }

/*
 Name: path_init_cache
 Desc: Initialize the buffer for path cache.
 Params: None.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t path_init_cache(void)
  {
  list_head_t *list;
  path_cache_entry_t *entry;
  int32_t i, j;


  list = &path_cache_lru;
  entry = &path_cache_table[0];

  /* Initialize all list */
  list_init(list);

  /* path cache list linked in order of entry. path_cache_lru is list head pointer */

  for(i = 0; i < PATH_CACHE_NUM; i++, entry++)
    {
    list_init(&entry->head);
    list_init(&entry->hash_head);
    entry->flag = CACHE_FREE;
    entry->pfe = &path_entry_table[i];

    list_add_tail(list, &entry->head);
    }


  /* Initialize the hash table. */
  for(i = 0; i < VOLUME_NUM; i++)
    {
    for(j = 0; j < PATH_CACHE_HASH; j++)
      {
      list = &path_cache_hash_lru[i][j];
      list_init(list);
      }
    }

  return s_ok;
  }

/*
 Name: __path_alloc_centry
 Desc: Lock for a cache-entry which is not locked, and move it to the end of
       the normal and hash list.
 Params:
   - hash_list: The head of hash list about path cache.
 Returns:
   path_cache_entry_t *  value  on success. This value is cache entry pointer.
                         NULL   on fail.
 Caveats: None.
 */

static path_cache_entry_t *__path_alloc_centry(list_head_t *hash_list)
  {
  list_head_t *list;
  path_cache_entry_t *pos;


  /* search normal list */
  list = &path_cache_lru;

  pos = list_entry(list->next, path_cache_entry_t, head);
  if(&pos->head != list)
    {
    /* cache entry list's head position move to the list's end. */
    list_move_tail(list, &pos->head);
    /*  hash list's hash_head position move to the list's end. */
    list_move_tail(hash_list, &pos->hash_head);
    pos->flag = CACHE_VALID;
    return pos;
    }

  return (path_cache_entry_t *) NULL;
  }

/*
 Name: __path_free_centry
 Desc: Delete a pach-cache entry from hash list and change it in free state.
 Params:
   - entry: Pointer to the path_cache_entry_t structure to be removed.
 Returns: None.
 Caveats: None.
 */

static void __path_free_centry(path_cache_entry_t *entry)
  {
  list_del_init(&entry->hash_head);
  list_move(&path_cache_lru, &entry->head);
  entry->flag = CACHE_FREE;

#if defined( DBG )
  memset(entry->pfe, 0, sizeof (fat_fileent_t));
#endif
  }

/*
 Name: path_reinit_cache_vol
 Desc: Initialize all entries of cache again in this volume.
 Params:
   - vol_id: Volume's ID to be re-initialized.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t path_reinit_cache_vol(uint32_t vol_id)
  {
  list_head_t *list;
  path_cache_entry_t *pos, *n;
  int32_t i;


  for(i = 0; i < PATH_CACHE_HASH; i++)
    {
    list = &path_cache_hash_lru[vol_id][i];

    list_for_each_entry_safe(path_cache_entry_t, pos, n, list, hash_head)
      {
      /* The 'pos' entry is deleted from the hash list by __path_free_centry(). */
      __path_free_centry(pos);
      }
    }

  return s_ok;
  }

/*
 Name: __path_lookup_centry
 Desc: Search pach-cache entry from the hash list.
       If parameter's file-entry and file-entry that is searched has same name,
       return pointer to the cache entry searched.
 Params:
   - hash_list: The head of hash list about path cache.
   - fe: Pointer to the file entry to be looked up.
 Returns:
   path_cache_entry_t*  value on success. The value returned is the pointer to
                              the cache entry searched.
                        NULL on fail. Not found.
 Caveats: None.
 */

static path_cache_entry_t *__path_lookup_centry(list_head_t *hash_list, fat_fileent_t *fe)
  {
  path_cache_entry_t *pos;
  fat_fileent_t *pfe;

  list_for_each_entry(path_cache_entry_t, pos, hash_list, hash_head)
    {
    pfe = pos->pfe;
    if(pfe->name_len == fe->name_len && pfe->parent_clust == fe->parent_clust)
      {
      if(!stricmp(pfe->name, fe->name))
        return pos;
      }
    }

  return (path_cache_entry_t *) NULL;
  }

/*
 Name: __path_lookup2_centry
 Desc: Search pach-cache entry from the hash list.
       If parameter's file-entry and file-entry that is searched has same
       index and parent's sector, return pointer to the cache entry searched.
 Params:
   - hash_list: The head of hash list about path cache.
   - fe: Pointer to the file entry.
 Returns:
   path_cache_entry_t*  value on success.
                        NULL on fail.
 Caveats: None.
 */

static path_cache_entry_t *__path_lookup2_centry(list_head_t *hash_list, const fat_fileent_t *fe)
  {
  path_cache_entry_t *pos;
  fat_fileent_t *pfe;

  list_for_each_entry(path_cache_entry_t, pos, hash_list, hash_head)
    {
    pfe = pos->pfe;
    if(pfe->name_len == fe->name_len &&
       pfe->parent_sect == fe->parent_sect &&
       pfe->parent_ent_idx == fe->parent_ent_idx)
      return pos;
    }

  return (path_cache_entry_t *) NULL;
  }

/*
 Name: __cp_get_path
 Desc: Copy file entry information to dstination buffer from source buffer.
 Params:
   - dst: Destination file entry pointer
   - src: Source file entry pointer
 Returns: None.
 Caveats: None.
 */

static void __cp_get_path(fat_fileent_t *dst, fat_fileent_t *src)
  {
#if 1
  dst->parent_ent_idx = src->parent_ent_idx;
  dst->lfn_shortent_idx = src->lfn_shortent_idx;
  dst->ent_cnt = src->ent_cnt;
  dst->parent_clust = src->parent_clust;
  dst->parent_sect = src->parent_sect;
  dst->lfn_short_sect = src->lfn_short_sect;
  dst->flag = src->flag;
  memcpy(&dst->dir, &src->dir, sizeof (dst->dir));
#else
  int32_t size, pre_gap, post_gap;


  pre_gap = offsetof(dst, vol_id);
  post_gap = sizeof (fat_fileent_t) - offsetof(dst, name_len);
  size = sizeof (fat_fileent_t) - (pre_gap + post_gap);

  memcpy(dst, src, size);
#endif
  }

/*
 Name: __cp_store_path
 Desc:  Copy the file entry to the path cache's file entry.
 Params:
   - dst: Destination file entry pointer
   - src: Source file entry pointer
 Returns: None.
 Caveats: None.
 */

static void __cp_store_path(fat_fileent_t *dst, const fat_fileent_t *src)
  {
  memcpy(dst, src, PATH_STR_CP_SIZE);
  }

/*
 Name: __cp_update_path
 Desc: Copy the file entry to the path cache's file entry.
 Params:
   - dst: Destination file entry pointer
   - src: Source file entry pointer
 Returns: None.
 Caveats: None.
 */

static void __cp_update_path(fat_fileent_t *dst, const fat_fileent_t *src)
  {
  memcpy(dst, src, PATH_UPD_CP_SIZE);
  }

/*
 Name: path_store_centry
 Desc: Store the file-entry that is allocated to hash list.
 Params:
   - fe: file entry pointer to be saved file path at the cache.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t path_store_centry(const fat_fileent_t *fe)
  {
  path_cache_entry_t *entry;
  list_head_t *hash_list;
  int32_t hash;


  fsm_assert3(eFAT_ATTR_DIR & fe->dir.attr);

  /* Get the hash. */
  hash = PATH_GET_CACHE_HASH(fe);
  /* Search the table using the hash. */
  hash_list = &path_cache_hash_lru[fe->vol_id][hash];

  path_lock();
  entry = __path_alloc_centry(hash_list);
  if(entry)
    {
    /* Copy the file entry to the path cache's file entry. */
    __cp_store_path(entry->pfe, fe);
    path_unlock();
    return s_ok;
    }

  path_unlock();
  return e_nomem;
  }

/*
 Name: path_update_centry
 Desc: Update the path-cache entry which is existed in the hash list.
 Params:
   - fe: Pointer to the file entry.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t path_update_centry(const fat_fileent_t *fe)
  {
  path_cache_entry_t *entry;
  list_head_t *hash_list;
  int32_t hash;


  fsm_assert3(eFAT_ATTR_DIR & fe->dir.attr);

  /* Get the hash. */
  hash = PATH_GET_CACHE_HASH(fe);
  /* Search the table using the hash. */
  hash_list = &path_cache_hash_lru[fe->vol_id][hash];

  path_lock();
  entry = __path_lookup2_centry(hash_list, fe);
  if(entry)
    {
    list_move_tail(hash_list, &entry->hash_head);
    __cp_update_path(entry->pfe, fe);
    path_unlock();
    return s_ok;
    }

  path_unlock();
  return e_noent;
  }

/*
 Name: path_update_store_centry
 Desc: Copy the file entry to the path cache's file entry.
 Params:
   - fe: Pointer to the file entry.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t path_update_store_centry(const fat_fileent_t *fe)
  {
  path_cache_entry_t *pce;
  list_head_t *hash_list;
  int32_t hash;


  /* search hash list */
  hash = PATH_GET_CACHE_HASH(fe);
  hash_list = &path_cache_hash_lru[fe->vol_id][hash];

  path_lock();
  pce = __path_lookup2_centry(hash_list, fe);
  path_unlock();

  if(NULL == pce)
    return path_store_centry(fe);
  else
    return path_update_centry(fe);
  }

/*
 Name: path_get_centry
 Desc: Search a entry specified by the 'fe' parameter in the cache.
 Params:
   - fe: Pointer to the file entry.
 Returns:
   int32_t  =0 on success. It means the entry is found.
            <0 on fail. It means the entry does not exist in the cache.
 Caveats: The information of the File-Entry should be set before calling this
          function.
 */

int32_t path_get_centry(fat_fileent_t *fe)
  {
  path_cache_entry_t *entry;
  list_head_t *hash_list;
  int32_t hash;


  /* Get the hash from the pointer to the File-Entry. */
  hash = PATH_GET_CACHE_HASH(fe);
  /* Search the table using the hash. */
  hash_list = &path_cache_hash_lru[fe->vol_id][hash];

  path_lock();
  entry = __path_lookup_centry(hash_list, fe);
  if(entry)
    {
    tr_fat_inc_cpath_hits(fe->vol_id);

    list_move_tail(hash_list, &entry->hash_head);

    /* Copy information of the found path. */
    __cp_get_path(fe, entry->pfe);

    fsm_assert3(eFAT_ATTR_DIR & fe->dir.attr);

    path_unlock();
    return s_ok;
    }

  path_unlock();
  return -1;
  }

/*
 Name: path_del_centry
 Desc:  Remove a entry specified by the 'fe' parameter in the cache.
 Params:
   - fe: Pointer to the fat_fileent_t structure to be removed in the cache.
 Returns: None.
 Caveats: None.
 */

void path_del_centry(fat_fileent_t *fe)
  {
  path_cache_entry_t *pos;
  list_head_t *hash_list;
  int32_t hash;


  /* Get the hash from the pointer to the File-Entry. */
  hash = PATH_GET_CACHE_HASH(fe);
  /* Search the table using the hash. */
  hash_list = &path_cache_hash_lru[fe->vol_id][hash];

  path_lock();

  pos = __path_lookup_centry(hash_list, fe);
  if(pos)
    __path_free_centry(pos);

  path_unlock();
  }

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

