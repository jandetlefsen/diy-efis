/* FILE: ion_pim_mmc.c */
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


#include "../global/global.h"

#if ( IONFS_BD & IONFS_BD_MMC )

#include "pim_mmc.h"




extern bool_t pim_dev_inited[];


/*
 Name: __convert_ionFS_status
 Desc:
 Params:
   - pim_status:
 Returns:
   int32_t status
 Caveats: None.
*/

static int32_t __convert_ionFS_status( int32_t pim_status )
{
   int32_t status;

   switch ( pim_status ) {
      default:
         status = IONFS_EIO;
         break;
   }

   return status;
}




/*
 Name: pim_setup_mmc
 Desc:
 Params:
   - de:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_setup_mmc( pim_devinfo_t *de, char_t * name )
{
   if (  NULL == de )
      return IONFS_EINVAL;

   de->dev_flag = ePIM_Removable;

   de->op.ioctl = pim_ioctl_mmc;
   de->op.read_sector = pim_readsector_mmc;
   de->op.write_sector = pim_writesector_mmc;
   de->op.erase_sector = pim_erasesector_mmc;

   de->dev_name = name;

   return IONFS_OK;
}




/*
 Name: _pim_ioctl_mmc_format
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  0(=IONFS_OK) always.
 Caveats: None.
*/

static int32_t __pim_ioctl_mmc_format( int32_t dev_id )
{

   return IONFS_OK;
}




/*
 Name: _pim_ioctl_mmc_init
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  0(=IONFS_OK) always.
 Caveats: None.
*/

static int32_t __pim_ioctl_mmc_init( int32_t dev_id )
{
   if ( true == pim_dev_inited[dev_id] )
      return IONFS_OK;

   //TO DO

   pim_dev_inited[dev_id] = true;
   return IONFS_OK;
}




/*
 Name: _pim_ioctl_mmc_terminate
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  0(=IONFS_OK) always.
 Caveats: None.
*/

static int32_t __pim_ioctl_mmc_terminate( int32_t dev_id )
{
   pim_dev_inited[dev_id] = false;
   return IONFS_OK;
}




/*
 Name: _pim_ioctl_mmc_open
 Desc:
 Params:
   - dev_id: Device's id.
   - de:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

static int32_t __pim_ioctl_mmc_open( int32_t dev_id, pim_devinfo_t *de )
{
   if ( NULL == de )
      return IONFS_EINVAL;

   if ( true == de->opened )
      return IONFS_OK;

   #if defined( TLC_PLATFORM ) && defined( TARGET )
   de->totsect_cnt = MMCSDCard.lba;
   de->start_sect = IONFS_MMC_START_SECTOR;
   de->end_sect = de->totsect_cnt - 1;
   de->bytes_per_sect = IONFS_MMC_SECTOR_SIZE;
   #endif

   /* Set the number of sectors per physical block.
       If block erase operation does not need, you should set '1' value. */
   if( de->dev_flag != ePIM_Removable ) /* if block erase operation needs, */
      de->sects_per_block = 256;
   else
      de->sects_per_block = 1;

   return IONFS_OK;
}




/*
 Name: _pim_ioctl_mmc_close
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

static int32_t __pim_ioctl_mmc_close( int32_t dev_id )
{
   return IONFS_OK;
}




/*
 Name: pim_ioctl_mmc
 Desc:
 Params:
   - dev_id: Device's id.
   - cmd:
   - arg:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_ioctl_mmc( int32_t dev_id, pim_ioctl_cmd_t cmd, void *arg )
{
   switch ( cmd ) {
      case eIOCTL_format:
         return __pim_ioctl_mmc_format( dev_id );
      case eIOCTL_init:
         return __pim_ioctl_mmc_init( dev_id );
      case eIOCTL_terminate:
         return __pim_ioctl_mmc_terminate( dev_id );
      case eIOCTL_open:
         return __pim_ioctl_mmc_open( dev_id, (pim_devinfo_t *)arg );
      case eIOCTL_close:
         return __pim_ioctl_mmc_close( dev_id );
   }

   return IONFS_EINVAL;
}




/*
 Name: pim_readsector_mmc
 Desc:
 Params:
   - dev_id: Device's id.
   - sect_no:
   - buf:
   - cnt:
 Returns:
   int32_t  >=0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_readsector_mmc( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t cnt )
{
   int32_t rtn=0;

   #if defined( TLC_PLATFORM ) && defined( TARGET )
   rtn =  mmcsdDrv_ReadBlock( sect_no, buf, cnt, NULL );
   #endif
   if ( rtn < 0 )
      return __convert_ionFS_status( rtn );

   return cnt;
}




/*
 Name: pim_writesector_mmc
 Desc:
 Params:
   - dev_id: Device's id.
   - sect_no:
   - buf:
   - cnt:
 Returns:
   int32_t  >=0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_writesector_mmc( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t cnt )
{
   int32_t rtn=0;

   #if defined( TLC_PLATFORM ) && defined( TARGET )
   rtn = mmcsdDrv_WriteBlock( sect_no, buf, cnt, NULL );
   #endif
   if ( rtn < 0 )
      return __convert_ionFS_status( rtn );

   return cnt;
}




/*
  Name: pim_erasesector_mmc
  Desc:
  Params:
   - dev_id: Device's id.
   - sect_no:
   - cnt:
  Returns:
   int32_t  0(=IONFS_OK) always.
  Caveats: None.
*/

int32_t pim_erasesector_mmc( int32_t dev_id, uint32_t sect_no, uint32_t cnt )
{
   return IONFS_OK;
}

#endif

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

