/* FILE: ion_pim_ion.c */
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

#if ( IONFS_BD & IONFS_BD_L2P )
#include "pim_ion.h"

#if defined( L2P_DEMANDING )
#include "../l2p_page/l2p.h"
#else
#include "l2p.h"
#endif

extern bool_t pim_dev_inited[];


/*
 Name: __convert_ionFS_status
 Desc:
 Params:
   - pim_status:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

static int32_t __convert_ionFS_status( int32_t pim_status )
{
   int32_t status;

   switch ( pim_status ) {
      case L2P_OK:
         status = IONFS_OK;
         break;
      default:
         status = IONFS_EIO;
         break;
   }

   return status;
}


/*
 Name: pim_setup_ion
 Desc:
 Params:
   - de:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_setup_ion( pim_devinfo_t *de, char_t * name )
{
   if (  NULL == de )
      return IONFS_EINVAL;

   de->dev_flag = ePIM_NeedErase;

   de->op.ioctl = pim_ioctl_ion;
   de->op.read_sector = pim_readsector_ion;
   de->op.write_sector = pim_writesector_ion;
   de->op.erase_sector = pim_erasesector_ion;

   de->dev_name = name;

   return IONFS_OK;
}




/*
 Name: __pim_ioctl_ion_format
 Desc:
 Params: None.
 Returns:
   int32_t  0(=IONFS_OK) always.
 Caveats: None.
*/

static int32_t __pim_ioctl_ion_format( int32_t dev_id )
{
   l2p_format( dev_id );

   return IONFS_OK;
}




/*
 Name: __pim_ioctl_ion_init
 Desc:
 Params: None.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

static int32_t __pim_ioctl_ion_init( int32_t dev_id )
{
   int32_t rtn;

   if ( true == pim_dev_inited[dev_id] )
      return IONFS_OK;

   rtn = l2p_init();
   if ( (L2P_OK != rtn))
      return __convert_ionFS_status( rtn );

   pim_dev_inited[dev_id] = true;
   return IONFS_OK;
}




/*
 Name: __pim_ioctl_ion_terminate
 Desc:
 Params: None.
 Returns:
   int32_t  0(=IONFS_OK) always.
 Caveats: None.
*/

static int32_t __pim_ioctl_ion_terminate( int32_t dev_id )
{
   pim_dev_inited[dev_id] = false;
   return IONFS_OK;
}




/*
 Name: __pim_ioctl_ion_open
 Desc:
 Params:
   - de:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

static int32_t __pim_ioctl_ion_open( int32_t dev_id, pim_devinfo_t *de )
{
   int32_t rtn;

   if ( NULL == de )
      return IONFS_EINVAL;

   if ( true == de->opened )
      return IONFS_OK;

   rtn = l2p_open( dev_id, &de->totsect_cnt );

   if ( 0 != rtn )
      return __convert_ionFS_status( rtn );

   //de->totsect_cnt = dev->totsect_cnt;
   de->start_sect = IONFS_ION_START_SECTOR;
   de->end_sect = de->totsect_cnt - 1;
   de->bytes_per_sect = IONFS_ION_SECTOR_SIZE;

   /* Set the number of sectors per physical block.
       If block erase operation does not need, you should set '1' value. */
   if( de->dev_flag != ePIM_Removable ) /* if block erase operation needs, */
      de->sects_per_block = l2p_get_sects_per_blk();
   else
      de->sects_per_block = 1;

   return IONFS_OK;
}




/*
 Name: __pim_ioctl_ion_close
 Desc:
 Params: None.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

static int32_t __pim_ioctl_ion_close( int32_t dev_id )
{
   int32_t rtn;


   rtn = l2p_close( dev_id );

   if ( L2P_OK != rtn )
      return __convert_ionFS_status( rtn );


   return IONFS_OK;
}




/*
 Name: pim_ioctl_ion
 Desc:
 Params:
   - cmd:
   - arg:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_ioctl_ion( int32_t dev_id, pim_ioctl_cmd_t cmd, void *arg )
{
   switch ( cmd ) {
      case eIOCTL_format:
         return __pim_ioctl_ion_format( dev_id );
      case eIOCTL_init:
         return __pim_ioctl_ion_init( dev_id );
      case eIOCTL_terminate:
         return __pim_ioctl_ion_terminate( dev_id );
      case eIOCTL_open:
         return __pim_ioctl_ion_open( dev_id, (pim_devinfo_t *)arg );
      case eIOCTL_close:
         return __pim_ioctl_ion_close( dev_id );
   }

   return IONFS_EINVAL;
}




/*
 Name: pim_readsector_ion
 Desc:
 Params:
   - sect_no:
   - buf:
   - count:
 Returns:
   int32_t  >=0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_readsector_ion( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t count )
{
   int32_t rtn;

   rtn = l2p_read( dev_id, sect_no, count, buf );

   if ( L2P_OK != rtn )
      return __convert_ionFS_status( rtn );

   return count;
}




/*
 Name: pim_writesector_ion
 Desc:
 Params:
   - sect_no:
   - buf:
   - count:
 Returns:
   int32_t  >=0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_writesector_ion( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t count )
{
   int32_t rtn;

   rtn = l2p_write( dev_id, sect_no, count, buf );

   if ( L2P_OK != rtn )
      return __convert_ionFS_status( rtn );

   return count;
}




/*
 Name: pim_erasesector_ion
 Desc:
 Params:
   - sect_no:
   - count:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats:
*/

int32_t pim_erasesector_ion( int32_t dev_id, uint32_t sect_no, uint32_t count )
{
   int32_t rtn;

   rtn = l2p_delete( dev_id, sect_no, count );

   if ( L2P_OK != rtn )
      return __convert_ionFS_status( rtn );

   return IONFS_OK;
}

#endif

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

