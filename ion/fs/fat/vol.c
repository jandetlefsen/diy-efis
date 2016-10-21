/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "fat.h"
#include "vol.h"
#include "file.h"
#include "misc.h"
#include "chkdsk.h"
#include "log.h"
#include "../lim/lim.h"
#include "../osd/osd.h"
#include "../util/lib.h"

#include <string.h>


/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES
-----------------------------------------------------------------------------*/

/* This structure describes the relation between sector and cluster. */
typedef struct
  {
  uint32_t sect_cnt; /* the number of all sectors */
  uint8_t sect_per_clust; /* the number of sectors per cluster */

  } fat_sect_per_clust;

/*
 *This is the table for FAT16 drives. NOTE that this table includes
 * entries for disk sizes larger than 512 MB even though typically
 * only the entries for disks < 512 MB in size are used.
 * The way this table is accessed is to look for the first entry
 * in the table for which the disk size is less than or equal
 * to the DiskSize field in that table entry. For this table to
 * work properly BPB_RsvdSecCnt must be 1, BPB_NumFATs
 * must be 2, and BPB_RootEntCnt must be 512. Any of these values
 * being different may require the first table entries DiskSize value
 * to be changed otherwise the cluster count may be to low for FAT16.
 */

static fat_sect_per_clust dsk_table_fat16[] = {
#if defined( STRICT_FAT )
  { 8400, 0}, /* up to 4.1MB, the 0 value for trips an error */
  { 32680, 2}, /* up to 16MB */
  { 262144, 4}, /* up to 128MB */
  { 524288, 8}, /* up to 256MB */
  { 1048576, 16}, /* up to 512MB */
                                               /* The entries after this point are not used unless FAT16 is forced. */
  { 2097152, 32}, /* up to 1GB */
  { 4194304, 64}, /* up to 2GB */
  { 0xFFFFFFFF, 0} /* any disk greater than 2GB, 0 value trips an error */
#else
  { 8400, 1}, /* up to 4.1MB */
  { 32680, 2}, /* up to 16MB */
  { 262144, 8}, /* up to 128MB */
  { 524288, 8}, /* up to 256MB */
  { 1048576, 16}, /* up to 512MB */
                                               /* The entries after this point are not used unless FAT16 is forced. */
  { 2097152, 32}, /* up to 1GB */
  { 4194304, 64}, /* up to 2GB */
  { 0xFFFFFFFF, 0} /* any disk greater than 2GB, 0 value trips an error */
#endif
  };

/*
 * This is the table for FAT32 drives. NOTE that this table includes
 * entries for disk sizes smaller than 512 MB even though typically
 * only the entries for disks >= 512 MB in size are used.
 * The way this table is accessed is to look for the first entry
 * in the table for which the disk size is less than or equal
 * to the DiskSize field in that table entry. For this table to
 * work properly BPB_RsvdSecCnt must be 32, and BPB_NumFATs
 * must be 2. Any of these values being different may require the first
 * table entries DiskSize value to be changed otherwise the cluster count
 * may be to low for FAT32.
 */

static fat_sect_per_clust dsk_table_fat32[] = {
#if defined( STRICT_FAT )
  { 66600, 0}, /* up to 32.5MB, the 0 value for trips an error */
  { 532480, 1}, /* up to 260MB */
  { 16777216, 8}, /* up to 8GB */
  { 33554432, 16}, /* up to 16GB */
  { 67108864, 32}, /* up to 32GB */
  { 0xFFFFFFFF, 0} /* any disk greater than 32GB */
#else
  { 66600, 1}, /* up to 32.5MB */
  { 532480, 4}, /* up to 260MB */
  { 16777216, 8}, /* up to 8GB */
  { 33554432, 16}, /* up to 16GB */
  { 67108864, 32}, /* up to 32GB */
  { 0xFFFFFFFF, 64} /* any disk greater than 32GB */
#endif
  };

/* The volume table in the file system. */
fat_volinfo_t fat_vol[VOLUME_NUM];
/* The flag which indicates that volume is initialized or not */
static bool vol_inited;

static uint32_t fat_log2(uint32_t val)
  {
  uint32_t i = 0;

  while ((val & 0x00000001) == 0)
    {
    val >>= 1;
    i++;
    }

  return i;
  }

/*
 Name: __fat_zinit_vol
 Desc: Zero initialize all data.
 Params: None.
 Returns: None.
 Caveats: None.
 */

static void __fat_zinit_vol(void)
  {
  memset(&fat_vol, 0, sizeof (fat_vol));
  memset(&vol_inited, 0, sizeof (vol_inited));
  }

/*
 Name: fat_zero_init
 Desc: Zero initialize all data.
 Params: None.
 Returns: None.
 Caveats: None.
 */

void fat_zero_init(void)
  {
  __fat_zinit_vol();
  fat_zinit_fat();
  fat_zinit_file();
  fat_zinit_dir();
#if defined( LOG )
  fat_zinit_log();
#endif
#if defined( CPATH )
  path_zinit_path();
#endif
  os_zinit_osd();
  lim_zinit_lim();
  pim_zinit_pim();
  }

/*
 Name: __fat_init_vol
 Desc: Initialize the volume table.
 Params: None.
 Returns:
   int32_t  0 always.
 Caveats: None.
 */

static result_t __fat_init_vol(void)
  {
  int32_t i;


  memset(fat_vol, 0, sizeof (fat_vol));

  for (i = 0; i < VOLUME_NUM; i++)
    fat_vol[i].vol_id = -1;

  return s_ok;
  }

/*
 Name: __fat_reinit_vol_cache
 Desc: Initialize caches which will be used in the file system.
 Params:
   - vol_id: The ID of the volume.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: This function initializes all caches in the LIM and FAT layer.
 */

static result_t __fat_reinit_vol_cache(int32_t vol_id)
  {
  int32_t rtn;


  if (false == vol_inited)
    return set_errno(e_noinit);

  rtn = fat_reinit_cache_vol(vol_id);
  if (0 > rtn) return rtn;

  rtn = lim_reinit_cache_vol(vol_id);
  if (0 > rtn) return rtn;

#if defined( CPATH )
  rtn = path_reinit_cache_vol(vol_id);
#endif

  return rtn;
  }

/*
 Name: __fat_get_clust_size
 Desc: Takes the number of sectors per cluster in the whole sector, at the
       table which presents relation between defined sectors and cluster
 Params:
   - table: The table describing the relation between sector and cluster.
   - sect_cnt: The number of all sectors.
 Returns:
   int32_t  >0 on success. The value returned is the number of sectors per cluster.
            =0 on fail. Not found from the table.
 Caveats: None.
 */

static uint8_t __fat_get_clust_size(const fat_sect_per_clust *table, uint32_t sect_cnt)
  {
  while (table->sect_cnt != 0xFFFFFFFF)
    {
    if (table->sect_cnt >= sect_cnt)
      return table->sect_per_clust;

    table++;
    }

  return 0;
  }

/*
 Name: __fat_get_fstype
 Desc: Get the type of file system.
 Params:
   - bs: Pointer to the boot sector.
 Returns:
   fat_type_t  One of the following symbols.
               eFAT32_SIZE - FAT32 file system.
               eFAT16_SIZE - FAT16 file system.
               eFAT_UNKNOWN - Unknown file system.
 Caveats: None.
 */

static fat_type_t __fat_get_fstype(fat_bootsect_t *bs)
  {
#if 1
  if ((0 == bs->c.fatz16 && 0 != bs->u.f32.fatsz32) &&
      (0 == ARR8_2_UINT16(bs->c.rootent_cnt)) &&
      (0 == ARR8_2_UINT16(bs->c.tot_sect16)))
    return eFAT32_SIZE;
  else if ((0 != bs->c.fatz16) &&
           (0 != ARR8_2_UINT16(bs->c.rootent_cnt)))
    return eFAT16_SIZE;
  else
    return eFAT_UNKNOWN;
#else
  uint32_t data_clust_cnt,
    dwTotSect;

  dwTotSect = ARR8_2_UINT16(bs->c.tot_sect16);
  if (0 == dwTotSect)
    dwTotSect = bs->c.tot_sect32;

  data_clust_cnt = cdiv(dwTotSect, bs->c.sect_per_clust);

  if (MS_FAT12_CLUSTER_MAX > data_clust_cnt)
    return eFAT_UNKNOWN;
  else if (MS_FAT16_CLUSTER_MAX > data_clust_cnt)
    return eFAT16_SIZE;
  else
    return eFAT32_SIZE;
#endif
  }

/*
 Name: __fat_setup_bpb
 Desc: Setup the BPB which had FAT volume information.
 Params:
   - fvi: A pointer to a buffer of fat_volinfo_t struct type where volume
          information is returned.
 Returns:
   int32_t =0 on success.
           <0 on fail.
 Caveats: The first important data structure on a FAT volume is called the
          BPB(BIOS parameter block), which is located in the first sector of
          the volume in the resereved region.
 */

static int32_t __fat_setup_bpb(fat_volinfo_t *fvi)
  {
  fat_bootsect_t bs;
  fat32_fsinfo_t f32_info;
  fat_type_t efat_type;
  uint32_t root_sect_cnt,
    total_sect_cnt,
    data_sect_cnt,
    bytes_per_sect;
  uint8_t sectbuf[LIM_ALLOW_MAX_SECT_SIZE],
    *buf;
  int32_t rtn;


  buf = sectbuf;

  /* Read boot sector of volume to buffer. 'fvi' is local variable which is fat_volinfo_t structure. */
  rtn = lim_read_sector(fvi->vol_id, 0, buf, 1);
  if (0 > rtn) return rtn;

  /* Vertify the boot sector. */
  if ((eFAT_FAT_SIG1 != buf[eFAT_FAT_OFFSET_SIG1]) ||
      (eFAT_FAT_SIG2 != buf[eFAT_FAT_OFFSET_SIG2]))
    return set_errno(e_nofmt);

  memcpy(&bs, buf, sizeof (fat_bootsect_t));

  efat_type = __fat_get_fstype(&bs);
  if (eFAT_UNKNOWN == efat_type)
    return set_errno(e_nofmt);

  fvi->br.efat_type = efat_type;

  bytes_per_sect = (uint32_t) ARR8_2_UINT16(bs.c.bytes_per_sect);

  /* Setup the size of the sector and cluster */
  fvi->br.bytes_per_sect = (uint16_t) bytes_per_sect;
  fvi->br.bytes_per_sect_mask = (uint16_t) (bytes_per_sect - 1);
  fvi->br.bits_per_sect = (uint16_t) fat_log2(bytes_per_sect);
  fvi->br.sects_per_clust = bs.c.sect_per_clust;
  fvi->br.sects_per_clust_mask = bs.c.sect_per_clust - 1;
  fvi->br.bits_per_clustsect = (uint16_t) fat_log2(bs.c.sect_per_clust);
  fvi->br.bits_per_clust = fvi->br.bits_per_sect + fvi->br.bits_per_clustsect;
  fvi->br.bytes_per_clust_mask = (1 << fvi->br.bits_per_clust) - 1;
  fvi->br.ents_per_sect = fvi->br.bytes_per_sect / sizeof (fat_dirent_t);

  /* Setup the FAT Region */
  fvi->br.first_fat_sect = bs.c.rsvd_sect_cnt;
  fvi->br.fat_table_cnt = bs.c.fat_table_cnt;

  fvi->br.rootent_cnt = ARR8_2_UINT16(bs.c.rootent_cnt);
  root_sect_cnt = (fvi->br.rootent_cnt * sizeof (fat_dirent_t)) / bytes_per_sect;


  if (eFAT16_SIZE == efat_type)
    {
    memcpy(fvi->name, bs.u.f16.vlabel, eFAT_MAX_LABEL_LEN);

    fvi->br.fat_sect_cnt = bs.c.fatz16;

    fvi->br.first_root_sect = fvi->br.first_fat_sect +
      (fvi->br.fat_table_cnt * fvi->br.fat_sect_cnt);

    fvi->br.first_data_sect = fvi->br.first_fat_sect
      + (fvi->br.fat_sect_cnt * fvi->br.fat_table_cnt)
      + root_sect_cnt;
    }
  else
    { /* eFAT32_SIZE */
    memcpy(fvi->name, bs.u.f32.vlabel, eFAT_MAX_LABEL_LEN);

    fvi->br.fat_sect_cnt = (uint16_t) bs.u.f32.fatsz32;

    fvi->br.rootent_cnt = 0;
    root_sect_cnt = 0;

    fvi->br.first_data_sect = fvi->br.first_fat_sect
      + (fvi->br.fat_sect_cnt * fvi->br.fat_table_cnt);

    fvi->br.first_root_sect = (uint16_t) (((bs.u.f32.root_clust - 2) << fvi->br.bits_per_clustsect)
                                          + fvi->br.first_data_sect);
    }

  fvi->br.last_root_sect = (uint16_t) (fvi->br.first_root_sect + root_sect_cnt - 1);

  fvi->name[eFAT_MAX_LABEL_LEN] = '\0';


  /* Get the total sectors */
  total_sect_cnt = ARR8_2_UINT16(bs.c.tot_sect16);
  if (0 == total_sect_cnt)
    total_sect_cnt = bs.c.tot_sect32;

  data_sect_cnt = total_sect_cnt - ((bs.c.rsvd_sect_cnt
                                     + (fvi->br.fat_table_cnt * fvi->br.fat_sect_cnt)
                                     + root_sect_cnt));

  fvi->br.last_data_sect = fvi->br.first_data_sect + data_sect_cnt - 1;

  fvi->br.data_clust_cnt = data_sect_cnt >> fvi->br.bits_per_clustsect;
  fvi->br.last_data_clust = D_SECT_2_CLUST(fvi, fvi->br.first_data_sect)
    + fvi->br.data_clust_cnt - 1;


  if (eFAT16_SIZE == efat_type)
    {
    fvi->br.srch_free_clust = 2;
    }
  else
    { /* eFAT32_SIZE */
    /* Read FS-Info */
    rtn = lim_read_sector(fvi->vol_id, bs.u.f32.fsinfo, buf, 1);
    if (0 > rtn) return rtn;

    memcpy(&f32_info, buf, sizeof (fat32_fsinfo_t));

    if (((uint32_t) eFAT_LEAD_SIG != f32_info.lead_sig) ||
        ((uint32_t) eFAT_STRUC_SIG != f32_info.struc_sig) ||
        ((uint32_t) eFAT_TRAIL_SIG != f32_info.trail_sig))
      return set_errno(e_nofmt);

    fvi->br.srch_free_clust = f32_info.next_free;
    }

  fvi->br.free_clust_cnt = (uint32_t) eFAT_INVALID_FREECOUNT;

  return s_ok;
  }

/*
 Name: __fat_setup_volinfo
 Desc: Setup the volume's boot sector and initialize the FAT bitmap.
 Params:
   - fvi: A pointer to a buffer of fat_volinfo_t struct type where volume
          information is returned.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: The volume's important information is located in the first(=boot)
          sector of volume. The area is called the BPB(BIOS Parameter Block).
 */

static int32_t __fat_setup_volinfo(fat_volinfo_t *fvi)
  {
  int32_t rtn;


  rtn = __fat_setup_bpb(fvi);
  if (0 > rtn) return rtn;

  /* Initialize bit-map for FAT file system. Refer to chapter related to FAT. */
  return fat_map_init(fvi->vol_id);
  }

/*
 Name: __fat_adjust_data_sect
 Desc: Calcuate the number of reserved sectors in Boot Record. The reserved sectors align
          the first sector of Data area with the first sector of physical block. It improves performance.
 Params:
   - physical_first_fat_sect: FAT area's first physical sector number.
   - fat_table_cnt: Count of fat table(normaly 1 or 2).
   - fat_sect_cnt: Count of sectors in FAT area.
 Returns:
   int32_t      the sector's count of Reserved area to align as the DATA area.
 Caveats: BR increases the number of sectors in Reserved area until DATA area align as the physical block.
 */

static uint32_t __fat_adjust_data_sect(uint32_t physical_first_data_sect,
                                       uint32_t rootent_sect_cnt, uint32_t sects_per_block)
  {
  uint32_t no_align;

  /*
     The first physical sector of Data area is guaranteed that it align as the number of secters per physical block.
     Refer to __mbr_adjust_lim_info().
   */

  fsm_assert1(0 != sects_per_block);

#if ( ALIGN & ALIGN_DATA_SECT )
  physical_first_data_sect += rootent_sect_cnt;
#endif


#if ( ALIGN & (ALIGN_DATA_SECT | ALIGN_ROOT_DIR) )
  /*The reserved sectors align the first sector of Data area with the first sector of physical block.*/
  no_align = sects_per_block - (physical_first_data_sect % sects_per_block);
#else
  /*The reserved sectors align the first sector of Data area with the first sector of physical page.*/
  no_align = ALIGN_PAGE_FIRST_DATA - (physical_first_data_sect % ALIGN_PAGE_FIRST_DATA);
#endif

  return no_align;
  }

/*
 Name: fat_setup
 Desc: Register the functions which will be used in the FAT file system.
 Params:
   - op: Pointer to the FSM file operation.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None
 */

int32_t fat_setup(fsm_op_t *op)
  {
  if (NULL == op)
    return e_noinit;

  /* volume functions */
  op->init = fat_init;
  op->format = fat_format;
  op->mount = fat_mount;
  op->umount = fat_umount;
  op->sync = fat_sync;
  op->statfs = fat_statfs;

  /* directory functions */
  op->mkdir = fat_mkdir;
  op->rmdir = fat_rmdir;
  op->opendir = fat_opendir;
  op->readdir = fat_readdir;
  op->rewinddir = fat_rewinddir;
  op->closedir = fat_closedir;
  op->cleandir = fat_cleandir;
  op->statdir = fat_statdir;

  /* file functions */
  op->access = fat_access;
  op->creat = fat_creat;
  op->open = fat_open;
  op->read = fat_read;
  op->write = fat_write;
  op->lseek = fat_lseek;
  op->fsync = fat_fsync;
  op->close = fat_close;
  op->closeall = fat_closeall;
  op->unlink = fat_unlink;
  op->truncate = fat_truncate;
  op->tell = fat_tell;
  op->rename = fat_rename;
  op->stat = fat_stat;
  op->fstat = fat_fstat;
  op->getattr = fat_getattr;
  op->fgetattr = fat_fgetattr;
  op->setattr = fat_setattr;
  op->fsetattr = fat_fsetattr;

  return s_ok;
  }

/*
 Name: fat_init
 Desc: Initialize the whole configuration for the file system, not each volume.
 Params: None
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None
 */

int32_t fat_init(void)
  {
  int32_t rtn = s_ok;


  if (true == vol_inited)
    return rtn;

  fat_lock();

  rtn = __fat_init_vol();
  if (0 > rtn) goto End;

  rtn = fat_init_cache();
  if (0 > rtn) goto End;

  rtn = fat_init_dir();
  if (0 > rtn) goto End;

  rtn = fat_init_file_entry();
  if (0 > rtn) goto End;

#if defined( CPATH )
  rtn = path_init_cache();
  if (0 > rtn) goto End;
#endif

  vol_inited = true;

End:
  fat_unlock();

  return s_ok;
  }

/*
 Name: fat_format
 Desc: Format the given volume.
 Params:
 - vol_id: The ID of the device to be formatted.
 - label: The label for the file system.
 - flag: The type of file system. The type is one of the following symbols.
         eFAT16_SIZE - FAT16 system.
         eFAT32_SIZE - FAT32 system.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None
 */

int32_t fat_format(int32_t vol_id, const char *label, uint32_t flag)
  {
  pim_devinfo_t *pdi = GET_LIM_DEV(vol_id);
  lim_volinfo_t *lvi = GET_LIM_VOL(vol_id);
  fat_bootsect_t bs;
  fat32_fsinfo_t f32_info;
  fat_dirent_t *rootdir;
  fat_type_t efat_type;
  tm_t *local_time;
  uint32_t totsect_cnt,
    bytes_per_sect,
    rootent_sect_cnt,
    data_sect_cnt,
    clust_cnt,
    fat_sect_cnt,
    tmpval,
    physical_first_fat_sect,
    first_data_sect,
    physical_first_data_sect,
    rsvd_sects,
    sect_offs,
    i;

  time_t time_val;
  uint16_t rootent_cnt;
  uint8_t fat_table_cnt,
    sectbuf[LIM_ALLOW_MAX_SECT_SIZE],
    *buf;
  int32_t rtn;


  fat_lock();

  /* The volume should be initialized by fat_init() function. */
  if (false == vol_inited)
    {
    fat_unlock();
    return set_errno(e_noinit);
    }

  if (flag & eFAT16_SIZE)
    efat_type = eFAT16_SIZE;
  else if (flag & eFAT32_SIZE)
    efat_type = eFAT32_SIZE;
  else
    {
    fat_unlock();
    return set_errno(e_inval);
    }

  switch (lvi->bytes_per_sect)
    {
    case 512: case 1024: case 2048: case 4096:
      break;
    default:
      fat_unlock();
      return set_errno(e_nofmt);
    }


  memset(&bs, 0, sizeof (fat_bootsect_t));

  totsect_cnt = lvi->totsect_cnt;
  bytes_per_sect = lvi->bytes_per_sect;

  /* Setup boot record. 'bs' is local variable which is fat_bootsect_t structure. */
  bs.c.jmp_boot[0] = (uint8_t) eFAT_BRANCH_INST1;
  bs.c.jmp_boot[1] = (uint8_t) eFAT_BRANCH_INST2;
  bs.c.jmp_boot[2] = (uint8_t) eFAT_BRANCH_INST3;

  memcpy(&bs.c.oem_name, FAT_OEM_NAME, 8);
  UINT16_2_ARR8(bs.c.bytes_per_sect, bytes_per_sect);

  /* Determine the number of sectors per one cluster. */
  if (eFAT16_SIZE == efat_type)
    bs.c.sect_per_clust = __fat_get_clust_size(&dsk_table_fat16[0], totsect_cnt);
  else if (eFAT32_SIZE == efat_type)
    bs.c.sect_per_clust = __fat_get_clust_size(&dsk_table_fat32[0], totsect_cnt);

  if (0 == bs.c.sect_per_clust)
    {
    fat_unlock();
    return set_errno(e_inval);
    }

  if ((32 * 1024) < (bs.c.sect_per_clust * bytes_per_sect))
    {
    fat_unlock();
    return set_errno(e_inval);
    }

  bs.c.rsvd_sect_cnt = (uint16_t) 1;
  bs.c.fat_table_cnt = fat_table_cnt = (uint8_t) FAT_TABLE_CNT;

  bs.c.tot_sect16[0] = (uint8_t) 0;
  bs.c.tot_sect16[1] = (uint8_t) 0;
  bs.c.tot_sect32 = totsect_cnt;

  if (eFAT16_SIZE == efat_type)
    {
    /* Determine the size of the root entry at the FAT16. */
    if (FAT_ROOT_DIR_NUM)
      {
      if ((0 > (int32_t) FAT_ROOT_DIR_NUM) ||
          (FAT_ROOT_DIR_NUM * sizeof (fat_dirent_t)) % bytes_per_sect)
        {
        fat_unlock();
        return set_errno(e_inval);
        }

      rootent_cnt = FAT_ROOT_DIR_NUM;
      }
    else
      rootent_cnt = 512;

    rootent_sect_cnt = (rootent_cnt * sizeof (fat_dirent_t)) / bytes_per_sect;

    if (0x10000 > totsect_cnt)
      {
      UINT16_2_ARR8(bs.c.tot_sect16, totsect_cnt);
      bs.c.tot_sect32 = (uint32_t) 0;
      }
    }
  else
    { /* eFAT32_SIZE */
    rootent_cnt = 0;
    rootent_sect_cnt = 0;
    bs.c.rsvd_sect_cnt = (uint16_t) 0x20;
    }

  UINT16_2_ARR8(bs.c.rootent_cnt, rootent_cnt);

  bs.c.media = (uint8_t) eFAT_MediaFixed;
  bs.c.sect_per_track = (uint16_t) 0x3F;
  bs.c.num_head = (uint16_t) 0xFF;
  bs.c.hidd_sect = (uint32_t) 0;

  data_sect_cnt = totsect_cnt - (bs.c.rsvd_sect_cnt + rootent_sect_cnt);

  physical_first_fat_sect = lvi->start_sect + bs.c.rsvd_sect_cnt;

  /* NOTE : Confirm the calculation routine of the fat_sect_cnt necessarily,
            when the size of bytes_per_sect is over 512. */
  if (eFAT16_SIZE == efat_type)
    {
    tmpval = (bs.c.sect_per_clust * (bytes_per_sect / 2)) + fat_table_cnt;
    fat_sect_cnt = data_sect_cnt / tmpval;

    physical_first_data_sect = physical_first_fat_sect + (fat_table_cnt * fat_sect_cnt);
    rsvd_sects = __fat_adjust_data_sect(physical_first_data_sect, rootent_sect_cnt, pdi->sects_per_block);
    bs.c.rsvd_sect_cnt += (uint16_t) rsvd_sects;

    bs.c.fatz16 = (uint16_t) fat_sect_cnt;
    bs.u.f32.fatsz32 = 0;
    }
  else
    { /* eFAT32_SIZE */
    tmpval = (bs.c.sect_per_clust * (bytes_per_sect / 2)) + fat_table_cnt;
    tmpval /= 2;
    fat_sect_cnt = data_sect_cnt / tmpval;

    physical_first_data_sect = physical_first_fat_sect + (fat_table_cnt * fat_sect_cnt);
    rsvd_sects = __fat_adjust_data_sect(physical_first_data_sect, rootent_sect_cnt, pdi->sects_per_block);
    bs.c.rsvd_sect_cnt += (uint16_t) rsvd_sects;

    bs.c.fatz16 = (uint16_t) 0;
    bs.u.f32.fatsz32 = fat_sect_cnt;

    bs.u.f32.ext_flags = 0;
    bs.u.f32.fsver = 0;
    bs.u.f32.root_clust = 2;
    bs.u.f32.fsinfo = 1;
    bs.u.f32.bk_bootsect = 6;
    memset(&bs.u.f32.rsvds, 0, sizeof (bs.u.f32.rsvds));
    }

  data_sect_cnt = totsect_cnt -
    (bs.c.rsvd_sect_cnt + (fat_sect_cnt * fat_table_cnt) + rootent_cnt);
  clust_cnt = data_sect_cnt / bs.c.sect_per_clust;

#if defined( STRICT_FAT )
  if (eFAT16_SIZE == efat_type)
    {
    if (FAT12_CLUSTER_MAX > clust_cnt)
      {
      fat_unlock();
      return set_errno(e_inval);
      }
    }
  else
    { /* eFAT32_SIZE */
    if (FAT16_CLUSTER_MAX > clust_cnt)
      {
      fat_unlock();
      return set_errno(e_inval);
      }
    }
#endif


  time_val = time(0);

  if (eFAT16_SIZE == efat_type)
    {
    if (0xFFF0 < clust_cnt)
      return set_errno(e_port);
    bs.u.f16.drv_num = 0;
    bs.u.f16.rsvd1 = 0;
    bs.u.f16.boot_sig = eFAT_BOOT_SIG;
    memcpy(&bs.u.f16.vol_id, &time_val, sizeof (bs.u.f16.vol_id));
    strncpy((char *) &bs.u.f16.vlabel, label, (uint32_t)sizeof (bs.u.f16.vlabel) - 1);
    memcpy(&bs.u.f16.fstype_str, FAT16_NAME, sizeof (bs.u.f16.fstype_str));
    }
  else
    { /* eFAT32_SIZE */
    if (0xFFFFFFF0 < clust_cnt)
      return set_errno(e_port);
    bs.u.f32.drv_num = 0;
    bs.u.f32.rsvd1 = 0;
    bs.u.f32.boot_sig = (uint8_t) eFAT_BOOT_SIG;
    memcpy(&bs.u.f32.vol_id, &time_val, sizeof (bs.u.f32.vol_id));
    strncpy((char *) &bs.u.f32.vlabel, label, sizeof (bs.u.f32.vlabel) - 1);
    memcpy(&bs.u.f32.fstype_str, FAT32_NAME, sizeof (bs.u.f32.fstype_str));

    memset(&f32_info, 0, sizeof (f32_info));
    f32_info.lead_sig = (uint32_t) eFAT_LEAD_SIG;
    f32_info.struc_sig = (uint32_t) eFAT_STRUC_SIG;
    f32_info.free_cnt = clust_cnt;
    f32_info.next_free = 2;
    f32_info.trail_sig = (uint32_t) eFAT_TRAIL_SIG;
    }


  first_data_sect = bs.c.rsvd_sect_cnt + (fat_table_cnt * fat_sect_cnt) + rootent_sect_cnt;

  /* Erase all sectors in volume */
  if (ePIM_NeedErase & pdi->dev_flag)
    {
    rtn = pim_erase_sector(lvi->dev_id, lvi->start_sect, totsect_cnt);
    if (s_ok != rtn)
      {
      fat_unlock();
      return rtn;
      }
    }


  /* Initialize the FAT area. */
  buf = sectbuf;
  memset(buf, 0, bytes_per_sect);

  sect_offs = bs.c.rsvd_sect_cnt;

  while (fat_table_cnt--)
    {
    if (eFAT16_SIZE == efat_type)
      {
      UINT16_2_ARR8(buf + (0 * eFAT16_SIZE), (uint32_t) eFAT16_EOC);
      UINT16_2_ARR8(buf + (1 * eFAT16_SIZE), (uint32_t) eFAT16_EOC);
      }
    else
      { /* eFAT32_SIZE */
      UINT32_2_ARR8(buf + (0 * eFAT32_SIZE), (uint32_t) eFAT32_EOC);
      UINT32_2_ARR8(buf + (1 * eFAT32_SIZE), (uint32_t) eFAT_EOC);
      UINT32_2_ARR8(buf + (2 * eFAT32_SIZE), (uint32_t) eFAT32_EOC);
      }
    buf[0] = bs.c.media;

    for (i = 0; i < fat_sect_cnt; i++)
      {
      rtn = lim_write_sector(vol_id, sect_offs, buf, 1);
      if (0 > rtn)
        {
        fat_unlock();
        return -1;
        }
      sect_offs++;
      if (0 == i)
        memset(buf, 0, bytes_per_sect);
      }
    }


  memset(buf, 0, bytes_per_sect);

  sect_offs = first_data_sect - rootent_sect_cnt;

  /* Initialize the root-directory to zero. */
  for (i = 0; i < rootent_sect_cnt; i++)
    {
    rtn = lim_write_sector(vol_id, sect_offs, buf, 1);
    if (0 > rtn)
      {
      fat_unlock();
      return -1;
      }
    sect_offs++;
    }

  /* Initialize the first cluster to zero. */
  if (eFAT32_SIZE == efat_type)
    {
    for (i = 0; i < bs.c.sect_per_clust; i++)
      {
      rtn = lim_write_sector(vol_id, sect_offs, buf, 1);
      if (0 > rtn)
        {
        fat_unlock();
        return -1;
        }
      sect_offs++;
      }
    }

  rootdir = (fat_dirent_t*) buf;
  if (0 != label[0])
    {
    time_t now = time(0);
    memcpy(rootdir->name, &label[0], sizeof (rootdir->name));
    rootdir->attr = eFAT_ATTR_VOL;
    local_time = localtime(&now);
    if (NULL == local_time)
      {
      fat_unlock();
      return -1;
      }

    /* Setup the write time. */
    fat_set_ent_time(rootdir, TM_WRITE_ENT);

    /* Calculate the first sector number of root-directory */
    sect_offs = first_data_sect - rootent_sect_cnt;
    rtn = lim_write_sector(vol_id, sect_offs, buf, 1);
    if (0 > rtn)
      {
      fat_unlock();
      return -1;
      }
    }


  /* Write BPB */
  memset(buf, 0, bytes_per_sect);
  memcpy(buf, &bs, sizeof (bs));
  buf[eFAT_FAT_OFFSET_SIG1] = eFAT_FAT_SIG1;
  buf[eFAT_FAT_OFFSET_SIG2] = eFAT_FAT_SIG2;
  buf[bytes_per_sect - 2] = eFAT_FAT_SIG1;
  buf[bytes_per_sect - 1] = eFAT_FAT_SIG2;

  rtn = lim_write_sector(vol_id, 0, buf, 1);
  if (0 > rtn)
    {
    fat_unlock();
    return -1;
    }

  if (eFAT32_SIZE == efat_type)
    {
    rtn = lim_write_sector(vol_id, bs.u.f32.bk_bootsect, buf, 1);
    if (0 > rtn)
      {
      fat_unlock();
      return -1;
      }


    /* Write information of file system. */
    memset(buf, 0, bytes_per_sect);
    memcpy(buf, &f32_info, sizeof (f32_info));
    buf[bytes_per_sect - 2] = eFAT_FAT_SIG1;
    buf[bytes_per_sect - 1] = eFAT_FAT_SIG2;

    rtn = lim_write_sector(vol_id, bs.u.f32.fsinfo, buf, 1);
    if (0 > rtn)
      {
      fat_unlock();
      return -1;
      }
    }

  fat_unlock();
  return s_ok;
  }

/*
 Name: fat_mount
 Desc: Mount the given volume.
 Params:
   - vol_id: The ID of volume to be mounted.
   - falg: The reserved flag.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_mount(int32_t vol_id, uint32_t flag)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  int32_t rtn;


  fat_lock();
  rtn = __fat_reinit_vol_cache(vol_id);
  if (0 > rtn) goto End;
  fat_unlock();

  rtn = fat_reinit_vol_file_entry(vol_id);
  if (0 > rtn) goto End;

  fat_lock();

  fvi->fat_mounted = false;

  /* Initialize volume information. */
  fvi->vol_id = vol_id;
  rtn = __fat_setup_volinfo(fvi);
  if (s_ok != rtn) goto End;

#if defined( LOG )
  rtn = fat_log_init(vol_id);
  if (0 > rtn) goto End;
#endif

#if defined( CHKDISK )
  rtn = fat_chk_init(vol_id);
  if (0 > rtn) goto End;
#endif

  fvi->fat_mounted = true;

  tr_fat_init(vol_id);
  tr_fat_set_log_rcv_cnt(vol_id, rtn);

End:
  fat_unlock();
  return rtn;
  }

/*
 Name: fat_umount
 Desc: Unmount the given volume.
 Params:
   - vol_id: The ID of volume to be unmounted.
   - flag: The reserved flag.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t fat_umount(int32_t vol_id, uint32_t flag)
  {
  fat_volinfo_t *fvi = GET_FAT_VOL(vol_id);
  int32_t rtn;


  /* Close all files */
  rtn = fat_closeall(vol_id);

  fat_lock();

  /* De-initialize fat-map */
  fat_map_deinit(vol_id);

#if defined( CHKDISK )
  rtn |= fat_chk_deinit(vol_id);
#endif

  if (s_ok == rtn)
    {
    fvi->vol_id = -1;
    fvi->fat_mounted = false;
    }

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_sync
 Desc: Synchronize between the cache data and physical data at the given volume.
 Params:
   - vol_id: The ID of volume.
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
 */

int32_t fat_sync(int32_t vol_id)
  {
  int32_t rtn;


  fat_lock();

#if defined( WB )
  rtn = fat_sync_fs_wb(vol_id);
  if (0 > rtn)
    return -1;
#endif

  /* Flush the FAT cache to the physical device. */
  fat_sync_table(vol_id, true);

  /* Flush the LIM cache to the physical device. */
  lim_flush_csectors(vol_id, NULL);

  /* Re-initialize all caches. */
  rtn = __fat_reinit_vol_cache(vol_id);
  if (0 > rtn) return rtn;

  /* Re-initialize the FAT map table for synchronization. */
  rtn = fat_map_reinit(vol_id);

  fat_unlock();
  return rtn;
  }

/*
 Name: fat_statfs
 Desc: Get information of the mounted volume.
 Params:
   - vol_id: The volume ID of the file system which is to obtain information.
   - statbuf: A pointer to a buffer of statfs_t struct type where file
              system status information is returned.
 Returns:
   int32_t  0(=s_ok) always.
 Caveats: None.
 */

int32_t fat_statfs(int32_t vol_id, statfs_t *statbuf)
  {
  fat_volinfo_t *fvi;


  fat_lock();

  fvi = GET_FAT_VOL(vol_id);

  /* Fill the status buffer to the volume's information */
  statbuf->blk_size = 1 << fvi->br.bits_per_clust;
  statbuf->io_size = statbuf->blk_size;
  statbuf->blocks = fvi->br.data_clust_cnt;
  statbuf->free_blks = fvi->br.free_clust_cnt;

  if (eFAT16_SIZE == fvi->br.efat_type)
    statbuf->type = FAT16;
  else /* eFAT32_SIZE */
    statbuf->type = FAT32;

  fat_unlock();
  return s_ok;
  }

/*
 Name: fat_is_system_file
 Desc: Test the given file is the forbid file.
 Params:
   - fe: Ponter to the fat_fileent_t structure.
   - cmp_name: Resolve type of test.
               true: Test a file name.
               false: Test a position of entry.
 Returns:
   bool  true on success.
           false on fail.
 Caveats: None.
 */

bool fat_is_system_file(fat_fileent_t *fe, bool cmp_name)
  {
#if defined( LOG )
  /*
     The chklog-file should not erase.
     So if try chklog-file to erase, return s_ok without unlink operation.
   */
  if (fat_is_log_file(fe, cmp_name))
    return true;
#endif
#if defined( CHKDISK )
  /*
     The chk-file should not erase.
     So if try chk-file to erase, return s_ok without unlink operation.
   */
  if (fat_is_chk_file(fe, cmp_name))
    return true;
#endif

  return false;
  }

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

