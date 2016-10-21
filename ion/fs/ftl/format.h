#ifndef FORMAT_H
#define FORMAT_H 


#define FTL_DEVICE_NOR                    (10)
#define FTL_DEVICE_NAND                   (11)
#define FTL_DEVICE_TYPE                   FTL_DEVICE_NOR

#define FTL_SUPER_SYSEB_ENABLE            (15)
#define FTL_SUPER_SYSEB_DISABLE           (16)
#define FTL_SUPER_SYSEB_MODE              FTL_SUPER_SYSEB_ENABLE

#define FTL_START_EBLOCK                  (0)
#define NUMBER_OF_DEVICES                 (1)
#define NUMBER_OF_BYTES_PER_SECTOR        (512)
#define NUMBER_OF_SECTORS_PER_PAGE        (4) /* For the option FTL_DEVICE_NAND for FTL_DEVICE_TYPE, the only supported value is 4*/
#define SECTOR_SIZE                       (NUMBER_OF_BYTES_PER_SECTOR)
#define EBLOCK_SIZE                       (0x20000)  /*32K, 16 pages @ 2 K per page*/

#define MANAGED_REGIONS  false
#define NUM_MANAGED_REGIONS (2)

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
#define SPARE_SIZE                        (0)
#else
#define SPARE_SIZE                        (64)
#endif

#define NUMBER_OF_BYTES_PER_PAGE          ((NUMBER_OF_BYTES_PER_SECTOR * NUMBER_OF_SECTORS_PER_PAGE) + SPARE_SIZE)

#define PFT_RECOVERY_RESERVE              (4)
#define NUM_TRANSACTLOG_EBLOCKS       (5) /*should always be more than 2. i.e. 3 or more*/
#define NUM_FLUSH_LOG_EBLOCKS             (2)
#define NUM_SYSTEM_RESERVE_EBLOCK_REQUEST (4)
#define NUMBER_OF_ERASE_BLOCKS            (512)
#define NUM_CHAIN_EBLOCK_REQUEST          (14)
#if(NUM_CHAIN_EBLOCK_REQUEST < PFT_RECOVERY_RESERVE)
  #error "NUM_CHAIN_EBLOCK_REQUEST is less than PFT_RECOVERY_RESERVE"
#endif
#define NUM_CHAIN_EBLOCKS                 (NUM_CHAIN_EBLOCK_REQUEST - PFT_RECOVERY_RESERVE)

#if (FTL_SUPER_SYSEB_MODE == FTL_SUPER_SYSEB_ENABLE)
#define NUM_SUPER_SYS_EBLOCKS             (1)
#define MAX_NUM_SYS_EBLOCKS               (NUM_TRANSACTLOG_EBLOCKS + NUM_FLUSH_LOG_EBLOCKS)
#endif  // #if (FTL_SUPER_SYSEB_MODE == FTL_SUPER_SYSEB_ENABLE)

#define NUM_COMMON_RESERVE_EBLOCKS        (NUM_CHAIN_EBLOCK_REQUEST + NUM_SYSTEM_RESERVE_EBLOCK_REQUEST)

#define MAX_ERASE_COUNT                   (100000)

#define NUMBER_OF_PAGES_PER_EBLOCK        (EBLOCK_SIZE / VIRTUAL_PAGE_SIZE)

#if (FTL_SUPER_SYSEB_MODE == FTL_SUPER_SYSEB_ENABLE)
#define NUMBER_OF_SYSTEM_EBLOCKS          (NUM_TRANSACTLOG_EBLOCKS + NUM_FLUSH_LOG_EBLOCKS + NUM_SUPER_SYS_EBLOCKS + NUM_COMMON_RESERVE_EBLOCKS)
#else
#define NUMBER_OF_SYSTEM_EBLOCKS          (NUM_TRANSACTLOG_EBLOCKS + NUM_FLUSH_LOG_EBLOCKS + NUM_COMMON_RESERVE_EBLOCKS)
#endif

#if (FTL_SUPER_SYSEB_MODE == FTL_SUPER_SYSEB_ENABLE)
#define SUPER_SYS_START_EBLOCKS           (NUMBER_OF_ERASE_BLOCKS - NUMBER_OF_SYSTEM_EBLOCKS)
#endif  // #if (FTL_SUPER_SYSEB_MODE == FTL_SUPER_SYSEB_ENABLE)

/****/
#define VIRTUAL_PAGE_SIZE                 (NUMBER_OF_BYTES_PER_SECTOR * NUMBER_OF_SECTORS_PER_PAGE)
#define NUMBER_OF_DATA_EBLOCKS            (NUMBER_OF_ERASE_BLOCKS - NUMBER_OF_SYSTEM_EBLOCKS)
#define NUM_SYSTEM_EBLOCKS                (NUMBER_OF_SYSTEM_EBLOCKS) //simple rename for keeping system happy
#define NUM_DATA_EBLOCKS                  (NUMBER_OF_DATA_EBLOCKS)
/***/

#define MAX_NUMBER_LBA                    (NUMBER_OF_DEVICES * \
                                           NUMBER_OF_DATA_EBLOCKS * \
                                           NUMBER_OF_PAGES_PER_EBLOCK * \
                                           NUMBER_OF_SECTORS_PER_PAGE)

#endif

