#include "../lim/lim.h"
#include "pim.h"
#include "pim_spansion.h"
#include "../fat/mbr.h"
#include "../fat/path.h"

#include <string.h>

typedef int32_t(* pim_devsetupcallback_t)(pim_devinfo_t *, char *);

typedef struct pim_setup_s
  {
  char * dev_name;
  pim_devsetupcallback_t dev_setup_callback;

  } pim_setup_t;

static pim_setup_t dev_setup[] = {
  {("nor"), pim_setup_span},
  {NULL, NULL}
  };

pim_devinfo_t pim_dev[DEVICE_NUM];

bool pim_dev_inited[DEVICE_NUM] = {false,};

/*
 Name: pim_zinit_pim
 Desc: Zero initialize all data.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void pim_zinit_pim(void)
  {
  memset(&pim_dev, 0, sizeof (pim_dev));
  memset(&pim_dev_inited, 0, sizeof (pim_dev_inited));
  }

/*
 Name: pim_init
 Desc: Initialize all devices.
 Params: None.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: This function check whether the setup function in each device exists
          or not. If setup function doesn't exist, it returns e_port error.
 */

int32_t pim_init(void)
  {
  pim_setup_t *setup = &dev_setup[0];
  pim_devinfo_t *pdi = &pim_dev[0];
  uint32_t i;


  for (i = 0; i < DEVICE_NUM; i++)
    {
    if (NULL == setup[i].dev_setup_callback)
      return set_errno(e_port);

    pdi[i].opened = false;
    }

  return s_ok;
  }

/*
 Name: pim_open
 Desc: Open a specific device.
 Params:
   - dev_id: Device's id to be opened.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: This function calls the setup function. The setup function in each
          device defines functions about each operation. If setup function
          doesn't exist, it returns e_port error.
 */

int32_t pim_open(int32_t dev_id)
  {
  pim_setup_t setup = dev_setup[dev_id];
  pim_devinfo_t *pdi = &pim_dev[dev_id];
  int32_t rtn;


  if (NULL == setup.dev_setup_callback || NULL == pdi)
    return set_errno(e_port);

  if (true == pdi->opened)
    {
    if (ePIM_Removable & pdi->dev_flag)
      {
      if (0 > pim_close(dev_id))
        return set_errno(e_io);
      }
    else
      return s_ok;
    }

  rtn = setup.dev_setup_callback(pdi, setup.dev_name);
  if (0 > rtn) return set_errno(rtn);

  rtn = pdi->op.ioctl(dev_id, eIOCTL_init, NULL);
  if (0 > rtn) return set_errno(rtn);

  rtn = pdi->op.ioctl(dev_id, eIOCTL_open, pdi);
  if (0 > rtn) return set_errno(rtn);

  pdi->opened = true;

  return s_ok;
  }

/*
 Name: pim_close
 Desc: close a specific device.
 Params:
   - dev_id: Device's id to be closed.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t pim_close(int32_t dev_id)
  {
  pim_devinfo_t *pdi = &pim_dev[dev_id];


  if (true == pdi->opened)
    {
    pdi->op.ioctl(dev_id, eIOCTL_close, NULL);
    pdi->opened = false;
    }

  return s_ok;
  }

/*
 Name: pim_terminate
 Desc: Terminate all devices.
 Params: None.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: After this function is called, the file system can't be used anymore.
 */

int32_t pim_terminate(void)
  {
  pim_devinfo_t *pdi = &pim_dev[0];
  int32_t i;


  for (i = 0; i < DEVICE_NUM; i++)
    {
    if (true == pdi[i].opened)
      {
      pdi[i].op.ioctl(i, eIOCTL_close, NULL);
      pdi[i].op.ioctl(i, eIOCTL_terminate, NULL);
      pdi[i].opened = false;
      }
    }

  return s_ok;
  }

/*
 Name: pim_read_sector
 Desc: Read data from a sector in a specific device.
 Params:
   - dev_id: An ID of device.
   - sect_no: The sector number to be read.
   - buf: Pointer to a buffer in which the bytes read are placed.
   - count: The number of bytes to be read.
 Returns:
   int32_t  >0 on success.
            <0 on fail.
 Caveats: This function does sector-read function registered for device.
 */

int32_t pim_read_sector(int32_t dev_id, uint32_t sect_no, uint8_t *buf,
                        uint32_t count)
  {
  /* Read the operation's pointer of device */
  pim_devop_t *pdo = &pim_dev[dev_id].op;

  /* Call the Device read function */
  return pdo->read_sector(dev_id, sect_no, buf, count);
  }

/*
 Name: pim_read_at_sector
 Desc: Read data from a specific offset of sector in a specific device.
 Params:
   - dev_id: An ID of device.
   - sect_no: The sector number to be read.
   - offs: The amount byte offset in the sector to be read.
   - len: The number of bytes to be read.
   - buf: Pointer to a buffer in which the bytes read are placed.
 Returns:
   int32_t  >0 on success.
            <0 on fail.
 Caveats: This function does sector-read-at-offset function registered for
          device.
 */

int32_t pim_read_at_sector(int32_t dev_id, uint32_t sect_no, uint32_t offs,
                           uint32_t len, uint8_t *buf)
  {
  /* Read the operation's pointer of device */
  pim_devop_t *pdo = &pim_dev[dev_id].op;

  /* Read data from a specific offset of sector */
  return pdo->read_at_sector(dev_id, sect_no, offs, len, buf);
  }

/*
 Name: pim_write_sector
 Desc: Write data to a sector in a specific device.
 Params:
   - dev_id: An ID of device.
   - sect_no: The sector number which data is written to.
   - buf: Pointer to a buffer containing the data to be written.
   - count: The size in bytes of the data to be written.
 Returns:
   int32_t  >0 on success.
            <0 on fail.
 Caveats: This function does sector-write function registered for device.
 */

int32_t pim_write_sector(int32_t dev_id, uint32_t sect_no, uint8_t *buf,
                         uint32_t count)
  {
  /* Read the operation's pointer of device */
  pim_devop_t *pdo = &pim_dev[dev_id].op;

  /* Call the Device write function */
  return pdo->write_sector(dev_id, sect_no, buf, count);
  }

/*
 Name: pim_erase_sector
 Desc: Erase a sector in a specific device.
 Params:
   - dev_id: An ID of device.
   - sect_no: Sector number to be erased.
   - count: A number of sectors to be erased.
 Returns:
   int32_t  >0 on success.
            <0 on fail.
 Caveats: This function does sector-erase function registered for device.
 */

int32_t pim_erase_sector(int32_t dev_id, uint32_t sect_no, uint32_t count)
  {
  pim_devop_t *pdo = &pim_dev[dev_id].op;

  /* Call the PIM's erase function */
  return pdo->erase_sector(dev_id, sect_no, count);
  }

/*
 Name: pim_get_sectors
 Desc: Get the number of sectors in a specific device.
 Params:
   - dev_id: An ID of device.
 Returns:
   int32_t  >0 on success. The return value is the number of sectors.
            <0 on fail.
 Caveats: If the device didn't open, this function returns e_noinit error.
          This function returns the number of sectors in the device except MBR.
 */

int32_t pim_get_sectors(int32_t dev_id)
  {
  pim_devinfo_t *pdi = &pim_dev[dev_id];

  /* Check the state of PIM's initialization */
  if (!pdi->opened)
    return e_noinit;

  /* return the initialized-sector's count  */
  return pim_dev[dev_id].totsect_cnt;
  }

/*
 Name: pim_get_devicetype
 Desc: Get name of specific device.
 Params:
   - dev_id: An ID of device.
   - name: Device type name.
 Returns:
   int32_t  >0 on success. The return value is the number of sectors.
            <0 on fail.
 Caveats: If the device didn't open, this function returns e_noinit error.
 */

result_t pim_get_devicetype(int32_t dev_id, char * name)
  {
  pim_devinfo_t *pdi = &pim_dev[dev_id];

  /* Check the PIM's initialize state */
  if (!pdi->opened)
    return e_noinit;

  strcpy(name, pdi->dev_name);

  return s_ok;
  }

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

