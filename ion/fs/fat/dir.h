/* FILE: ion_dir.h */
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

#if !defined( FAT_DIR_H_13122005 )
#define FAT_DIR_H_13122005

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "../global/global.h"
#include "file.h"
#include "../fat/path.h"




/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES & DEFINITIONS
-----------------------------------------------------------------------------*/

/* Structure representing information about entry position. */
typedef struct fat_entpos_s {
   uint32_t own_clust,      /* the own cluster number. */
            parent_sect;    /* the sector number of own entry which is located
                               in parent cluster. */
   uint8_t parent_ent_idx;  /* the entry's offset in 'parent_sect'. */

} fat_entpos_t;


/* entry attribute info. */
typedef struct fat_entattr_s {
   uint32_t own_clust,     /* the own cluster number */
            parent_sect;   /* the sector number of own entry which is located in
                             parent cluster */
   union {
      uint32_t flag,       /* entry's flag value */
               filesize;   /* the size of file. */
   } u;
   uint8_t parent_ent_idx, /* the entry's offset in 'parent_sect'. */
           attr;           /* the attribute value */

} fat_entattr_t;


/* directory's status info. */
typedef struct fat_statdir_s {
   uint32_t entries,       /* the count of entries in directory */
            size,          /* size of entries in directory */
            alloc_size;    /* real allocated size of entries in directory */

} fat_statdir_t;


/* Determine the type of file entry */
typedef enum {
   eFAT_FILE = (1<<1),
   eFAT_DIR = (1<<2),
   eFAT_ALL = (eFAT_DIR | eFAT_FILE)

}fat_entry_type_t;




/* Set the own cluster number. */
#define SET_OWN_CLUST(de, clust) (((fat_dirent_t*)(de))->fst_clust_hi =\
                                 (uint16_t)((uint32_t)(clust)>>16),\
                                 ((fat_dirent_t*)(de))->fst_clust_lo =\
                                 (uint16_t)(clust))
/* Get the own cluster number. */
#define GET_OWN_CLUST(de) ((uint32_t)((((fat_dirent_t*)(de))->fst_clust_hi<<16)\
                          | ((fat_dirent_t*)(de))->fst_clust_lo))




/*-----------------------------------------------------------------------------
 DECLARE FUNCTIONS PROTO-TYPE
-----------------------------------------------------------------------------*/

/* directory access functiosns */
void fat_zinit_dir( void );
int32_t fat_init_dir( void );
void fat_recreat_short_info( fat_dirent_t *dst, fat_dirent_t *src );
int32_t fat_alloc_entry_pos( fat_fileent_t *fe );
int32_t fat_get_entry_info( fat_fileent_t *fe, fat_entattr_t *ea );
int32_t fat_resolve_lfn_shortname( fat_fileent_t *fe );
int32_t fat_creat_entry( fat_fileent_t *fe, bool is_new );
int32_t fat_unlink_entry_long( fat_fileent_t *fe );
int32_t fat_unlink_entry_short( fat_fileent_t *fe );
int32_t fat_unlink_entry( fat_fileent_t *fe );
int32_t fat_lookup_entry( fat_arg_t *arg, fat_fileent_t *fe, int32_t ent_type );
int32_t fat_update_sentry( fat_fileent_t *fe, bool flush );
int32_t fat_mkdir( int32_t vol_id, char const *path, mod_t mode );
int32_t fat_rmdir( int32_t vol_id, char const *path );
dir_t* fat_opendir( int32_t vol_id, const char *path );
dirent_t* fat_readdir( dir_t *debuf );
int32_t fat_rewinddir( dir_t *debuf );
int32_t fat_closedir( dir_t *debuf );
int32_t fat_cleandir( int32_t vol_id, const char *path );
int32_t fat_statdir( int32_t vol_id, const char *path, statdir_t *statbuf );
int32_t fat_change_dotdots_dir( fat_fileent_t *fe );

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

