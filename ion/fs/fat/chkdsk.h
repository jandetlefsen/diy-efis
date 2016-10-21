/* FILE: ion_chkdsk.h */
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

#if !defined( CHKDSK_H_27042006 )
#define CHKDSK_H_27042006

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "../global/global.h"
#include "file.h"




/*-----------------------------------------------------------------------------
 DEFINE DEFINITIONS
-----------------------------------------------------------------------------*/

#define CLUST_PER_CHK      1  /* Number of clusters per chk. */
#define FAT_CHK_BOOT_SIG   0x544F4F42
#define FAT_CHK_UMNT_SIG   0x544E4D55
#define FAT_CHK_FILE       "%chkdsk%.ion"

#define IS_NOTINIT_ENTRY(pEntry) (eFAT_EOC==((uint32_t*)(pEntry))[0])


/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES
-----------------------------------------------------------------------------*/

#if defined( DBG )
typedef struct chk_dbg_s {
   int32_t update_entry_cnt;
   int32_t diff_clust_cnt;
   uint32_t total_free_clust_cnt;
   uint32_t total_recovery_cnt;

} chk_dbg_t;

extern chk_dbg_t chk_dbg;
#endif




/*-----------------------------------------------------------------------------
 DECLARE FUNCTIONS PROTO-TYPE
-----------------------------------------------------------------------------*/

int32_t fat_chk_fattable( int32_t vol_id );
int32_t fat_get_fs_dir_size( int32_t vol_id, uint32_t *fs_size_ptr, uint32_t *dir_size_ptr );

#if defined( CHKDISK )
int32_t fat_chk_init( int32_t vol_id );
int32_t fat_chk_deinit( int32_t vol_id );
bool fat_is_chk_file( fat_fileent_t *fe, bool cmp_name );
int32_t fat_chkdisk_recover( int32_t vol_id );
#endif

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

