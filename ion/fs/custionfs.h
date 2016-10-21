
#if !defined( CUSTH_26052009 )
#define CUSTH_26052009

#define IONFS
#undef SFILEMODE
#undef STRICT_FAT
#undef UNICODE
#define CPATH
#define CDATA
#define LOG
#undef CHKDISK
#define WB
#define FORBID_CHAR

#undef DBG
#undef TRACE
#undef PL_T32
#undef PL_WIN32

#define USE_STD_LIB

#define ALIGN_NONE (1<<1)
#define ALIGN_DATA_SECT (1<<2)
#define ALIGN_ROOT_DIR (1<<3)

#define ALIGN (ALIGN_DATA_SECT)

#define OS_NONE (1<<1)
#define OS_NUCLEUS (1<<2)
#define OS_REX (1<<3)
#define OS_ionOS (1<<4)
#define OS_uCOS_II (1<<5)
#define OS_WIN32 (1<<6)
#define OS_LINUX (1<<7)
#define OS_yourOS (1<<8)

#define OS (OS_yourOS)

#define DEV_OneNAND (1<<1)
#define DEV_NOR (1<<2)
#define DEV_MMC (1<<3)
#define DEV_RAM (1<<4)
#define DEV_NAND (1<<5)
#define DEV_ORNAND (1<<6)
#define DEV_yourDEV (1<<7)

#define DEVS (DEV_NOR)

#define BD_L2P (1<<1)
#define BD_RAM (1<<2)
#define BD_MMC (1<<3)
#define BD_SPANSION (1<<4)
#define BD_yourBD (1<<5)

#define BD (BD_SPANSION)

#define DEVICE_NUM (1)	       /*2BDSOL* change to 2 for 2 BDs */
#define VOLUME_NUM (1)           /*2BDSOL* change to 2 for 2 BDs */

#define FAT_HEAP_SIZE (2*1024)

#define LIM_CACHE_NUM (64)
#define FAT_CACHE_NUM (32)
#define PATH_CACHE_NUM (4)

#define SECTOR_NUM_PER_CACHE (1)

#define FILE_WB_CNT 1
#define FILE_WB_SIZE (16*1024)

#define FAT_MAX_ODIR 10
#define FAT_MAX_FILE (20)
#define FAT_MAX_OFILE (FAT_MAX_FILE+(FAT_MAX_FILE/2))
#define READ_CRITICAL_SECTION

#endif /**/

