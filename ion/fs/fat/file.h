/* FILE: ion_file.h */
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


#if !defined( FAT_FILE_H_13122005 )
#define FAT_FILE_H_13122005

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "../global/global.h"
#include "fat.h"




/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES
-----------------------------------------------------------------------------*/

/* The enumeration value for the status of a file. */
typedef enum fat_fileflag_e {
   eFILE_FREE = (1<<0),
   eFILE_ALLOC = (1<<1),
   eFILE_OPEN = (1<<2),
   eFILE_DIRTY = (1<<3),

   eFILE_LONGENTRY = (1<<30),

   eFILE_END

} fat_fileflag_t;




#if defined( WB )
typedef struct fat_wb_entry_s {
   list_head_t head;

   int32_t w_offs;             /* Write offset in WB (This means the size of data in WB.) */
   offs_t wb_cur_offs;         /* It takes current file position when writing at the first position of WB. */
   uint8_t buf[FILE_WB_SIZE];  /* Write buffer */

   int32_t hold_free_clust;    /* WB is holding free clusters as excess size of the end of file. */

   struct fat_ofileent_s *ofe; /* Pointer to the Open-File Entry */

} fat_wb_entry_t;
#endif




/* Structure for File Entry. */
typedef struct fat_fileent_s {
   list_head_t head,          /* file table list head */
             hash_head;       /* opened list only */

   uint16_t idx,
            ref_cnt;          /* reference count */

   int8_t vol_id;
   uint8_t parent_ent_idx,    /* entry offset in 'parent_sect'. */
           lfn_shortent_idx,  /* entry offset in 'lfn_short_sect'. */
           ent_cnt;           /* count of entries (in short-entry 1)*/

   uint32_t parent_clust,     /* first cluster number of parent */
            parent_sect,      /* sector number of own entry which is located in parent cluster */
            lfn_short_sect,   /* sector number of LFN's short-entry */
            flag;

   #if defined( WB )
   fat_wb_entry_t *wb;
   #endif

   fat_dirent_t dir;

   uint16_t name_len;         /* length of 'name' */
   char name[FAT_LONGNAME_SIZE+1/*null char*/+5/*align at lfn-entry*/];

} fat_fileent_t;   /* file entry */




/* Structure for Open-File Entry. */
typedef struct fat_ofileent_s {
   list_head_t head;       /* ofile table list head */

   uint16_t fd,            /* file descriptor */
            state;         /* entry's open state */
   uint32_t oflag,         /* open file mode and flag */
            cur_sect;      /* sector number to read or write at the current
                              position. It offers the fast searching speed to
                              the sector number to read or write data. */
   offs_t seek_offs,       /* file offset to be moved */
         cur_offs;         /* current file offset */
   uint16_t cur_offs_sect; /* byte offset in the 'cur_sect'. */

   fat_fileent_t *fe;

} fat_ofileent_t;          /* open file entry */




/*-----------------------------------------------------------------------------
 DECLARE FUNTION PROTO-TYPE
-----------------------------------------------------------------------------*/

void fat_zinit_file( void );

/* file entry functions */
int32_t fat_init_file_entry( void );
int32_t fat_reinit_vol_file_entry( int32_t vol_id );
fat_fileent_t *fat_alloc_file_entry( int32_t vol_id  );
int32_t fat_free_file_entry( fat_fileent_t *fe );
fat_fileent_t *fat_get_opend_file_entry( fat_fileent_t *fe );
bool fat_is_alloc_file_entry( int32_t vol_id, uint32_t parent_clust );
fat_fileent_t *fat_get_file_entry_fromid( int32_t fd );

/* file access functions */
int32_t fat_access( int32_t vol_id, const char *path, int32_t amode );
int32_t fat_do_truncate( fat_fileent_t *fe, size_t new_size );
int32_t fat_creat( int32_t vol_id, const char *path, mod_t mode );
int32_t fat_open( int32_t vol_id, const char *path, uint32_t flag, mod_t mode );
size_t fat_read( int32_t fd, void *buf, size_t bytes );
int32_t fat_sync_fs_wb( int32_t vol_id );
size_t fat_write( int32_t fd, const void *buf, size_t bytes );
offs_t fat_lseek( int32_t fd, offs_t offset, int32_t whence );
int32_t fat_fsync( int32_t fd );
int32_t fat_close( int32_t fd );
int32_t fat_closeall( int32_t vol_id );
int32_t fat_unlink( int32_t vol_id, const char *path );
int32_t fat_truncate( int32_t fd, size_t new_size );
int32_t fat_tell( int32_t fd );
int32_t fat_rename( int32_t vol_id, const char *oldpath, const char *newpath );
int32_t fat_stat( int32_t vol_id, const char *path, stat_t *statbuf );
int32_t fat_fstat( int32_t fd, stat_t *statbuf );
int32_t fat_getattr( int32_t vol_id, const char *path, uint32_t *attrbuf );
int32_t fat_setattr( int32_t vol_id, const char *path, uint32_t set_attr );
int32_t fat_fgetattr( int32_t fd, uint32_t *attrbuf );
int32_t fat_fsetattr( int32_t fd, uint32_t set_attr );

#endif

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

