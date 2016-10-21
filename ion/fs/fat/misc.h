/* FILE: ion_misc.h */
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


#if !defined( FAT_MISC_H_23122005 )
#define FAT_MISC_H_23122005

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "../global/global.h"
#include "fat.h"
#include "file.h"




/*-----------------------------------------------------------------------------
 DEFINE ENUMERATIONS
-----------------------------------------------------------------------------*/

typedef enum fat_tm_type_e {
   TM_CREAT_ENT = 1,  /* It means the time when a file is created. */
   TM_WRITE_ENT = 2,  /* It means the time when a file is written. */
   TM_ACCESS_ENT = 4,  /* It means the time when a file is accessed. */
   TM_ALL_ENT = TM_CREAT_ENT | TM_WRITE_ENT | TM_ACCESS_ENT,
   TM_END

} fat_tm_type_t;




/*-----------------------------------------------------------------------------
 DECLARE FUNTION PROTO-TYPE
-----------------------------------------------------------------------------*/

void fat_ftime_2_gtime( uint32_t *ctime, uint32_t *atime, uint32_t *wtime, fat_dirent_t *de );
void fat_gtime_2_ftime( fat_dirent_t *de, uint32_t *ctime, uint32_t *atime, uint32_t *wtime );
int32_t fat_set_ent_time( fat_dirent_t *de, uint32_t type );

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

