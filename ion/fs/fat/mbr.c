#include "../global/global.h"
#include "../lim/lim.h"
#include "mbr.h"
#include "../fsm/fsm.h"
#include "fat.h"
#include "vol.h"
#include "../fat/path.h"

#include <string.h>


/*-----------------------------------------------------------------------------
 DEFINE DEFINITIONS
-----------------------------------------------------------------------------*/

/* boot indicator */
#define BOOTABLE_IND          0x80   /* bootable partition */
#define NON_BOTTALBE_IND      0x00   /* non bootable partition */

#define MBR_SIZEOF_PARTITION  0x10
#define MBR_OFFSET_PARTITION  0x1BE
#define MBR_OFFSET_SIGNATURE  0x1FE

#define MBR_SIGNATURE1        0x55
#define MBR_SIGNATURE2        0xAA

#define GET_MBR_PARTITION(mbr, num) (((uint8_t*)mbr)+MBR_OFFSET_PARTITION \
                                     + ((uint8_t)num*MBR_SIZEOF_PARTITION))
#define GET_MBR_SIGNATURE(mbr) ((mbr) + MBR_OFFSET_SIGNATURE)




/*-----------------------------------------------------------------------------
 DEFINE FUNCTIONS
-----------------------------------------------------------------------------*/

/*
 Name: __mbr_read
 Desc: Read MBR in a device specified by the 'dev_id' parameter.
 Params:
   - dev_id: The ID of device.
   - buf: Pointer to a buffer in which the bytes read are placed.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static result_t __mbr_read(uint32_t dev_id, uint8_t *buf)
  {
  int32_t rtn;

  /* Read MBR through PIM Layer. Sector #0 is MBR. */
  rtn = pim_read_sector(dev_id, 0, buf, 1);

  if (0 > rtn)
    return set_errno(rtn);

  return s_ok;
  }

/*
 Name: __mbr_write
 Desc: Write data to MBR in a device specified by the 'dev_id' parameter.
 Params:
   - dev_id: The ID of device.
   - buf: Pointer to a buffer containing the data to be written.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static result_t __mbr_write(uint32_t dev_id, uint8_t *buf)
  {
  int32_t rtn;

  /* Write to MBR through PIM Layer. Sector #0 is MBR. */
  rtn = pim_write_sector(dev_id, 0, buf, 1);

  if (0 > rtn)
    return set_errno(rtn);

  return s_ok;
  }

/*
 Name: __mbr_check_mbr_partition
 Desc: Check whether MBR data is valid or not.
 Params:
   - dev_id: The ID of device.
   - mbrbuf: The size in bytes of the data to be written.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

static result_t __mbr_check_mbr_partition(uint32_t dev_id, uint8_t *mbrbuf, uint32_t part_no)
  {
  pim_devinfo_t *dev;
  mbr_partition_t part;
  uint8_t *sig_mbr;


  dev = GET_PIM_DEV(dev_id);

  memcpy(&part, GET_MBR_PARTITION(mbrbuf, part_no), sizeof (mbr_partition_t));

  sig_mbr = GET_MBR_SIGNATURE(mbrbuf);

  /* Check whether the MBR is valid or not. */
  if ((MBR_SIGNATURE1 != sig_mbr[0]) || (MBR_SIGNATURE2 != sig_mbr[1]))
    return set_errno(e_mbr);

  if (!((BOOTABLE_IND == part.boot_ind) || (NON_BOTTALBE_IND == part.boot_ind)))
    return set_errno(e_mbr);

  if ((part.start_sect < dev->start_sect) || (part.totsect_cnt > dev->totsect_cnt))
    return set_errno(e_mbr);

  return s_ok;
  }

/*
 Name: __init_mbr_partition
 Desc: Initialize the buffer for the MBR.
 Params:
   - mbr: Pointer to mbr_partition_t structure to be initialized.
 Returns: None.
 Caveats: None.
 */

static void __init_mbr_partition(mbr_partition_t *mbr)
  {
  memset(mbr, 0, sizeof (mbr_partition_t));

  mbr->boot_ind = (uint8_t) NON_BOTTALBE_IND;
  }

/*
 Name: lim_vol_info_2_mbr_partition
 Desc: Put LIM's partition information into the MBR buffer.
 Params:
   - lvi: Pointer to the structure which includes partition information of LIM.
   - mbr: Pointer to the mbr_partition_t structure which includes MBR
          infromation.
 Returns:
   int32_t  0(s_ok) always.
 Caveats: None.
 */

result_t lim_vol_info_2_mbr_partition(lim_volinfo_t *lvi, mbr_partition_t *mbr)
  {
  uint8_t start_head,
    end_head,
    start_sect,
    end_sect;
  uint16_t start_cyl,
    end_cyl;


  /* Use LBA mode */
  start_head = MBR_LBA_HEAD;
  end_head = MBR_LBA_HEAD;

  start_sect = MBR_LBA_SECTOR;
  end_sect = MBR_LBA_SECTOR;

  start_cyl = MBR_LBA_CYLINDER;
  end_cyl = MBR_LBA_CYLINDER;

  /* Fill data in the MBR area. */
  mbr->start_head = start_head;
  mbr->start_sect_cyl = (uint8_t) (((start_cyl & 0x0300) >> 2) | (start_sect & 0x3F));
  mbr->start_cyl = (uint8_t) (start_cyl & 0xFF);
  mbr->end_head = end_head;
  mbr->end_sect_cyl = (uint8_t) (((end_cyl & 0x0300) >> 2) | (end_sect & 0x3F));
  mbr->end_cyl = (uint8_t) (end_cyl & 0xFF);

  /* Get the sector information. */
  memcpy(&mbr->start_sect, &lvi->start_sect, 4);
  memcpy(&mbr->totsect_cnt, &lvi->totsect_cnt, 4);

  return s_ok;
  }

/*
 Name: __mbr_partition_2_lim_vol_info
 Desc: Put MBR data in the structure which includes partition information of
       LIM.
 Params:
   - mbr: Pointer to the mbr_partition_t structure which includes MBR
          infromation.
   - lvi: Pointer to the lim_volinfo_t structure which includes volume
          information of LIM.
 Returns:
   int32_t  0(s_ok) always.
 Caveats: None.
 */

static result_t __mbr_partition_2_lim_vol_info(mbr_partition_t *mbr, lim_volinfo_t *lvi)
  {
  memcpy(&lvi->start_sect, &mbr->start_sect, 4);
  memcpy(&lvi->totsect_cnt, &mbr->totsect_cnt, 4);

  lvi->end_sect = lvi->start_sect + lvi->totsect_cnt - 1;

  return s_ok;
  }

/*
 Name: __mbr_adjust_lim_info
 Desc: When a partitioin is created, update the volume information.
       Update the value of LIM volume structure according to the number of
       sector of creating partition.
 Params:
   - lvi: Pointer to the structure which includes information of LIM.
   - start_sect: The start sector number.
   - count: The number of sector which the partition has.
 Returns: None.
 Caveats: If start sector number is '0', the partition will create from sector
          number '1' to except the sector for MBR.
 */

static void __mbr_adjust_lim_info(lim_volinfo_t *lvi, uint32_t start_sect, uint32_t count)
  {
  pim_devinfo_t *pdi = GET_PIM_DEV(lvi->dev_id);
#if ( ALIGN & (ALIGN_DATA_SECT | ALIGN_ROOT_DIR) )
  uint32_t no_align;
#endif

  /*
     On specific Device or FTL, when first sector of MBR or BR in case of using multi-volume is aligned
     as the number of sectors per physical block, it performs well.
   */

  fsm_assert1(0 != pdi->sects_per_block);

#if ( ALIGN & (ALIGN_DATA_SECT | ALIGN_ROOT_DIR) )
  no_align = (start_sect) % pdi->sects_per_block;
#endif

  /* Is sector MBR? */
  if (0 == start_sect)
    {
    /* Skip MBR. */
    start_sect++;
    count--;
    }

#if ( ALIGN & (ALIGN_DATA_SECT | ALIGN_ROOT_DIR) )
  if (no_align)
    {
    start_sect += (pdi->sects_per_block - no_align);
    count -= (pdi->sects_per_block - no_align);
    }
#endif

  lvi->start_sect = start_sect;
  lvi->end_sect = lvi->start_sect + count - 1;
  lvi->totsect_cnt = lvi->end_sect - lvi->start_sect + 1;
  }

/*
 Name: __pim_2_lim
 Desc: Get LIM information from PIM information.
 Params:
   - dev_id: The ID of device.
   - pdi: Pointer to the pim_devinfo_t structure which includes device
          information of PIM.
   - lvi: Pointer to the lim_volinfo_t structure that has LIM volume
          information to put the device information of PIM.
 Returns: None.
 Caveats: None.
 */

static void __pim_2_lim(uint32_t dev_id, pim_devinfo_t *pdi, lim_volinfo_t *lvi)
  {
  lvi->dev_id = dev_id;

  lvi->totsect_cnt = pdi->totsect_cnt;
  lvi->start_sect = pdi->start_sect;
  lvi->end_sect = pdi->end_sect;

  lvi->bytes_per_sect = pdi->bytes_per_sect;
  }

/*
 Name: mbr_creat_partition
 Desc: Create a partition and write the partition information to MBR.
 Params:
   - dev_id: The ID of device to be created partition.
   - part_no: The partition number to be created.
   - start_sec: The beginning sector number of the partition to be created.
   - count: The number of the partition's sector to be created.
   - fs_type: The file system type. The type is one of the following string.
                 "FAT6" - FAT16 system.
                 "FAT32" - FAT32 system.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: The ID of device should be smaller than DEVICE_NUM. The partition
          number should be smaller than VOLUME_MAX. Alose ionFS file system
          supports four volums per device, and doesn't support FAT12.
 */

result_t mbr_creat_partition(uint32_t dev_id, uint32_t part_no, uint32_t start_sec, uint32_t count, const char *fs_type)
  {
  pim_devinfo_t *dev = GET_PIM_DEV(dev_id);
  int32_t rtn;
  uint8_t buf_mbr[LIM_ALLOW_MAX_SECT_SIZE];
  uint8_t *sig_mbr;
  mbr_partition_t tmp_mbr,
    *p_part_mbr;
  lim_volinfo_t lvi;
  mbr_partition_type_t type;


  if ((DEVICE_NUM <= dev_id) ||
      (VOLUME_PER_DEVICE_MAX <= part_no) ||
      (8400 /*dsk_table_fat16[0]*/ > count) ||
      (NULL == fs_type))
    {
    set_errno(e_inval);
    return -1;
    }

  if (!stricmp(FSNAME_FAT16, fs_type))
    type = eDOS_FAT16_L32;
  else if (!stricmp(FSNAME_FAT32, fs_type))
    type = eWIN95_FAT32;
  else
    return set_errno(e_inval);

  if (dev->totsect_cnt < (start_sec + count))
    return set_errno(e_outof);

  /* Get LIM information from PIM information. */
  __pim_2_lim(dev_id, GET_PIM_DEV(dev_id), &lvi);

  /* Removable devices don't use MBR. So, Partition doesn't created. */
  if (ePIM_Removable & dev->dev_flag)
    return s_ok;

  /* Adjust sector information at the LIM. */
  __mbr_adjust_lim_info(&lvi, start_sec, count);

  if (s_ok != (rtn = __mbr_read(dev_id, buf_mbr)))
    return set_errno(rtn);

  /* Get address of partition information. */
  sig_mbr = GET_MBR_SIGNATURE(buf_mbr);
  p_part_mbr = (mbr_partition_t *) GET_MBR_PARTITION(buf_mbr, part_no);

  /* Initialize the buffer for the MBR. */
  __init_mbr_partition(&tmp_mbr);
  tmp_mbr.sys_ind = (uint8_t) type;

  /* Write LIM's partition information to the buffer */
  lim_vol_info_2_mbr_partition(&lvi, &tmp_mbr);

  memcpy(p_part_mbr, &tmp_mbr, sizeof (mbr_partition_t));

  sig_mbr[0] = (uint8_t) MBR_SIGNATURE1;
  sig_mbr[1] = (uint8_t) MBR_SIGNATURE2;

  rtn = __mbr_write(dev_id, buf_mbr);

  if (s_ok != rtn)
    return set_errno(rtn);

  return s_ok;
  }

/*
 Name: mbr_load_partition
 Desc: Road MBR data into the lim_volinfo_t structure which includes
       information of LIM.
 Params:
   - dev_id: The ID of device.
   - part_no: The partition number.
   - lvi: Pointer to lim_volinfo_t structure that has LIM information to put
          the MBR data.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: When the memory device dose not support MBR, it dose not operate.
 */

result_t mbr_load_partition(uint32_t dev_id, uint32_t part_no, lim_volinfo_t *lvi)
  {
  pim_devinfo_t *dev = GET_PIM_DEV(dev_id);
  uint8_t buf_mbr[LIM_ALLOW_MAX_SECT_SIZE];
  mbr_partition_t *p_part_mbr;
  int32_t rtn,
    totsect_cnt;


  __pim_2_lim(dev_id, GET_PIM_DEV(dev_id), lvi);

  /* Removable devices don't use MBR. It is started from the sector of fat-format without MBR. */
  if (ePIM_Removable & dev->dev_flag)
    return s_ok;

  if (s_ok != (rtn = __mbr_read(dev_id, buf_mbr)))
    return rtn;

  /* Get the address of partition information as the partition number. */
  p_part_mbr = (mbr_partition_t *) GET_MBR_PARTITION(buf_mbr, part_no);

  memcpy(&totsect_cnt, &(p_part_mbr->totsect_cnt), 4);

  /* Put partition information into the structure which includes information
     of LIM. */
  __mbr_partition_2_lim_vol_info(p_part_mbr, lvi);

  rtn = __mbr_check_mbr_partition(dev_id, buf_mbr, part_no);

  return rtn;
  }

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

