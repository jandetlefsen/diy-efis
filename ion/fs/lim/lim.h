/* FILE: ion_lim.h */
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


#if !defined( LIM_H_29112005 )
#define LIM_H_29112005

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
 ----------------------------------------------------------------------------*/

#include "../global/global.h"
#include "../pim/pim.h"




/*-----------------------------------------------------------------------------
 DEFINE DEFINITIONS & STRUCTURES
-----------------------------------------------------------------------------*/

/* The maximum size of setor. */
#define LIM_ALLOW_MAX_SECT_SIZE PIM_ALLOW_MAX_SECT_SIZE




/* The flag for I/O control at the LIM. */
typedef enum lim_ioctl_cmd_e {
   LIM_IO_Unknown,

   LIM_IO_format,
   LIM_IO_init,
   LIM_IO_terminate,
   LIM_IO_PartInfo,
   LIM_MS_attached,
   LIM_DEV_attached,
   LIM_IO_eject,

   LIM_IO_end

} lim_ioctl_cmd_t;


/* The structure which includes cache information of LIM's local sector. */
typedef struct lim_cacheent_s {
   list_head_t head,    /* the linked-list to free & active cache entries */
             hash_head; /* only the linked-list to the active cache entries */

   uint8_t vol_id,      /* the volume id */
         flag;          /* refer to the cache_flag_t enumeration. */
   uint16_t ref_cnt;    /* the number of references */
   uint32_t sect_no;    /* the sector number */
   uint8_t *buf;        /* the cache buffer */

} lim_cacheent_t, lim_cachedat_t;


#if ( 0 < TRACE )
typedef struct lim_trace_s {
   int32_t cache_refs;           /* count that fat-cache referred */

} lim_trace_t;
#define tr_lim_init(vol)               memset(&lim_vol[vol].tr, 0, sizeof(&fat_vol[vol].tr))
#define tr_lim_inc_cache_refs(vol)     (lim_vol[vol].tr.cache_refs++)
#define tr_lim_dec_cache_refs(vol)     (lim_vol[vol].tr.cache_refs--)
#else
#define tr_lim_init(vol)
#define tr_lim_inc_cache_refs(vol)
#define tr_lim_dec_cache_refs(vol)
#endif


/* The structure which includes volume information of LIM. */
typedef struct lim_volinfo_s {
   bool opened;                /* the open flag */

   uint32_t dev_id,              /* the device id */
              start_sect,        /* the volume's start sector number */
              end_sect,          /* the volume's end sector number */
              totsect_cnt,       /* the number of all sectors in the volume */
              bytes_per_sect,    /* the number of bytes per sector */
              bits_per_sectsize, /* the bit for the sector size */
              io_flag;           /* refer to the lim_ioctl_cmd_t enumeration */

   #if ( 0 < TRACE )
   lim_trace_t tr;
   #endif

} lim_volinfo_t;




extern lim_volinfo_t lim_vol[VOLUME_NUM];

#define GET_LIM_VOL(vol)   (&lim_vol[vol])
#define GET_LIM_DEV(vol)   GET_PIM_DEV(lim_vol[vol].dev_id)




/*----------------------------------------------------------------------------
 DECLARE FUNCTIONS
----------------------------------------------------------------------------*/

void lim_zinit_lim( void );
int32_t lim_reinit_cache_vol( int32_t vol_id );
int32_t lim_init( void );
int32_t lim_open( int32_t vol_id, int32_t dev_id, int32_t part_no );
int32_t lim_terminate( void );
int32_t lim_flush_cdsector( void );
int32_t lim_load_cdsector( uint32_t vol_id, uint32_t sect_no, uint8_t *buf );
int32_t lim_flush_csector( lim_cacheent_t *entry );
int32_t lim_flush_csectors( int32_t vol_id, list_head_t *list );
void lim_clean_csector( lim_cacheent_t *entry );
void lim_clean_csectors( int32_t vol_id );
lim_cacheent_t *lim_get_sector( int32_t vol_id, uint32_t sect_no );
lim_cacheent_t *lim_get_csector( int32_t vol_id, uint32_t sect_no );
int32_t lim_rel_csector( lim_cacheent_t *entry );
void lim_mark_dirty_csector( lim_cacheent_t *entry, list_head_t *list );
int32_t lim_ioctl( int32_t vol_id, uint32_t func, void *param );
int32_t lim_read_sector( int32_t vol_id, uint32_t sect_no, void *buf, uint32_t cnt );
int32_t lim_read_at_sector( int32_t vol_id, uint32_t sect_no, uint32_t offs, uint32_t len, void *buf );
int32_t lim_write_sector( int32_t vol_id, uint32_t sect_no, void *buf, uint32_t cnt );
int32_t lim_erase_sector( int32_t vol_id, uint32_t sect_no, uint32_t cnt );

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

