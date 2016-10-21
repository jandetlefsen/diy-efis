/* FILE: ion_vol.h */
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


#if !defined( FAT_VOL_H_13122005 )
#define FAT_VOL_H_13122005

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "../global/global.h"
#include "../lim/lim.h"
#include "dir.h"
#include "file.h"




/*-----------------------------------------------------------------------------
 DEFINE DEFINITIONS
-----------------------------------------------------------------------------*/

#define MS_FAT12_CLUSTER_MAX           4085
#define MS_FAT16_CLUSTER_MAX           65525

/* The number of entries in the root-directory. */
#define FAT_ROOT_DIR_NUM               512
/* The maximum number of clusters. */
#define FAT12_CLUSTER_MAX              (MS_FAT12_CLUSTER_MAX-16)
#define FAT16_CLUSTER_MAX              (MS_FAT16_CLUSTER_MAX-16)

#define CACHE_SECTOR_NUM                       SECTOR_NUM_PER_CACHE
#define CACHE_BUFFER_SIZE                      (LIM_ALLOW_MAX_SECT_SIZE*CACHE_SECTOR_NUM)
#define CACHE_MIN_SECT_NUM(sect, base)        (base+(((sect-base)/CACHE_SECTOR_NUM)*CACHE_SECTOR_NUM))
#define FAT_REAL_BUF_ADDR(addr, sect)         (addr +(((sect)%CACHE_SECTOR_NUM)*LIM_ALLOW_MAX_SECT_SIZE))
#define LIM_REAL_BUF_ADDR(addr, sect, base)   (addr +(((sect-base)%CACHE_SECTOR_NUM)*LIM_ALLOW_MAX_SECT_SIZE))
#define FAT_ALLOW_MAX_SECT_SIZE        LIM_ALLOW_MAX_SECT_SIZE


#define FAT_OEM_NAME                   "MSWIN4.1"

#define FAT16_NAME                     "FAT16   "
#define FAT32_NAME                     "FAT32   "

#define FAT_TABLE_CNT                  1

#define ALIGN_PAGE_FIRST_DATA          4



/* Get a pointer to volume information using a ID. */
#define GET_FAT_VOL(vol) &fat_vol[vol]




/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES
-----------------------------------------------------------------------------*/

/* The data structure for a boot record. */
typedef struct fat_bootrecord_s {
   fat_type_t efat_type;            /* the type of file system */

   /* Size of Sector & Clustor */
   uint16_t bytes_per_sect,         /* the number of bytes per sector. */
            bytes_per_sect_mask,    /* the number of bytes per sector-mask. */
            bits_per_sect,          /* log2(bytes_per_sect) */
            sects_per_clust,        /* the number sectors per cluster */
            sects_per_clust_mask,   /* the number of bytes per cluster-mask. */
            bits_per_clustsect,     /* log2(sects_per_clust) */
            bits_per_clust,         /* bits_per_sect + bits_per_clustsect */
            bytes_per_clust_mask,   /* (1<<bits_per_clust) - 1 */
            ents_per_sect;          /* count of directory per sector */

   /* FAT Region */
   uint16_t first_fat_sect,         /* the number of first fat sector */
            fat_table_cnt,          /* count of FAT */
            fat_sect_cnt;           /* count of sector */

   /* Root Dir Region */
   uint16_t rootent_cnt,            /* count of root-entry */
            first_root_sect,        /* the number of first root sector */
            last_root_sect;         /* the number of last root sector */

   /* Data Region */
   uint32_t first_data_sect,        /* the number of first data sector */
            last_data_sect,         /* the number of last data sector */
            last_data_clust,        /* number of last-cluster */
            data_clust_cnt,         /* count of data cluster */

            srch_free_clust,        /* point to first searching cluster in data clusters for allocating */
            free_clust_cnt;         /* count of free clusters */

   uint32_t *fat_map;               /* FAT allocate policy */

} fat_bootrecord_t;

#if ( 0 < TRACE )
typedef struct fat_trace_s {
   int32_t log_rcv_cnt,          /* count of recovered log-file */
           afile_ents,           /* allocated file enties */
           ofile_ents,           /* opend file enties */
           cache_refs,           /* count that fat-cache referred */
           cpath_hits,           /* hit cache-paths */
           open_cnt,
           read_cnt,
           write_cnt,
           unlink_cnt,
           mkdir_cnt;

} fat_trace_t;
#define tr_fat_init(vol)    memset(&fat_vol[vol].tr, 0, sizeof(&fat_vol[vol].tr))
#define tr_fat_set_log_rcv_cnt(vol, rcv)   (fat_vol[vol].tr.log_rcv_cnt=rcv)
#define tr_fat_inc_afile_ents(vol)         (fat_vol[vol].tr.afile_ents++)
#define tr_fat_dec_afile_ents(vol)         (fat_vol[vol].tr.afile_ents--)
#define tr_fat_inc_ofile_ents(vol)         (fat_vol[vol].tr.ofile_ents++)
#define tr_fat_dec_ofile_ents(vol)         (fat_vol[vol].tr.ofile_ents--)
#define tr_fat_reset_cache_refs(vol)       (fat_vol[vol].tr.cache_refs=0)
#define tr_fat_inc_cache_refs(vol)         (fat_vol[vol].tr.cache_refs++)
#define tr_fat_dec_cache_refs(vol)         (fat_vol[vol].tr.cache_refs--)
#define tr_fat_inc_cpath_hits(vol)         (fat_vol[vol].tr.cpath_hits++)
#define tr_fat_inc_open_cnt(vol)           (fat_vol[vol].tr.open_cnt++)
#define tr_fat_inc_read_cnt(vol)           (fat_vol[vol].tr.read_cnt++)
#define tr_fat_inc_write_cnt(vol)          (fat_vol[vol].tr.write_cnt++)
#define tr_fat_inc_unlink_cnt(vol)         (fat_vol[vol].tr.unlink_cnt++)
#define tr_fat_inc_mkdir_cnt(vol)          (fat_vol[vol].tr.mkdir_cnt++)
#else
#define tr_fat_init(vol)
#define tr_fat_set_log_rcv_cnt(vol, rcv)
#define tr_fat_inc_afile_ents(vol)
#define tr_fat_dec_afile_ents(vol)
#define tr_fat_inc_ofile_ents(vol)
#define tr_fat_dec_ofile_ents(vol)
#define tr_fat_reset_cache_refs(vol)
#define tr_fat_inc_cache_refs(vol)
#define tr_fat_dec_cache_refs(vol)
#define tr_fat_inc_cpath_hits(vol)
#define tr_fat_inc_open_cnt(vol)
#define tr_fat_inc_read_cnt(vol)
#define tr_fat_inc_write_cnt(vol)
#define tr_fat_inc_unlink_cnt(vol)
#define tr_fat_inc_mkdir_cnt(vol)
#endif


typedef struct fat_volinfo_s {
   int32_t vol_id;                        /* the ID of volume */
   uint8_t name[eFAT_MAX_LABEL_LEN+1];    /* the name of volume */

   fat_bootrecord_t br;                   /* the boot-record structure */

   #if ( 0 < TRACE )
   fat_trace_t tr;
   #endif

   bool fat_mounted;

} fat_volinfo_t;




/*-----------------------------------------------------------------------------
 Global variables
-----------------------------------------------------------------------------*/

extern fat_volinfo_t fat_vol[VOLUME_NUM];




/*-----------------------------------------------------------------------------
 DECLARE FUNTION PROTO-TYPE
-----------------------------------------------------------------------------*/

void fat_zero_init( void );
int32_t fat_setup( fsm_op_t *op );
int32_t fat_init( void );
int32_t fat_format( int32_t vol_id, const char *label, uint32_t flag );
int32_t fat_mount( int32_t vol_id, uint32_t flag );
int32_t fat_umount( int32_t vol_id, uint32_t flag );
int32_t fat_sync( int32_t vol_id );
int32_t fat_statfs( int32_t vol_id, statfs_t *statbuf );
bool fat_is_system_file( fat_fileent_t *fe, bool cmp_name );

#endif

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

