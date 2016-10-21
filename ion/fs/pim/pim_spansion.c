#include "../global/global.h"
#include "pim_spansion.h"
#include "../ftl/if_ex.h"
#include "../ftl/flash.h"

extern bool pim_dev_inited[];
uint8_t ftl_initialized[DEVICE_NUM] = {0};



/***********************************************************************/

/*
 Name: __convert_status
 Desc:
 Params:
   - ftl_status:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __convert_status(FTL_STATUS ftl_status)
  {
  int32_t status;

  switch(ftl_status)
    {
    case FTL_ERR_PASS:
      status = OK;
      break;

    default:
      status = e_io;
      break;
    }

  return status;
  }

/*
 Name: pim_setup_span
 Desc:
 Params:
   - de:
   - name:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t pim_setup_span(pim_devinfo_t *de, char * name)
  {
  if(de == NULL)
    {
    return e_inval;
    }

  de->dev_flag = ePIM_NeedErase;

  de->op.ioctl = pim_ioctl_span;
  de->op.read_sector = pim_readsector_span;
  de->op.write_sector = pim_writesector_span;
  de->op.erase_sector = pim_erasesector_span;

  de->dev_name = name;

  return OK;
  }

/*
 Name: __pim_ioctl_span_format
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  0(=OK) always.
 Caveats: None.
 */

static int32_t __pim_ioctl_span_format(int32_t dev_id)
  {
  FTL_INIT_STRUCT initStruct;
  FTL_STATUS status;
#if (CACHE_RAM_BD_MODULE == true && CACHE_DYNAMIC_ALLOCATION == true)
  initStruct.format = FTL_DONT_FORMAT;
  initStruct.os_type = FTL_RTOS_INT; /* not use */
  initStruct.table_storage = FTL_TABLE_NV; /* not use */
  initStruct.allocate = FTL_ALLOCATE; /* not use */
  initStruct.mode = 0;
  initStruct.total_ram_allowed = gTotal_ram_allowed;
#endif

  switch(dev_id)
    {
    case SPAN_DEVA:


#if (CACHE_RAM_BD_MODULE == true && CACHE_DYNAMIC_ALLOCATION == true)
      if((status = FTL_Format(&initStruct)) != FTL_ERR_PASS)
#else
      if((status = FTL_Format()) != FTL_ERR_PASS)
#endif
        {
        return __convert_status(status);
        }
      initStruct.format = FTL_DONT_FORMAT; // Don't Format
      initStruct.os_type = FTL_RTOS_INT; // Use Blocking Code
      initStruct.table_storage = FTL_TABLE_LOCATION;
      initStruct.allocate = FTL_ALLOCATE; // Allocate tables
#if (CACHE_RAM_BD_MODULE == true && CACHE_DYNAMIC_ALLOCATION == true)
      initStruct.total_ram_allowed = gTotal_ram_allowed;
#endif
      if((status = FTL_InitAll(&initStruct)) != FTL_ERR_PASS)
        {
        return __convert_status(status);
        }
      break;
    default:
      return __convert_status(FTL_ERR_FAIL);
    }
  ftl_initialized[dev_id] = 1;
  return OK;
  }

/*
 Name: pim_ioctl_span_init
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t pim_ioctl_span_init(int32_t dev_id)
  {
  FTL_INIT_STRUCT initStruct;
  FTL_STATUS status;
#if (CACHE_RAM_BD_MODULE == true && CACHE_DYNAMIC_ALLOCATION == true)
  initStruct.format = FTL_DONT_FORMAT;
  initStruct.os_type = FTL_RTOS_INT; /* not use */
  initStruct.table_storage = FTL_TABLE_NV; /* not use */
  initStruct.allocate = FTL_ALLOCATE; /* not use */
  initStruct.mode = 0;
  initStruct.total_ram_allowed = gTotal_ram_allowed;
#endif

  if(pim_dev_inited[dev_id] == true)
    {
    return OK;
    }
  switch(dev_id)
    {
    case SPAN_DEVA:
      if(ftl_initialized[dev_id] == 0)
        {
        initStruct.format = FTL_DONT_FORMAT; // Format as needed
        initStruct.os_type = FTL_RTOS_INT; // Use Blocking Code
        initStruct.table_storage = FTL_TABLE_LOCATION;
        initStruct.allocate = FTL_ALLOCATE; // Allocate tables
#if (CACHE_RAM_BD_MODULE == true && CACHE_DYNAMIC_ALLOCATION == true)
        initStruct.total_ram_allowed = gTotal_ram_allowed;
#endif

        if((status = FTL_InitAll(&initStruct)) != FTL_ERR_PASS)
          {
          if(status == FTL_ERR_NOT_FORMATTED)
            {
#if (CACHE_RAM_BD_MODULE == true && CACHE_DYNAMIC_ALLOCATION == true)
            if((status = FTL_Format(&initStruct)) != FTL_ERR_PASS)
#else
            if((status = FTL_Format()) != FTL_ERR_PASS)
#endif
              {
              return __convert_status(status);
              }
            if((status = FTL_InitAll(&initStruct)) != FTL_ERR_PASS)
              {
              return __convert_status(status);
              }
            }
          else
            {
            return __convert_status(status);
            }

          }
        ftl_initialized[dev_id] = 1;
        }
      break;
    default:
      return __convert_status(FTL_ERR_FAIL);
    }
  pim_dev_inited[dev_id] = true;
  return OK;
  }

/*
 Name: __pim_ioctl_span_terminate
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  0(=OK) always.
 Caveats: None.
 */

static int32_t __pim_ioctl_span_terminate(int32_t dev_id)
  {
  FTL_STATUS status;
  switch(dev_id)
    {
    case SPAN_DEVA:
      if((status = FTL_Shutdown()) != FTL_ERR_PASS)
        {
        return __convert_status(status);
        }
      break;
    default:
      return __convert_status(FTL_ERR_FAIL);
    }
  ftl_initialized[dev_id] = 0;
  pim_dev_inited[dev_id] = false;
  return OK;
  }

/*
 Name: __pim_ioctl_span_open
 Desc:
 Params:
   - dev_id: Device's id.
   - de:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __pim_ioctl_span_open(int32_t dev_id, pim_devinfo_t *de)
  {
  FTL_STATUS status;
  FTL_CAPACITY cap;
#if ( DEVICE_NUM > 1 ) 
  DEV_1_FTL_CAPACITY DEV_1_cap;
#endif
  if(de == NULL)
    {
    return e_inval;
    }

  if(de->opened == true)
    {
    return OK;
    }
  switch(dev_id)
    {
    case SPAN_DEVA:
      if((status = FTL_GetCapacity(&cap)) != FTL_ERR_PASS)
        {
        return __convert_status(status);
        }
      de->totsect_cnt = cap.numBlocks;
      de->sects_per_block = cap.blockSize / SPAN_SECTOR_SIZE;

      break;
    default:
      return __convert_status(FTL_ERR_FAIL);
    }

  de->start_sect = SPAN_START_SECTOR;
  de->end_sect = de->totsect_cnt - 1;
  de->bytes_per_sect = SPAN_SECTOR_SIZE;
  de->bits_per_sectsize = SPAN_SECTOR_SIZE_BITS; // log2(bytes_per_sect)

  return OK;
  }

/*
 Name: __pim_ioctl_span_close
 Desc:
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static int32_t __pim_ioctl_span_close(int32_t dev_id)
  {
  /* NULL */
  return OK;
  }

/*
 Name: pim_ioctl_span
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

int32_t pim_ioctl_span(int32_t dev_id, pim_ioctl_cmd_t cmd, void *arg)
  {
  switch(cmd)
    {
    case eIOCTL_format:
      return __pim_ioctl_span_format(dev_id);

    case eIOCTL_init:
      return pim_ioctl_span_init(dev_id);

    case eIOCTL_terminate:
      return __pim_ioctl_span_terminate(dev_id);

    case eIOCTL_open:
      return __pim_ioctl_span_open(dev_id, (pim_devinfo_t *) arg);

    case eIOCTL_close:
      return __pim_ioctl_span_close(dev_id);
    }

  return e_inval;
  }

/*
 Name: pim_readsector_span
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

int32_t pim_readsector_span(int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t cnt)
  {
  FTL_STATUS status;
  uint32_t done;
  switch(dev_id)
    {
    case SPAN_DEVA:
      if((status = FTL_DeviceObjectsRead(buf, sect_no, cnt, &done)) != FTL_ERR_PASS)
        {
        return __convert_status(status);
        }
      break;
    default:
      return __convert_status(FTL_ERR_FAIL);
    }
  return OK;
  }

/*
 Name: pim_writesector_span
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

int32_t pim_writesector_span(int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t cnt)
  {
  FTL_STATUS status;
  uint32_t done;
  switch(dev_id)
    {
    case SPAN_DEVA:
      if((status = FTL_DeviceObjectsWrite(buf, sect_no, cnt, &done)) != FTL_ERR_PASS)
        {
        return __convert_status(status);
        }
      break;
    default:
      return __convert_status(FTL_ERR_FAIL);
    }
  return OK;
  }

/*
 Name: pim_erasesector_span
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

int32_t pim_erasesector_span(int32_t dev_id, uint32_t sect_no, uint32_t cnt)
  {
  FTL_STATUS status;
  uint32_t done;
  switch(dev_id)
    {
    case SPAN_DEVA:
      if((status = FTL_DeviceObjectsDelete(sect_no, cnt, &done)) != FTL_ERR_PASS)
        {
        return __convert_status(status);
        }
      break;
    default:
      return __convert_status(FTL_ERR_FAIL);
    }
  return OK;
  }
