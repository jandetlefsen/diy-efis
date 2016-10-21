#include "../global/global.h"
#include "pim_ram.h"
#include "ramif.h"

#if ( IONFS_BD & IONFS_BD_RAM )
#define IONFS_RAM_DEVICE_NUM IONFS_DEVICE_NUM

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
      case RAM_OK:
         status = IONFS_OK;
         break;

      default:
         status = IONFS_EIO;
         break;
   }

   return status;
}




/*
 Name: pim_setup_ram
 Desc:
 Params:
   - de:
   - name:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_setup_ram( pim_devinfo_t *de, char_t * name )
{
   if (  NULL == de )
      return IONFS_EINVAL;

   de->dev_flag = ePIM_NeedErase;

   de->op.ioctl = pim_ioctl_ram;
   de->op.read_sector = pim_readsector_ram;
   de->op.write_sector = pim_writesector_ram;
   de->op.erase_sector = pim_erasesector_ram;

   de->dev_name = name;

   return IONFS_OK;
}




/*
 Name: _pim_ioctl_ram_format
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  0(=IONFS_OK) always.
 Caveats: None.
*/

static int32_t __pim_ioctl_ram_format( int32_t dev_id )
{
   return IONFS_OK;
}




/*
 Name: _pim_ioctl_ram_init
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_ioctl_ram_init( int32_t dev_id )
{
   int32_t rtn;

   if ( true == pim_dev_inited[dev_id] )
      return IONFS_OK;

   rtn = ram_init( dev_id );

   if ( RAM_OK != rtn )
      return __convert_ionFS_status( rtn );

   pim_dev_inited[dev_id] = true;
   return IONFS_OK;
}




/*
 Name: _pim_ioctl_ram_terminate
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  0(=IONFS_OK) always.
 Caveats: None.
*/

static int32_t __pim_ioctl_ram_terminate( int32_t dev_id )
{
   pim_dev_inited[dev_id] = false;
   return IONFS_OK;
}




/*
 Name: _pim_ioctl_ram_open
 Desc:
 Params:
   - dev_id: Device's id.
   - de:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

static int32_t __pim_ioctl_ram_open( int32_t dev_id, pim_devinfo_t *de )
{
   uint32_t sect_cnt;
   int32_t rtn;


   if ( NULL == de )
      return IONFS_EINVAL;

   if ( true == de->opened )
      return IONFS_OK;

   rtn = ram_open( dev_id, &sect_cnt );
   if ( RAM_OK != rtn )
      return __convert_ionFS_status( rtn );

   de->totsect_cnt = sect_cnt;
   de->start_sect = 0;
   de->end_sect = de->totsect_cnt - 1;
   de->bytes_per_sect = IONFS_RAM_SECTOR_SIZE;

   /* Set the number of sectors per physical block.
       If block erase operation does not need, you should set '1' value. */
   if( de->dev_flag != ePIM_Removable ) /* if block erase operation needs, */
      de->sects_per_block = ram_get_sects_per_blk();
   else
      de->sects_per_block = 1;

   return IONFS_OK;
}




/*
 Name: _pim_ioctl_ram_close
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

static int32_t __pim_ioctl_ram_close( int32_t dev_id )
{
   int32_t rtn;


   rtn = ram_close( dev_id );

   if ( RAM_OK != rtn )
      return __convert_ionFS_status( rtn );

   return IONFS_OK;
}




/*
 Name: pim_ioctl_ram
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

int32_t pim_ioctl_ram( int32_t dev_id, pim_ioctl_cmd_t cmd, void *arg )
{
   switch ( cmd ) {
      case eIOCTL_format:
         return __pim_ioctl_ram_format( dev_id );
      case eIOCTL_init:
         return pim_ioctl_ram_init( dev_id );
      case eIOCTL_terminate:
         return __pim_ioctl_ram_terminate( dev_id );
      case eIOCTL_open:
         return __pim_ioctl_ram_open( dev_id, (pim_devinfo_t *)arg );
      case eIOCTL_close:
         return __pim_ioctl_ram_close( dev_id );
   }

   return IONFS_EINVAL;
}




/*
 Name: pim_readsector_ram
 Desc:
 Params:
   - dev_id: Device's id.
   - sect_no:
   - buf:
   - cnt:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_readsector_ram( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t cnt )
{
   int32_t rtn;

   rtn = ram_read( dev_id, sect_no, buf, cnt );
   if ( RAM_OK != rtn )
      return __convert_ionFS_status( rtn );

   return IONFS_OK;
}




/*
 Name: pim_writesector_ram
 Desc:
 Params:
   - dev_id: Device's id.
   - sect_no:
   - buf:
   - cnt:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_writesector_ram( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t cnt )
{
   int32_t rtn;

   rtn = ram_write( dev_id, sect_no, buf, cnt );
   if ( RAM_OK != rtn )
      return __convert_ionFS_status( rtn );

   return IONFS_OK;
}




/*
 Name: pim_erasesector_ram
 Desc:
 Params:
   - dev_id: Device's id.
   - sect_no:
   - cnt:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t pim_erasesector_ram( int32_t dev_id, uint32_t sect_no, uint32_t cnt )
{
   int32_t rtn;

   rtn = ram_delete( dev_id, sect_no, cnt );
   if ( RAM_OK != rtn )
      return __convert_ionFS_status( rtn );

   return IONFS_OK;
}

#endif

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

