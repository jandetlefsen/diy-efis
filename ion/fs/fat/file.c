#include "vol.h"
#include "file.h"
#include "dir.h"
#include "name.h"
#include "misc.h"
#include "log.h"
#include "../fat/path.h"
#include "../lim/lim.h"
#include "../osd/osd.h"
#include <string.h>


/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES & DEFINITIONS
-----------------------------------------------------------------------------*/

/* Define hash information */
#if ( 4 >= FAT_MAX_FILE )
#define FILE_TABLE_HASH 1
#elif ( 8 >= FAT_MAX_FILE )
#define FILE_TABLE_HASH 2
#else
#define FILE_TABLE_HASH 4
#endif

/* It uses the subordinate 8 bit and it selects the hash
   LRU(Least Recently Used).*/
#define FILE_TABLE_GET_HASH(fe) \
   (((fe)->parent_ent_idx + (fe)->parent_sect) & (FILE_TABLE_HASH-1))




#define FILE_WRITE 1
#define FILE_READ 2
typedef int32_t(*f_readwrite_t)(int32_t, uint32_t, void *, uint32_t);




#if 0
#define use_write_fat_unlock()         fat_unlock()
#define use_write_fat_lock()           fat_lock()
#else
#define use_write_fat_unlock()
#define use_write_fat_lock()
#endif


#if defined( READ_CRITICAL_SECTION )
#define use_read_fat_unlock()
#define use_read_fat_lock()
#else
#define use_read_fat_unlock()          fat_unlock()
#define use_read_fat_lock()            fat_lock()
#endif



/*-----------------------------------------------------------------------------
 DEFINE GLOBAL VARIABLES
-----------------------------------------------------------------------------*/

/* Entry informations */

/* Free & active list of File Entry (don't link dirty)  : ACTIVE--LIST--FREE */
static list_head_t file_table_lru;
/* Hash list on ACTIVE. */
static list_head_t file_table_hash_lru[FILE_TABLE_HASH]; /* open file */
/* Entries of File Entry. */
#define REAL_FAT_MAX_FILE  (FAT_MAX_FILE+1)
static fat_fileent_t file_table[REAL_FAT_MAX_FILE];


/* Free & active list of Open-File Entry : ACTIVE--LIST--FREE */
static list_head_t ofile_table_lru;
/* Entries of Open-File Entry. */
static fat_ofileent_t ofile_table[FAT_MAX_OFILE];

#if defined( WB )
static list_head_t wb_free_lru;
static list_head_t wb_alloc_lru;

/* Entries of wB. */
static fat_wb_entry_t fat_wb[FILE_WB_CNT];

static fat_wb_entry_t* __fat_alloc_wb(fat_ofileent_t *ofe);
static int32_t __fat_free_wb(fat_fileent_t *fe);
static fat_ofileent_t* __fat_lookup_wb(fat_fileent_t *fe);
static int32_t __fat_get_out_size_wb(fat_fileent_t *fe);
static size_t __fat_flush_wb(fat_ofileent_t *ofe);
static int32_t __fat_hold_clust_wb(fat_volinfo_t *fvi, fat_ofileent_t *ofe);
static size_t __fat_write_through_wb(fat_ofileent_t *ofe, uint8_t *buf, size_t bytes);

#define wb_written_pos(fe)             (fe->wb->w_offs+fe->wb->wb_cur_offs)
#define fat_rehold_clust_wb(fvi,fe)    ((fvi)->br.free_clust_cnt -= (fe)->wb->hold_free_clust)
#define fat_release_clust_wb(fvi,fe)   ((fvi)->br.free_clust_cnt += (fe)->wb->hold_free_clust)
#endif




/* Static functions */
static int32_t __fat_open_file_entry(fat_fileent_t *fe);
static int32_t __fat_close_file_entry(fat_fileent_t *fe);
static int32_t __fat_free_ofile_entry(fat_ofileent_t *ofe);

#ifdef SFILEMODE
void __fat_set_file_information(fat_fileent_t *fe, mod_t mode, uint8_t attr);
#define TO_NTRES(z) (((z&0x08)<<2) | (z&0x07))
#define FROM_NTRES(z) (((z&0x20)>>2) | (z&0x07))
#endif

/*-----------------------------------------------------------------------------
 DEFINE FUNCTIONS
-----------------------------------------------------------------------------*/

/*
 Name: fat_zinit_file
 Desc: Zero initialize all data.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void fat_zinit_file(void)
  {
  memset(&file_table_lru, 0, sizeof (file_table_lru));
  memset(&file_table_hash_lru, 0, sizeof (file_table_hash_lru));
  memset(&file_table, 0, sizeof (file_table));
  memset(&ofile_table_lru, 0, sizeof (ofile_table_lru));
  memset(&ofile_table, 0, sizeof (ofile_table));
  }

/*
 Name: fat_init_file_entry
 Desc: Initialize the whole configuration for the file system.
 Params: None
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t fat_init_file_entry(void)
  {
  list_head_t *list;
  fat_fileent_t *fe;
  fat_ofileent_t *ofe;
#if defined( WB )
  fat_wb_entry_t *wb;
#endif
  uint32_t i;


  /* Initialize the file table. */
  memset(file_table, 0, sizeof (file_table));

  list = &file_table_lru;

  ofile_lock();

  list_init(list);

  fe = &file_table[0];

  /* It allocates the index of the File Entry */
  for (i = 0; i < REAL_FAT_MAX_FILE; i++, fe++)
    {
    list_init(&fe->head);
    fe->idx = (uint16_t) i;
    fe->vol_id = -1;
    fe->flag = eFILE_FREE;

    list_add_tail(list, &fe->head);
    }

  /* Initialize hash list */
  for (i = 0; i < FILE_TABLE_HASH; i++)
    list_init(&file_table_hash_lru[i]);


  /* Initialize the table about Open-File Entry. */
  memset(ofile_table, 0, sizeof (ofile_table));

  list = &ofile_table_lru;
  list_init(list);

  ofe = &ofile_table[0];

  /* Link all Open-File Entry. */
  for (i = 0; i < FAT_MAX_OFILE; i++, ofe++)
    {
    list_init(&ofe->head);
    ofe->fd = (uint16_t) i;
    ofe->state = eFILE_FREE;

    list_add_tail(list, &ofe->head);
    }

#if defined( WB )
  /* Initialize free-list. */
  list = &wb_free_lru;
  list_init(list);

  for (i = 0; i < FILE_WB_CNT; i++)
    {
    wb = &fat_wb[i];
    list_init(&wb->head);
    wb->w_offs = 0;
    wb->wb_cur_offs = 0;
    wb->hold_free_clust = 0;
    wb->ofe = NULL;
    list_add_tail(list, &wb->head);
    }

  /* Initialize alloc-list. */
  list = &wb_alloc_lru;
  list_init(list);
#endif

  ofile_unlock();

  return s_ok;
  }

/*
 Name: fat_reinit_vol_file_entry
 Desc:  Re-initialize to th File Entry.
 Params:
   - vol_id: The ID of the volume.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t fat_reinit_vol_file_entry(int32_t vol_id)
  {
  return fat_closeall(vol_id);
  }

/*
 Name: fat_alloc_file_entry
 Desc: Allocate File Entry available from the File Entry list.
 Params:
   - vol_id: The ID of the volume.
 Returns:
   fat_fileent_t*  value on success. The returned value is the pointer to the
                         allocated File Entry.
                   NULL on fail.
 Caveats: None.
 */

fat_fileent_t *fat_alloc_file_entry(int32_t vol_id)
  {
  list_head_t *list;
  fat_fileent_t *fe;


  ofile_lock();

  list = &file_table_lru;

  /* Searches the File Entry, it will be able to use in the File Entry list. */
  list_for_each_entry(fat_fileent_t, fe, list, head)
    {
    /* Changes from Free File Entry to allocated File Entry. */
    if (eFILE_FREE == fe->flag)
      {
      fe->vol_id = (int8_t) vol_id;
      fe->flag = eFILE_ALLOC;
      fe->parent_clust = 0;
      fe->parent_sect = 0;
      list_move_tail(list, &fe->head);

      /* Moves the searched entry in the alloc-entry list */
#if ( 0 < TRACE )
      {
        fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
        fvi->tr.afile_ents++;
      }
#endif
      ofile_unlock();
      return fe;
      }
    }

  ofile_unlock();
  set_errno(e_nfile);
  return (fat_fileent_t *) NULL;
  }

/*
 Name: __fat_open_file_entry
 Desc: Modify File Entry to the open status.
 Params:
   - fe: Pointer to the File Entry.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

static int32_t __fat_open_file_entry(fat_fileent_t *fe)
  {
  list_head_t *list;
  int32_t hash;


  hash = FILE_TABLE_GET_HASH(fe); /* select hash-lru*/
  list = &file_table_hash_lru[hash];

  /* Increases the reference value of the file entry */
  fe->ref_cnt++;

  /* If the File Entry is already opened, it is moved to the end of the hash-lru list */
  if (eFILE_OPEN & fe->flag)
    {
    list_move_tail(list, &fe->hash_head);
    return s_ok;
    }

  fe->flag |= eFILE_OPEN;

  /* Adds the File Entry to the hash-lru(List of opened File Entry). */
  list_add_tail(list, &fe->hash_head);
  return s_ok;
  }

/*
 Name: __fat_close_file_entry
 Desc: Close to the file entry.
 Params:
   - fe: Pointer to the File Entry.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

static int32_t __fat_close_file_entry(fat_fileent_t *fe)
  {
  fe->flag &= ~(uint32_t) eFILE_OPEN;

  /* Delete File Entry from hash-lru(List of opened File Entry)*/
  list_del_init(&fe->hash_head);
  return s_ok;
  }

/*
 Name: __fat_free_file_entry
 Desc: Free the allocated File Entry.
 Params:
   - fe: Pointer to the File Entry.
 Returns:
   int32_t  =0 on success.
            >0 on fail.
 Caveats: None.
 */

static int32_t __fat_free_file_entry(fat_fileent_t *fe)
  {
#if ( 0 < TRACE )
  fat_volinfo_t *fvi = GET_FAT_VOL(fe->vol_id);
  fvi->tr.afile_ents--;
#endif


#if defined( WB )
  fsm_assert2(NULL == fe->wb);
#endif

  /* if ( eFILE_FREE & fe->flag ) return set_errno( e_cfs ); */
  fsm_assert1(!(eFILE_FREE & fe->flag));

  /* Modify opened File-Entry to the close status. */
  if (eFILE_OPEN & fe->flag)
    __fat_close_file_entry(fe);

  fe->ref_cnt = 0;

  fe->vol_id = -1;
  fe->flag = eFILE_FREE;

  /* Move an element to a head of a specific list. */
  list_move(&file_table_lru, &fe->head);

  return s_ok;
  }

/*
 Name: fat_free_file_entry
 Desc: Free the allocated File Entry.
 Params:
   - fe: Pointer to the File Entry.
 Returns:
   int32_t  =0 on success.
            >0 on fail.
 Caveats: None.
 */

int32_t fat_free_file_entry(fat_fileent_t *fe)
  {
  int32_t rtn;


  ofile_lock();

  rtn = __fat_free_file_entry(fe);
  ofile_unlock();

  return rtn;
  }

/*
 Name: fat_get_opend_file_entry
 Desc: Check if file related with File Entry is already oepend.
 Params:
   - fe: File entry pointer.
 Returns:
   fat_fileent_t *   entry position  on success.
                     NULL  on fail.
 Caveats: If file is already opened, get File Entry this file is using.
 */

fat_fileent_t *fat_get_opend_file_entry(fat_fileent_t *fe)
  {
  fat_fileent_t *pos;
  list_head_t *list;
  int32_t hash;


  hash = FILE_TABLE_GET_HASH(fe); /* Select hash-lru*/
  list = &file_table_hash_lru[hash];

  ofile_lock();

  /* Check the the file/directory should have been opened already. */

  /* If it is being opened already, this returns the opened file entry.*/
  list_for_each_entry_rev(fat_fileent_t, pos, list, hash_head)
    {
    if (eFILE_OPEN & pos->flag)
      {
      if ((pos->parent_sect == fe->parent_sect) &&
          (pos->parent_ent_idx == fe->parent_ent_idx) &&
          (pos->vol_id == fe->vol_id))
        {
        ofile_unlock();
#if defined( WB )
        fsm_assert2(NULL == fe->wb);
#endif
        return pos;
        }
      }
    }

  ofile_unlock();
  return (fat_fileent_t *) NULL;
  }

/*
 Name: fat_is_alloc_file_entry
 Desc: Check the file entry allocated or not.
 Params:
   - vol_id: Volume's ID to be allocated.
   - parent_clust: parent cluster number.
 Returns:
   bool   true on success.
           false on fail.
 Caveats: None.
 */

bool fat_is_alloc_file_entry(int32_t vol_id, uint32_t parent_clust)
  {
  list_head_t *list;
  fat_fileent_t *fe;


  list = &file_table_lru;

  ofile_lock();

  /* Search File entry that parent cluster of file entry is the same as the
     cluster specified by the 'parent_clust' parameter */
  list_for_each_entry_rev(fat_fileent_t, fe, list, head)
    {
    if (eFILE_FREE == fe->flag) break;

    /* Check the parent cluster & volume id */
    if (fe->parent_clust == parent_clust && fe->vol_id == vol_id)
      {
      ofile_unlock();
      return true;
      }
    }

  ofile_unlock();
  return false;
  }

/*
 Name: fat_get_file_entry_fromid
 Desc: Get File Entry related with a file descriptor.
 Params:
   - fd: The file descriptor.
 Returns:
   fat_fileent_t *  File entry pointer on success.
                    NULL on fail.
 Caveats: None.
 */

fat_fileent_t *fat_get_file_entry_fromid(int32_t fd)
  {
  fat_fileent_t *fe;

  fe = &file_table[fd];

  if (REAL_FAT_MAX_FILE <= fd)
    fe = (fat_fileent_t *) NULL;

  if (!(fe->flag & eFILE_ALLOC))
    fe = (fat_fileent_t *) NULL;

  return fe;
  }

/*
 Name: __fat_alloc_ofile_entry
 Desc: Allocate the buffer for the Open-File Entry and the File Entry specified
       by the 'fe' parameter is registered at allocated Open-File Entry.
 Params:
   - fe: Pointer to the File Entry.
   - oflag: The flag about open file.
            O_RDONLY: Open for reading only.
            O_WRONLY: Open for writing only.
            O_RDWR: Open for reading and writing.
            O_APPEND: Writes done at EOF.
            O_CREAT: Create and open file.
            O_TRUNC: Open and truncate.
            O_EXCL: Open only if file doesn't already exist.
 Returns:
   fat_ofileent_t*  value on success. The returned value is the pointer to the
                          Open-File Entry.
                    NULL on fail.
 Caveats: None.
 */

static fat_ofileent_t *__fat_alloc_ofile_entry(fat_fileent_t *fe, uint32_t oflag)
  {
  list_head_t *list;
  fat_fileent_t *pos;
  fat_ofileent_t *ofe, *pos2;


  /* Get the opened File-Entry about the same name. */
  pos = fat_get_opend_file_entry(fe);
  if (pos)
    {
    if (O_TRUNC & oflag)
      memcpy(&pos->dir, &fe->dir, sizeof (fat_dirent_t));

    /* Free the buffer for File-Entry. */
    fat_free_file_entry(fe);
    /* If the same File Entry exist already, then use existing File Entry. */
    fe = pos;

#if defined( WB )
    ofe = __fat_lookup_wb(fe);
    if (ofe)
      {
      size_t rtn;
      fsm_assert3(ofe->fe == fe);

      ofile_lock();
      rtn = __fat_flush_wb(ofe);
      ofile_unlock();
      __fat_free_wb(fe);
      if (0 > rtn)
        return NULL;
      }
#endif
    }

  ofe = (fat_ofileent_t *) NULL;

  list = &ofile_table_lru;

  ofile_lock();

  /*Search Open-File Entry available on the Open-File Entry list*/
  list_for_each_entry(fat_ofileent_t, pos2, list, head)
    {
    if (eFILE_FREE & pos2->state)
      {
      list_move_tail(list, &pos2->head);
      ofe = pos2;
      /* Modify File Entry to Open status */
      __fat_open_file_entry(fe);

      /* Modify Open-File Etnry to Allocate status,
         and then given the File Entry register Open-File Entry*/
      ofe->state = eFILE_ALLOC;
      ofe->oflag = oflag;
      ofe->fe = fe;
      break;
      }
    }

  if (NULL == ofe)
    {
    set_errno(e_nfile);
    ofile_unlock();
    return (fat_ofileent_t *) NULL;
    }

  tr_fat_inc_ofile_ents(fe->vol_id);

  ofile_unlock();
  return ofe;
  }

/*
 Name: __fat_get_ofile_entry
 Desc: Obtain Open-File Entry indicated by a file descriptor.
 Params:
   - fd: The file descripter.
 Returns:
   fat_ofileent_t *  Pointer to the Open-File Entry on success.
                     NULL on fail.
 Caveats: None.
 */

static fat_ofileent_t *__fat_get_ofile_entry(int32_t fd)
  {
  fat_ofileent_t *ofe;


  if (FAT_MAX_OFILE <= fd)
    {
    set_errno(e_badf);
    return (fat_ofileent_t *) NULL;
    }

  ofe = &ofile_table[fd];

  if (!(eFILE_ALLOC & ofe->state))
    {
    set_errno(e_badf);
    return (fat_ofileent_t *) NULL;
    }

#if defined( WB )
  fsm_assert2(!(1 < ofe->fe->ref_cnt && ofe->fe->wb));
#endif
  return ofe;
  }

/*
 Name: __fat_free_ofile_entry
 Desc: To close opened file, the status of allocated Open-File Entry changes to
       Free status.
 Params:
   - ofe: Pointer to the Open-File Entry to change to Free status.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_free_ofile_entry(fat_ofileent_t *ofe)
  {
  int32_t rtn = s_ok;


  if (eFILE_FREE & ofe->state)
    return set_errno(e_badf);

  tr_fat_dec_ofile_ents(ofe->fe->vol_id);

  /* Modify the Open-File Entry to Free status */
  ofe->state = eFILE_FREE;
  ofe->oflag = 0;
  list_move(&ofile_table_lru, &ofe->head);
  ofe->fe->ref_cnt--;

  /* If File Entry is not used, File Entry changes to Free status*/
  if (ofe->fe->ref_cnt == 0)
    rtn = __fat_free_file_entry(ofe->fe);

  return rtn;
  }

/*
 Name: __fat_invalidate_ofile_pos
 Desc: Open file position(offset) reset. Except the 'seek_offs'.
       Reset current position of opened the file
 Params:
   - fe: File Entry pointer to reset opened file position.
 Returns: None.
 Caveats: None.
 */

static void __fat_invalidate_ofile_pos(fat_fileent_t *fe)
  {
  fat_volinfo_t *fvi;
  list_head_t *list;
  fat_ofileent_t *ofe;
  uint32_t clust_no,
    cur_sect;


  fvi = GET_FAT_VOL(fe->vol_id);
  clust_no = GET_OWN_CLUST(&fe->dir);
  cur_sect = clust_no ? D_CLUST_2_SECT(fvi, clust_no) : 0;

  list = &ofile_table_lru;

  ofile_lock();

  /* Search Open-File Entry using the File Entry specified by the 'fe'
     parameter. */
  list_for_each_entry_rev(fat_ofileent_t, ofe, list, head)
    {
    if (eFILE_FREE & ofe->state) break;

    if ((ofe->fe == fe) ||
        ((ofe->fe->parent_sect == fe->parent_sect) &&
         (ofe->fe->parent_ent_idx == fe->parent_ent_idx) &&
         (ofe->fe->vol_id == fe->vol_id)))
      {
      /* Reset current offset except the 'seek_offs' */
      ofe->cur_offs = ofe->cur_offs_sect = 0;
      ofe->cur_sect = cur_sect;
      }
    }

  ofile_unlock();
  }

/*
 Name: __fat_do_ftrunc
 Desc: Truncate file data as given the number of cluster.
 Params:
   - fe: The file entry pointer.
   - last_clust_no: Last cluster number.
   - new_clust_cnt: Need cluster count.
   - free_clust_cnt: Left cluster count.
 Returns:
   int32_t  >=0 on success.
            < 0 on fail.
 Caveats: None.
 */

static int32_t __fat_do_ftrunc(fat_fileent_t *fe, int32_t last_clust_no,
                               int32_t new_clust_cnt, int32_t free_clust_cnt)
  {
  int32_t vol_id = fe->vol_id,
    rtn;
  bool need_mark_eoc;
#if defined( LOG )
  int32_t cl_id;
#endif

  /* Flush the LIM data cache to the physical device. */
  if (0 > lim_flush_cdsector()) return -1;

#if defined( LOG )
  cl_id = fat_log_on(fe, NULL);
  if (0 > cl_id) return cl_id;
#endif

  if (0 == new_clust_cnt)
    {
    need_mark_eoc = false;
    SET_OWN_CLUST(&fe->dir, 0);
    }
  else
    need_mark_eoc = true;

  /* Change write time and access time to present time. */
  fat_set_ent_time(&fe->dir, TM_WRITE_ENT | TM_ACCESS_ENT);

  /* Update the changed entry. */
  rtn = fat_update_sentry(fe, true);
  if (s_ok == rtn)
    {
    rtn = fat_unlink_clusts(vol_id, last_clust_no, need_mark_eoc);
    }

#if defined( LOG )
  fat_log_off(vol_id, cl_id);
#endif

  return rtn;
  }

/*
 Name: __fat_do_falloc
 Desc: Allocate clusters as the given number of the cluster.
 Params:
   - fe: The file entry pointer.
   - last_clust_no: Last cluster number.
   - alloc_clust_cnt: Need cluster count for allocation.
   - new_filesize: New file size.
 Returns:
   int32_t  >0 on success. First cluster number that allocated newly.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_do_falloc(fat_fileent_t *fe, uint32_t last_clust_no,
                               uint32_t alloc_clust_cnt, uint32_t new_filesize)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(fe->vol_id);
  int32_t vol_id,
    alloc_cnt,
    left_cnt,
    first_clust_no,
    rtn;
  uint32_t *alloced_list;
  ionfs_local uint32_t clust_list[1/*Last cluster*/ + CLUST_LIST_BUF_CNT];
  int32_t update_rtn = -1;
#if defined( LOG )
  int32_t cl_id = -1;
#endif


  vol_id = fe->vol_id;
  left_cnt = alloc_clust_cnt;
  first_clust_no = 0;

  if (fvi->br.free_clust_cnt < alloc_clust_cnt)
    return set_errno(e_nospc);

  do
    {
    alloc_cnt = (CLUST_LIST_BUF_CNT - 1) > left_cnt ? left_cnt : (CLUST_LIST_BUF_CNT - 1);
    left_cnt -= alloc_cnt;

    alloced_list = &clust_list[1];

    /* Search the free-cluster from bit-map. */
    rtn = fat_map_alloc_clusts(vol_id, alloc_cnt, alloced_list);
    if (0 > rtn) break;

    if (0 == first_clust_no)
      first_clust_no = alloced_list[0];

    clust_list[0] = last_clust_no;

    if (-1 == update_rtn)
      {
      if (0 == last_clust_no)
        SET_OWN_CLUST(&fe->dir, alloced_list[0]);

      fe->dir.filesize = new_filesize;

#if defined( LOG )
      cl_id = fat_log_on(fe, NULL);
      if (0 > cl_id) return cl_id;
#endif

      /* Change write time and access time to present time. */
      fat_set_ent_time(&fe->dir, TM_WRITE_ENT | TM_ACCESS_ENT);

      /* Update the changed entry. */
      rtn = update_rtn = fat_update_sentry(fe, true);
      if (0 > rtn)
        return rtn;
      }

    /* Link allocated free cluster by using FAT entry. */
    rtn = fat_stamp_clusts(vol_id, alloc_cnt, clust_list);
    if (0 > rtn) break;

    last_clust_no = clust_list[alloc_cnt];
    }
  while (left_cnt);

#if defined( LOG )
  if (s_ok == rtn)
    /* Save the changed FAT Entry to the physical device. */
    rtn = fat_sync_table(vol_id, true);
#endif

#if defined( LOG )
  if (0 <= cl_id)
    fat_log_off(vol_id, cl_id);
#endif

  if (s_ok == rtn)
    return first_clust_no;
  else
    return rtn;
  }

/*
 Name: __fat_file_fill_gap
 Desc: Lengthen file size as given size.
 Params:
   - ofe: Open-File Entry pointer.
   - gap_bytes: Gap byte for allocation.
   - need_bytes: Need bytes for cluster count .
 Returns:
   int32_t  >0 on success. First cluster number that allocated newly.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_file_fill_gap(fat_ofileent_t *ofe, uint32_t gap_bytes, uint32_t need_bytes)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(ofe->fe->vol_id);
  fat_fileent_t *fe = ofe->fe;
  uint32_t clust_no,
    last_clust_offs,
    last_clust_no,
    alloc_cnt;
  int32_t rtn;


  clust_no = GET_OWN_CLUST(&ofe->fe->dir);
  if (clust_no)
    {
    /* Obtain offset which is in the data's last cluster. */
    last_clust_offs = ofe->fe->dir.filesize / fvi->br.bits_per_clust;
    if (!(ofe->fe->dir.filesize & fvi->br.bytes_per_clust_mask))
      last_clust_offs--;

    /* Searche last cluster which is allocated to a file. */
    rtn = fat_get_clustno(ofe->fe->vol_id, clust_no, last_clust_offs);
    if (0 > rtn) return rtn;
    last_clust_no = rtn;
    }
  else
    last_clust_no = 0;

  /* Obtain the number of cluster. */
  alloc_cnt = need_bytes / fvi->br.bits_per_clust;

  /* Allocate the additional cluster to file. */
  rtn = __fat_do_falloc(fe, last_clust_no, alloc_cnt, fe->dir.filesize + gap_bytes);
  return rtn;
  }

/*
 Name: __fat_adjust_pos_in
 Desc: Change current position of file to seek-offset.
 Params:
   - ofe: Pointer to the Open-File Entry pointer.
   - adjust_offs: offset to be adjusted.
 Returns:
   int32_t  >=0 on success. The returned value is the current offset of Open-File Entry.
            < 0 on fail.
 Caveats: None.
 */

static int32_t __fat_adjust_pos_in(fat_ofileent_t *ofe, offs_t adjust_offs)
  {
  fat_volinfo_t *fvi;
  fat_fileent_t *fe;
  uint32_t clust_no,
    cur_sect,
    seek_sect_idx, /* Sector unit offset */
    cur_sect_idx, /* Sector unit offset */
    cur_offs_sect, /* Byte unit offset in sector */
    sect_idx_offs;
  offs_t cur_offs;
  int32_t rtn;


  fe = ofe->fe;
  fvi = GET_FAT_VOL(fe->vol_id);

  cur_offs = ofe->cur_offs;
  cur_offs_sect = adjust_offs & fvi->br.bytes_per_sect_mask;

  /* It uses the seek offset and it searches the current-sector and the offset of that inside*/
  if (0 > (adjust_offs - cur_offs))
    {
    clust_no = GET_OWN_CLUST(&fe->dir);
    cur_sect = D_CLUST_2_SECT(fvi, clust_no);
    sect_idx_offs = adjust_offs >> fvi->br.bits_per_sect;
    }
  else
    { /* 0 <= (adjust_offs - cur_offs) */
    seek_sect_idx = adjust_offs >> fvi->br.bits_per_sect;
    cur_sect_idx = ofe->cur_offs >> fvi->br.bits_per_sect;
    sect_idx_offs = seek_sect_idx - cur_sect_idx;
    cur_sect = ofe->cur_sect;
    }

  rtn = fat_get_sectno(fe->vol_id, cur_sect, sect_idx_offs);
  if (0 > rtn) return rtn;
  cur_sect = rtn;

  ofe->cur_offs = adjust_offs;
  ofe->cur_sect = cur_sect;
  ofe->cur_offs_sect = (uint16_t) cur_offs_sect;

  return ofe->cur_offs;
  }

/*
 Name: __fat_adjust_pos_out
 Desc: Lenghen file size as size of seek offset,
       and current position of file changes to seek offset.
 Params:
   - ofe: Pointer to the Open-File Entry pointer.
   - adjust_offs: offset to be adjusted.
 Returns:
   int32_t  >=0 on success. The returned value is the current offset of Open-File Entry.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_adjust_pos_out(fat_ofileent_t *ofe, offs_t adjust_offs)
  {
  fat_volinfo_t *fvi;
  fat_fileent_t *fe;
  uint32_t cur_sect,
    seek_sect_idx, /* Sector unit offset */
    cur_sect_idx, /* Sector unit offset */
    cur_offs_sect, /* Byte unit offset in sector */
    sect_idx_offs,
    filesize,
    gap_bytes,
    need_bytes,
    available_bytes, /* Available bytes in last cluster */
    used_bytes; /* Used bytes in last cluster */
  offs_t cur_offs;
  int32_t rtn;


  fe = ofe->fe;
  fvi = GET_FAT_VOL(fe->vol_id);

  filesize = fe->dir.filesize;
  cur_sect = ofe->cur_sect;
  cur_offs = ofe->cur_offs;

  seek_sect_idx = adjust_offs >> fvi->br.bits_per_sect;
  cur_sect_idx = ofe->cur_offs >> fvi->br.bits_per_sect;
  sect_idx_offs = seek_sect_idx - cur_sect_idx;
  cur_offs_sect = adjust_offs & fvi->br.bytes_per_sect_mask;

  if ((0 == (cur_offs & fvi->br.bytes_per_clust_mask)) && (filesize == cur_offs) && cur_offs)
    {
    /* If 'cur_offs' is aligned with cluster size, incerase 'seek_sect_offs' as 1.
       refer to NOTE(GRP2331). */
    sect_idx_offs++;
    }

  gap_bytes = adjust_offs - filesize;
  used_bytes = fvi->br.bytes_per_clust_mask & filesize;
  if (used_bytes) /* There are remained bytes at the last-cluster. */
    available_bytes = (1 << fvi->br.bits_per_clust) - used_bytes;
  else
    available_bytes = 0;

  if (gap_bytes > available_bytes)
    {
    need_bytes = gap_bytes - available_bytes;
    /* Allocate new area to change file size to seek offset. */
    rtn = __fat_file_fill_gap(ofe, gap_bytes, need_bytes);
    if (0 > rtn) return rtn;
    if (0 == filesize)
      cur_sect = D_CLUST_2_SECT(fvi, rtn);
    }
  else
    fe->dir.filesize += gap_bytes;

  fsm_assert3(fe->dir.filesize == adjust_offs);

  if (0 == (adjust_offs & fvi->br.bytes_per_clust_mask))
    {
    /* The seek_offs is aligned to the cluster-unit */
    /* The file pointer is over the last sector of the current file. */
    sect_idx_offs--; /* Move to the last sector */
    }

  rtn = fat_get_sectno(fe->vol_id, cur_sect, sect_idx_offs);
  if (0 > rtn) return rtn;
  cur_sect = rtn;

  ofe->cur_offs = adjust_offs;
  ofe->cur_sect = cur_sect;
  ofe->cur_offs_sect = (uint16_t) cur_offs_sect;

  return ofe->cur_offs;
  }

/*
 Name: __fat_adjust_pos
 Desc: Change the current position of a file to do some operations such as
       file-write, file-read.
 Params:
   - ofe: Pointer to the Open-File Entry.
 Returns:
   int32_t  >=0 on success. The returned value is the offset of position.
            < 0 on fail.
 Caveats: None.
 */

static int32_t __fat_adjust_pos(fat_ofileent_t *ofe, offs_t adjust_offs)
  {
  fat_volinfo_t *fvi;
  fat_fileent_t *fe = ofe->fe;
  uint32_t clust_no;


  /* Return this funciton if current position is the same as seek position. */
#if defined( WB )
  if (fe->wb && fe->wb->w_offs)
    {
    if (adjust_offs == wb_written_pos(fe))
      return s_ok;
    }
  else if (adjust_offs == ofe->cur_offs)
    return s_ok;
#else
  if (adjust_offs == ofe->cur_offs)
    return s_ok;
#endif

  if (0 == ofe->cur_sect && 0 != fe->dir.filesize)
    {
    /* This file is truncated in another Open-File Entry. */
    fvi = GET_FAT_VOL(fe->vol_id);
    clust_no = GET_OWN_CLUST(&fe->dir);
    ofe->cur_sect = D_CLUST_2_SECT(fvi, clust_no);
    fsm_assert1(ofe->cur_offs <= fvi->br.bytes_per_sect_mask);
    }

  /* Change current position to the seek-offset */
  if (adjust_offs < (int32_t) fe->dir.filesize)
    /* In case the seek-position is less than file size. */
    return __fat_adjust_pos_in(ofe, adjust_offs);
  else
    /* In case the seek-position is more than file size. */
    return __fat_adjust_pos_out(ofe, adjust_offs);
  }

/*
 Name: __fat_lseek
 Desc:  Re-designate to the file position
 Params:
   - ofe: Open-file entry pointer.
   - offset: File position offset
   - whence: The value to determine how the offset is to be interpreted.
            It has one of the following symbols.
            SEEK_SET: Sets the file pointer to the value of the offset parameter.
            SEEK_CUR: Sets the file pointer to its current location plus the
                      value of the offset parameter.
            SEEK_END: Sets the file pointer to the size of the file plus the
                      value of the Offset parameter.
 Returns:
   offs_t   Seek offset.
 Caveats: This function does not change the file size although the file
          position in the file is larger than the file size.
 */

static offs_t __fat_lseek(fat_ofileent_t *ofe, offs_t offset, int32_t whence)
  {
  fat_fileent_t *fe;
  offs_t seek_offs,
    offs;


  fe = ofe->fe;
  seek_offs = ofe->seek_offs;

  if (SEEK_SET == whence)
    offs = offset;
  else if (SEEK_CUR == whence)
    offs = seek_offs + offset;
  else /* SEEK_END */
#if defined( WB )
    if (fe->wb && fe->wb->w_offs)
    offs = fe->dir.filesize + __fat_get_out_size_wb(fe) + offset;
  else
    offs = fe->dir.filesize + offset;
#else
    offs = fe->dir.filesize + offset;
#endif

  if (0 > offs)
    ofe->seek_offs = 0;
  else
    ofe->seek_offs = offs;

  return ofe->seek_offs;
  }

/*
 Name: fat_access
 Desc: Check whether it is possible to approach to pertinent file or directory.
    specified by the path parameter.
 Params:
   - vol_id: Volume's ID to be accessed.
   - path: The pointer to the null-terminated path name of the file or
           directory to be checked.
   - amode: One of the file or directory access permissions. This is not
            supported in this version.
            The access mode should be one of the following symbols.
            R_OK: Read authority
            W_OK: Write authority
            X_OK: Execution authority.
            F_OK: File existence.
 Returns:
   int32_t  =0 on success. It means the file which has same path name exists
               in the volume.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_access(int32_t vol_id, const char *path, int32_t amode)
  {
  fat_fileent_t *fe;
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  int32_t rtn;


  /* Parse the path name. */
  rtn = fat_parse_path(path_buf, &arg, path);
  if (s_ok != rtn) return rtn;

  fat_lock();

  /* Allocate the buffer for File-Entry. */
  if (NULL == (fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return -1;
    }

  /* Search the parsed name in a volume and fill the information into the
     File-Entry structure. If the file exists, fat_lookup_entry() funtion
     returns the value equal to the 'arg.argc'. */
  rtn = fat_lookup_entry(&arg, fe, eFAT_ALL);

  /* Free the buffer for File-Entry. */
  fat_free_file_entry(fe);

  fat_unlock();

  if (0 > rtn)
    return -1;

  if (arg.argc == rtn)
    return s_ok;
  else if (arg.argc - 1 != rtn)
    /* Path not exist */
    return set_errno(e_notdir);
  else
    return -1; /* e_noent */
  }

/*
 Name: fat_do_truncate
 Desc: Truncate the size of file specified by fd parameter to new size.
 Params:
   - fe: The descriptor of the file to which the length is to be changed.
   - new_size: The new length of the file in bytes.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_do_truncate(fat_fileent_t *fe, size_t new_size)
  {
  fat_volinfo_t *fvi;
  int32_t old_cnt,
    new_cnt,
    free_cnt,
    first_clust_no,
    last_clust_no,
    last_clust_offs,
    alloc_cnt,
    rtn = s_ok;


  fvi = GET_FAT_VOL(fe->vol_id);

  if (new_size > fat_fs_total_bytes(fvi))
    return set_errno(e_nospc);

  /* Change the old number of clusters to the new number of clusters. */
  new_cnt = new_size / fvi->br.bits_per_clust;
  old_cnt = fe->dir.filesize / fvi->br.bits_per_clust;
  first_clust_no = GET_OWN_CLUST(&fe->dir);

  if (old_cnt == new_cnt)
    {
    if (new_size == fe->dir.filesize)
      return s_ok;

    /* If the old number of clusters is equal to tthe new number of clusters,
       update the write time, the access time and file size.*/
    fe->dir.filesize = new_size;
    fat_set_ent_time(&fe->dir, TM_WRITE_ENT | TM_ACCESS_ENT);
    rtn = fat_update_sentry(fe, true);
    }
  else if (old_cnt > new_cnt)
    {
    /* Free clusters */
    if (0 == first_clust_no) return set_errno(e_inval);

    /* If the new number of cluster is less than the old number of cluster,
       it reduces the number of cluster with the __fat_do_ftrunc() function. */
    if (0 == new_cnt)
      last_clust_offs = 0;
    else
      last_clust_offs = new_cnt - 1;
    free_cnt = old_cnt - new_cnt;
    last_clust_no = fat_get_clustno(fe->vol_id, first_clust_no, last_clust_offs);
    if (0 > last_clust_no) return -1;

    fe->dir.filesize = new_size;
    rtn = __fat_do_ftrunc(fe, last_clust_no, new_cnt, free_cnt);
    if (0 < rtn) rtn = s_ok;
    }
  else
    {
    /* Allocated clusters */
    if (0 == first_clust_no)
      last_clust_no = 0;
    else
      {
      last_clust_no = fat_get_clustno(fe->vol_id, first_clust_no, old_cnt - 1/*start index 0*/);
      if (0 > last_clust_no) return -1;
      }

    /* If the new number of cluster is more than the old number of cluster,
       it increases the number of cluster with the __fat_do_ftrunc() function. */
    alloc_cnt = new_cnt - old_cnt;
    rtn = __fat_do_falloc(fe, last_clust_no, alloc_cnt, new_size);
    if (0 < rtn) rtn = s_ok;
    }


  if (0 <= rtn)
    /* Invalidate physical file position of opend files. */
    __fat_invalidate_ofile_pos(fe);

  return rtn;
  }

/*
 Name: __fat_creat
 Desc: Create a file using a given File-Entry.
 Params:
   - fe: Pointer to the File-Entry.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __fat_creat(fat_fileent_t *fe, mod_t mode)
  {
  int32_t rtn;
#if defined( LOG )
  int32_t cl_id;
#endif


#if 0
  if (NULL != fat_get_opend_file_entry(fe))
    return set_errno(e_busy); /* This file is opened */
#endif

  fe->dir.attr = (uint8_t) eFAT_ATTR_ARCH;
  SET_OWN_CLUST(&fe->dir, 0);
  fe->dir.filesize = 0;
#ifdef SFILEMODE
  __fat_set_file_information(fe, mode, fe->dir.attr);
#endif

  /* Flush the LIM data cache to the physical device. */
  if (0 > lim_flush_cdsector()) return -1;

  /* Allocate the physical space for the File-Entry. */
  rtn = fat_alloc_entry_pos(fe);

#if defined( LOG )
  cl_id = fat_log_on(fe, NULL);
  if (0 > cl_id) return cl_id;
#endif

  /* Check for about fat_alloc_entry_pos(). */
  if (s_ok == rtn)
    {
    /* Save the created entry of new file */
    rtn = fat_creat_entry(fe, true);

#if defined( LOG )
    /* Flush the cache data to the physical device. */
    fat_sync_table(fe->vol_id, true);
#endif
    }

#if defined( LOG )
  fat_log_off(fe->vol_id, cl_id);
#endif

  return rtn;
  }

/*
 Name: fat_creat
 Desc: Create a file specified by the 'path' parameter. If the file already
       exists, it removes the file and then sets the size of file to zero to
       reuse.
 Params:
   - vol_id: The ID of the volume including the file to be created.
   - path: The pointer to the null-terminated path name of the file to be opened.
   - mode: The file mode indicates the file permission. (Not supported.)
 Returns:
    int32_t  >=0 on success. The value returned is the file descriptor.
             < 0 on fail.
 Caveats: If the file already exists, it removes the file and then sets the
          size of file to zero to reuse.
 */

int32_t fat_creat(int32_t vol_id, const char *path, mod_t mode)
  {
  uint32_t oflag;


  oflag = O_CREAT | O_WRONLY | O_TRUNC;

  return fat_open(vol_id, path, oflag, mode);
  }

/*
 Name: fat_open
 Desc: Open a file specified by the path parameter.
 Params:
   - vol_id: The ID of the volume including the file to be opened.
   - path: Pointer to the file name to be opend.
   - flag: The status flags and access modes of the file to be opened.
            O_RDONLY: Open file for reading only.
            O_WRONLY: Open file for reading only.
            O_RDWR: Open file for both reading and writing.
            O_APPEND: Open file with position the file offset at the end of the
                      file.
            O_CREAT: If the file being opened does not exist, it is created and
                     then opened.
            O_TRUNC: Truncate the file to zero length if the file exists.
            O_EXCL: Ignored if O_CREAT is not set. It causes the call to
                    open() to fail if the file already exists.
   - mode: Permission bits to use if a file is created. It is not supported in
           this version.
 Returns:
   int32_t  >=0 on success. The value returned is the file descriptor.
            < 0 on fail.
 Caveats: None.
 */

int32_t fat_open(int32_t vol_id, const char *path, uint32_t flag, mod_t mode)
  {
  fat_volinfo_t *fvi;
  fat_fileent_t *fe;
  fat_ofileent_t *ofe;
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  int32_t clust_no,
    rtn;


  fsm_assert2(0 <= vol_id);

  /* Parse the path name. */
  rtn = fat_parse_path(path_buf, &arg, path);

  if (!arg.argc)
    {
    set_errno(e_path);
    return -1;
    }

  if (s_ok != rtn) return rtn;

  fat_lock();

  /* Allocate the buffer for File-Entry. */
  if (NULL == (fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return -1;
    }

  /* Search the parsed name in a volume and fill the information into the
     File-Entry structure. If the file exists, fat_lookup_entry() funtion
     returns the value equal to the 'arg.argc'.*/
  rtn = fat_lookup_entry(&arg, fe, eFAT_FILE);
  if (0 > rtn) goto Error;

  if (arg.argc == rtn)
    {
    if ((O_CREAT & flag) && (O_EXCL & flag))
      {
      set_errno(e_exist);
      goto Error;
      }
    else if (O_TRUNC & flag)
      {
      /* Truncate file size to 0 */
      if (0 > fat_do_truncate(fe, 0))
        goto Error;
      }
    }
  else
    {
    if ((O_CREAT & flag) && (arg.argc - 1 == rtn))
      {
      if (0 > __fat_creat(fe, mode))
        goto Error;
      }
    else
      {
      if (arg.argc - 1 != rtn)
        set_errno(e_notdir);
      goto Error;
      }
    }

  ofe = __fat_alloc_ofile_entry(fe, flag);
  if (NULL == ofe) goto Error;

  /* Setup to first offset */
  fvi = GET_FAT_VOL(vol_id);
  clust_no = GET_OWN_CLUST(&ofe->fe->dir);
  ofe->cur_offs = ofe->seek_offs = 0;
  ofe->cur_sect = clust_no ? D_CLUST_2_SECT(fvi, clust_no) : 0;
  ofe->cur_offs_sect = 0;

  rtn = ofe->fd; /* Success */

  tr_fat_inc_open_cnt(vol_id);

  fat_unlock();
  return rtn;

Error:
  if (eFILE_ALLOC & fe->flag)
    /* Free the buffer for File-Entry. */
    fat_free_file_entry(fe);

  fat_unlock();
  return -1;
  }




#ifdef SFILEMODE

/*
Function __fat_set_file_information

Description
  File mode is defined as below. ctime_tenth field in directory entry will be used.
    #define S_IFIFO  0010000        // FIFO
    #define S_IFCHR  0020000        // Character device
    #define S_IFDIR  0040000        // Directory
    #define S_IFBLK  0060000        // Block device
    #define S_IFREG  0100000        // Regular file
    #define S_IFLNK  0120000        // Symlink
    #define S_IFSOCK 0140000        // Socket
    #define S_IFITM  0160000        // Item File
    #define S_IFMT   0170000        // Mask of all values
 */
void __fat_set_file_information(fat_fileent_t *fe, mod_t mode, uint8_t attr)
  {
  uint8_t file_mode;

  file_mode = (uint8_t) (mode >> 12);

  if (file_mode == 0)
    {
    switch (attr)
      {
      case eFAT_ATTR_ARCH:
        file_mode = 0x8; //     S_IFREG;
        break;
      case eFAT_ATTR_DIR:
      case 0x30:
        file_mode = 0x4; //S_IFDIR;
        break;
      default:
        file_mode = 0x8; //     S_IFREG;
        break;
      }
    }
  fe->dir.char_case = (fe->dir.char_case & 0x18) | TO_NTRES(file_mode);
  }
#endif

/*
 Name: __file_read_first_clust
 Desc: Read data from the current file position to the previous position of
       the next cluster.
 Params:
   - fd: Pointer to the Open-File Entry.
   - buf: The pointer to a buffer in which the bytes read are placed.
   - cnt: The number of bytes to be read.
 Returns:
   int32_t  >=0 on success. The value returned is the number of bytes actually
                read. If the value is '0', the remained offset in the cluster
                is not exist.
            < 0 on fail.
 Caveats: None.
 */

static int32_t __file_read_first_clust(fat_ofileent_t *ofe, uint8_t *buf, uint32_t cnt)
  {
  pim_devinfo_t *pdi;
  fat_volinfo_t *fvi;
  fat_fileent_t *fe;
  lim_cacheent_t *ce;
  uint32_t size,
    real_cnt,
    left_cnt, /* remain count */
    left_bytes_in_lastsect,
    offs_in_clust,
    sect_cnt,
    cur_offs_sect,
    cur_sect;
  int32_t rtn;
  offs_t cur_offs;
  uint8_t *p;


  fe = ofe->fe;
  fvi = GET_FAT_VOL(fe->vol_id);
  pdi = GET_LIM_DEV(fe->vol_id);

  /* Calculate byte offset in current cluster */
  offs_in_clust = ofe->cur_offs & fvi->br.bytes_per_clust_mask;

  if (0 == offs_in_clust)
    {
    fsm_assert1(0 == ofe->cur_offs_sect);
    /* The remained offset in the cluster is not exist. */
    return 0;
    }

  /* Load current file pointer */
  cur_offs = ofe->cur_offs;
  cur_sect = ofe->cur_sect;
  cur_offs_sect = ofe->cur_offs_sect;

  /* Calculate real reading/writing size */
  real_cnt /* Left bytes in cluster */ = (1 << fvi->br.bits_per_clust) - offs_in_clust;
  left_cnt = real_cnt = (real_cnt > cnt) ? cnt : real_cnt;


  /*************** Read first sector ***************/
  if (cur_offs_sect)
    {
    left_bytes_in_lastsect = (1 << fvi->br.bits_per_sect)/*sector size*/ - ofe->cur_offs_sect;
    size = (left_bytes_in_lastsect > left_cnt) ? left_cnt : left_bytes_in_lastsect;

    if (ePIM_PartialRead & pdi->dev_flag)
      {
      /* If memory device is able to read in bytes like NOR Flash memory, */
      use_read_fat_unlock();
      rtn = lim_read_at_sector(fe->vol_id, cur_sect, cur_offs_sect, size, buf);
      use_read_fat_lock();
      if (0 > rtn)
        return -1;
      }
    else
      {
      /* If memory device is able to read in sectors like NAND Flash memory, */
      ce = lim_get_sector(fe->vol_id, cur_sect);
      if (NULL == ce) return -1;
      p = LIM_REAL_BUF_ADDR(ce->buf, cur_sect, fvi->br.first_root_sect) + cur_offs_sect;
      memcpy(buf, p, size);

      lim_rel_csector(ce);
      }

    left_cnt -= size;
    buf += size;
    cur_offs += size;
    cur_offs_sect = (cur_offs_sect + size) & fvi->br.bytes_per_sect_mask;
    if (0 == cur_offs_sect)
      cur_sect++;
    }

  /* Read the remained align sectors in the cluster. */
  if (left_cnt)
    {
    /* Calculate the remained sectors in current cluster */
    sect_cnt = left_cnt >> fvi->br.bits_per_sect;

    if (sect_cnt)
      {
      use_read_fat_unlock();
      rtn = lim_read_sector(fe->vol_id, cur_sect, buf, sect_cnt);
      use_read_fat_lock();
      if (0 > rtn)
        return -1;

      size = sect_cnt << fvi->br.bits_per_sect;
      left_cnt -= size;
      buf += size;
      cur_offs += size;
      cur_sect += sect_cnt;
      cur_offs_sect = 0;
      }
    }

  /* Read the remained bytes. */
  if (left_cnt)
    {
    fsm_assert1(left_cnt <= fvi->br.bytes_per_sect_mask);

    if (ePIM_PartialRead & pdi->dev_flag)
      {
      use_read_fat_unlock();
      rtn = lim_read_at_sector(fe->vol_id, cur_sect, 0, left_cnt, buf);
      use_read_fat_lock();
      if (0 > rtn)
        return -1;
      }
    else
      {
      ce = lim_get_sector(fe->vol_id, cur_sect);
      if (NULL == ce) return -1;
      p = LIM_REAL_BUF_ADDR(ce->buf, cur_sect, fvi->br.first_root_sect);
      memcpy(buf, p, left_cnt);

      lim_rel_csector(ce);
      }

    cur_offs += left_cnt;
    cur_offs_sect = left_cnt;
    }


  /* Move file pointer */
  ofe->cur_offs = cur_offs;

  if (D_SECT_2_CLUST(fvi, ofe->cur_sect) != D_SECT_2_CLUST(fvi, cur_sect))
    {
    int32_t tmp = fat_get_next_sectno(fe->vol_id, cur_sect - 1);
    if (eFAT_EOF == tmp)
      ofe->cur_sect = cur_sect - 1;
    else
      ofe->cur_sect = tmp;
    ofe->cur_offs_sect = 0;
    }
  else
    {
    ofe->cur_sect = cur_sect;
    ofe->cur_offs_sect = (uint16_t) cur_offs_sect;
    }

  return real_cnt;
  }

/*
 Name: __file_read_align_clust
 Desc: Read data from the first position of a cluster as reading size.
 Params:
   - fd: Pointer to the Open-File Entry.
   - buf: The pointer to a buffer in which the bytes read are placed.
   - cnt: The number of bytes to be read.
 Returns:
   int32_t  >=0 on success. The value returned is the number of bytes actually
                read.
            < 0 on fail.
 Caveats: None.
 */

static int32_t __file_read_align_clust(fat_ofileent_t *ofe, uint8_t *buf, uint32_t cnt)
  {
  pim_devinfo_t *pdi;
  fat_volinfo_t *fvi;
  fat_fileent_t *fe;
  lim_cacheent_t *ce;
  uint32_t real_cnt, /* Real write size */
    left_bytes_in_lastclust, /* The number of remained bytes in the last cluster. */
    left_sects_in_lastclust, /* The number of remained sectors in the last cluster. */
    left_bytes_in_lastsect, /* The number of remained bytes in the last sector. */
    sect_cnt,
    cur_offs_sect,
    cur_sect,
    dwReadClustCnt,
    alloc_clust_cnt,
    left_clust_cnt,
    last_clust_no,
    sect_no,
    start_idx,
    clust_idx,
    tmp;
  ionfs_local uint32_t clust_list[CLUST_LIST_BUF_CNT + 1/*Next free cluster*/];
  int32_t rtn;
  uint8_t *p;
  bool has_dregs; /* In cluster */


  if (0 == cnt)
    return set_errno(e_inval);

  fe = ofe->fe;
  fvi = GET_FAT_VOL(fe->vol_id);
  pdi = GET_LIM_DEV(fe->vol_id);

  /* Is current offset located in the end of the last cluster? */
  fsm_assert1(0 == (ofe->cur_offs & fvi->br.bytes_per_clust_mask));
  fsm_assert1(0 == ofe->cur_offs_sect && 0 == GET_SECT_IDX(fvi, ofe->cur_sect));

  /* Calculate real writing size */
  left_bytes_in_lastclust = ((1 << fvi->br.bits_per_clust) -
                             (fe->dir.filesize & fvi->br.bytes_per_clust_mask)) &
    fvi->br.bytes_per_clust_mask;
  real_cnt = (fe->dir.filesize + left_bytes_in_lastclust) - ofe->cur_offs;
  real_cnt = (real_cnt > cnt) ? cnt : real_cnt;
  left_clust_cnt = real_cnt >> fvi->br.bits_per_clust;
  left_sects_in_lastclust = (real_cnt >> fvi->br.bits_per_sect) & fvi->br.sects_per_clust_mask;
  left_bytes_in_lastsect = real_cnt & fvi->br.bytes_per_sect_mask;

  /* Prevent "Warning: C2874W: sect_no may be used before being set" */
  sect_no = ofe->cur_sect;
  sect_cnt = 0;

  has_dregs = (left_sects_in_lastclust || left_bytes_in_lastsect) ? true : false;
  last_clust_no = D_SECT_2_CLUST(fvi, ofe->cur_sect);

  while (left_clust_cnt)
    {
    alloc_clust_cnt = (CLUST_LIST_BUF_CNT - 1) > left_clust_cnt ? left_clust_cnt : (CLUST_LIST_BUF_CNT - 1);

    if ((CLUST_LIST_BUF_CNT - 1) == alloc_clust_cnt)
      dwReadClustCnt = alloc_clust_cnt - 1;
    else
      {
      dwReadClustCnt = alloc_clust_cnt;
      /* If there is remained bytes in the last cluster. */
      if (true == has_dregs)
        alloc_clust_cnt++;
      }

    rtn = fat_get_clust_list(fe->vol_id, last_clust_no, alloc_clust_cnt, clust_list);
    if (alloc_clust_cnt != rtn)
      return set_errno(e_cfs);

    last_clust_no = clust_list[dwReadClustCnt]; /* Save the last cluster */
    left_clust_cnt -= dwReadClustCnt;

    /* Read/write that aligned cluster */

    use_read_fat_unlock();
    clust_list[dwReadClustCnt] = 0; /* In the following expression access 'clust_idx+1' */

    for (start_idx = clust_idx = 0; clust_idx < dwReadClustCnt; clust_idx++)
      {
      if (clust_list[clust_idx] + 1 != clust_list[clust_idx + 1])
        {
        /* Read sequential clusters */
        sect_no = D_CLUST_2_SECT(fvi, clust_list[start_idx]);
        sect_cnt = (clust_idx - start_idx + 1) << fvi->br.bits_per_clustsect;

        rtn = lim_read_sector(fe->vol_id, sect_no, buf, sect_cnt);
        if (0 > rtn)
          {
          use_read_fat_lock();
          return rtn;
          }

        buf += (sect_cnt << fvi->br.bits_per_sect);
        start_idx = clust_idx + 1/*Next index*/;
        }
      }

    use_read_fat_lock();
    }


  if (0 == has_dregs)
    { /* ( 0 == left_sects_in_lastclust && 0 == left_bytes_in_lastsect ) */
    /* Check the file pointer. */
    cur_sect = sect_no + sect_cnt;
    tmp = fat_get_next_sectno(fe->vol_id, cur_sect - 1);
    if (eFAT_EOF == tmp)
      cur_sect--; /* NOTE(GRP2331): Move last sector */
    else
      cur_sect = tmp;
    cur_offs_sect = 0;
    }
  else
    {
    cur_sect = D_CLUST_2_SECT(fvi, last_clust_no);
    cur_offs_sect = left_bytes_in_lastsect;

    if (left_sects_in_lastclust)
      {
      use_read_fat_unlock();
      /* Read remained sectors in the last cluster */
      rtn = lim_read_sector(fe->vol_id, cur_sect, buf, left_sects_in_lastclust);
      use_read_fat_lock();
      if (0 > rtn) return rtn;

      cur_sect += left_sects_in_lastclust;
      buf += (left_sects_in_lastclust << fvi->br.bits_per_sect);
      }

    /* Read remained bytes in the last sector */
    if (left_bytes_in_lastsect)
      {
      if (ePIM_PartialRead & pdi->dev_flag)
        {
        use_read_fat_unlock();
        rtn = lim_read_at_sector(fe->vol_id, cur_sect, 0, cur_offs_sect, buf);
        use_read_fat_lock();
        if (0 > rtn)
          return -1;
        }
      else
        {
        ce = lim_get_sector(fe->vol_id, cur_sect);
        if (NULL == ce) return -1;
        p = LIM_REAL_BUF_ADDR(ce->buf, cur_sect, fvi->br.first_root_sect);
        memcpy(buf, p, left_bytes_in_lastsect);

        lim_rel_csector(ce);
        }
      }
    }

  /* Move the file pointer. */
  ofe->cur_offs += real_cnt;
  ofe->cur_sect = cur_sect;
  ofe->cur_offs_sect = (uint16_t) cur_offs_sect;

  return real_cnt;
  }

/*
 Name: fat_read
 Desc: Read data from a file specified by the fd parameter.
 Params:
   - fd: The file descriptor to be read.
   - buf: The pointer to a buffer in which the bytes read are placed.
   - bytes: The number of bytes to be read.
 Returns:
   size_t  >=0 on success. The value returned is the number of bytes actually
               read and placed in 'buf' buffer.
           < 0  on fail.
 Caveats: If the number of bytes to be read was larger than the remained size
          of file at the current position of file, it may be less than bytes.
 */

size_t fat_read(int32_t fd, void *buf, size_t bytes)
  {
  fat_ofileent_t *ofe;
  int32_t left_cnt,
    read_cnt,
    rtn;
  uint8_t *p_read_pos = (uint8_t *) buf;


  fat_lock();

  /* Allocate the buffer for the opened File-Entry indicated by the file
     descriptor. */
  ofe = __fat_get_ofile_entry(fd);
  if ((fat_ofileent_t *) NULL == ofe)
    {
    fat_unlock();
    return -1;
    }

  /* Check the access permission. */
  if (ofe->oflag & O_WRONLY)
    {
    fat_unlock();
    return set_errno(e_access);
    }

#if defined( WB )
  if (ofe->fe->wb && ofe->fe->wb->w_offs)
    {
    ofile_lock();
    rtn = __fat_flush_wb(ofe);
    ofile_unlock();

    if (0 > rtn)
      {
      fat_unlock();
      return -1;
      }
    }
#endif

  /* File size is '0' */
  if (0 == ofe->cur_sect)
    {
    fat_unlock();
    return 0;
    }

  /* End of file */
  if (ofe->seek_offs >= (offs_t) ofe->fe->dir.filesize)
    {
    fat_unlock();
    return 0;
    }

  /* Set the position of file pointer to be read. */
  rtn = __fat_adjust_pos(ofe, ofe->seek_offs);
  if (0 > rtn)
    {
    fat_unlock();
    return (size_t) rtn;
    }

  if (ofe->fe->dir.filesize <= (ofe->cur_offs + bytes))
    bytes = ofe->fe->dir.filesize - ofe->cur_offs;

  left_cnt = bytes;

  /* Read the cluster including the current position. */
  if (left_cnt)
    {
    read_cnt = __file_read_first_clust(ofe, p_read_pos, left_cnt);
    if (0 > read_cnt)
      {
      fat_unlock();
      return -1;
      }
    p_read_pos += read_cnt;
    left_cnt -= read_cnt;
    }

  /*************** Read body sectors ***************/
  if (left_cnt)
    {
    read_cnt = __file_read_align_clust(ofe, p_read_pos, left_cnt);
    if (left_cnt != read_cnt)
      {
      fat_unlock();
      return -1;
      }
    }

  /*
  fat_set_ent_time( &fe->dir, TM_ACCESS_ENT );
  fat_update_sentry( ofe->fe, true );
   */
  ofe->seek_offs += bytes;
  fsm_assert2(ofe->seek_offs == ofe->cur_offs);

  tr_fat_inc_read_cnt(ofe->fe->vol_id);

  fat_unlock();
  return bytes;
  }

/*
 Name: __file_write_exi_first_clust
 Desc: Write data from the current file position to the previous position of
       the next cluster.
 Params:
   - ofe: Pointer to the Open-File Entry.
   - buf: Pointer to a buffer containing the data to be written.
   - cnt: The size in bytes of the data to be written.
 Returns:
   int32_t  >=0 on success. The returned value is the number of bytes actually
                written. If the value is '0', it means the remained offset in
                the cluster is not exist.
            < 0 on fail.
 Caveats: None.
 */

static int32_t __file_write_exi_first_clust(fat_ofileent_t *ofe, uint8_t *buf, uint32_t cnt)
  {
  fat_volinfo_t *fvi;
  fat_fileent_t *fe;
  uint32_t size,
    real_cnt,
    left_cnt, /* Remain count */
    left_bytes_in_lastsect,
    offs_in_clust,
    sect_cnt,
    cur_offs_sect,
    cur_sect;
  int32_t rtn;
  offs_t cur_offs;
  uint8_t *p, align_buf[FAT_ALLOW_MAX_SECT_SIZE];

  fe = ofe->fe;
  fvi = GET_FAT_VOL(fe->vol_id);

  /* Calculate byte offset in current cluster */
  offs_in_clust = ofe->cur_offs & fvi->br.bytes_per_clust_mask;

  if (0 == offs_in_clust)
    {
    fsm_assert1(0 == ofe->cur_offs_sect);
    /* Remained offset in the cluster is not exist. */
    return 0;
    }

  /* Load current file pointer */
  cur_offs = ofe->cur_offs;
  cur_sect = ofe->cur_sect;
  cur_offs_sect = ofe->cur_offs_sect;

  /* Calculate real reading/writing size */
  real_cnt /* Left bytes in cluster */ = (1 << fvi->br.bits_per_clust) - offs_in_clust;
  left_cnt = real_cnt = (real_cnt > cnt) ? cnt : real_cnt;


  /*************** Write first sector ***************/
  if (cur_offs_sect)
    {
    rtn = lim_load_cdsector(fe->vol_id, cur_sect, align_buf);
    if (0 > rtn) return rtn;
    p = align_buf + cur_offs_sect;

    left_bytes_in_lastsect = (1 << fvi->br.bits_per_sect)/*sector size*/ - ofe->cur_offs_sect;
    size = (left_bytes_in_lastsect > left_cnt) ? left_cnt : left_bytes_in_lastsect;
    memcpy(p, buf, size);

    rtn = lim_write_sector(fe->vol_id, cur_sect, (void *) align_buf, 1);
    if (0 > rtn) return rtn;

    left_cnt -= size;
    buf += size;
    cur_offs += size;
    cur_offs_sect = (cur_offs_sect + size) & fvi->br.bytes_per_sect_mask;
    if (0 == cur_offs_sect)
      cur_sect++;
    }

  /* Write remained align sectors in cluster */
  if (left_cnt)
    {
    /* Calculate remained sectors in current cluster */
    sect_cnt = left_cnt >> fvi->br.bits_per_sect;

    if (sect_cnt)
      {
      /* Flush the LIM data cache to the physical device. */
      if (0 > lim_flush_cdsector()) return -1;
      use_write_fat_unlock();
      rtn = lim_write_sector(fe->vol_id, cur_sect, buf, sect_cnt);
      use_write_fat_lock();
      if (0 > rtn)
        return -1;

      size = sect_cnt << fvi->br.bits_per_sect;
      left_cnt -= size;
      buf += size;
      cur_offs += size;
      cur_sect += sect_cnt;
      cur_offs_sect = 0;
      }
    }

  /* Write the remained bytes */
  if (left_cnt)
    {
    fsm_assert1(left_cnt <= fvi->br.bytes_per_sect_mask);
    rtn = lim_load_cdsector(fe->vol_id, cur_sect, align_buf);
    if (0 > rtn) return rtn;

    p = align_buf;
    memcpy(p, buf, left_cnt);
    rtn = lim_write_sector(fe->vol_id, cur_sect, (void *) align_buf, 1);
    if (0 > rtn) return rtn;

    cur_offs += left_cnt;
    cur_offs_sect = left_cnt;
    }


  /* Move the file pointer. */
  ofe->cur_offs = cur_offs;

  if (D_SECT_2_CLUST(fvi, ofe->cur_sect) != D_SECT_2_CLUST(fvi, cur_sect))
    {
    int32_t tmp = fat_get_next_sectno(fe->vol_id, cur_sect - 1);
    if (eFAT_EOF == tmp)
      ofe->cur_sect = cur_sect - 1;
    else
      ofe->cur_sect = tmp;
    ofe->cur_offs_sect = 0;
    }
  else
    {
    ofe->cur_sect = cur_sect;
    ofe->cur_offs_sect = (uint16_t) cur_offs_sect;
    }

  return real_cnt;
  }

/*
 Name: __file_write_exi_align_clust
 Desc: Write data from the first position of a cluster as writing size.
 Params:
   - ofe: Pointer to the Open-File Entry.
   - buf: Pointer to a buffer containing the data to be written.
   - cnt: The size in bytes of the data to be written.
 Returns:
   int32_t  >=0 on success. The returned value is the number of bytes actually
                written.
            < 0  on fail.
 Caveats: None.
 */

static int32_t __file_write_exi_align_clust(fat_ofileent_t *ofe, uint8_t *buf, uint32_t cnt)
  {
  fat_volinfo_t *fvi;
  fat_fileent_t *fe;
  uint32_t real_cnt, /* Real write size */
    left_bytes_in_lastclust, /* the number of remained bytes in the last cluster. */
    left_sects_in_lastclust, /* the number of remained sectors in the last cluster. */
    left_bytes_in_lastsect, /* The number of remained bytes in the last sector. */
    sect_cnt,
    cur_offs_sect,
    cur_sect,
    write_clust_cnt,
    alloc_clust_cnt,
    left_clust_cnt,
    last_clust_no,
    sect_no,
    start_idx,
    clust_idx,
    tmp;
  ionfs_local uint32_t clust_list[CLUST_LIST_BUF_CNT + 1/*Next free cluster*/];
  int32_t rtn;
  uint8_t *p, align_buf[FAT_ALLOW_MAX_SECT_SIZE];
  bool has_dregs; /* In cluster */


  if (0 == cnt)
    return set_errno(e_inval);

  fe = ofe->fe;
  fvi = GET_FAT_VOL(fe->vol_id);

  /* Is current offset located in the end of the last cluster? */
  fsm_assert1(0 == (ofe->cur_offs & fvi->br.bytes_per_clust_mask));
  fsm_assert1(0 == ofe->cur_offs_sect && 0 == GET_SECT_IDX(fvi, ofe->cur_sect));

  /* Calculate real writing size */
  left_bytes_in_lastclust = ((1 << fvi->br.bits_per_clust) -
                             (fe->dir.filesize & fvi->br.bytes_per_clust_mask)) &
    fvi->br.bytes_per_clust_mask;
  real_cnt = (fe->dir.filesize + left_bytes_in_lastclust) - ofe->cur_offs;
  real_cnt = (real_cnt > cnt) ? cnt : real_cnt;
  left_clust_cnt = real_cnt >> fvi->br.bits_per_clust;
  left_sects_in_lastclust = (real_cnt >> fvi->br.bits_per_sect) & fvi->br.sects_per_clust_mask;
  left_bytes_in_lastsect = real_cnt & fvi->br.bytes_per_sect_mask;

  /* Prevent "Warning: C2874W: sect_no may be used before being set" */
  sect_no = ofe->cur_sect;
  sect_cnt = 0;

  has_dregs = (left_sects_in_lastclust || left_bytes_in_lastsect) ? true : false;
  last_clust_no = D_SECT_2_CLUST(fvi, ofe->cur_sect);

  while (left_clust_cnt)
    {
    alloc_clust_cnt = (CLUST_LIST_BUF_CNT - 1) > left_clust_cnt ? left_clust_cnt : (CLUST_LIST_BUF_CNT - 1);

    if ((CLUST_LIST_BUF_CNT - 1) == alloc_clust_cnt)
      write_clust_cnt = alloc_clust_cnt - 1;
    else
      {
      write_clust_cnt = alloc_clust_cnt;
      /* If there is remained bytes in the last cluster. */
      if (true == has_dregs)
        alloc_clust_cnt++;
      }

    rtn = fat_get_clust_list(fe->vol_id, last_clust_no, alloc_clust_cnt, clust_list);
    if (alloc_clust_cnt != rtn)
      {
      if (0 > rtn)
        return -1;
      else
        return set_errno(e_cfat);
      }

    /* Save the last cluster */
    last_clust_no = clust_list[write_clust_cnt];
    left_clust_cnt -= write_clust_cnt;

    /* Write that aligned cluster. */

    use_write_fat_unlock();
    /* In the following expression access 'clust_idx+1' */
    clust_list[write_clust_cnt] = 0;

    for (start_idx = clust_idx = 0; clust_idx < write_clust_cnt; clust_idx++)
      {
      if (clust_list[clust_idx] + 1 != clust_list[clust_idx + 1])
        {
        /* Write the sequential clusters */
        sect_no = D_CLUST_2_SECT(fvi, clust_list[start_idx]);
        sect_cnt = (clust_idx - start_idx + 1) << fvi->br.bits_per_clustsect;
        /* Flush the LIM data cache to the physical device. */
        if (0 > lim_flush_cdsector())
          {
          use_write_fat_lock();
          return -1;
          }

        rtn = lim_write_sector(fe->vol_id, sect_no, buf, sect_cnt);
        if (0 > rtn)
          {
          use_write_fat_lock();
          return rtn;
          }

        buf += (sect_cnt << fvi->br.bits_per_sect);
        start_idx = clust_idx + 1/*Next index*/;
        }
      }

    use_write_fat_lock();
    }


  if (0 == has_dregs)
    { /* ( 0 == left_sects_in_lastclust && 0 == left_bytes_in_lastsect ) */
    /* Check the file pointer. */
    cur_sect = sect_no + sect_cnt;
    tmp = fat_get_next_sectno(fe->vol_id, cur_sect - 1);
    if (eFAT_EOF == tmp)
      cur_sect--; /* NOTE(GRP2331): Move last sector */
    else
      cur_sect = tmp;
    cur_offs_sect = 0;
    }
  else
    {
    cur_sect = D_CLUST_2_SECT(fvi, last_clust_no);
    cur_offs_sect = left_bytes_in_lastsect;

    if (left_sects_in_lastclust)
      {
      /* Flush the LIM data cache to the physical device. */
      if (0 > lim_flush_cdsector()) return -1;
      use_write_fat_unlock();
      /* Write the remained sectors in the last cluster. */
      rtn = lim_write_sector(fe->vol_id, cur_sect, buf, left_sects_in_lastclust);
      use_write_fat_lock();
      if (0 > rtn) return rtn;

      cur_sect += left_sects_in_lastclust;
      buf += (left_sects_in_lastclust << fvi->br.bits_per_sect);
      }

    /* Write remained bytes in the last sector.*/
    if (left_bytes_in_lastsect)
      {
      rtn = lim_load_cdsector(fe->vol_id, cur_sect, align_buf);
      if (0 > rtn) return rtn;

      p = align_buf;
      memcpy(p, buf, left_bytes_in_lastsect);
      rtn = lim_write_sector(fe->vol_id, cur_sect, (void *) align_buf, 1);
      if (0 > rtn) return rtn;
      }
    }

  /* Move the file pointer. */
  ofe->cur_offs += real_cnt;
  ofe->cur_sect = cur_sect;
  ofe->cur_offs_sect = (uint16_t) cur_offs_sect;

  return real_cnt;
  }

/*
 Name: __file_write_new_clust
 Desc: Write data at the allocated cluster.
 Params:
    - ofe: Pointer to the Open-File Entry.
    - buf: Pointer to a buffer containing the data to be written.
    - cnt: The size in bytes of the data to be written.
 Returns:
   int32_t  >=0 on success. The returned value is the number of bytes actually
                 written.
            < 0  on fail.
 Caveats: The cluster should be allocated newly.
 */

static int32_t __file_write_new_clust(fat_ofileent_t *ofe, uint8_t *buf, uint32_t cnt)
  {
  fat_volinfo_t *fvi;
  fat_fileent_t *fe;
  uint32_t need_clusts,
    align_clusts,
    last_clust_no,
    clust_idx,
    start_idx,
    sect_no,
    ori_file_size,
    remember_clust,
    written_bytes,
    *alloced_list;
  ionfs_local uint32_t clust_list[1/*Last cluster*/ + CLUST_LIST_BUF_CNT + 1/*For alignment writing*/];
  int32_t left_clusts,
    left_sects_in_lastclust,
    left_bytes_in_lastsect,
    sect_cnt,
    rtn;
  uint8_t *p = (uint8_t *) buf, align_buf[FAT_ALLOW_MAX_SECT_SIZE];
  bool break_loop = false;
#if defined( LOG )
  int32_t cl_id = -1;
#endif


  fe = ofe->fe;
  fvi = GET_FAT_VOL(fe->vol_id);

  if (cnt > fat_fs_total_bytes(fvi))
    return set_errno(e_nospc);

  fsm_assert1(0 == ofe->cur_offs_sect);

  /* Calculate needed clusters */
  left_clusts = cnt / fvi->br.bits_per_clust;
  /* Calculate the remained sectors */
  left_sects_in_lastclust = cnt & fvi->br.bytes_per_clust_mask / fvi->br.bits_per_sect;
  /* Calculate the remained bytes. */
  left_bytes_in_lastsect = cnt & fvi->br.bytes_per_sect_mask;

  /* Prevent "Warning: C2874W: sect_no may be used before being set" */
  sect_no = ofe->cur_sect;
  written_bytes = sect_cnt = 0;
  ori_file_size = fe->dir.filesize; /* The filesize is changed */

  /* Get the last cluster. */
  if (0 == ofe->cur_sect) last_clust_no = 0;
  else last_clust_no = D_SECT_2_CLUST(fvi, ofe->cur_sect);

  /* Loop for the writing*/
  do
    {
    need_clusts = ((CLUST_LIST_BUF_CNT - 1) > left_clusts) ? left_clusts : (CLUST_LIST_BUF_CNT - 1);

    clust_list[0] = last_clust_no;
    left_clusts -= need_clusts;
    alloced_list = &clust_list[1];

    /* Allocate the needed free clusters from FAT Table. */
    rtn = fat_map_alloc_clusts(fe->vol_id, need_clusts, alloced_list);
    if (0 > rtn && e_nospc == get_errno())
      break;

    if (0 == last_clust_no && 0 == fe->dir.filesize)
      { /* File size is 0 */
      fsm_assert1(0 == GET_OWN_CLUST(&fe->dir));
      SET_OWN_CLUST(&fe->dir, *alloced_list);
      }
    /* Save before loop's last cluster number */
    last_clust_no = alloced_list[need_clusts - 1];

    /* If a sector exists in the last cluster.. */
    if (0 == left_clusts && 0 != left_sects_in_lastclust)
      align_clusts = need_clusts - 1;
    else
      align_clusts = need_clusts;

    use_write_fat_unlock();

    /*
       Write sequential clusters.
       NOTE(GRP1): At the following expression, we access 'clust_idx+1'.
     */
    remember_clust = alloced_list[align_clusts];
    alloced_list[align_clusts] = 0;
    for (start_idx = clust_idx = 0; clust_idx < align_clusts; clust_idx++)
      {
      if (alloced_list[clust_idx] + 1 != alloced_list[clust_idx + 1])
        {
        /* Write sequential clusters. */
        sect_no = D_CLUST_2_SECT(fvi, alloced_list[start_idx]);
        sect_cnt = (clust_idx - start_idx + 1) << fvi->br.bits_per_clustsect;

        /* Flush the LIM data cache to the physical device. */
        if (0 > lim_flush_cdsector())
          {
          use_write_fat_lock();
          alloced_list[align_clusts] = remember_clust;
          fat_map_free_clusts(fe->vol_id, need_clusts, alloced_list);
          return -1;
          }

        rtn = lim_write_sector(fe->vol_id, sect_no, p, sect_cnt);
        if (0 > rtn)
          {
          use_write_fat_lock();
          alloced_list[align_clusts] = remember_clust;
          fat_map_free_clusts(fe->vol_id, need_clusts, alloced_list);
          return -1;
          }

        written_bytes += sect_cnt << fvi->br.bits_per_sect;
        p += (sect_cnt << fvi->br.bits_per_sect);
        /* Wrote up to current index */
        start_idx = clust_idx + 1;
        sect_no += sect_cnt;
        }
      }
    alloced_list[align_clusts] = remember_clust;

    use_write_fat_lock();

    if (0 >= left_clusts)
      {
      fsm_assert1(0 == left_clusts);
      if (left_sects_in_lastclust)
        {
        /* Write the remained sectors in the last cluster. */
        sect_no = D_CLUST_2_SECT(fvi, last_clust_no);

        if (left_bytes_in_lastsect)
          sect_cnt = left_sects_in_lastclust - 1/*the last sector*/;
        else
          sect_cnt = left_sects_in_lastclust;

        if (sect_cnt)
          {
          /* Flush the LIM data cache to the physical device. */
          if (0 > lim_flush_cdsector())
            {
            fat_map_free_clusts(fe->vol_id, need_clusts, alloced_list);
            return -1;
            }

          use_write_fat_unlock();
          rtn = lim_write_sector(fe->vol_id, sect_no, p, sect_cnt);
          use_write_fat_lock();
          if (0 > rtn)
            {
            fat_map_free_clusts(fe->vol_id, need_clusts, alloced_list);
            return -1;
            }
          sect_no += sect_cnt;
          written_bytes += sect_cnt << fvi->br.bits_per_sect;
          p += (sect_cnt << fvi->br.bits_per_sect);
          }

        if (left_bytes_in_lastsect)
          {
          memcpy(align_buf, p, left_bytes_in_lastsect);
          rtn = lim_write_sector(fe->vol_id, sect_no, (void *) align_buf, 1);
          if (0 > rtn) return rtn;
          written_bytes += left_bytes_in_lastsect;
          }
        }
      /* The Writing is done. */
      break_loop = true;
      }

#if defined( LOG )
    /* We call fat_log_on() once. */
    if (-1 == cl_id)
      {
      cl_id = fat_log_on(fe, NULL);
      if (0 > cl_id) return cl_id;
      }
#endif

    /* Link the allocated free clusters using by FAT Entry. */
    rtn = fat_stamp_clusts(fe->vol_id, need_clusts, clust_list);
#if defined( LOG )
    if (s_ok == rtn)
      /* Save the changed FAT Entry to the physical device. */
      rtn = fat_sync_table(fe->vol_id, true);
#endif

    if (0 > rtn)
      {
#if defined( LOG )
      fat_log_off(fe->vol_id, cl_id);
#endif
      return rtn;
      }
    }
  while (false == break_loop);

  fsm_assert1(cnt >= written_bytes);

  /* Update the write-access time and the file size. */
  fe->dir.filesize = ori_file_size + written_bytes;
  fat_set_ent_time(&ofe->fe->dir, TM_WRITE_ENT | TM_ACCESS_ENT);
#if defined( LOG )
  fat_update_sentry(ofe->fe, true);
#else
  fat_update_sentry(ofe->fe, false);
  ofe->state |= eFILE_DIRTY;
#endif

#if defined( LOG )
  fat_log_off(fe->vol_id, cl_id);
#endif

  if (written_bytes)
    {
    /* Calculate file pointer */
    ofe->cur_offs_sect = (uint16_t) (written_bytes & fvi->br.bytes_per_sect_mask);
    ofe->cur_offs += written_bytes;

    /* If it is the end of last cluster? */
    if (0 == (ofe->cur_offs & fvi->br.bytes_per_clust_mask))
      {
      /* NOTE(GRP2331): Move last sector */
      ofe->cur_sect = sect_no - 1;
      if (e_nospc != get_errno())
        fsm_assert1(0 == left_bytes_in_lastsect);
      fsm_assert1(0 == ofe->cur_offs_sect);
      }
    else
      ofe->cur_sect = sect_no;
    }

  return written_bytes;
  }

/*
 Name: __fat_do_write
 Desc: Write file's data to the physical device.
 Params:
   - ofe: Pointer to the Open-File Entry.
   - buf: Pointer to a buffer containing the data to be written.
   - bytes: The size in bytes of the data to be written.
 Returns:
   size_t  >=0 on success. The returned value is the number of written bytes.
           < 0 on fail.
 Caveats: None.
 */

static size_t __fat_do_write(fat_ofileent_t *ofe, uint8_t *buf, uint32_t bytes)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(ofe->fe->vol_id);
  int32_t left_cnt,
    written_cnt;
  uint8_t *p_written_pos = buf;


  left_cnt = bytes;
  written_cnt = 0;

  if (ofe->cur_offs & fvi->br.bytes_per_clust_mask)
    {
    /* Write bytes to cur_offs that not aligned at cluster-unit.
       The cluster already exists. */
    written_cnt = __file_write_exi_first_clust(ofe, p_written_pos, left_cnt);
    if (0 > written_cnt) return -1;
    p_written_pos += written_cnt;
    left_cnt -= written_cnt;
    }

  if (left_cnt)
    {
    if (ofe->cur_sect && 0 == (ofe->cur_offs & fvi->br.bytes_per_clust_mask)
        && 0 == GET_SECT_IDX(fvi, ofe->cur_sect)
        && (int32_t) ofe->fe->dir.filesize > ofe->cur_offs)
      {
      /* Write bytes to cur_offs that aligned at cluster-unit.
         The cluster already exists. */
      written_cnt = __file_write_exi_align_clust(ofe, p_written_pos, left_cnt);
      if (0 > written_cnt) return -1;
      p_written_pos += written_cnt;
      left_cnt -= written_cnt;
      }
    }

  if ((offs_t) ofe->fe->dir.filesize < ofe->cur_offs)
    {
    /* The remained bytes in the last cluster was used. */
    ofe->fe->dir.filesize = ofe->cur_offs;
    /* Flush the LIM data cache to the physical device. */
    if (0 > lim_flush_cdsector())
      return -1;
    }

  /* Some bytes are written by the above operation. */
  if (left_cnt != bytes && 0 == left_cnt)
    {
    fat_set_ent_time(&ofe->fe->dir, TM_WRITE_ENT | TM_ACCESS_ENT);
#if defined( LOG )
    /* Update data to the physical device. */
    fat_update_sentry(ofe->fe, true);
#else
    fat_update_sentry(ofe->fe, false);
    ofe->state |= eFILE_DIRTY;
#endif
    }

  if (left_cnt)
    {
    /* Write bytes to cur_offs that aligned at cluster-unit.
       The cluster should be allocated newly. */
    written_cnt = __file_write_new_clust(ofe, p_written_pos, left_cnt);
    if (0 > written_cnt) return -1;
    p_written_pos += written_cnt;
    }

  return (size_t) (p_written_pos - buf);
  }




#if defined( WB )

/*
 Name: __fat_alloc_wb
 Desc: Allocate write buffer
 Params:
   - ofe: Pointer to the Open-File Entry to allocate Write-Buffer.
 Returns:
   fat_wb_entry_t*  value on success. The returned value is a pointer to the
                          Write-Buffer Entry.
                    NULL on fail.
 Caveats: The number of Write-Buffer must be aligned as power of 2.
 */

static fat_wb_entry_t* __fat_alloc_wb(fat_ofileent_t *ofe)
  {
  list_head_t *list;
  fat_wb_entry_t *wb;


  fsm_assert2(NULL == ofe->fe->wb);

  list = &wb_free_lru;
  if (list->next == list)
    /* Too many allocated. */
    return NULL;

  /* Allocate free WB entry. */
  wb = list_entry(list->next, fat_wb_entry_t, head);

  /* Move to alloc-list. */
  list = &wb_alloc_lru;
  list_move_tail(list, &wb->head);

  wb->ofe = ofe;
  fsm_assert2(NULL == ofe->fe->wb);
  ofe->fe->wb = wb;

  return wb;
  }

/*
 Name: __fat_free_wb
 Desc: Free allocated Write-Buffer.
 Params:
   - fe: Pointer to File-Entry to free Write-Buffer.
 Returns:
   int32_t  =0(=s_ok) always.
 Caveats: None.
 */

static int32_t __fat_free_wb(fat_fileent_t *fe)
  {
  list_head_t *list;
  fat_wb_entry_t *wb = fe->wb;


  fsm_assert3(NULL != wb);

  fsm_assert2((uint8_t *) wb >= (uint8_t *) & fat_wb[0]);
  fsm_assert2((uint8_t *) wb < (uint8_t *) & fat_wb[FILE_WB_CNT]);

  /* Move to free-list. */
  list = &wb_free_lru;
  list_move(list, &wb->head);

  wb->w_offs = 0;
  wb->wb_cur_offs = 0;
  wb->hold_free_clust = 0;
  wb->ofe = NULL;

  fe->wb = NULL;

  return s_ok;
  }

/*
 Name: __fat_lookup_wb
 Desc: Search a Write-Buffer about a specific File-Entry.
 Params:
   - fe: Pointer to the File-Entry.
 Returns:
   fat_ofileent_t*  value on success. The returned value is the pointer to the
                          Open-File Entry.
                    NULL on fail.
 Caveats: None.
 */

static fat_ofileent_t* __fat_lookup_wb(fat_fileent_t *fe)
  {
  list_head_t *list = &wb_alloc_lru;
  fat_wb_entry_t *wb;

  list_for_each_entry_rev(fat_wb_entry_t, wb, list, head)
    {
    fsm_assert3(NULL != wb->ofe);
    if (wb->ofe->fe == fe)
      return wb->ofe;
    }

  return NULL;
  }

/*
 Name: __fat_get_out_size_wb
 Desc: Gets data size of outside of a file.
 Params:
   - fe: Pointer to the File-Entry.
 Returns:
   int32_t  outside data size.
 Caveats: None.
 */

static int32_t __fat_get_out_size_wb(fat_fileent_t *fe)
  {
  int32_t outside_size;


  fsm_assert2((NULL != fe->wb) && fe->wb->w_offs);

  outside_size = (fe->wb->wb_cur_offs + fe->wb->w_offs) - fe->dir.filesize;

  if (0 < outside_size)
    return outside_size;
  else
    return 0;
  }

/*
 Name: __fat_flush_wb
 Desc: Flush Write-Buffer of Open-File Entry.
 Params:
   - ofe: Pointer to the Open-File Entry.
 Returns:
   size_t  >=0 on success. The value returned is the number of bytes actually
              flushed.
           < 0 on fail.
 Caveats: None.
 */

static size_t __fat_flush_wb(fat_ofileent_t *ofe)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(ofe->fe->vol_id);
  fat_wb_entry_t *wb = ofe->fe->wb;
  size_t written;
  offs_t adjust_offs;
  int32_t w_offs, rtn;


  fsm_assert3(NULL != wb);

  w_offs = wb->w_offs;

  if (w_offs)
    {
    if (wb->wb_cur_offs != ofe->cur_offs)
      {
      adjust_offs = wb->wb_cur_offs;
      wb->wb_cur_offs = -1;

      rtn = __fat_adjust_pos(ofe, adjust_offs);
      if (0 > rtn)
        {
        return (size_t) rtn;
        }
      }

    fat_release_clust_wb(fvi, ofe->fe);

    ofile_unlock();
    written = __fat_do_write(ofe, wb->buf, w_offs);
    ofile_lock();

    if (written != w_offs)
      {
      fat_rehold_clust_wb(fvi, ofe->fe);
      if (0 > written)
        return written;
      else
        return set_errno(e_wbf);
      }

    ofe->fe->wb->w_offs = 0;
    return written;
    }

  return 0;
  }

/*
 Name: fat_sync_fs_wb
 Desc: Flush WBs of All Open-File Entry
 Params:
   - vol_id: Volume's ID to be allocated cluster.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats:
 */

int32_t fat_sync_fs_wb(int32_t vol_id)
  {
  list_head_t *list;
  fat_ofileent_t *ofe;


  /* open-file entry list */
  list = &ofile_table_lru;

  ofile_lock();

  /* Search all allocated Open-File Entry*/
  list_for_each_entry_rev(fat_ofileent_t, ofe, list, head)
    {
    if (eFILE_FREE & ofe->state) break;

    if (ofe->fe->wb && vol_id == ofe->fe->vol_id)
      {
      if (0 > __fat_flush_wb(ofe))
        {
        ofile_unlock();
        return -1;
        }
      }
    }

  ofile_unlock();
  return s_ok;
  }

/*
 Name: __fat_hold_clust_wb
 Desc: Obtain free clusters that should be obtain to write after.
 Params:
   - fvi: Pointer to fat_volinfo_t structure has information of volume.
   - ofe: Open-File Entry pointer.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats:
 */

static int32_t __fat_hold_clust_wb(fat_volinfo_t *fvi, fat_ofileent_t *ofe)
  {
  fat_fileent_t *fe = ofe->fe;
  int32_t file_clust,
    wb_clust,
    gap_clust;


  if ((int32_t) fe->dir.filesize < wb_written_pos(fe))
    {
    file_clust = fe->dir.filesize / fvi->br.bits_per_clust;
    wb_clust = wb_written_pos(fe) / fvi->br.bits_per_clust;
    gap_clust = wb_clust - file_clust;
    if (gap_clust)
      {
      if (fe->wb->hold_free_clust < gap_clust)
        {
        if ((int32_t) fvi->br.free_clust_cnt < (gap_clust - fe->wb->hold_free_clust))
          return set_errno(e_nospc);
        fvi->br.free_clust_cnt -= (gap_clust - fe->wb->hold_free_clust);
        fe->wb->hold_free_clust = gap_clust;
        }
      }
    }

  return s_ok;
  }

/*
 Name: __fat_write_through_wb
 Desc: Write file's data to the physical device or write-buffer(WB).
 Params:
   - ofe: Open-File Entry pointer.
   - buf: Pointer to a buffer containing the data to be written.
   - bytes: The size in bytes of the data to be written.
 Returns:
   size_t  >=0 on success. The returned value is the number of bytes that is in disk or WB.
           < 0 on fail.
 Caveats:
 */

static size_t __fat_write_through_wb(fat_ofileent_t *ofe, uint8_t *buf, size_t bytes)
  {
  const uint32_t file_wb_size = FILE_WB_SIZE;
  const uint32_t file_wb_size_mask = (FILE_WB_SIZE - 1);
  fat_volinfo_t *fvi = GET_FAT_VOL(ofe->fe->vol_id);
  fat_fileent_t *fe = ofe->fe;
  fat_wb_entry_t *wb = fe->wb;
  uint8_t *pbuf = buf;
  size_t written = 0;
  size_t head_size = 0, /* It is written at WB */
    body_size, /* It is written at disk(It is not written at WB). */
    tail_size = 0, /* It is written at WB */
    left_wb_cnt = 0,
    no_align,
    write_size;
  int32_t rtn;


  if (wb && wb->w_offs)
    {
    /* We have used WB already. */
    fsm_assert2((uint32_t) wb->w_offs < file_wb_size);

    /* Calc left size in WB. */
    left_wb_cnt = file_wb_size - wb->w_offs;
    if (left_wb_cnt > bytes)
      {
      head_size = bytes;
      }
    else
      {
      head_size = left_wb_cnt;
      tail_size = (bytes - head_size) & file_wb_size_mask;
      }
    }
  else if (!(ofe->cur_offs & file_wb_size_mask) && (file_wb_size > bytes))
    {
    if (NULL == wb)
      wb = __fat_alloc_wb(ofe);

    if (wb)
      {
      left_wb_cnt = file_wb_size;
      /* For using WB, the ofe->cur_offs must be aligned with size of WB. */
      head_size = bytes;
      /* If the 'bytes' is less than 'file_wb_size', 'tail_size' is 0. */
      /* tail_size = 0; */
      wb->wb_cur_offs = ofe->cur_offs;
      wb->hold_free_clust = 0;
      }
    }
  else
    {
    /*
       'ofe->cur_offs' is not aligned with 'file_wb_size' or the 'bytes' is not less than size of WB,
       'head_size' is 0.
       Bcs, WB must be aligned with size of WB always.
     */
    /* head_size = 0; */

    no_align = file_wb_size - (ofe->cur_offs & file_wb_size_mask);
    if (no_align < bytes)
      {
      if (NULL == wb)
        wb = __fat_alloc_wb(ofe);
      tail_size = (ofe->cur_offs + bytes) & file_wb_size_mask;
      }
    }

  if ((NULL == wb) && (head_size || tail_size))
    /* If there is a size of head or tail, WB must be allocated already. */
    return __fat_do_write(ofe, buf, bytes);

  body_size = bytes - head_size - tail_size;

  if (head_size)
    {
    if (left_wb_cnt > head_size)
      {
      write_size = head_size;
      memcpy(&wb->buf[wb->w_offs], pbuf, write_size);
      wb->w_offs += write_size;
      if (0 > __fat_hold_clust_wb(fvi, ofe))
        {
        wb->w_offs -= write_size;
        write_size -= head_size;
        }
      return write_size;
      }
    else
      {
      fsm_assert1(NULL != wb->buf);
      write_size = left_wb_cnt;
      memcpy(&wb->buf[wb->w_offs], pbuf, write_size);

      fat_release_clust_wb(fvi, ofe->fe);

      /* Flush WB. */
      written = __fat_do_write(ofe, wb->buf, file_wb_size);
      if (file_wb_size != written)
        {
        if (e_nospc == get_errno())
          {
          /* WB will be flushed in close(). So the clusters that WB was held must hold again.*/
          fat_rehold_clust_wb(fvi, ofe->fe);
          return written;
          }
        else
          return -1;
        }

      fe->wb->w_offs = 0;

      /* At this time additional size is write_size. */
      written = write_size;
      pbuf += write_size;

      fsm_assert2(head_size == write_size);
      }
    }

  if (body_size)
    {
    rtn = __fat_do_write(ofe, pbuf, body_size);
    if (0 > rtn)
      return -1;
    pbuf += rtn;
    written += rtn;
    }

  if (tail_size)
    {
    fsm_assert1(0 == wb->w_offs);
    memcpy(wb->buf, pbuf, tail_size);
    written += tail_size;
    wb->w_offs = tail_size;
    wb->wb_cur_offs = ofe->cur_offs;
    wb->hold_free_clust = 0;
    if (0 > __fat_hold_clust_wb(fvi, ofe))
      {
      wb->w_offs = 0;
      written -= tail_size;
      }
    }

#if ( FSM_ASSERT >= 2 )
  if (wb)
    fsm_assert2((uint32_t) wb->w_offs < file_wb_size);
#endif
  return written;
  }
#endif

/*
 Name: fat_write
 Desc: Write data to a file specified by the 'fd' parameter.
 Params:
   - fd: The descriptor of the file to which the data is to be written.
   - buf: The pointer to a buffer containing the data to be written.
   - bytes: The size in bytes of the data to be written.
 Returns:
   size_t  >=0 on success. The value returned is the number of bytes actually
              written.
           < 0 on fail.
 Caveats: None.
 */

size_t fat_write(int32_t fd, const void *buf, size_t bytes)
  {
  fat_ofileent_t *ofe;
  uint8_t *pbuf = (uint8_t *) buf;
  size_t written;
  int32_t rtn;


  fat_lock();

  set_errno(s_ok);

  /* Allocate the buffer for the opened File-Entry indicated by the file
     descriptor. */
  ofe = __fat_get_ofile_entry(fd);
  if ((fat_ofileent_t *) NULL == ofe)
    {
    fat_unlock();
    return -1;
    }

  /* Check the access permission. */
  if (!(ofe->oflag & (O_WRONLY | O_RDWR | O_APPEND)))
    {
    fat_unlock();
    return set_errno(e_access);
    }

  if (ofe->oflag & O_APPEND)
    {
    /* Set the file pointer to the end of the file. */
    if (0 > __fat_lseek(ofe, 0, SEEK_END))
      {
      fat_unlock();
      return -1;
      }
    }

#if defined( WB )
  if (ofe->fe->wb && ofe->fe->wb->w_offs)
    {
    if (wb_written_pos(ofe->fe) != ofe->seek_offs)
      {
      ofile_lock();
      rtn = __fat_flush_wb(ofe);
      ofile_unlock();
      if (0 > rtn)
        {
        fat_unlock();
        return -1;
        }
      }
    }
#endif

  /* Adjust the seek file offset and the current file offset. */
  rtn = __fat_adjust_pos(ofe, ofe->seek_offs);
  if (0 > rtn)
    {
    if ((ofe->seek_offs >= (int32_t) ofe->fe->dir.filesize) && (e_nospc == get_errno()))
      rtn = 0;
    fat_unlock();
    return (size_t) rtn;
    }

  fsm_assert1((offs_t) ofe->fe->dir.filesize >= ofe->cur_offs);

#if defined( WB )
  if (1 == ofe->fe->ref_cnt)
    {
    written = __fat_write_through_wb(ofe, pbuf, bytes);
    if (0 < written)
      ofe->seek_offs += written;
    }
  else
#endif
    {
    written = __fat_do_write(ofe, pbuf, bytes);
    if (0 < written)
      ofe->seek_offs += written;
    fsm_assert2(ofe->seek_offs == ofe->cur_offs);
    }

  tr_fat_inc_write_cnt(ofe->fe->vol_id);

  if (1 < ofe->fe->ref_cnt)
    /* Invalidate physical file position of opend files. */
    __fat_invalidate_ofile_pos(ofe->fe);

  fat_unlock();
  return written;
  }

/*
 Name: fat_lseek
 Desc: Change the position of file pointer in the file which the 'fd' parameter
       point to.
 Params:
   - fd: The file descriptor whose current file offset you wanted to change.
   - offset: The amount byte offset is to be changed. The sign indicates whether
            the offset is to be moved forward (positive) or backward (negative).
   - whence: The value to determine how the offset is to be interpreted.
            It has one of the following symbols.
            SEEK_SET: Sets the file pointer to the value of the offset parameter.
            SEEK_CUR: Sets the file pointer to its current location plus the
                      value of the offset parameter.
            SEEK_END: Sets the file pointer to the size of the file plus the
                      value of the Offset parameter.
 Returns:
   offs_t  >=0 on success. The value returned is the new file offset, measured
               in bytes from the beginning of the file.
           < 0 on fail.
 Caveats: None.
 */

offs_t fat_lseek(int32_t fd, offs_t offset, int32_t whence)
  {
  fat_ofileent_t *ofe;
  offs_t rtn;


  if ((SEEK_SET != whence) && (SEEK_CUR != whence) && (SEEK_END != whence))
    return set_errno(e_inval);

  fat_lock();

  /* Allocate the buffer for the opened File-Entry indicated by the file
     descriptor. */
  ofe = __fat_get_ofile_entry(fd);
  if ((fat_ofileent_t *) NULL == ofe)
    {
    fat_unlock();
    return -1;
    }

  rtn = __fat_lseek(ofe, offset, whence);

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_fsync
 Desc: Synchronize the File-Entry indicated by a file descriptor.
 Params:
   - fd: The file descriptor to be synchronized.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_fsync(int32_t fd)
  {
  fat_ofileent_t *ofe;
  int32_t rtn = s_ok;


  fat_lock();

  /* Allocate the buffer for the opened File-Entry indicated by the file
     descriptor. */
  ofe = __fat_get_ofile_entry(fd);
  if ((fat_ofileent_t *) NULL == ofe)
    {
    fat_unlock();
    return -1;
    }

#if defined( WB )
  ofile_lock();
  if (ofe->fe->wb && ofe->fe->wb->w_offs)
    rtn = __fat_flush_wb(ofe);
  ofile_unlock();
#endif

#if !defined( LOG )
  /* Flush the cache data to the physical device. */
  rtn |= fat_sync_table(ofe->fe->vol_id, true);

  if (eFILE_DIRTY & ofe->state)
    {
    rtn |= fat_update_sentry(ofe->fe, true);
    ofe->state &= ~eFILE_DIRTY;
    }
#endif

  if (0 < rtn) rtn = s_ok;

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_close
 Desc: Close a file specified by the 'fd' parameter.
 Params:
   - fd: The file descriptor of opened file to be closed.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_close(int32_t fd)
  {
  fat_ofileent_t *ofe;
  int32_t rtn = s_ok;


  fat_lock();

  /* Allocate the buffer for the opened File-Entry indicated by the file
     descriptor. */
  ofe = __fat_get_ofile_entry(fd);
  if ((fat_ofileent_t *) NULL == ofe)
    {
    fat_unlock();
    return -1;
    }

#if defined( WB )
  if (ofe->fe->wb)
    {
    ofile_lock();
    if (ofe->fe->wb->w_offs)
      rtn = __fat_flush_wb(ofe);
    __fat_free_wb(ofe->fe);
    ofile_unlock();
    if (0 > rtn)
      return -1;
    }
#endif

  /* Flush the LIM data cache to the physical device. */
  if (0 > lim_flush_cdsector())
    {
    fat_unlock();
    return -1;
    }

#if !defined( LOG )
  /* Flush the cache data to the physical device. */
  if (ofe->oflag != O_RDONLY)
    rtn = fat_sync_table(ofe->fe->vol_id, true);

  if (eFILE_DIRTY & ofe->state)
    {
    rtn |= fat_update_sentry(ofe->fe, true);
    ofe->state &= ~eFILE_DIRTY;
    }

  if (0 > rtn)
    {
    __fat_free_ofile_entry(ofe);
    fat_unlock();
    return -1;
    }
#endif

  ofile_lock();
  /* Free the File Entry which is register in the Open-File Entry.
     & Convert the free status. */
  rtn = __fat_free_ofile_entry(ofe);
  ofile_unlock();

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_closeall
 Desc: Close all files within a volume specified by the 'vol_id' parameter.
 Params:
   - vol_id: The ID of volume that all files will be closed.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_closeall(int32_t vol_id)
  {
  list_head_t *list;
  fat_ofileent_t *ofe, *n;
  int32_t rtn = s_ok;


  fat_lock();
  ofile_lock();

  /* open-file entry list */
  list = &ofile_table_lru;

  /* Flush the LIM data cache to the physical device. */
  if (0 > lim_flush_cdsector())
    {
    ofile_unlock();
    fat_unlock();
    return -1;
    }

#if !defined( LOG )
  /* Flush the cache data to the physical device. */
  rtn = fat_sync_table(vol_id, true);
#endif

  /* Search all allocated Open-File Entry*/
  list_for_each_entry_safe_rev(fat_ofileent_t, ofe, n, list, head)
    {
    if (eFILE_FREE & ofe->state) break;

    if (vol_id == ofe->fe->vol_id)
      {
#if defined( WB )
      if (ofe->fe->wb)
        {
        if (ofe->fe->wb->w_offs)
          rtn = __fat_flush_wb(ofe);
        __fat_free_wb(ofe->fe);
        if (0 > rtn)
          break;
        rtn = 0;
        }
#endif
#if !defined( LOG )
      if (eFILE_DIRTY & ofe->state)
        {
        rtn |= fat_update_sentry(ofe->fe, true);
        ofe->state &= ~eFILE_DIRTY;
        }
#endif
      rtn |= __fat_free_ofile_entry(ofe);

      if (0 > rtn)
        break;
      }
    }

  ofile_unlock();
  fat_unlock();
  return rtn;
  }

/*
 Name: fat_unlink
 Desc: Remove the file specified by the 'path' parameter.
 Params:
   - vol_id: The ID of volume including the file to be removed.
   - path: Pointer to the null-terminated path name of the file to be removed.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: Before the file is removed, it should be closed.
 */

int32_t fat_unlink(int32_t vol_id, const char *path)
  {
  fat_fileent_t *fe;
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  int32_t rtn;


  /* Parse the path name. */
  rtn = fat_parse_path(path_buf, &arg, path);
  if (s_ok != rtn) return rtn;

  fat_lock();

  /* Allocate the buffer for File-Entry. */
  if (NULL == (fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return -1;
    }

  /* Search the parsed name in a volume and fill the information into the
     File-Entry structure. If the file exists, fat_lookup_entry() funtion
     returns the value equal to the 'arg.argc'. */
  rtn = fat_lookup_entry(&arg, fe, eFAT_FILE);
  if (0 > rtn) goto End;

  if (arg.argc != rtn)
    {
    if (arg.argc - 1 != rtn)
      /* Path not exist */
      set_errno(e_notdir);
    rtn = -1;
    goto End;
    }

  rtn = fat_unlink_entry(fe);

  tr_fat_inc_unlink_cnt(vol_id);

End:
  /* Free the buffer for File-Entry. */
  fat_free_file_entry(fe);

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_truncate
 Desc: Truncate the file size from indicated file by the 'fd' parameter to new
       size.
 Params:
   - fd: The descriptor of the file to which the length is to be changed.
   - new_size: The new length of the file in bytes.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_truncate(int32_t fd, size_t new_size)
  {
  fat_volinfo_t *fvi;
  fat_ofileent_t *ofe;
  uint32_t clust_no;
  int32_t rtn;


  fat_lock();

  /* Allocate the buffer for the opened File-Entry indicated by the file
     descriptor. */
  ofe = __fat_get_ofile_entry(fd);
  if ((fat_ofileent_t *) NULL == ofe)
    {
    fat_unlock();
    return -1;
    }

  /* Check the access permission. */
  if (!(ofe->oflag & (O_WRONLY | O_RDWR | O_TRUNC)))
    {
    fat_unlock();
    return set_errno(e_access);
    }

#if defined( WB )
  if (ofe->fe->wb)
    {
    ofile_lock();
    rtn = __fat_flush_wb(ofe);
    ofile_unlock();
    if (0 > rtn)
      {
      fat_unlock();
      return -1;
      }
    }
#endif

  rtn = fat_do_truncate(ofe->fe, new_size);

  /*
     If file position is not moved, adjust file position.
     Bcs, ofe->cur_sect must be allocated with the first sector of new cluster.
   */
  if ((0 == ofe->cur_sect) && (0 == ofe->cur_offs_sect)
      && (0 == ofe->seek_offs) && (0 == ofe->cur_offs))
    {
    /* Setup to first offset */
    fvi = GET_FAT_VOL(ofe->fe->vol_id);
    clust_no = GET_OWN_CLUST(&ofe->fe->dir);
    ofe->cur_sect = clust_no ? D_CLUST_2_SECT(fvi, clust_no) : 0;
    }

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_tell
 Desc: Return the position of file pointer in the file which the 'fd' parameter
       point to.
 Params:
   - fd: The file descriptor whose current file offset you wanted to obtain.
 Returns:
   int32_t  >=0 on success. The value returned is the current file offset,
              measured in bytes from the beginning of the file.
            < 0 on fail.
 Caveats: None.
 */

int32_t fat_tell(int32_t fd)
  {
  fat_ofileent_t *ofe;
  offs_t offs;


  fat_lock();

  /* Allocate the buffer for the opened File-Entry indicated by the file
     descriptor. */
  ofe = __fat_get_ofile_entry(fd);
  if ((fat_ofileent_t *) NULL == ofe)
    {
    fat_unlock();
    return -1;
    }

  offs = ofe->seek_offs;

  fat_unlock();
  return offs;
  }

/*
 Name: fat_rename
 Desc: Rename a file or moves a file across directories of the same volume.
 Params:
   - vol_id: The ID of volume including the file to be renamed.
   - oldpath: The pointer to null-terminated path name of the file or directory
              to be renamed.
   - newpath: The pointer to null-terminated new path name of the file or
              directory.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: Opened files cannot be renamed.
 */

int32_t fat_rename(int32_t vol_id, const char *oldpath, const char *newpath)
  {
  fat_fileent_t *old_fe, *new_fe;
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  int32_t rtn;
#if defined( LOG )
  int32_t cl_id = -1;
#endif


  /* Parse the path name of the old file. */
  rtn = fat_parse_path(path_buf, &arg, oldpath);
  if (!arg.argc)
    {
    set_errno(e_path);
    return -1;
    }
  if (s_ok != rtn) return rtn;

  fat_lock();

  /* Allocate the buffer for File-Entry. It's used for the old file. */
  if (NULL == (old_fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return -1;
    }

  /* Allocate the buffer to the  File-Entry for the new file and then, free
     the buffer to the File-Entry for the old file. */
  if (NULL == (new_fe = fat_alloc_file_entry(vol_id)))
    {
    fat_free_file_entry(old_fe);
    fat_unlock();
    return -1;
    }

  /* Search the parsed name about the old file in a volume and fill the
     information into the File-Entry structure. If the file exists,
     fat_lookup_entry() funtioreturns the value equal to the 'arg.argc'. */
  rtn = fat_lookup_entry(&arg, old_fe, eFAT_ALL);
  if (0 > rtn) goto End;

  if (arg.argc != rtn)
    {
    rtn = set_errno(e_noent);
    goto End;
    }

  /* Get the opened File-Entry about the same name. */
  if (NULL != fat_get_opend_file_entry(old_fe))
    {
    rtn = set_errno(e_busy);
    goto End;
    }

  /* Parse the path name of new file. */
  rtn = fat_parse_path(path_buf, &arg, newpath);
  if (s_ok != rtn) goto End;

  /* Search the parsed name about the new file in a volume and fill the
     information into the File-Entry structure. If the file exists,
     fat_lookup_entry() funtion returns the value equal to the 'arg.argc'.*/
  rtn = fat_lookup_entry(&arg, new_fe, eFAT_ALL);
  if (0 > rtn) goto End;

  if (arg.argc == rtn)
    {
    /* Check the case of file path names. */
    if (!strcmp(oldpath, newpath))
      {
      if ((old_fe->parent_sect == new_fe->parent_sect) &&
          (old_fe->parent_ent_idx == new_fe->parent_ent_idx))
        {
        /* The same path. */
        rtn = s_ok;
        goto End;
        }
      }
    rtn = set_errno(e_exist);
    goto End;
    }
  else if (arg.argc - 1 != rtn)
    {
    /* Path not exist */
    rtn = set_errno(e_notdir);
    goto End;
    }

  /* Allocate the physical space for the File-Entry. */
  rtn = fat_alloc_entry_pos(new_fe);
  if (0 > rtn) goto End;

  /* Copy the previous information into the new Directory Entry. */
  fat_recreat_short_info(&new_fe->dir, &old_fe->dir);

#if defined( LOG )
  rtn = fat_log_on(old_fe, new_fe);
  if (0 > rtn) goto End;
  cl_id = rtn;
#endif

  /* Create the new File-Entry from the old entry information. */
  rtn = fat_creat_entry(new_fe, false);
  if (0 > rtn) goto End;

  /* Change the dotdot-directory's cluster */
  if (new_fe->dir.attr & eFAT_ATTR_DIR)
    {
    rtn = fat_change_dotdots_dir(new_fe);
    if (0 > rtn) goto End;
    }

  /* Unlink the old entry's own entry in parent cluster. */
  if ((uint32_t) eFILE_LONGENTRY & old_fe->flag)
    rtn = fat_unlink_entry_long(old_fe);
  else
    rtn = fat_unlink_entry_short(old_fe);
  if (0 > rtn) goto End;

#if defined( LOG )
  /* Flush the cache data to the physical device. */
  rtn = fat_sync_table(vol_id, true);
  if (0 > rtn) goto End;
#endif

#if defined( CPATH )
  if (eFAT_ATTR_DIR & old_fe->dir.attr)
    {
    /* Remove the old File-Entry in the cache. */
    path_del_centry(old_fe);
    /* Add the new File-Entry in the cache. */
    path_store_centry(new_fe);
    }
#endif

End:
#if defined( LOG )
  if (0 <= cl_id)
    fat_log_off(vol_id, cl_id);
#endif

  /* Free the buffer to the File-Entry. */
  fat_free_file_entry(old_fe);
  fat_free_file_entry(new_fe);

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_stat
 Desc: Get information of file or directory specified by 'path' parameter
 Params:
   - vol_id: The ID of volume including the file to get the information.
   - path: The pointer to the null-terminated path name of the file or
           directory to obtain information.
   - statbuf: The pointer to a buffer of type structure stat_t where file
              status information is returned.
 Returns:
   int32_t  =0 on success.
            >0 on fail.
 Caveats: None.
 */

result_t fat_stat(int32_t vol_id, const char *path, stat_t *statbuf)
  {
  lim_volinfo_t *lvi;
  fat_fileent_t *fe;
#if defined( WB )
  fat_fileent_t *fe2;
#endif
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  int32_t rtn;


  /* Parse the path name. */
  rtn = fat_parse_path(path_buf, &arg, path);
  if (s_ok != rtn) return rtn;

  fat_lock();

  /* Allocate the buffer for File-Entry. */
  if (NULL == (fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return -1;
    }

  if (arg.argc)
    { /*isn't the Root-Directory. */
    /* Search the parsed name in a volume and fill the information into the
       File-Entry structure. If the file exists, fat_lookup_entry() funtion
       returns the value equal to the 'arg.argc'. */
    rtn = fat_lookup_entry(&arg, fe, eFAT_ALL);
    if (0 > rtn) goto End;

    if (arg.argc != rtn)
      {
      if (arg.argc - 1 != rtn)
        /* Path not exist */
        set_errno(e_notdir);
      rtn = -1;
      goto End;
      }
    }
  else
    { /* Root-Directory */
    fe->dir.attr = eFAT_ATTR_DIR;
    fe->dir.filesize = 0;
    fe->dir.ctime = 0;
    fe->dir.cdate = 0;
    fe->dir.adate = 0;
    fe->dir.wdate = 0;
    fe->dir.wtime = 0;
#ifdef SFILEMODE
    fe->dir.char_case = (fe->dir.char_case & 0x18) | TO_NTRES(0x04); //S_IFDIR;
#endif
    }

  rtn = s_ok;

  lvi = GET_LIM_VOL(vol_id);

  /* Fill the information obtained from File-Entry into the buffer. */
  statbuf->dev = statbuf->rdev = (uint16_t) lvi->dev_id;
  statbuf->i_no = 0;
  statbuf->mode = fe->dir.attr;
  statbuf->nlink = 0;
#ifdef SFILEMODE
  statbuf->filemode = FROM_NTRES(fe->dir.char_case);
  if (statbuf->filemode == 0)
    {
    statbuf->filemode = fe->dir.attr; //S_IFDIR;
    }
#endif
  statbuf->uid = 0;
  statbuf->gid = 0;
  statbuf->rdev = 0;
  statbuf->size = fe->dir.filesize;
#if defined( WB )
  fe2 = fat_get_opend_file_entry(fe);
  if (fe2 && fe2->wb && fe2->wb->w_offs)
    /* If file has opened, calculate pointer of WB. */
    statbuf->size += __fat_get_out_size_wb(fe2);
#endif
  fat_ftime_2_gtime(&statbuf->ctime, &statbuf->atime, &statbuf->mtime, &fe->dir);

End:
  /* Free the buffer for File-Entry. */
  fat_free_file_entry(fe);

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_fstat
 Desc: Get information of file specified by file descriptor 'fd' parameter
 Params:
   - fd: The file descriptor of the file to obtain information.
   - statbuf: The pointer to a buffer of type structure stat_t where file
              status information is returned.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

result_t fat_fstat(int32_t fd, stat_t *statbuf)
  {
  lim_volinfo_t *lvi;
  fat_ofileent_t *ofe;
  fat_fileent_t *fe;


  fat_lock();

  /* Allocate the buffer for the opened File-Entry indicated by the file
     descriptor. */
  ofe = __fat_get_ofile_entry(fd);
  if ((fat_ofileent_t *) NULL == ofe)
    {
    fat_unlock();
    return -1;
    }

  fe = ofe->fe;
  lvi = GET_LIM_VOL(fe->vol_id);

  /* Fill the information obtained from File-Entry into the buffer. */
  statbuf->dev = statbuf->rdev = (uint16_t) lvi->dev_id;
  statbuf->i_no = 0;
  statbuf->mode = fe->dir.attr;
  statbuf->nlink = 0;
#ifdef SFILEMODE
  statbuf->filemode = FROM_NTRES(fe->dir.char_case);
  if (statbuf->filemode == 0)
    {
    statbuf->filemode = fe->dir.attr; //S_IFDIR;
    }
#endif
  statbuf->uid = 0;
  statbuf->gid = 0;
  statbuf->rdev = 0;
  statbuf->size = fe->dir.filesize;
#if defined( WB )
  if (fe->wb && fe->wb->w_offs)
    statbuf->size += __fat_get_out_size_wb(fe);
#endif
  fat_ftime_2_gtime(&statbuf->ctime, &statbuf->atime, &statbuf->mtime, &fe->dir);

  fat_unlock();
  return s_ok;
  }

/*
 Name: fat_getattr
 Desc: Get the attribute information of file or directory specified by path
       'path' parameter.
 Params:
   - vol_id: The ID of volume including the file to get the attribute.
   - path: The pointer to the null-terminated path name of the file or
           directory to obtain an attribute.
   - attrbuf: The pointer to a buffer where an attribute is returned.
 Returns:
   int32_t  >=0 on success.
            < 0 on fail.
 Caveats: None.
 */

result_t fat_getattr(int32_t vol_id, const char *path, uint32_t *attrbuf)
  {
  fat_fileent_t *fe;
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  int32_t rtn;


  /* Parse the path name. */
  rtn = fat_parse_path(path_buf, &arg, path);
  if (s_ok != rtn) return rtn;

  fat_lock();

  /* Allocate the buffer for File-Entry. */
  if (NULL == (fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return -1;
    }

  /* Search the parsed name in a volume and fill the information into the
     File-Entry structure. If the file exists, fat_lookup_entry() funtion
     returns the value equal to the 'arg.argc'. */
  rtn = fat_lookup_entry(&arg, fe, eFAT_ALL);
  if (0 > rtn) goto End;

  if (arg.argc != rtn)
    {
    if (arg.argc - 1 != rtn)
      /* Path not exist */
      set_errno(e_notdir);
    rtn = -1;
    goto End;
    }

  *attrbuf = fe->dir.attr;

End:
  /* Free the buffer for File-Entry. */
  fat_free_file_entry(fe);

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_fgetattr
 Desc: Get the attribute information of file specified by file descriptor
       'fd' parameter.
 Params:
   - fd: The file descriptor of the file to obtain an attribute.
   - attrbuf: The pointer to a buffer where an attribute is returned.
 Returns:
   int32_t  >=0 on success.
            < 0 on fail.
 Caveats: None.
 */

result_t fat_fgetattr(int32_t fd, uint32_t *attrbuf)
  {
  fat_ofileent_t *ofe;


  fat_lock();

  /* Allocate the buffer for the opened File-Entry indicated by the file
     descriptor. */
  ofe = __fat_get_ofile_entry(fd);
  if ((fat_ofileent_t *) NULL == ofe)
    {
    fat_unlock();
    return -1;
    }

  *attrbuf = ofe->fe->dir.attr;

  fat_unlock();
  return s_ok;
  }

/*
 Name: fat_setattr
 Desc: Set the attribute information of the file or directory specified by the
       path 'path' parameter.
 Params:
   - vol_id: The ID of volume including the file to set the attribute.
   - path: The pointer to the null-terminated path name of the file or
          directory to set an attribute.
   - set_attr: The attribute to be set. The attribute should be one of the
               following symbols.
               FA_RDONLY: A file for reading only.
               FA_HIDDEN: A hidden file.
               FA_SYSTEM: A system file.
               FA_ARCHIVE: A archive file."
 Returns:
   int32_t  >=0 on success.
            < 0 on fail.
 Caveats: None.
 */

result_t fat_setattr(int32_t vol_id, const char *path, uint32_t set_attr)
  {
  fat_fileent_t *fe, *o_fe;
  ionfs_local char path_buf[ALLPATH_LEN_MAX];
  fat_arg_t arg;
  uint8_t new_attr = 0, get_attr;
  int32_t rtn;


  /* Parse the path name. */
  rtn = fat_parse_path(path_buf, &arg, path);
  if (s_ok != rtn)
    return rtn;

  fat_lock();

  /* Allocate the buffer for File-Entry. */
  if (NULL == (fe = fat_alloc_file_entry(vol_id)))
    {
    fat_unlock();
    return -1;
    }

  /* Search the parsed name in a volume and fill the information into the
     File-Entry structure. If the file exists, fat_lookup_entry() funtion
     returns the value equal to the 'arg.argc'. */
  rtn = fat_lookup_entry(&arg, fe, eFAT_ALL);
  if (0 > rtn) goto End;

  if (arg.argc != rtn)
    {
    if (arg.argc - 1 != rtn)
      /* Path not exist */
      set_errno(e_notdir);
    rtn = -1;
    goto End;
    }

  if (FA_RDONLY & set_attr)
    new_attr |= eFAT_ATTR_RO;
  if (FA_HIDDEN & set_attr)
    new_attr |= eFAT_ATTR_HIDDEN;
  if (FA_SYSTEM & set_attr)
    new_attr |= eFAT_ATTR_SYS;
  if (FA_ARCHIVE & set_attr)
    new_attr |= eFAT_ATTR_ARCH;

  /* Keep the previous attribute. */
  get_attr = fe->dir.attr;
  get_attr &= ~(uint8_t) (eFAT_ATTR_RO | eFAT_ATTR_HIDDEN | eFAT_ATTR_SYS | eFAT_ATTR_ARCH);
  fe->dir.attr = get_attr | new_attr;

  /* Get the opened File-Entry about the same name. */
  o_fe = fat_get_opend_file_entry(fe);
  if (o_fe)
    o_fe->dir.attr = fe->dir.attr;

  /* Update data to the physical device. */
  rtn = fat_update_sentry(fe, true);

#if defined( CPATH )
  if (eFAT_ATTR_DIR & fe->dir.attr)
    /* Update data to the cache. */
    rtn |= path_update_centry(fe);
#endif

End:
  /* Free the buffer for File-Entry. */
  fat_free_file_entry(fe);

  fat_unlock();
  return rtn;
  }

/*
Name: fat_fsetattr
Desc: Set the attribute information of the file specified by file descriptor
      'fd' parameter.
Params:
   - fd: The file descriptor of the file to set an attribute.
   - set_attr: The attribute to be set. The attribute should be one of the
               following symbols.
               FA_RDONLY: A file for reading only.
               FA_HIDDEN: A hidden file.
               FA_SYSTEM: A system file.
               FA_ARCHIVE: A archive file.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_fsetattr(int32_t fd, uint32_t set_attr)
  {
  fat_ofileent_t *ofe;
  fat_fileent_t *fe;
  uint8_t new_attr = 0, get_attr;
  int32_t rtn;


  fat_lock();

  /* Allocate the buffer for the opened File-Entry indicated by the file
     descriptor. */
  ofe = __fat_get_ofile_entry(fd);
  if ((fat_ofileent_t *) NULL == ofe)
    {
    fat_unlock();
    return -1;
    }

  fe = ofe->fe;

  if (FA_RDONLY & set_attr)
    new_attr |= eFAT_ATTR_RO;
  if (FA_HIDDEN & set_attr)
    new_attr |= eFAT_ATTR_HIDDEN;
  if (FA_SYSTEM & set_attr)
    new_attr |= eFAT_ATTR_SYS;
  if (FA_ARCHIVE & set_attr)
    new_attr |= eFAT_ATTR_ARCH;

  get_attr = fe->dir.attr;
  get_attr &= ~(uint8_t) (eFAT_ATTR_RO | eFAT_ATTR_HIDDEN | eFAT_ATTR_SYS | eFAT_ATTR_ARCH);
  fe->dir.attr = get_attr | new_attr;

  /* Update data to the physical device. */
#if defined( LOG )
  rtn = fat_update_sentry(fe, true);
#else
  rtn = fat_update_sentry(fe, false);
  ofe->state |= eFILE_DIRTY;
#endif

#if defined( CPATH )
  if (eFAT_ATTR_DIR & fe->dir.attr)
    /* Update data to the cache. */
    rtn |= path_update_centry(fe);
#endif

  fat_unlock();
  return rtn;
  }

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

