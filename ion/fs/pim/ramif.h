/* FILE: ion_ramif.h */
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


#if !defined( RAMIF_H_15122005 )
#define RAMIF_H_15122005

#include "../../ion.h"




#define RAM_OK 0
#define RAM_ERROR -1

#if ( SYS_HW & HW_CLABSYS )
#define RAM_AREA_SIZE  (24*1024*1024)
#else
#define RAM_AREA_SIZE  (128*1024*1024)
#endif

#define RAM_SECTS_PER_BLK (256)

extern uint8_t ram_disk[DEVICE_NUM][RAM_AREA_SIZE];




/* ------------------------------- Global Funtions ------------------------------- */
void ram_on_delete( bool On );
int32_t ram_init( int32_t dev_id );
int32_t ram_open( int32_t dev_id, uint32_t *p_sect_cnt );
int32_t ram_format( int32_t dev_id );
int32_t ram_write( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t count );
int32_t ram_read( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t count );
int32_t ram_delete( int32_t dev_id, uint32_t sect_no, uint32_t count );
int32_t ram_close( int32_t dev_id );
int32_t ram_get_sects_per_blk(void);

#endif

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

