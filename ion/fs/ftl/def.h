#ifndef FTL_DEF_H
#define FTL_DEF_H

#include "if_ex.h"
#include "if_in.h"

#define FTL_RELEASE_NAME "FTL Version:"
#define FTL_RELEASE_VERSION "04.00.05"
#define FTL_FLASH_IMAGE_VERSION "01.02.11"
#define FTL_RELEASE_DATE "2015.10.30"
#define FTL_RELEASE_TIME "09.00 PM PST"
//#define FTL_RELEASE_DATE __DATE__
//#define FTL_RELEASE_TIME __TIME__
#define PRINT_VERSION DBG_PrintStringMsg(FTL_RELEASE_NAME " " FTL_RELEASE_VERSION " " FTL_RELEASE_DATE " " FTL_RELEASE_TIME, 0)

#define FTL_BITS_PER_PAGE            (2)
#define FTL_BITS_PER_BYTE            (8)

#define NUM_WORDS_OF_VERSION         (4)
#define TOTAL_BITS_PER_EB_NEEDED     (NUM_PAGES_PER_EB * FTL_BITS_PER_PAGE)
#define NUMBER_OF_BYTES_NEEDED       (TOTAL_BITS_PER_EB_NEEDED / FTL_BITS_PER_BYTE)
#define EBLOCK_MAPPING_TABLE_BIT_MAP_BYTE   (NUMBER_OF_BYTES_NEEDED)
#define NUM_PAGES_PER_EBLOCK         (EBLOCK_SIZE / VIRTUAL_PAGE_SIZE)
#define DEVICE_SIZE_IN_PAGES         (NUM_EBLOCKS_PER_DEVICE * NUM_PAGES_PER_EBLOCK)
#define DEVICE_SIZE_IN_BYTES         (DEVICE_SIZE_IN_PAGES * VIRTUAL_PAGE_SIZE)
#define NUM_SECTORS_PER_PAGE         (VIRTUAL_PAGE_SIZE / SECTOR_SIZE)
#define SYSTEM_START_EBLOCK          (NUM_DATA_EBLOCKS)
#define TRANSACTLOG_START_EBLOCK (SYSTEM_START_EBLOCK + NUM_CHAIN_EBLOCK_REQUEST)
#define FLUSH_LOG_START_EBLOCK       (SYSTEM_START_EBLOCK + NUM_CHAIN_EBLOCK_REQUEST + NUM_TRANSACTLOG_EBLOCKS)
#if (FTL_SUPER_SYS_EBLOCK == true)
#define SUPER_LOG_START_EBLOCK       (SYSTEM_START_EBLOCK + NUM_CHAIN_EBLOCK_REQUEST + NUM_TRANSACTLOG_EBLOCKS + NUM_FLUSH_LOG_EBLOCKS)
#endif
#define TRANSACTLOG_END_EBLOCK   (FLUSH_LOG_START_EBLOCK - 1)
#define FLUSH_LOG_END_EBLOCK         (FLUSH_LOG_START_EBLOCK + NUM_FLUSH_LOG_EBLOCKS - 1)
//#define FLUSH_GC_START_EBLOCK        (NUM_EBLOCKS_PER_DEVICE - NUM_SYSTEM_RESERVE_EBLOCKS) // 2013/05/31 by saito
#define FTL_START_OFFSET             (FTL_START_EBLOCK * EBLOCK_SIZE)
#define NUMBER_OF_PAGES_PER_DEVICE   ((NUMBER_OF_ERASE_BLOCKS - 1) * NUMBER_OF_PAGES_PER_EBLOCK)

#define MAX_TRANSFER_EBLOCKS         (2)
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
#define MAX_BLOCKS_TO_SAVE           (6)
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
#if(NUM_COMMON_RESERVE_EBLOCKS < ((6) + (((MAX_BAD_BLOCK_SANITY_TRIES-1)*3))))
  #error "NUM_COMMON_RESERVE_EBLOCKS is less than MAX_BLOCKS_TO_SAVE"
#endif
#define MAX_BLOCKS_TO_SAVE           ((6) + (((MAX_BAD_BLOCK_SANITY_TRIES-1)*3)))
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
#define MAX_LOG_ENTRIES              ((EBLOCK_SIZE / LOG_ENTRY_DELTA) - 1)
#define TOTAL_BYTES_PER_PAGE         VIRTUAL_PAGE_SIZE

#define NUM_SECTORS_PER_EBLOCK       (NUM_SECTORS_PER_PAGE * NUM_PAGES_PER_EBLOCK)
#define NUM_DEVICES                  NUMBER_OF_DEVICES

#include <stdint.h>
#include <stdbool.h>

#define OLD_SYS_BLOCK_SIGNATURE_SIZE (4)
#define OLD_SYS_BLOCK_SIGNATURE      (0xABCD)
#define FLUSH_DONE_SIGNATURE         (0x12)
#define END_POINT_SIGNATURE          (0x55)
#define FULL_FLUSH_SIGNATURE         (0xAA55)
#define EBLOCK_MAP_TABLE_FLUSH       (0x2)
#define PPA_MAP_TABLE_FLUSH          (0x3)

#define NUM_TRANSFER_MAP_ENTRIES     (NUM_PAGES_PER_EBLOCK+1)  /*715 bytes*//*num entries has to be more than 8, because of transaction log entries*/

#ifndef FTL_DEV
#define FTL_DEV              uint8_t
#endif  // #ifndef FTL_DEV

#define NUM_PAGES_PER_EB             NUM_PAGES_PER_EBLOCK
#define EMPTY_BYTE                   (0xFF)
#define EMPTY_DWORD                  (0xFFFFFFFF)
#define EMPTY_WORD                   (0xFFFF)
#define SECTOR_BIT_SHIFT             (0)
#define PAGE_ADDRESS_BIT_MAP         (0xFFFFFFFF)

#define SYS_EBLOCK_INFO_LOG          (0x01)
#define SYS_EBLOCK_INFO_FLUSH        (0x02)
#if (FTL_SUPER_SYS_EBLOCK == true)
#define SYS_EBLOCK_INFO_SUPER        (0x03)
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
#define INIT_NOT_DONE                (0)
#define INIT_DONE                    (1)
#define INIT_FORMATTED               (2)

#define UPDATED_NOT_DONE             (0)
#define UPDATED_DONE                 (1)

#define DIRTY_BIT                    (1)
#define CLEAN_BIT                    (0)

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  #define LOG_ENTRY_DELTA            (32)
  #if (CACHE_RAM_BD_MODULE == false)
  #define FLUSH_RAM_TABLE_SIZE       (SECTOR_SIZE)
  #define MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK (EBLOCK_SIZE / (SECTOR_SIZE + LOG_ENTRY_DELTA))
  #endif
  #define SPANSCRC32      false
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  #define LOG_ENTRY_DELTA            (512)
#if (CACHE_RAM_BD_MODULE == true)
  #define FLUSH_RAM_TABLE_SIZE       (SECTOR_SIZE - FLUSH_INFO_SIZE - ((NUMBER_OF_PAGES_PER_EBLOCK * PPA_MAPPING_ENTRY_SIZE) - FLUSH_INFO_SIZE))
#else
  #define FLUSH_RAM_TABLE_SIZE       (SECTOR_SIZE - FLUSH_INFO_SIZE)
#endif
  #define MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK (EBLOCK_SIZE / SECTOR_SIZE)
  #define SPANSCRC32      true
#endif

#define SYS_EBLOCK_INFO_GC           (0x03)
#define BLOCK_INFO_EMPTY_PAGE        (0x00)
#define BLOCK_INFO_VALID_PAGE        (0x01)
#define BLOCK_INFO_STALE_PAGE        (0x02)
#define GC_MOVED_PAGE                (1)
#define GC_NOT_MOVED_PAGE            (0)
#define SYS_INFO_SIZE                (16)
#define LOG_ENTRY_SIZE               (16)
#define FLUSH_INFO_SIZE              (16)
#define NUM_LOG_ENTRIES_PER_EBLOCK   (EBLOCK_SIZE / LOG_ENTRY_DELTA)
#if (CACHE_RAM_BD_MODULE == false)
#define MAX_FLUSH_ENTRIES            (((EBLOCK_SIZE / (SECTOR_SIZE + LOG_ENTRY_DELTA)) - 1) * NUM_FLUSH_LOG_EBLOCKS)
#endif
#define NUM_BITS_EB_CHAIN                 (16)
#define EB_LOGICAL_CHAIN_MASK             (0xFFFF0000)
#define EB_PHYSICAL_CHAIN_MASK            (0x0000FFFF)
#define PAGE_CHAINED                      (0xFFFFFFFE)
#define ERASE_STATUS_CLEAR_WORD_MASK      (0x7FFF)
#define ERASE_STATUS_CLEAR_DWORD_MASK     (0xFFFDFFFF)
#define ERASE_STATUS_GET_WORD_MASK        (0x8000)
#define ERASE_COUNT_GET_WORD_DIRTY_MASK   (0xC000)
#define ERASE_COUNT_CLEAR_WORD_DIRTY_MASK (0x3FFF)
#define ERASE_COUNT_DWORD_DIRTY_SHIFT     (1)     
#define ERASE_STATUS_GET_DWORD_MASK       (0x20000) 
#define ERASE_STATUS_DWORD_SHIFT          (17)      
#define ERASE_COUNT_LIMIT                 (100000)
#define CHAIN_FLAG                        (0x8000)
#define DEBUG_FTL_API_ANNOUNCE            (0)
#define FLUSH_NORMAL_MODE                 (1)
#define FLUSH_GC_MODE                     (2)
#define FLUSH_SHUTDOWN_MODE               (3)
#define CLEAR_GC_SAVE_INIT_MODE           (0)
#define CLEAR_GC_SAVE_RUNTIME_MODE        (1)

/*OPTIONAL MODULES*/
#define DEBUG_CHECK_TABLES           (0)
/*END OPTIONAL MODULES*/

#if (1 == NUMBER_OF_SECTORS_PER_PAGE)
  #define SECTOR_BIT_MAP    (0x00000000)
  #define DEVICE_BIT_SHIFT  (0)
#else  // 
  #if (2 == NUMBER_OF_SECTORS_PER_PAGE)
    #define SECTOR_BIT_MAP    (0x00000001)
    #define DEVICE_BIT_SHIFT  (1)
  #else
    #if (4 == NUMBER_OF_SECTORS_PER_PAGE)
      #define SECTOR_BIT_MAP    (0x00000003)
      #define DEVICE_BIT_SHIFT  (2)
    #else
      #if (8 == NUMBER_OF_SECTORS_PER_PAGE)
        #define SECTOR_BIT_MAP    (0x00000007)
        #define DEVICE_BIT_SHIFT  (3)
      #else
        #if (16 == NUMBER_OF_SECTORS_PER_PAGE)
          #define SECTORS_BIT_MAP  (0x000000F)
          #define DEVICE_BIT_SHIFT  (4)
        #else
          #error "NUMBER_OF_SECTORS_PER_PAGE not supported"
        #endif
      #endif
    #endif
  #endif
#endif

#if (1 == NUMBER_OF_DEVICES)
  #define DEVICE_BIT_MAP  (0x00000000 << DEVICE_BIT_SHIFT)
  #define PAGE_ADDRESS_SHIFT  (DEVICE_BIT_SHIFT + 0)
#else
  #if (2 == NUMBER_OF_DEVICES)
    #define DEVICE_BIT_MAP  (0x00000001 << DEVICE_BIT_SHIFT)
    #define PAGE_ADDRESS_SHIFT  (DEVICE_BIT_SHIFT + 1)
  #else
    #if (4 == NUMBER_OF_DEVICES)
      #define DEVICE_BIT_MAP  (0x00000003 << DEVICE_BIT_SHIFT)
      #define PAGE_ADDRESS_SHIFT  (DEVICE_BIT_SHIFT + 2)
    #else
      #if (8 == NUMBER_OF_DEVICES)
        #define DEVICE_BIT_MAP  (0x00000007 << DEVICE_BIT_SHIFT)
        #define PAGE_ADDRESS_SHIFT  (DEVICE_BIT_SHIFT + 3)
      #else
        #if (16 == NUMBER_OF_DEVICES)
          #define DEVICE_BIT_MAP  (0x0000000F) << DEVICE_BIT_SHIFT)
          #define PAGE_ADDRESS_SHIFT  (DEVICE_BIT_SHIFT + 4)
        #else
          #error "NUMBER_OF_DEVICES not supported"
        #endif
      #endif
    #endif
  #endif
#endif

typedef struct _addressStruct{
    FTL_DEV devID;
    uint8_t pageOffset;
    uint32_t logicalPageNum;
} ADDRESS_STRUCT;

typedef  ADDRESS_STRUCT* ADDRESS_STRUCT_PTR;

#define SYS_INFO_DATA_WORDS         ((SYS_INFO_SIZE/2)-1)
#define SYS_INFO_CHECK_WORD         (0)
#define SYS_INFO_DATA_START         (1)
#define NUM_SYS_RESERVED_BYTES      (1)

/**********************FLASH STRUCTURES******************/
typedef struct _sysEblockInfo
{
    uint16_t checkWord;
    uint8_t type;
    uint8_t reserved[NUM_SYS_RESERVED_BYTES];
    uint16_t phyAddrThisEBlock;
    uint16_t checkVersion;
    uint16_t oldSysBlock;
    uint16_t fullFlushSig;
    uint32_t incNumber; /*used as a key*/
} SYS_EBLOCK_INFO, *SYS_EBLOCK_INFO_PTR;
/********************************************************/

typedef struct _logPhyPageLocation
{
    uint16_t logEBNum;
    uint16_t phyEBOffset;
} LOG_PHY_PAGE_LOCATION, *LOG_PHY_PAGE_LOCATPTR;

#define TRANS_LOG_TYPE_A         (0x01)
#define NUM_ENTRIES_TYPE_A       (2)
#define TRANS_LOG_TYPE_B         (0x02)
#define NUM_ENTRIES_TYPE_B       (3)
#define TRANS_LOG_TYPE_C         (0x03)
#define GC_TYPE_A                (0x04)
#define GC_TYPE_B                (0x05)
#define NUM_ENTRIES_GC_TYPE_B    (12)
#define NUM_GC_TYPE_B            (((NUMBER_OF_PAGES_PER_EBLOCK / 8) / NUM_ENTRIES_GC_TYPE_B) + 1)
#define GC_MOVE_BITMAP           (NUM_ENTRIES_GC_TYPE_B * NUM_GC_TYPE_B)
#define TEMP_B_ENTRIES           (((NUM_TRANSFER_MAP_ENTRIES - NUM_ENTRIES_TYPE_A) / NUM_ENTRIES_TYPE_B) + 1)
#define CHAIN_LOG_TYPE           (10)
#define EBSWAP_LOG_TYPE          (11)
#define UNLINK_LOG_TYPE_A1       (12)
#define UNLINK_LOG_TYPE_A2       (13)
#define UNLINK_LOG_TYPE_B        (14)
#define SPARE_LOG_TYPE           (15)

#define MIN_LOG_ENTRIES_NEEDED   (2)

#define LOG_ENTRY_DATA_WORDS     ((LOG_ENTRY_SIZE/2)-1)
#define LOG_ENTRY_CHECK_WORD     (0)
#define LOG_ENTRY_DATA_START     (1)

typedef struct _transLogEntryA
{
    uint16_t checkWord;             /* first word */
    uint8_t type;                   /* second word */
    uint8_t seqNum;                 /*       1 byte*/
    LOG_PHY_PAGE_LOCATION pageLoc[NUM_ENTRIES_TYPE_A]; /* 4*2 = 8 bytes*/
    uint32_t LBA;                   /*       4 bytes*/
} TRANS_LOG_ENTRY_A, *TRANS_LOG_ENTRY_A_PTR; /*Total =16 bytes*/

typedef struct _transLogEntryB
{
    uint16_t checkWord;              /* first word */
    uint8_t type;                    /* second word */
    uint8_t seqNum;                  /*       1 byte*/
    LOG_PHY_PAGE_LOCATION pageLoc[NUM_ENTRIES_TYPE_B];  /*8*2 = 12 bytes*/
} TRANS_LOG_ENTRY_B, *TRANS_LOG_ENTRY_B_PTR;  /*Total=16 bytes*/

#define TRANS_LOG_ENTRY_C_RESERVED  (8)
typedef struct _transLogEntryC
{
    uint16_t checkWord;                 /* first word */
    uint8_t type;                       /* second word */
    uint8_t seqNum;                     /*       1 byte*/
    uint8_t reserved[TRANS_LOG_ENTRY_C_RESERVED];
    uint32_t GCNum;                     /*       4 bytes*/
} TRANS_LOG_ENTRY_C, *TRANS_LOG_ENTRY_C_PTR; /*Total =16 bytes*/

typedef struct _transLogEntry
{
   TRANS_LOG_ENTRY_A entryA;
   TRANS_LOG_ENTRY_B entryB[TEMP_B_ENTRIES];
   TRANS_LOG_ENTRY_C entryC;
} TRANS_LOG_ENTRY, *TRANS_LOG_ENTRY_PTR;

typedef struct _transLogCache
{
   TRANS_LOG_ENTRY_C entryC;
   uint8_t flag;
} TRANS_LOG_CACHE, *TRANS_LOG_CACHE_PTR;

/********************************************************/

#define GC_LOG_ENTRY_A_RESERVED  (4)
typedef struct _GCLogEntryA
{
    uint16_t checkWord;                 /* first word */
    uint8_t type;                       /* second word */
    uint8_t holdForMerge;
    uint8_t reserved[GC_LOG_ENTRY_A_RESERVED];
    uint16_t logicalEBAddr;
    uint16_t reservedEBAddr;
    uint32_t GCNum;
} GC_LOG_ENTRY_A, *GC_LOG_ENTRY_A_PTR;

#define GC_LOG_ENTRY_B_RESERVED  (1)
typedef struct _GCLogEntryB
{
    uint16_t checkWord;               /* first word */
    uint8_t type;                     /* second word */
    uint8_t pageMovedBitMap[NUM_ENTRIES_GC_TYPE_B];
    uint8_t reserved[GC_LOG_ENTRY_B_RESERVED];
} GC_LOG_ENTRY_B, *GC_LOG_ENTRY_B_PTR;

typedef struct _GCLogEntry
{
    GC_LOG_ENTRY_A  partA;
    GC_LOG_ENTRY_B  partB[NUM_GC_TYPE_B];
} GC_LOG_ENTRY, *GC_LOG_ENTRY_PTR;


#define SPARE_LOG_ENTRY_RESERVED  (11)
typedef struct _SpareLogEntry
{
    uint16_t checkWord;             /* first word */
    uint8_t type;                   /* second word */
    uint8_t reserved[SPARE_LOG_ENTRY_RESERVED];
    uint16_t logicalEBNum;
} SPARE_LOG_ENTRY, *SPARE_LOG_ENTRY_PTR; /*Total =16 bytes*/

#define SPARE_INFO_SIZE             (16)
#define SPARE_INFO_DATA_WORDS       ((SPARE_INFO_SIZE/2)-1)
#define SPARE_INFO_CHECK_WORD       (0)
#define SPARE_INFO_DATA_START       (1)

#if (SPANSCRC32 == true)
#define SPARE_INFO_RESERVED  (4)
#else
#define SPARE_INFO_RESERVED  (8)
#endif

typedef struct _SpareInfo
{
    uint16_t checkWord;             /* first word */
    uint16_t pad;                   /* for alignment */
    uint32_t logicalPageAddr;
    #if (SPANSCRC32 == true)
    uint32_t crc32;
    #endif
    uint8_t reserved[SPARE_INFO_RESERVED];
} SPARE_INFO, *SPARE_INFO_PTR; /*Total =16 bytes*/

#if (FTL_SUPER_SYS_EBLOCK == true)
#define NUM_SYS_EB_ENTRY            (5)
#define SYS_EBLOCK_INFO_SYSEB       (0x04)
#define SYS_EBLOCK_INFO_CHANGED     (0x05)
typedef struct _SuperSysInfo
{
    uint16_t checkWord;             /* first word */
    uint8_t type;
    uint8_t decNumber;
    uint16_t PhyEBNum[NUM_SYS_EB_ENTRY];
    uint16_t EntryNumThisIndex;
} SUPER_SYS_INFO, *SUPER_SYS_INFO_PTR; /*Total =16 bytes*/
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
/********************************************************/

#define FLUSH_INFO_DATA_WORDS     ((FLUSH_INFO_SIZE/2)-1)
#define FLUSH_INFO_CHECK_WORD     (0)
#define FLUSH_INFO_DATA_START     (1)
#define FLUSH_INFO_TABLE_CHECK_WORD (5)
#define FLUSH_INFO_TABLE_START      (FLUSH_INFO_SIZE/2)

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
#define SYS_EBLOCK_FLUSH_INFO_RESERVED  (2)
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

typedef struct _sysEBFlushInfo
{
    uint16_t checkWord;            /* first word */
    uint8_t type;                  /* second word */
    uint8_t endPoint;
    uint16_t eBlockNumLoc;
    uint16_t entryIndexLoc;
    uint16_t tableOffset;

    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    uint8_t reserved[SYS_EBLOCK_FLUSH_INFO_RESERVED];

    #elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    uint16_t tableCheckWord;
    #endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

    uint32_t logIncNum;
} SYS_EBLOCK_FLUSH_INFO, *SYS_EBLOCK_FLUSH_INFO_PTR;

#define CHAIN_LOG_ENTRY_RESERVED  (5)
typedef struct _chainLogEntry
{
    uint16_t checkWord;            /* first word */
    uint8_t  type;                 /* second word */
    uint8_t  reserved[CHAIN_LOG_ENTRY_RESERVED];
    uint16_t logicalFrom;
    uint16_t phyFrom;
    uint16_t logicalTo;
    uint16_t phyTo;
} CHAIN_LOG_ENTRY, *CHAIN_LOG_ENTRY_PTR;

#define EBSWAP_LOG_ENTRY_RESERVED  (9)
typedef struct _ebSwapLogEntry
{
    uint16_t checkWord;            /* first word */
    uint8_t  type;                 /* second word */
    uint8_t  reserved[EBSWAP_LOG_ENTRY_RESERVED];
    uint16_t logicalDataEB;
    uint16_t logicalReservedEB;
} EBSWAP_LOG_ENTRY, *EBSWAP_LOG_ENTRY_PTR;

#if(FTL_UNLINK_GC == true)
#define UNLINK_LOG_ENTRY_A_RESERVED   (9)
#define NUM_ENTRIES_UNLINK_TYPE_B     (NUM_ENTRIES_GC_TYPE_B)
#define NUM_UNLINK_TYPE_B             (NUM_GC_TYPE_B)

typedef struct _UnlinkLogEntryA
{
    uint16_t checkWord;                 /* first word */
    uint8_t type;
    uint8_t reserved[UNLINK_LOG_ENTRY_A_RESERVED];
    uint16_t fromLogicalEBAddr;
    uint16_t toLogicalEBAddr;
} UNLINK_LOG_ENTRY_A, *UNLINK_LOG_ENTRY_A_PTR;

typedef GC_LOG_ENTRY_B UNLINK_LOG_ENTRY_B;

typedef struct _UnlinkLogEntry
{
    UNLINK_LOG_ENTRY_A partA;
    UNLINK_LOG_ENTRY_B partB[NUM_UNLINK_TYPE_B];
} UNLINK_LOG_ENTRY, *UNLINK_LOG_ENTRY_PTR;
#endif  // #if(FTL_UNLINK_GC == true)

typedef struct _LogEntryLoc
{
    uint16_t eBlockNum;
    uint16_t entryIndex;
} LOG_ENTRY_LOC, *LOG_ENTRY_LOC_PTR;

/**********************RAM STRUCTURES******************/

#if (NUMBER_OF_PAGES_PER_EBLOCK < 256)
#define PPA_MAPPING_ENTRY            uint8_t
#define EMPTY_INVALID                (EMPTY_BYTE)
#define CHAIN_INVALID                (0xFE)
#define PPA_MASK                     (EMPTY_BYTE)
#define PPA_MAPPING_ENTRY_SIZE       (1)
#else
#define PPA_MAPPING_ENTRY            uint16_t
#define EMPTY_INVALID                (EMPTY_WORD)
#define CHAIN_INVALID                (0xFFFE)
#define PPA_MASK                     (0x7FFF)
#define PPA_MAPPING_ENTRY_SIZE       (2)
#endif

#if (CACHE_RAM_BD_MODULE == true)
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  #if(32 < (PPA_MAPPING_ENTRY_SIZE * NUM_PAGES_PER_EBLOCK))
    #define FLUSH_RAM_TABLE_SIZE     (PPA_MAPPING_ENTRY_SIZE * NUM_PAGES_PER_EBLOCK)
  #else
    #define FLUSH_RAM_TABLE_SIZE     (32)
  #endif
  #define MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK ((EBLOCK_SIZE / (FLUSH_RAM_TABLE_SIZE + LOG_ENTRY_DELTA)))
  #define MAX_FLUSH_ENTRIES            (((EBLOCK_SIZE / (FLUSH_RAM_TABLE_SIZE + LOG_ENTRY_DELTA)) - 1) * NUM_FLUSH_LOG_EBLOCKS)
#else
#define MAX_FLUSH_ENTRIES            (((EBLOCK_SIZE / (SECTOR_SIZE + LOG_ENTRY_DELTA)) - 1) * NUM_FLUSH_LOG_EBLOCKS)
#endif
#endif

// n structures must fit into a sector
#if (0 != (SECTOR_SIZE % PPA_MAPPING_ENTRY_SIZE))
    #error "PPA_MAPPING_ENTRY_SIZE is not valid"
#endif  // #if (0 != (SECTOR_SIZE % PPA_MAPPING_ENTRY_SIZE))

typedef uint8_t FREE_BIT_MAP_TYPE;

#if (FTL_DEFECT_MANAGEMENT == true)
    #define NON_ARRAY_SIZE            (15)
#else
    #define NON_ARRAY_SIZE            (14)
#endif

#define TEMP_EBLOCK_MAPPING_STRUCT_SIZE  (NON_ARRAY_SIZE + EBLOCK_MAPPING_TABLE_BIT_MAP_BYTE)
#if((TEMP_EBLOCK_MAPPING_STRUCT_SIZE % 4) == 0)
#define EBLOCK_MAPPING_TABLE_PAD  (0)
#else
#define EBLOCK_MAPPING_TABLE_PAD  (4 - (TEMP_EBLOCK_MAPPING_STRUCT_SIZE % 4))
#endif

// Note: The following constant must be set to the size of the entries in the
//         EBLOCK_MAPPING_ENTRY that are not arrays
//-- Managed Regions
#if(MANAGED_REGIONS == true)
typedef struct _managedRegions
{
    uint16_t numberEblocksManaged;
    uint32_t baseAddr;
    uint8_t deviceId;
    uint32_t deviceOffset;
} MANAGED_REGIONS_STRUCT, *MANAGED_REGIONS_STRUCT_PTR;

typedef struct _managedRegionsInfo
{
    uint32_t baseAddr;
    uint8_t deviceId;
    uint32_t deviceOffsetBytes;
} MANAGED_REGIONS_INFO_STRUCT, *MANAGED_REGIONS_INFO_STRUCT_PTR;

#endif


typedef struct _eBlockMappingEntry
{
    uint16_t phyEBAddr;
    uint16_t dirtyCount;
    uint32_t freePage_GCNum;  /*flush blocks, freePage, Data Eblocks use it as GC Number*/
    uint32_t chainToFrom;     /*upper 16 bits logical, lower 16 bits physical, the FROM EB has the TO info, the TO EB has the FROM info*/
    uint16_t eraseCount;
    FREE_BIT_MAP_TYPE freeBitMap[EBLOCK_MAPPING_TABLE_BIT_MAP_BYTE];

    #if (EBLOCK_MAPPING_TABLE_PAD != 0)
      uint8_t reserved[EBLOCK_MAPPING_TABLE_PAD];
    #endif
    #if (FTL_DEFECT_MANAGEMENT == true)
      uint8_t isBadBlock;
    #endif
} EBLOCK_MAPPING_ENTRY, *EBLOCK_MAPPING_ENTRY_PTR;

#if (FTL_DEFECT_MANAGEMENT == true)
   #define FTL_SWAP_RESERVE_ERASE               0x10

    typedef struct _badBlockInfo
    {
        FTL_DEV devID;
        uint8_t TransLogEBFailed;
        uint16_t TransLogEBNum;
        uint16_t reserved;
        uint16_t sourceLogicalEBNum;
        uint16_t targetLogicalEBNum;
        uint16_t operation;
        EBLOCK_MAPPING_ENTRY   sourceEBMap;
        EBLOCK_MAPPING_ENTRY   targetEBMap;
        PPA_MAPPING_ENTRY      sourcePPA[NUM_PAGES_PER_EBLOCK];
        PPA_MAPPING_ENTRY      targetPPA[NUM_PAGES_PER_EBLOCK];
    } BAD_BLOCK_INFO, *BAD_BLOCK_INFO_PTR;


    #define FTL_BAD_BLOCK_WRITING           (0x1)
    #define FTL_ERR_DATA_RESERVE            (0x2)
    #define FTL_ERR_CHAIN_FULL_EB           (0x100)
    #define FTL_ERR_CHAIN_NOT_FULL_EB       (0x101)
    #define FTL_ERR_BAD_BLOCK_SOURCE        (0x3)
    #define FTL_ERR_BAD_BLOCK_TARGET        (0x4)
#endif

#define EBLOCK_MAPPING_ENTRY_SIZE (NON_ARRAY_SIZE + EBLOCK_MAPPING_TABLE_BIT_MAP_BYTE + EBLOCK_MAPPING_TABLE_PAD)


#if (CACHE_RAM_BD_MODULE == true)

// defined cache
#define CACHE_EBLOCKMAP (0x0)
#define CACHE_PPAMAP    (0x1)

#define CACHE_FREE      (0x0)
#define CACHE_CLEAN     (0x1)
#define CACHE_DIRTY     (0x2)

#define CACHE_NO_DEPEND      (0x0)
#define CACHE_DEPEND_UP      (0x1)
#define CACHE_DEPEND_DOWN    (0x2)
#define CACHE_DEPEND_UP_DOWN (0x3)

#define CACHE_EBM_PPA_PRESENT (0x0)
#define CACHE_EBM_PRESENT     (0x1)
#define CACHE_NO_PRESENT      (0x2)

#define CACHE_WRITE_TYPE           (0x0)
#define CACHE_READ_TYPE            (0x1)
#define CACHE_INIT_TYPE            (0x2)

#if (FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
#if(EBLOCK_SIZE > 0x1000)
#define CACHE_EMPTY_ENTRY_INDEX     (0xFFF)
#define CACHE_EMPTY_FLASH_LOG_ARRAY (0xF)
#else
#define CACHE_EMPTY_ENTRY_INDEX     (0xFF)
#define CACHE_EMPTY_FLASH_LOG_ARRAY (0xFF)
#endif
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
#define CACHE_EMPTY_ENTRY_INDEX     (0x3FF)
#define CACHE_EMPTY_FLASH_LOG_ARRAY (0x1F)
#endif

#define CACHE_INIT_RAM_MAP_INDEX    (0x3FFFFFFF)
#define CACHE_INIT_EBM_CACHE_INDEX  (0x3000)
#define CACHE_EMPTY_EBM_CACHE_INDEX (0x3FFF)
#define CACHE_SAVE_WL_LOGICAL_MASK  (0xFFFF0000)
#define CACHE_SAVE_WL_LOGICAL_SHIFT (16)
#define CACHE_WL_HIGH               (0x0)
#define CACHE_WL_LOW                (0x1)

// LRU setting 
#define LIMIT_LRU  (0x3F) // can't change value
#define DIVIDE_LRU (0x1) 

// Two cross LEB allows the middle of algorithm, but outside of main interface.
#define NUM_CROSS_LEB  (2)

#if (FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
#if (EBLOCK_MAPPING_ENTRY_SIZE >= 32) // check EBLOCK_MAPPING_ENTRY_SIZE
   #define CACHE_EB_PAD                    (false)
   #define CACHE_EBLOCK_MAPPING_ENTRY_PAD (0)
#else
   #define CACHE_EB_PAD                    (true)
   #define CACHE_EBLOCK_MAPPING_ENTRY_PAD (32 - EBLOCK_MAPPING_ENTRY_SIZE)
#endif
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
#define CACHE_EB_PAD                    (false)
#define CACHE_EBLOCK_MAPPING_ENTRY_PAD (0)
#endif

// offset of PPAMappingCache
#if ((FLUSH_RAM_TABLE_SIZE % (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) == 0)
  #define SAVE_DATA_EB_INDEX (1) // minimum Data EB of index
  #define EBM_ENTRY_COUNT (FLUSH_RAM_TABLE_SIZE / (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD))
  #define CROSS_CASE_ON (false)
#else
  // cross boundary case
  #define SAVE_DATA_EB_INDEX (4) // minimum Data EB
  #define CROSS_CASE_ON (true)
  #define EBM_ENTRY_COUNT ((FLUSH_RAM_TABLE_SIZE / (EBLOCK_MAPPING_ENTRY_SIZE +CACHE_EBLOCK_MAPPING_ENTRY_PAD)) + 1)
#endif
#if ((FLUSH_RAM_TABLE_SIZE % (NUMBER_OF_PAGES_PER_EBLOCK * PPA_MAPPING_ENTRY_SIZE)) == 0)
  #define PPA_ENTRY_COUNT (FLUSH_RAM_TABLE_SIZE / (NUMBER_OF_PAGES_PER_EBLOCK * PPA_MAPPING_ENTRY_SIZE))
#else
  #define PPA_ENTRY_COUNT ((FLUSH_RAM_TABLE_SIZE / (NUMBER_OF_PAGES_PER_EBLOCK * PPA_MAPPING_ENTRY_SIZE)) + 1)
#endif
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
#if ((NUMBER_OF_PAGES_PER_EBLOCK * PPA_MAPPING_ENTRY_SIZE) >= FLUSH_RAM_TABLE_SIZE)
  #if (((EBM_ENTRY_COUNT * (NUMBER_OF_PAGES_PER_EBLOCK * PPA_MAPPING_ENTRY_SIZE)) % FLUSH_RAM_TABLE_SIZE) == 0)
    #define PPA_CACHE_TABLE_OFFSET ((EBM_ENTRY_COUNT * (NUMBER_OF_PAGES_PER_EBLOCK * PPA_MAPPING_ENTRY_SIZE)) / FLUSH_RAM_TABLE_SIZE)
  #else
    #define PPA_CACHE_TABLE_OFFSET ((EBM_ENTRY_COUNT * (NUMBER_OF_PAGES_PER_EBLOCK * PPA_MAPPING_ENTRY_SIZE)) / FLUSH_RAM_TABLE_SIZE) + 1
  #endif
#else
  #define PPA_CACHE_TABLE_OFFSET (1)
#endif
#elif (FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
#if ((EBM_ENTRY_COUNT % PPA_ENTRY_COUNT) == 0)
  #define PPA_CACHE_TABLE_OFFSET (EBM_ENTRY_COUNT / PPA_ENTRY_COUNT)
#else
  #error "Configuration error"
#endif
#endif

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  #define OTHER_RAM_TABLE 8192
#elif (FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  #define OTHER_RAM_TABLE 6144
#else
  #error "FTL_DEVICE_TYPE is not setting" 
#endif

// maximum EBlock/PPAMapIndex
#if (((NUMBER_OF_ERASE_BLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) % FLUSH_RAM_TABLE_SIZE) == 0)
  #define MAX_EBLOCK_MAP_INDEX ((NUMBER_OF_ERASE_BLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) / FLUSH_RAM_TABLE_SIZE)
#else
  #define MAX_EBLOCK_MAP_INDEX (((NUMBER_OF_ERASE_BLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) / FLUSH_RAM_TABLE_SIZE) + 1)
#endif
#define MAX_PPA_MAP_INDEX (MAX_EBLOCK_MAP_INDEX * PPA_CACHE_TABLE_OFFSET)

#define EBLOCKMAPINDEX_SIZE      (2/*uint16_t size*/ * MAX_EBLOCK_MAP_INDEX * NUMBER_OF_DEVICES)
#define PPAMAPINDEX_SIZE         (2/*uint16_t size*/ * MAX_PPA_MAP_INDEX * NUMBER_OF_DEVICES)
#define RAMMAPINDEX_SIZE         (4/*uint32_t size*/ * NUMBER_OF_ERASE_BLOCKS * NUMBER_OF_DEVICES)

#define TEMP_BD_RAM_SIZE (OTHER_RAM_TABLE + EBLOCKMAPINDEX_SIZE + PPAMAPINDEX_SIZE + RAMMAPINDEX_SIZE)

#if (CACHE_DYNAMIC_ALLOCATION == false)
#if (CACHE_MINIMUM_RAM == false && CACHE_MAXIMUM_RAM == false)

#define TEMP_USED_SIZE (CACHE_BD_RAM_SIZE - TEMP_BD_RAM_SIZE)
#define TEMP_COUNT (TEMP_USED_SIZE / ((FLUSH_RAM_TABLE_SIZE * (1/*EBlock*/ + PPA_CACHE_TABLE_OFFSET)) + 2/*ebm entry one size*/))

// Create EBlockMappingCache and PPAMappingCache
#if (((MAX_PPA_MAP_INDEX * FLUSH_RAM_TABLE_SIZE * NUMBER_OF_DEVICES) + (MAX_PPA_MAP_INDEX * FLUSH_RAM_TABLE_SIZE * NUMBER_OF_DEVICES) + (NUMBER_OF_ERASE_BLOCKS * NUMBER_OF_DEVICES * 2/*ebm entry one size*/)) < TEMP_USED_SIZE)
     #define NUM_EBLOCK_MAP_INDEX (MAX_EBLOCK_MAP_INDEX)
     #define NUM_PPA_MAP_INDEX    (NUM_EBLOCK_MAP_INDEX * PPA_CACHE_TABLE_OFFSET)
#else
     #define NUM_EBLOCK_MAP_INDEX (TEMP_COUNT)
     #define NUM_PPA_MAP_INDEX    (TEMP_COUNT * PPA_CACHE_TABLE_OFFSET)
#endif

// Set possible swap area of EBMCacheIndex
#if (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) % FLUSH_RAM_TABLE_SIZE) == 0)
    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    #if (NUM_EBLOCK_MAP_INDEX > ((((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) / FLUSH_RAM_TABLE_SIZE)) + SAVE_DATA_EB_INDEX))
       #define CACHE_INDEX_CHANGE_AREA (NUM_EBLOCK_MAP_INDEX - ((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) / FLUSH_RAM_TABLE_SIZE))
    #endif
    #elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    #if (NUM_EBLOCK_MAP_INDEX > (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE+ CACHE_EBLOCK_MAPPING_ENTRY_PAD)) / FLUSH_RAM_TABLE_SIZE) + 1 + SAVE_DATA_EB_INDEX))
       #define CACHE_INDEX_CHANGE_AREA (NUM_EBLOCK_MAP_INDEX - (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) / FLUSH_RAM_TABLE_SIZE) + 1))
    #endif
    #endif
#else
    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    #if (NUM_EBLOCK_MAP_INDEX > (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE+ CACHE_EBLOCK_MAPPING_ENTRY_PAD)) / FLUSH_RAM_TABLE_SIZE) + 1 + SAVE_DATA_EB_INDEX))
       #define CACHE_INDEX_CHANGE_AREA (NUM_EBLOCK_MAP_INDEX - (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) / FLUSH_RAM_TABLE_SIZE) + 1))
    #endif
    #elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    #if (NUM_EBLOCK_MAP_INDEX > (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE+ CACHE_EBLOCK_MAPPING_ENTRY_PAD)) / FLUSH_RAM_TABLE_SIZE) + 2 + SAVE_DATA_EB_INDEX))
       #define CACHE_INDEX_CHANGE_AREA (NUM_EBLOCK_MAP_INDEX - (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) / FLUSH_RAM_TABLE_SIZE) + 2))
    #endif
    #endif
#endif
#else
    #if (CACHE_MINIMUM_RAM == true)
    #if (FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
       #if ((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) % FLUSH_RAM_TABLE_SIZE) == 0)
          #define NUM_EBLOCK_MAP_INDEX ((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) / FLUSH_RAM_TABLE_SIZE) + SAVE_DATA_EB_INDEX)
          #define CACHE_INDEX_CHANGE_AREA (NUM_EBLOCK_MAP_INDEX - (NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) / FLUSH_RAM_TABLE_SIZE))
       #else
          #define NUM_EBLOCK_MAP_INDEX (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) / FLUSH_RAM_TABLE_SIZE) + 1) + SAVE_DATA_EB_INDEX)
          #define CACHE_INDEX_CHANGE_AREA (NUM_EBLOCK_MAP_INDEX - ((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) / FLUSH_RAM_TABLE_SIZE) + 1))
       #endif
    #elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
       #if ((NUMBER_OF_ERASE_BLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) % FLUSH_RAM_TABLE_SIZE) == 0)

          #if ((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) % FLUSH_RAM_TABLE_SIZE) == 0)
             #define NUM_EBLOCK_MAP_INDEX ((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) / FLUSH_RAM_TABLE_SIZE) + SAVE_DATA_EB_INDEX)
          #else
             #define NUM_EBLOCK_MAP_INDEX (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) / FLUSH_RAM_TABLE_SIZE) + 1) + SAVE_DATA_EB_INDEX)
          #endif
       #else
          #if (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) % FLUSH_RAM_TABLE_SIZE) == 0)
             #define NUM_EBLOCK_MAP_INDEX (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) / FLUSH_RAM_TABLE_SIZE)) + 1 + SAVE_DATA_EB_INDEX)
          #else
             #define NUM_EBLOCK_MAP_INDEX (((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) / FLUSH_RAM_TABLE_SIZE) + 2) + SAVE_DATA_EB_INDEX)
          #endif
       #endif
       #define CACHE_INDEX_CHANGE_AREA (1)
    #endif
       #define NUM_PPA_MAP_INDEX (NUM_EBLOCK_MAP_INDEX * PPA_CACHE_TABLE_OFFSET)
    #elif (CACHE_MAXIMUM_RAM == true)
       #define NUM_EBLOCK_MAP_INDEX (MAX_EBLOCK_MAP_INDEX)
       #define NUM_PPA_MAP_INDEX    (MAX_PPA_MAP_INDEX)
       #if ((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) % FLUSH_RAM_TABLE_SIZE) == 0)
          #define CACHE_INDEX_CHANGE_AREA (NUM_EBLOCK_MAP_INDEX - (NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) / FLUSH_RAM_TABLE_SIZE))
       #else
          #define CACHE_INDEX_CHANGE_AREA (NUM_EBLOCK_MAP_INDEX - ((NUMBER_OF_SYSTEM_EBLOCKS * (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD) / FLUSH_RAM_TABLE_SIZE) + 1))
       #endif
    #endif
#endif
#else 
#define NUM_EBLOCK_MAP_INDEX (numBlockMapIndex)
#define NUM_PPA_MAP_INDEX (numPpaMapIndex)
#define CACHE_INDEX_CHANGE_AREA (cacheIndexChangeArea)
#define THESHOLD_DIRTY_COUNT (thesholdDirtyCount)
#define EBMCACHEINDEX_SIZE (ebmCacheIndexSize)
#define EBLOCKMAPPINGCAACHE_SIZE (eblockMappingCacheSize)
#define PPAMAPPINGCACHE_SIZE (ppaMappingCacheSize)
#endif // // #if (CACHE_DYNAMIC_ALLOCATION == false)

// Theshold dirty count 
#if (CACHE_DYNAMIC_ALLOCATION == false)
#if (CROSS_CASE_ON == false)
    #if(CACHE_INDEX_CHANGE_AREA < 2)
        #define THESHOLD_DIRTY_COUNT (1)
    #else
        #define THESHOLD_DIRTY_COUNT (CACHE_INDEX_CHANGE_AREA - 1)
    #endif
#else
    #if(CACHE_INDEX_CHANGE_AREA < 3)
       #define THESHOLD_DIRTY_COUNT (1)
    #else
       #define THESHOLD_DIRTY_COUNT (CACHE_INDEX_CHANGE_AREA - 2)
    #endif
#endif
#if (THESHOLD_DIRTY_COUNT == 0)
   #error "THESHOLD_DIRTY_COUNT is setting 0"
#endif

// Check ram size
#define EBMCACHEINDEX_SIZE       (2/*uint16_t size*/ * NUM_EBLOCK_MAP_INDEX * NUMBER_OF_DEVICES)
#define EBLOCKMAPPINGCAACHE_SIZE (1/*uint8_t size*/ * NUM_EBLOCK_MAP_INDEX * NUMBER_OF_DEVICES * FLUSH_RAM_TABLE_SIZE)
#define PPAMAPPINGCACHE_SIZE     (1/*uint8_t size*/ *  NUM_PPA_MAP_INDEX * NUMBER_OF_DEVICES * FLUSH_RAM_TABLE_SIZE)

#define TOTAL_BD_RAM_SIZE (TEMP_BD_RAM_SIZE + EBMCACHEINDEX_SIZE + EBLOCKMAPPINGCAACHE_SIZE + PPAMAPPINGCACHE_SIZE)

#if (CACHE_MINIMUM_RAM == false && CACHE_MAXIMUM_RAM == false)
    #ifndef CACHE_INDEX_CHANGE_AREA
    #define CACHE_INDEX_CHANGE_AREA (0)
    #endif
    #if (CACHE_BD_RAM_SIZE < TOTAL_BD_RAM_SIZE || CACHE_INDEX_CHANGE_AREA == 0)
       #error "CACHE_BD_RAM_SIZE is less than minimum TOTAL_BD_RAM_SIZE"
    #endif
#endif

#endif // #if (CACHE_DYNAMIC_ALLOCATION == false)

#if (((MAX_EBLOCK_MAP_INDEX + MAX_PPA_MAP_INDEX) % (MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK - 1)) == 0)
#define MIN_FLUSH_GC_EBLOCKS         ((MAX_EBLOCK_MAP_INDEX + MAX_PPA_MAP_INDEX) / (MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK - 1))
#else
#define MIN_FLUSH_GC_EBLOCKS         (((MAX_EBLOCK_MAP_INDEX + MAX_PPA_MAP_INDEX) / (MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK - 1)) + 1)
#endif
#if(NUM_FLUSH_LOG_EBLOCKS < MIN_FLUSH_GC_EBLOCKS)
  #error "NUM_FLUSH_LOG_EBLOCKS is less than MIN_FLUSH_GC_EBLOCKS"
#endif

#if(NUM_FLUSH_LOG_EBLOCKS > CACHE_EMPTY_FLASH_LOG_ARRAY)
  #error "NUM_FLUSH_LOG_EBLOCKS is more than CACHE_EMPTY_FLASH_LOG_ARRAY"
#endif

#if(NUMBER_OF_SYSTEM_EBLOCKS >= EMPTY_BYTE)
  #error "NUMBER_OF_SYSTEM_EBLOCKS is more than unsinged 8bit"
#endif

#else
#define NUM_BYTES_IN_EBLOCK_MAPPING_TABLE  (EBLOCK_MAPPING_ENTRY_SIZE * NUM_DEVICES * NUM_EBLOCKS_PER_DEVICE)
#define EBLOCK_MAPPING_DEV_TABLE           (EBLOCK_MAPPING_ENTRY_SIZE * NUM_EBLOCKS_PER_DEVICE)
#define TEMP_BITS_EBLOCK_TEMP              (EBLOCK_MAPPING_DEV_TABLE % FLUSH_RAM_TABLE_SIZE)

#if (TEMP_BITS_EBLOCK_TEMP == 0)
#define BITS_EBLOCK_DIRTY_BITMAP_DEV_TABLE (EBLOCK_MAPPING_DEV_TABLE / FLUSH_RAM_TABLE_SIZE)
#else
#define BITS_EBLOCK_DIRTY_BITMAP_DEV_TABLE ((EBLOCK_MAPPING_DEV_TABLE / FLUSH_RAM_TABLE_SIZE) + 1)
#endif

#if ((BITS_EBLOCK_DIRTY_BITMAP_DEV_TABLE % FTL_BITS_PER_BYTE) == 0)
#define EBLOCK_DIRTY_BITMAP_DEV_TABLE_SIZE (BITS_EBLOCK_DIRTY_BITMAP_DEV_TABLE / FTL_BITS_PER_BYTE)
#else
#define EBLOCK_DIRTY_BITMAP_DEV_TABLE_SIZE ((BITS_EBLOCK_DIRTY_BITMAP_DEV_TABLE / FTL_BITS_PER_BYTE) + 1)
#endif

#define NUM_BYTES_IN_PPA_TABLE  (PPA_MAPPING_ENTRY_SIZE * NUM_DEVICES * NUM_EBLOCKS_PER_DEVICE * NUM_PAGES_PER_EBLOCK)
#define PPA_TABLE_DEV_TABLE     (PPA_MAPPING_ENTRY_SIZE * NUM_EBLOCKS_PER_DEVICE * NUM_PAGES_PER_EBLOCK)
#define TEMP_PPA_DIRTY_BIT_TEMP (PPA_TABLE_DEV_TABLE % FLUSH_RAM_TABLE_SIZE)

#if (TEMP_PPA_DIRTY_BIT_TEMP == 0)
#define BITS_PPA_DIRTY_BITMAP_DEV_TABLE  (PPA_TABLE_DEV_TABLE / FLUSH_RAM_TABLE_SIZE)
#else
#define BITS_PPA_DIRTY_BITMAP_DEV_TABLE  ((PPA_TABLE_DEV_TABLE / FLUSH_RAM_TABLE_SIZE) + 1)
#endif

#if ((BITS_PPA_DIRTY_BITMAP_DEV_TABLE % FTL_BITS_PER_BYTE) == 0)
#define PPA_DIRTY_BITMAP_DEV_TABLE_SIZE  (BITS_PPA_DIRTY_BITMAP_DEV_TABLE / FTL_BITS_PER_BYTE)
#else
#define PPA_DIRTY_BITMAP_DEV_TABLE_SIZE  ((BITS_PPA_DIRTY_BITMAP_DEV_TABLE / FTL_BITS_PER_BYTE) + 1)
#endif

#define TOTAL_BITS_DIRTY_BITMAP      (BITS_EBLOCK_DIRTY_BITMAP_DEV_TABLE + BITS_PPA_DIRTY_BITMAP_DEV_TABLE)
#if ((TOTAL_BITS_DIRTY_BITMAP % MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK) == 0)
#define MIN_FLUSH_GC_EBLOCKS         (TOTAL_BITS_DIRTY_BITMAP / (MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK - 1))
#else
#define MIN_FLUSH_GC_EBLOCKS         ((TOTAL_BITS_DIRTY_BITMAP / (MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK - 1)) + 1)
#endif
#if(NUM_FLUSH_LOG_EBLOCKS < MIN_FLUSH_GC_EBLOCKS)
  #error "NUM_FLUSH_LOG_EBLOCKS is less than MIN_FLUSH_GC_EBLOCKS"
#endif

#endif

#if(FTL_DEFECT_MANAGEMENT == true)
#define FLUSH_EBLOCK_RETRIES         (MAX_BAD_BLOCK_SANITY_TRIES)
#define MIN_SYSTEM_RESERVE_EBLOCKS   (FLUSH_EBLOCK_RETRIES * MIN_FLUSH_GC_EBLOCKS)
#else  // #if(FTL_DEFECT_MANAGEMENT == true)
#define MIN_SYSTEM_RESERVE_EBLOCKS   (MIN_FLUSH_GC_EBLOCKS)
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

#if(NUM_SYSTEM_RESERVE_EBLOCK_REQUEST < MIN_SYSTEM_RESERVE_EBLOCKS)
  #error "NUM_SYSTEM_RESERVE_EBLOCK_REQUEST is less than MIN_SYSTEM_RESERVE_EBLOCKS"
#endif

#if(FTL_DEFECT_MANAGEMENT == true)
#define LOG_EBLOCK_RETRIES           (MAX_BAD_BLOCK_SANITY_TRIES)
#define MIN_TRANSACTLOG_EBLOCKS  (1 + LOG_EBLOCK_RETRIES)
#else  // #if(FTL_DEFECT_MANAGEMENT == true)
#define MIN_TRANSACTLOG_EBLOCKS  (1)
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

#if(NUM_TRANSACTLOG_EBLOCKS < MIN_TRANSACTLOG_EBLOCKS)
  #error "NUM_TRANSACTLOG_EBLOCKS is less than MIN_TRANSACTLOG_EBLOCKS"
#endif

#if (FTL_SUPER_SYS_EBLOCK == true)
#define MIN_SUPER_SYS_EBLOCKS (1)
#define MIN_SYSTEM_EBLOCKS           (MIN_TRANSACTLOG_EBLOCKS + MIN_FLUSH_GC_EBLOCKS + MIN_SYSTEM_RESERVE_EBLOCKS + MIN_SUPER_SYS_EBLOCKS)
#else
#define MIN_SYSTEM_EBLOCKS           (MIN_TRANSACTLOG_EBLOCKS + MIN_FLUSH_GC_EBLOCKS + MIN_SYSTEM_RESERVE_EBLOCKS)
#endif

#define MIN_RESERVE_EBLOCKS          (MIN_SYSTEM_EBLOCKS + NUM_CHAIN_EBLOCK_REQUEST)
#if(NUMBER_OF_SYSTEM_EBLOCKS < MIN_RESERVE_EBLOCKS)
  #error "NUMBER_OF_SYSTEM_EBLOCKS is less than MIN_RESERVE_EBLOCKS"
#endif

/**********************************/
typedef FTL_INIT_STRUCT* FTL_INIT_STRUCT_PTR;
/**********************************/

typedef struct _keyTableEntry
{
    uint16_t phyAddr;
    uint16_t logicalEBNum;
    uint32_t key;
    #if (CACHE_RAM_BD_MODULE == true)
    uint8_t cacheNum;
    #endif
} KEY_TABLE_ENTRY;   /*Total 4 bytes*/

typedef struct _gcMerge{
    FTL_DEV DevID;
    uint16_t logicalEBNum;
    uint32_t logicalPageNum;
    uint32_t phyPageNum;
} GC_MERGE_STRUCT;

typedef struct _gcMergeTemp{
    uint8_t  pageOffset;
    uint8_t  numSectors;
} GC_MERGE_TEMP_STRUCT;

typedef struct _GCInfo{
    FTL_DEV devID; /*1*/
    uint16_t logicalEBlock; /*2*/
    GC_MERGE_STRUCT startMerge; /*1, 2, 4, 4*/
    GC_MERGE_STRUCT endMerge; /*1, 2, 4, 4*/
} GC_INFO;

typedef struct _GCSave{
    FTL_DEV devId;
    uint16_t phyEbNum;
} GC_SAVE;

typedef struct _moveEntryT
{
    uint16_t phyFromEBlock;
    uint16_t logicalEBNum;
    uint16_t tableOffset;
    uint8_t  type;
    uint8_t  entryNum;
} FLUSH_MOVE_ENTRY;  /*  8*/

typedef struct _chainInfo
{
    uint8_t isChained;
    FTL_DEV devID;
    uint16_t logChainToEB;
    uint16_t phyChainToEB;
    uint32_t phyPageAddr;
} CHAIN_INFO;  /*  5 */

typedef struct _emptyList
{
    uint16_t logEBNum;
    uint8_t isErased;
    uint32_t eraseScore;
} EMPTY_LIST, *EMPTY_LIST_PTR;  /*  5 */

typedef struct _transferEB{
    FTL_DEV devID;
    uint16_t logicalEBNum;
} TRANSFER_EB, *TRANSFER_EB_PTR;


#if (FTL_SUPER_SYS_EBLOCK == true)
typedef struct _superEBinfo
{
    uint8_t checkChanged;
    uint8_t checkLost;
    uint8_t checkSuperPF;
    uint8_t checkSysPF;
    uint32_t storeFreePage[NUM_SUPER_SYS_EBLOCKS];
} SUPER_EB_INFO;
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

/*FFFFFFFFFLLLLLLLLLLLAAAAAAAAAAAAAAASSSSSSSSSSSSSSSSSSHHHHHHHHHHHHHHHHHHH*/
#define FLASH_STATUS uint32_t
#define BYTES_PER_CL  LOG_ENTRY_DELTA

typedef struct _DeleteInfo
{
   uint8_t devID;
   uint32_t logicalPageAddr;
   uint8_t sector[NUMBER_OF_SECTORS_PER_PAGE];
} DEL_INFO;
typedef struct _staticWLInfo
{
    uint32_t threshold;
    uint32_t count;
    uint32_t staticWLCallCounter;
} STATIC_WL_INFO;

#if(FTL_RPB_CACHE == true)
typedef struct _cacheInfo
{
    uint8_t status;
    uint32_t logicalPageAddr;
} CACHE_INFO;

typedef struct _RPBCache
{
    CACHE_INFO cache;
    uint8_t RPB[VIRTUAL_PAGE_SIZE];
} RPB_CACHE;

typedef struct _RPBCacheReadGroup
{
    uint32_t LBA;
    uint32_t NB;
    uint8_t * byteBuffer;
} RPB_CACHE_READ_GROUP;

enum
{
    CACHE_EMPTY = EMPTY_BYTE,
    CACHE_CLEAN = 0x01,
    CACHE_DIRTY = 0x02
};
#endif  // #if(FTL_RPB_CACHE == true)

#if (CACHE_RAM_BD_MODULE == true)

// For EBlock/PPAMapIndex
typedef struct _cacheInfoEblockPpaMap
{
    uint16_t entryIndex;
    uint8_t flashLogEBArrayCount;
} CACHE_INFO_EBLOCK_PPAMAP, *CACHE_INFO_EBLOCK_PPAMAP_PTR;

// For RamMapIndex
typedef struct _cacheInfoRamMap
{
    uint8_t presentEBM;
    uint8_t presentPPA;
    uint16_t ebmCacheIndex;
    #if (NUMBER_OF_PAGES_PER_EBLOCK < 256)
    uint8_t indexOffset;
    #else
    uint16_t indexOffset;
    #endif
} CACHE_INFO_RAMMAP, *CACHE_INFO_RAMMAP_PTR;

// For EBMCacheIndex
typedef struct _cacheInfoEbmCache
{
    uint8_t cacheStatus;
    uint8_t dependency;
    uint8_t wLRUCount;
    uint8_t rLRUCount;
} CACHE_INFO_EBMCACHE, *CACHE_INFO_EBMCACHE_PTR;


typedef struct _saveStaticWL
{
    uint16_t HighestLogEBNum;
    uint16_t LowestLogEBNum;
    uint32_t HighestCount;
    uint32_t LowestCount;
} SAVE_STATIC_WL;

#define SAVE_CAHIN_VALID_USED_SIZE (6)

typedef struct _saveChainValidUsedPage
{
    uint16_t LogEBNum;
    uint16_t ValidPageCount;
    uint16_t UsedPageCount;
} SAVE_CAHIN_VAILD_USED_PAGE;

#endif // #if (CACHE_RAM_BD_MODULE == true)

#endif  // #ifndef FTL_DEF_H
