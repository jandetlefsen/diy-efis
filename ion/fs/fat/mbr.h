#ifndef __mbr_h__
#define __mbr_h__

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "../../ion.h"
#include "../lim/lim.h"




/*-----------------------------------------------------------------------------
 DEFINE DEFINITIONS
-----------------------------------------------------------------------------*/

/* The following value defined is written in the MBR area. */
#define MBR_LBA_CYLINDER  1023
#define MBR_LBA_HEAD  254
#define MBR_LBA_SECTOR  63




/*-----------------------------------------------------------------------------
 DEFINE STRUCTURES
-----------------------------------------------------------------------------*/

/* The enumuration value for the type of file system. */
typedef enum mbr_partition_type_e {
   eDOS_FAT16_L32 = 0x04,
   eDOS_FAT16_GE32 = 0x06,
   eWIN95_FAT32 = 0x0B

} mbr_partition_type_t;


/* Data structure on the MBR(Master Boot Record). */
typedef struct mbr_partition_s {
   uint8_t boot_ind;       /* boot indicator */
   uint8_t start_head;     /* beginning sector head number */
   uint8_t start_sect_cyl; /* beginning sector (2 high bits of cylinder) */
   uint8_t start_cyl;      /* beginning cylinder (low order bits of cylinder)*/
   uint8_t sys_ind;        /* partition type */
   uint8_t end_head;       /* ending sector head number */
   uint8_t end_sect_cyl;   /* ending sector (2 high bits of cylinder) */
   uint8_t end_cyl;        /* ending cylinder */
   uint32_t start_sect;    /* number of sectors preceding the partition */
   uint32_t totsect_cnt;   /* number of sectors in the partition */

} mbr_partition_t;




/*-----------------------------------------------------------------------------
 DECLARE FUNTION PROTO-TYPE
-----------------------------------------------------------------------------*/

int32_t mbr_creat_partition( uint32_t dev_id, uint32_t part_no, uint32_t start_cyl, uint32_t cnt, const char *fs_type );
int32_t mbr_load_partition( uint32_t dev_id, uint32_t part_no, lim_volinfo_t *lvi );

#endif

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

