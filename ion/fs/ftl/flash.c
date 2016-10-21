#include "common.h"
#include "../lld/lld.h"
#include <stdio.h>

#define NO_DEVICE                    (0xFFFFFFFF)

#ifdef __LLD
  #define FLASH_BYTES_PER_WORD       (LLD_BYTES_PER_OP)
#else
  #define FLASH_BYTES_PER_WORD       (2)
#endif

#define NAND_PAGE_PROGRAM_MAIN       (0)
#define NAND_PAGE_PROGRAM_CONFIRM    (1)

//======= Debugs =========
#define DEBUG_FLASH_ANNOUNCE         (0)
#define DEBUG_FLASH_CHECK_PARAM      (1)
#define DEBUG_FLASH_CHECK_BOUNDARY   (0)

//======= Globals ========
#if defined(__LLD) && (FTL_NON_ALIGNED_BUFFER == true)
uint32_t alignedBuf[NUMBER_OF_BYTES_PER_PAGE/4];
#endif
/* It should be initialized with address of device */
extern uint32_t base_addr_g;

//====== Timer Functions =========

//------ TIME_Get -----
FLASH_STATUS TIME_Get(uint32_t TimerId, uint32_t* TimerValue)
{
    return FLASH_PASS;
}

//------ TIME_TimeMark -----
FLASH_STATUS TIME_TimeMark(uint8_t op, uint8_t flag)
{
    return FLASH_PASS;
}

//====== Memory Functions =========

#if (CACHE_RAM_BD_MODULE == true && CACHE_DYNAMIC_ALLOCATION == true)
void *malloc(uint32_t size)
{
    return malloc(size);
}

void free(void *pointer)
{
    free(pointer);
}
#endif

//======= Internal Functions ========

#ifdef __LLD
//------ FLASH_Read -----
void FLASH_Read(uint32_t baseAddr, uint32_t addr, uint32_t size, uint8_t *dataBuf)
{
    uint32_t wordAddr = 0;                               /*4*/
    uint32_t wordCount = 0;                              /*4*/
    FLASHDATA *flashBuf = NULL;                        /*4*/

    wordAddr = addr / LLD_BYTES_PER_OP;
    wordCount = size / LLD_BYTES_PER_OP;
    flashBuf = (FLASHDATA *)dataBuf;

    while(wordCount)
    {
       *flashBuf = lld_ReadOp((FLASHDATA *)baseAddr, wordAddr);
       flashBuf++;
       wordAddr++;
       wordCount--;
    }
}
#endif
#if(MANAGED_REGIONS == true)
MANAGED_REGIONS_STRUCT DEVICE_MAP[NUM_MANAGED_REGIONS];

//------------ Managed Regions --------------
FTL_STATUS FLASH_GapSupportInit(void)
{
   uint16_t i = 0;
   uint32_t total = 0;
   DEVICE_MAP[0].numberEblocksManaged = 512;
   DEVICE_MAP[0].baseAddr = 0;//platform specific
   DEVICE_MAP[0].deviceId = 0; //platform specific
   DEVICE_MAP[0].deviceOffset = 0;

   DEVICE_MAP[1].numberEblocksManaged = 512;
   DEVICE_MAP[1].baseAddr = 0;//platform specific
   DEVICE_MAP[1].deviceId = 0; //platform specific
   DEVICE_MAP[1].deviceOffset = 0;

   for(i = 0;i<NUM_MANAGED_REGIONS;i++)
   {
      total = DEVICE_MAP[i].numberEblocksManaged;
      DEVICE_MAP[i].baseAddr = FLASH_DeviceBase(DEVICE_MAP[i].deviceId);
   }
   if(total != NUMBER_OF_ERASE_BLOCKS)
   {
      return FTL_ERR_FAIL;
   }
   return FTL_ERR_PASS;
}

uint32_t GetManagedRegionsInfo(uint32_t address, MANAGED_REGIONS_INFO_STRUCT_PTR managedRegionInfoPtr)
{
    int i = 0;
    uint32_t workingAddress = address;

    for(i = 0; i < NUM_MANAGED_REGIONS; i++)
    {
       if(address <= (uint32_t)(DEVICE_MAP[i].numberEblocksManaged * EBLOCK_SIZE))
       {
          managedRegionInfoPtr->baseAddr = DEVICE_MAP[i].baseAddr;
          managedRegionInfoPtr->deviceId = DEVICE_MAP[i].deviceId;  
          managedRegionInfoPtr->deviceOffsetBytes = address;    
          return FTL_ERR_PASS;
       }
       workingAddress = workingAddress - (DEVICE_MAP[i].numberEblocksManaged*EBLOCK_SIZE);
       if(i == NUM_MANAGED_REGIONS)
       {
          managedRegionInfoPtr->baseAddr = EMPTY_DWORD;
          managedRegionInfoPtr->deviceOffsetBytes = EMPTY_DWORD;     
          managedRegionInfoPtr->deviceId = EMPTY_BYTE;
          return FTL_ERR_FAIL;
       }
    }
    if(i != NUMBER_OF_ERASE_BLOCKS)
    {
       return FTL_ERR_FAIL;
    }
    return FTL_ERR_PASS;
}
#endif

//------ FLASH_GetAddress -----
uint32_t FLASH_GetAddress(uint32_t pageAddr, uint16_t pageOffset)
{
    uint32_t addr;
    uint32_t blockNum;
    uint32_t pageNum;

    #if (INVERT_FLASH_MMAP == true)
    pageAddr =  CALC_MixPageAddress(pageAddr);
    #endif

    blockNum = FTL_START_EBLOCK + (pageAddr / NUMBER_OF_PAGES_PER_EBLOCK);
    pageNum = pageAddr % NUMBER_OF_PAGES_PER_EBLOCK;
    addr = (blockNum * NUMBER_OF_PAGES_PER_EBLOCK + pageNum) * NUMBER_OF_BYTES_PER_PAGE + pageOffset;

    return addr;
}

//------ FLASH_ReadData -----
FLASH_STATUS FLASH_ReadData(FLASH_PAGE_INFO *pageInfo, uint8_t *dataBuf)
{
    uint32_t addr = 0;                                   /*4*/
    #if defined(HSSIMU) || defined(__LLD)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    uint32_t startSector = 0;                            /*4*/
    uint32_t numSectors = 0;                             /*4*/
    uint32_t sectorCount = 0;                            /*4*/
    #if (FTL_HAMMING_ECC == true)
    uint8_t eccCodes[3];                                /*3*/
    uint16_t correctLoc = 0;                             /*2*/
    #else  /* NLLD_BCH_ECC */
    uint8_t eccData[TOTAL_LENGTH_BYTE];                  /*524*/
    uint32_t errNum;                                     /*4*/
    uint8_t eccOffset;                                   /*1*/
    #endif
    nand_lld_status nlld_status = NLLD_SUCCESS;        /*4*/
    #endif
    #elif defined(__SLLD)
    SLLD_STATUS slld_status = SLLD_OK;                 /*4*/
    #elif defined(LINUX_MTD)
    size_t mtd_retlen = 0;                             /*4*/
    #elif defined(NAND_LLD)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    nand_lld *pDevice = NULL;                          /*4*/
    uint32_t startSector = 0;                            /*4*/
    uint32_t numSectors = 0;                             /*4*/
    uint32_t sectorCount = 0;                            /*4*/
    #if (FTL_HAMMING_ECC == true)
    uint8_t eccCodes[3];                                /*3*/
    uint16_t correctLoc = 0;                             /*2*/
    #else /* NLLD_BCH_ECC */
    uint8_t eccData[TOTAL_LENGTH_BYTE];                  /*524*/
    uint32_t errNum;                                     /*4*/
    uint8_t eccOffset;                                   /*1*/
    #endif
    nand_lld_status nlld_status = NLLD_SUCCESS;        /*4*/
    dev_address nand_addr;
    #endif
    #if(MANAGED_REGIONS == true)
    MANAGED_REGIONS_INFO_STRUCT managedInfo;
    FTL_STATUS managedStatus = FTL_ERR_PASS;
    #endif
    #if defined(HSSIMU) || defined(__LLD) || defined(NAND_LLD)
    baseAddr = FLASH_DeviceBase(pageInfo->devID);
    if(baseAddr == NO_DEVICE)
    {
       return FLASH_PARAM;
    }
    #endif

    addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset);
    #if(MANAGED_REGIONS == true)
    managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
    baseAddr = managedInfo.baseAddr;
    addr = managedInfo.deviceOffsetBytes;
    #endif
    #if defined(HSSIMU)
    hssim_Read_B((uint16_t *)baseAddr, addr, dataBuf, pageInfo->byteCount);

    #elif defined(__LLD)
    FLASH_Read(baseAddr, addr, pageInfo->byteCount, dataBuf);

    #elif defined(__SLLD)
    slld_status = slld_ReadOp(addr, dataBuf, pageInfo->byteCount);
    if(slld_status != SLLD_OK)
    {
       DBG_Printf("FLASH_ReadData: ERROR slld_status %d\n", slld_status, 0);
       return FLASH_FAIL;
    }

    #elif defined(LINUX_MTD)
    (*(dev->mtd->read))(dev->mtd, addr, pageInfo->byteCount, &mtd_retlen, dataBuf);
    if(pageInfo->byteCount != mtd_retlen)
    {
       DBG_Printf("FLASH_ReadData: ERROR mtd->read %d, ", mtd_retlen, 0);
       DBG_Printf("addr %d, ", addr, 0);
       DBG_Printf("byteCount %d\n", pageInfo->byteCount, 0);
       return FLASH_FAIL;
    }

    #elif defined(NAND_LLD)
    pDevice = (nand_lld *)baseAddr;
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
    if(pDevice->geoInfo.totalBlocks <= (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
    {
       pDevice = (nand_lld *)pNandLLDDev1;
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), pageInfo->vPage.pageOffset);
    }
    #endif
    nand_addr.ColAddr = addr % pDevice->geoInfo.pageSize;
    nand_addr.RowAddr = addr / pDevice->geoInfo.pageSize;
    if(pDevice->geoInfo.dataWidth == NAND_16BIT_DATA) nand_addr.ColAddr >>= 1;
    nlld_status = nlld_readPartialPage(pDevice, nand_addr, pageInfo->byteCount, dataBuf);
    if(nlld_status != NLLD_SUCCESS)
    {
       DBG_Printf("FLASH_ReadData: ERROR nlld_status %d\n", nlld_status, 0);
       return FLASH_FAIL;
    }
    #endif

    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
    #if defined(NAND_LLD)
    if(pDevice->geoInfo.totalBlocks > (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
    {
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, VIRTUAL_PAGE_SIZE);
    }else{
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), VIRTUAL_PAGE_SIZE);
    }
    #endif
    #else
    addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, VIRTUAL_PAGE_SIZE);
    #endif    
    #if(MANAGED_REGIONS == true)
    managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
    baseAddr = managedInfo.baseAddr;
    addr = managedInfo.deviceOffsetBytes;
    #endif
    #if defined(HSSIMU)
    hssim_Read_B((uint16_t *)baseAddr, addr, spareBuf, MAX_ECC_SIZE);

    #elif defined(NAND_LLD)
    nand_addr.ColAddr = addr % pDevice->geoInfo.pageSize;
    if(pDevice->geoInfo.dataWidth == NAND_16BIT_DATA) nand_addr.ColAddr >>= 1;
    nlld_status = nlld_randomDataOutput(pDevice, nand_addr.ColAddr, spareBuf, MAX_ECC_SIZE);
    if(nlld_status != NLLD_SUCCESS)
    {
       DBG_Printf("FLASH_ReadData: ERROR nlld_status %d\n", nlld_status, 0);
       return FLASH_FAIL;
    }
    #endif

    startSector = pageInfo->vPage.pageOffset / NUMBER_OF_BYTES_PER_SECTOR;
    numSectors = pageInfo->byteCount / NUMBER_OF_BYTES_PER_SECTOR;
    for(sectorCount = 0; sectorCount < numSectors; sectorCount++)
    {
       if(FLASH_CheckEmpty(&dataBuf[(sectorCount * NUMBER_OF_BYTES_PER_SECTOR)], NUMBER_OF_BYTES_PER_SECTOR) != FLASH_PASS)
       {
          #if (FTL_HAMMING_ECC == true)
          nlld_calEcc(&dataBuf[(sectorCount * NUMBER_OF_BYTES_PER_SECTOR)],512, eccCodes);      
          nlld_status = nlld_correctUseEcc(&dataBuf[(sectorCount * NUMBER_OF_BYTES_PER_SECTOR)], 512, &spareBuf[DATA_ECC_OFFSET[startSector + sectorCount]], eccCodes, &correctLoc, 1);
          if (nlld_status == NLLD_ECC_ERROR_UNCORRECTABLE)
          {
             DBG_Printf("FLASH_ReadData: ERROR ECC nlld_status %d\n", nlld_status, 0);
             return FLASH_ECC_FAIL;
          }
          #else  /* NLLD_BCH_ECC */
          memcpy(eccData, &dataBuf[(sectorCount * NUMBER_OF_BYTES_PER_SECTOR)], DATA_LENGTH_BYTE);
          memcpy(&eccData[DATA_LENGTH_BYTE], &spareBuf[DATA_ECC_OFFSET[startSector + sectorCount]], PARA_LENGTH_BYTE);
          for(eccOffset = 0; eccOffset < PARA_LENGTH_PADDING_BYTE; eccOffset++) /* padding 3 bytes */
          {
             eccData[(DATA_LENGTH_BYTE + PARA_LENGTH_BYTE) + eccOffset] = EMPTY_BYTE;
          }
          nlld_status = nlld_bch_decode(eccData, &errNum, ECC_MODE_FIX);
          if (nlld_status == NLLD_BCH_ERROR_NOT_CORRECTABLE)
          {
             DBG_Printf("FLASH_ReadData: ERROR ECC nlld_status %d\n", nlld_status, 0);
             return FLASH_ECC_FAIL;
          }
          memcpy(&dataBuf[(sectorCount * NUMBER_OF_BYTES_PER_SECTOR)], eccData, DATA_LENGTH_BYTE);
          #endif  /* #if (FTL_HAMMING_ECC == true) */
       }
    }
    #endif

    return FLASH_PASS;
}

//------ FLASH_WriteData -----
FLASH_STATUS FLASH_WriteData(FLASH_PAGE_INFO *pageInfo, uint8_t *dataBuf, uint8_t mode)
{
    uint32_t addr = 0;                                   /*4*/
    #if defined(HSSIMU)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    uint32_t sectorCount = 0;                            /*4*/
    uint32_t startSector = 0;                            /*4*/
    uint32_t numSectors = 0;                             /*4*/
    #endif
    #elif defined(__LLD)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    uint32_t wordAddr = 0;                               /*4*/
    uint32_t wordCount = 0;                              /*4*/
    FLASHDATA *flashBuf = NULL;                        /*4*/
    DEVSTATUS dev_status = DEV_NOT_BUSY;               /*4*/
    #elif defined(__SLLD)
    SLLD_STATUS slld_status = SLLD_OK;                 /*4*/
    DEVSTATUS dev_status = dev_not_busy;               /*4*/
    #elif defined(LINUX_MTD)
    size_t mtd_retlen = 0;                             /*4*/
    #elif defined(NAND_LLD)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    nand_lld *pDevice = NULL;                          /*4*/
    uint32_t sectorCount = 0;                            /*4*/
    uint32_t startSector = 0;                            /*4*/
    uint32_t numSectors = 0;                             /*4*/
    nand_lld_status nlld_status = NLLD_SUCCESS;        /*4*/
    dev_address nand_addr;
    #endif
    #if(MANAGED_REGIONS == true)
    MANAGED_REGIONS_INFO_STRUCT managedInfo;
    FTL_STATUS managedStatus = FTL_ERR_PASS;
    #endif
    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    #if (FTL_BCH_ECC == true)
    uint8_t eccData[TOTAL_LENGTH_BYTE];                 /*524*/
    #endif
    #endif

    #if defined(HSSIMU) || defined(__LLD) || defined(NAND_LLD)
    baseAddr = FLASH_DeviceBase(pageInfo->devID);
    if(baseAddr == NO_DEVICE)
    {
       return FLASH_PARAM;
    }
    #endif

    addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset);
    #if(MANAGED_REGIONS == true)
    managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
    baseAddr = managedInfo.baseAddr;
    addr = managedInfo.deviceOffsetBytes;
    #endif

    #if defined(HSSIMU)
    hssim_Write_B((uint16_t *)baseAddr, addr, dataBuf, pageInfo->byteCount);

    #elif defined(__LLD)
    #if(FTL_NON_ALIGNED_BUFFER == true)
    if(((uint32_t)dataBuf % LLD_BYTES_PER_OP) != 0)
    {
       memcpy((uint8_t *)&alignedBuf[0], dataBuf, pageInfo->byteCount);
       flashBuf = (FLASHDATA *)alignedBuf;
    }
    else
    {
       flashBuf = (FLASHDATA *)dataBuf;
    }
    #else

    flashBuf = (FLASHDATA *)dataBuf;
    #endif

    wordAddr = addr / LLD_BYTES_PER_OP;
    wordCount = pageInfo->byteCount / LLD_BYTES_PER_OP;
    dev_status = lld_memcpy((FLASHDATA *)baseAddr, wordAddr, wordCount, flashBuf);
    if(dev_status != DEV_NOT_BUSY)
    {
       DBG_Printf("FLASH_WriteData: ERROR: dev_status %d\n", dev_status, 0);
       return FLASH_PROGRAM_FAIL;
    }

    #elif defined(__SLLD)
    slld_status = slld_WriteOp(addr, dataBuf, pageInfo->byteCount, &dev_status);
    if(slld_status != SLLD_OK)
    {
       DBG_Printf("FLASH_WriteData: ERROR slld_status %d\n", slld_status, 0);
       return FLASH_PROGRAM_FAIL;
    }

    #elif defined(LINUX_MTD)
    (*(dev->mtd->write))(dev->mtd, addr, pageInfo->byteCount, &mtd_retlen, dataBuf);
    if(pageInfo->byteCount != mtd_retlen)
    {
       DBG_Printf("FLASH_WriteData: ERROR mtd->write %d, ", mtd_retlen, 0);
       DBG_Printf("addr %d, ", addr, 0);
       DBG_Printf("byteCount %d\n", pageInfo->byteCount, 0);
       return FLASH_PROGRAM_FAIL;
    }

    #elif defined(NAND_LLD)
    pDevice = (nand_lld *)baseAddr;
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
    if(pDevice->geoInfo.totalBlocks <= (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
    {
       pDevice = (nand_lld *)pNandLLDDev1;
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), pageInfo->vPage.pageOffset);
    }
    #endif
    nand_addr.ColAddr = addr % pDevice->geoInfo.pageSize;
    nand_addr.RowAddr = addr / pDevice->geoInfo.pageSize;
    if(pDevice->geoInfo.dataWidth == NAND_16BIT_DATA) nand_addr.ColAddr >>= 1;
    nlld_status = nlld_randomDataInputStart(pDevice, nand_addr, dataBuf, pageInfo->byteCount);
    if(nlld_status != NLLD_SUCCESS)
    {
       DBG_Printf("FLASH_WriteData: ERROR: nlld_status %d\n", nlld_status, 0);
       return FLASH_PARAM;
    }
    #endif

    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    if(pageInfo->byteCount >= NUMBER_OF_BYTES_PER_SECTOR)
    {
       startSector = pageInfo->vPage.pageOffset / NUMBER_OF_BYTES_PER_SECTOR;
       for(sectorCount = 0; sectorCount < MAX_ECC_SIZE; sectorCount++)
       {
          spareBuf[sectorCount] = 0xFF;
       }

       numSectors = pageInfo->byteCount / NUMBER_OF_BYTES_PER_SECTOR;
       for(sectorCount = 0; sectorCount < numSectors; sectorCount++)
       {
          #if (FTL_HAMMING_ECC == true)
          nlld_calEcc(&dataBuf[(sectorCount * NUMBER_OF_BYTES_PER_SECTOR)], 512, &spareBuf[DATA_ECC_OFFSET[startSector + sectorCount]]);
          #else  /* NLLD_BCH_ECC */
          memcpy(eccData, &dataBuf[(sectorCount * NUMBER_OF_BYTES_PER_SECTOR)], DATA_LENGTH_BYTE);
          nlld_bch_encode(eccData);
          memcpy(&spareBuf[DATA_ECC_OFFSET[startSector + sectorCount]], &eccData[DATA_LENGTH_BYTE], PARA_LENGTH_BYTE);
          #endif  /* #if (FTL_HAMMING_ECC == true) */
       }

       #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
       #if defined(NAND_LLD)
       if(pDevice->geoInfo.totalBlocks > (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
       {
          addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, VIRTUAL_PAGE_SIZE);
       }else{
          addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), VIRTUAL_PAGE_SIZE);
       }
       #endif
       #else
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, VIRTUAL_PAGE_SIZE);
       #endif    
       #if(MANAGED_REGIONS == true)
       managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
       baseAddr = managedInfo.baseAddr;
       addr = managedInfo.deviceOffsetBytes;
       #endif
       #if defined(HSSIMU)
       hssim_Write_B((uint16_t *)baseAddr, addr, spareBuf, MAX_ECC_SIZE);

       #elif defined(NAND_LLD)
       addr = addr % pDevice->geoInfo.pageSize; // Get ColAddr
       if(pDevice->geoInfo.dataWidth == NAND_16BIT_DATA) addr >>= 1;
       nlld_status = nlld_randomDataInput(pDevice, addr, spareBuf, MAX_ECC_SIZE);
       if(nlld_status != NLLD_SUCCESS)
       {
          DBG_Printf("FLASH_WriteData: ERROR nlld_status %d\n", nlld_status, 0);
          return FLASH_PARAM;
       }
       #endif
    }
    #if defined(NAND_LLD)
    if(mode == NAND_PAGE_PROGRAM_CONFIRM)
    {
       nlld_status = nlld_programConfirm(pDevice, NLLD_API_OP);
       if(nlld_status != NLLD_SUCCESS)
       {
          DBG_Printf("FLASH_WriteData: ERROR nlld_status %d\n", nlld_status, 0);
          return FLASH_PROGRAM_FAIL;
       }
    }
    #endif
    #endif

    return FLASH_PASS;
}

//======= Flash Functions ========
// For the 2 block driver solution, the two copies of the function need to return their approproate addresses
//-------- FLASH_DeviceBase --------
uint32_t FLASH_DeviceBase(uint8_t devID)
{
    if(devID == 0)
    {
       #if defined(HSSIMU) || defined(__LLD)
       return (uint32_t)base_addr_g;

       #elif defined(NAND_LLD)
       return (uint32_t)pNandLLDDev0;

       #else
       /* no device */
       return NO_DEVICE;
       #endif
    }
    else
    {
       /* no device */
       return NO_DEVICE;
    }
}

//------ FLASH_Init -----
FLASH_STATUS FLASH_Init(void)
{
    uint8_t devCount = 0;                                /*1*/

    #if(FTL_DEBUG_FLASH_USAGE == true)
    uint16_t eblockCount;
    #endif

    #if defined(__LLD)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    #ifdef LLD_COMBINED_BUILD
    uint32_t deviceType = 0;                         /*4*/
    #endif
    #elif defined(NAND_LLD)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    nand_lld *pDevice = NULL;                          /*4*/
    nand_lld_status nlld_status = NLLD_SUCCESS;        /*4*/
    #if (FTL_CHECK_BAD_BLOCK_LIMIT == true)
    uint8_t pageBuf[NUMBER_OF_BYTES_PER_PAGE];            /*2*/
    #endif
    #endif

    #if(DEBUG_FLASH_ANNOUNCE == 1)
    DBG_Printf("FLASH_Init: \n", 0, 0);
    #endif  // #if(DEBUG_FLASH_ANNOUNCE == 1)

    #if(MANAGED_REGIONS == true)
    if( FLASH_GapSupportInit() == FTL_ERR_FAIL)
    {
       return FLASH_PARAM;
    }
    #endif

    #if defined(NAND_LLD)
    pNandLLDDev0 = nlld_createInstance((u8 *)memBufDev0, NLLD_HEAP_SIZE);
    if(pNandLLDDev0 == NULL)
    {
       return FLASH_PARAM;
    }
    nlld_status = nlld_configHardware(pNandLLDDev0, base_addr_g, base_addr_g, base_addr_g);
    if(nlld_status != NLLD_SUCCESS)
    {
       return FLASH_PARAM;
    }
    nlld_status = nlld_probeDevice(pNandLLDDev0);
    if(nlld_status != NLLD_SUCCESS)
    {
       return FLASH_PARAM;
    }
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
    pNandLLDDev1 = nlld_createInstance((u8 *)memBufDev1, NLLD_HEAP_SIZE);
    if(pNandLLDDev1 == NULL)
    {
       return FLASH_PARAM;
    }
    nlld_status = nlld_configHardware(pNandLLDDev1, base_addr_dev1_g, base_addr_dev1_g, base_addr_dev1_g);
    if(nlld_status != NLLD_SUCCESS)
    {
       return FLASH_PARAM;
    }
    nlld_status = nlld_probeDevice(pNandLLDDev1);
    if(nlld_status != NLLD_SUCCESS)
    {
       return FLASH_PARAM;
    }
    #endif
    #endif

    for(devCount = 0; devCount < NUMBER_OF_DEVICES; devCount++)
    {
       #if defined(__LLD) 
       baseAddr = FLASH_DeviceBase(devCount);
       if(baseAddr == NO_DEVICE)
       {
          return FLASH_PARAM;
       }
       #ifdef LLD_COMBINED_BUILD
       deviceType = lld_InitCmd((FLASHDATA *)baseAddr);
       if((LLD_DEVICE_TYPE)deviceType == DEVICE_NONE)
       {
          DBG_Printf("No GLS or GLP found \n", 0, 0);
          return FLASH_PARAM;
       }
       #endif
       //lld_ResetCmd((FLASHDATA *)baseAddr);

       #elif defined(__SLLD)
       //slld_SRSTCmd();

       #elif defined(NAND_LLD)
       baseAddr = FLASH_DeviceBase(devCount);
       if(baseAddr == NO_DEVICE)
       {
          return FLASH_PARAM;
       }
       pDevice = (nand_lld *)baseAddr;
       nlld_status = nlld_reset(pDevice, NLLD_API_OP);
       if(nlld_status != NLLD_SUCCESS)
       {
          return FLASH_FAIL;
       }
       #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
       nlld_status = nlld_reset((nand_lld *)pNandLLDDev1, NLLD_API_OP);
       if(nlld_status != NLLD_SUCCESS)
       {
          return FLASH_FAIL;
       }
       #endif

       #if (FTL_CHECK_BAD_BLOCK_LIMIT == true)
       nlld_status = nlld_readParameterPage(pDevice, pageBuf);
       if(nlld_status != NLLD_SUCCESS)
       {
          return FLASH_FAIL;
       }

       /* Get Bad blocks maximum per LUN */
       gBBDevLimit[devCount] = (pageBuf[PARAM_PAGE_OFFSET_BB_MAX_LOWER_BYTE] | (pageBuf[PARAM_PAGE_OFFSET_BB_MAX_UPPER_BYTE] << PARAM_PAGE_BYTE_SHIFT));

       #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
       gBBDevLimit[devCount] = gBBDevLimit[devCount] * NAND_CHIP_COUNT; /* for 2 chip */
       #endif

       #endif /* #if (FTL_CHECK_BAD_BLOCK_LIMIT == true) */

       #endif
    }

    #if(FTL_DEBUG_FLASH_USAGE == true)
    for(devCount = 0; devCount < NUMBER_OF_DEVICES; devCount++)
    {
       for(eblockCount = 0; eblockCount < NUMBER_OF_ERASE_BLOCKS; eblockCount++)
       {
          DBGEraseCount[devCount][eblockCount] = 0;
       }
    }
    #endif

#if 0
    if(hssim_init_flag == 0)
    {
        base_addr = hssim_Init(((NUMBER_OF_BYTES_PER_PAGE * NUMBER_OF_PAGES_PER_EBLOCK) * NUMBER_OF_ERASE_BLOCKS),1);
        if (NULL == base_addr)
        {
           return (79);
        }
    base_addr_g = base_addr; 
    hssim_init_flag = 1; // don't init again
    }    
#endif
    return FLASH_PASS;
}

//------ FLASH_Shutdown ---------
FLASH_STATUS FLASH_Shutdown(void)
{
    #if 0 
    hssim_init_flag = 0;
    hssim_ShutDown();
    #endif

    #if defined(NAND_LLD)
    nand_lld_status nlld_status1 = NLLD_SUCCESS;       /*4*/
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
    nand_lld_status nlld_status2 = NLLD_SUCCESS;       /*4*/
    #endif

    nlld_status1 = nlld_removeInstance(pNandLLDDev0);    
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
    nlld_status2 = nlld_removeInstance(pNandLLDDev1);
    if(nlld_status2 != NLLD_SUCCESS)
    {
       return FLASH_PARAM;
    }
    #endif
    if(nlld_status1 != NLLD_SUCCESS)
    {
       return FLASH_PARAM;
    }
    #endif

    return FLASH_PASS;
}

//------ FLASH_PageLoad -------
FLASH_STATUS FLASH_PageLoad(FLASH_PAGE_INFO *pageInfo)
{
    return FLASH_PASS;
}

//------ FLASH_RamPageReadDataBlock -----
FLASH_STATUS FLASH_RamPageReadDataBlock(FLASH_PAGE_INFO *pageInfo, uint8_t *dataBuf)
{
    FLASH_STATUS status = FLASH_PASS;                  /*4*/

    #if(DEBUG_FLASH_ANNOUNCE == 1)
    DBG_Printf("FLASH_RamPageReadDataBlock: devID = %d, ", pageInfo->devID, 0);
    DBG_Printf("pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
    DBG_Printf("pageOffset = %d, ", pageInfo->vPage.pageOffset, 0);
    DBG_Printf("byteCount = %d, ", pageInfo->byteCount, 0);
    DBG_Printf("dataBuf = 0x%p\n", (uint32_t)dataBuf, 0);
    #endif  // #if(DEBUG_FLASH_ANNOUNCE == 1)

    #if(DEBUG_FLASH_CHECK_PARAM == 1)
    if((pageInfo->vPage.vPageAddr >= (NUMBER_OF_PAGES_PER_EBLOCK * NUMBER_OF_ERASE_BLOCKS)) ||
       (pageInfo->vPage.pageOffset >= NUMBER_OF_BYTES_PER_PAGE))
    {
       //DBG_Printf("FLASH_RamPageReadDataBlock: ERROR: pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
       //DBG_Printf("pageOffset = %d\n", pageInfo->vPage.pageOffset, 0);
       return FLASH_PARAM;
    }
    #if (CACHE_RAM_BD_MODULE == false || FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    if((pageInfo->vPage.pageOffset % NUMBER_OF_BYTES_PER_SECTOR) > 0)
    {
       //DBG_Printf("FLASH_RamPageReadDataBlock: ERROR: pageOffset should be multiple of NUMBER_OF_BYTES_PER_SECTOR\n", 0, 0);
       return FLASH_PARAM;
    }
    #else
    if ((pageInfo->vPage.pageOffset % FLUSH_RAM_TABLE_SIZE) > 0)
    {
       DBG_Printf("FLASH_RamPageReadDataBlock: ERROR: pageOffset should be multiple of FLUSH_RAM_TABLE_SIZE\n", 0, 0);
       return FLASH_PARAM;
    }
    #endif
    if(dataBuf == NULL)
    {
       //DBG_Printf("FLASH_RamPageReadDataBlock: ERROR: dataBuf should be non-zero\n", 0, 0);
       return FLASH_PARAM;
    }
    #endif  // #if(DEBUG_FLASH_CHECK_PARAM == 1)

    status = FLASH_ReadData(pageInfo, dataBuf);
    if(status != FLASH_PASS)
    {
       return status;
    }

    return FLASH_PASS;
}

//------ FLASH_RamPageReadMetaData -----
FLASH_STATUS FLASH_RamPageReadMetaData(FLASH_PAGE_INFO *pageInfo, uint8_t *dataBuf)
{
    FLASH_STATUS status = FLASH_PASS;                  /*4*/

    #if(DEBUG_FLASH_ANNOUNCE == 1)
    DBG_Printf("FLASH_RamPageReadMetaData: devID = %d, ", pageInfo->devID, 0);
    DBG_Printf("pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
    DBG_Printf("pageOffset = %d, ", pageInfo->vPage.pageOffset, 0);
    DBG_Printf("byteCount = %d, ", pageInfo->byteCount, 0);
    DBG_Printf("dataBuf = 0x%p\n", (uint32_t)dataBuf, 0);
    #endif  // #if(DEBUG_FLASH_ANNOUNCE == 1)

    #if(DEBUG_FLASH_CHECK_PARAM == 1)
    if((pageInfo->vPage.vPageAddr >= (NUMBER_OF_PAGES_PER_EBLOCK * NUMBER_OF_ERASE_BLOCKS)) ||
       (pageInfo->vPage.pageOffset >= NUMBER_OF_BYTES_PER_PAGE))
    {
       //DBG_Printf("FLASH_RamPageReadMetaData: ERROR: pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
       //DBG_Printf("pageOffset = %d\n", pageInfo->vPage.pageOffset, 0);
       return FLASH_PARAM;
    }
    if(pageInfo->byteCount > 16)
    {
       //DBG_Printf("FLASH_RamPageReadMetaData: ERROR: byteCount should be less than 16 bytes or equal to 16 bytes\n", 0, 0);
       return FLASH_PARAM;
    }
    if(dataBuf == NULL)
    {
       //DBG_Printf("FLASH_RamPageReadMetaData: ERROR: dataBuf should be non-zero\n", 0, 0);
       return FLASH_PARAM;
    }

    #if(DEBUG_FLASH_CHECK_BOUNDARY == 1)
    if((pageInfo->vPage.pageOffset % FLASH_BYTES_PER_WORD) > 0)
    {
       DBG_Printf("FLASH_RamPageReadMetaData: ERROR: pageOffset is NOT word boundary\n", 0, 0);
       return FLASH_PARAM;
    }
    if((pageInfo->byteCount % FLASH_BYTES_PER_WORD) > 0)
    {
       DBG_Printf("FLASH_RamPageReadMetaData: ERROR: byteCount is NOT word boundary\n", 0, 0);
       return FLASH_PARAM;
    }
    #endif  // #if(DEBUG_FLASH_CHECK_BOUNDARY == 1)
    #endif  // #if(DEBUG_FLASH_CHECK_PARAM == 1)

    status = FLASH_ReadData(pageInfo, dataBuf);
    if(status != FLASH_PASS)
    {
       return status;
    }

    return FLASH_PASS;
}

//------ FLASH_RamPageReadSpareData -----
FLASH_STATUS FLASH_RamPageReadSpareData(FLASH_PAGE_INFO *pageInfo, uint8_t *dataBuf)
{
    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    uint32_t addr = 0;                                   /*4*/
    nand_lld_status nlld_status = NLLD_SUCCESS;        /*4*/
    #if (FTL_HAMMING_ECC == true)
    uint8_t eccCodes[3];                                /*3*/
    uint16_t correctLoc = 0;                             /*2*/
    #else /* NLLD_BCH_ECC */
    uint8_t eccData[TOTAL_LENGTH_BYTE];                 /*524*/
    uint16_t eccOffset;                                 /*2*/
    uint32_t errNum;                                    /*4*/
    #endif
    #if defined(NAND_LLD)
    nand_lld *pDevice = NULL;                          /*4*/
    dev_address nand_addr;
    #endif
    #endif
    #if(MANAGED_REGIONS == true)
    MANAGED_REGIONS_INFO_STRUCT managedInfo;
    FTL_STATUS managedStatus = FTL_ERR_PASS;
    #endif

    #if(DEBUG_FLASH_ANNOUNCE == 1)
    DBG_Printf("FLASH_RamPageReadSpareData: devID = %d, ", pageInfo->devID, 0);
    DBG_Printf("pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
    DBG_Printf("pageOffset = %d, ", pageInfo->vPage.pageOffset, 0);
    DBG_Printf("byteCount = %d, ", pageInfo->byteCount, 0);
    DBG_Printf("dataBuf = 0x%p\n", (uint32_t)dataBuf, 0);
    #endif  // #if(DEBUG_FLASH_ANNOUNCE == 1)

    #if(DEBUG_FLASH_CHECK_PARAM == 1)
    if((pageInfo->vPage.vPageAddr >= (NUMBER_OF_PAGES_PER_EBLOCK * NUMBER_OF_ERASE_BLOCKS)) ||
       (pageInfo->vPage.pageOffset >= NUMBER_OF_BYTES_PER_PAGE))
    {
       //DBG_Printf("FLASH_RamPageReadSpareData: ERROR: pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
       //DBG_Printf("pageOffset = %d\n", pageInfo->vPage.pageOffset, 0);
       return FLASH_PARAM;
    }
    if(pageInfo->vPage.pageOffset < VIRTUAL_PAGE_SIZE)
    {
       //DBG_Printf("FLASH_RamPageReadSpareData: ERROR: pageOffset should be spare area\n", 0, 0);
       return FLASH_PARAM;
    }
    if(pageInfo->byteCount > 16)
    {
       //DBG_Printf("FLASH_RamPageReadSpareData: ERROR: byteCount should be less than 16 bytes or equal to 16 bytes\n", 0, 0);
       return FLASH_PARAM;
    }
    if(dataBuf == NULL)
    {
       //DBG_Printf("FLASH_RamPageReadSpareData: ERROR: dataBuf should be non-zero\n", 0, 0);
       return FLASH_PARAM;
    }

    #if(DEBUG_FLASH_CHECK_BOUNDARY == 1)
    if((pageInfo->vPage.pageOffset % FLASH_BYTES_PER_WORD) > 0)
    {
       DBG_Printf("FLASH_RamPageReadSpareData: ERROR: pageOffset is NOT word boundary\n", 0, 0);
       return FLASH_PARAM;
    }
    if((pageInfo->byteCount % FLASH_BYTES_PER_WORD) > 0)
    {
       DBG_Printf("FLASH_RamPageReadSpareData: ERROR: byteCount is NOT word boundary\n", 0, 0);
       return FLASH_PARAM;
    }
    #endif  // #if(DEBUG_FLASH_CHECK_BOUNDARY == 1)
    #endif  // #if(DEBUG_FLASH_CHECK_PARAM == 1)

    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    baseAddr = FLASH_DeviceBase(pageInfo->devID);
    if(baseAddr == NO_DEVICE)
    {
       return FLASH_PARAM;
    }

    addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset + SPARE_AREA_OFFSET);
    #if(MANAGED_REGIONS == true)
    managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
    baseAddr = managedInfo.baseAddr;
    addr = managedInfo.deviceOffsetBytes;
    #endif
    #if defined(HSSIMU)
    hssim_Read_B((uint16_t *)baseAddr, addr, dataBuf, pageInfo->byteCount);

    #elif defined(NAND_LLD)
    pDevice = (nand_lld *)baseAddr;
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
    if(pDevice->geoInfo.totalBlocks <= (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
    {
       pDevice = (nand_lld *)pNandLLDDev1;
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), pageInfo->vPage.pageOffset + SPARE_AREA_OFFSET);
    }
    #endif
    nand_addr.ColAddr = addr % pDevice->geoInfo.pageSize;
    nand_addr.RowAddr = addr / pDevice->geoInfo.pageSize;
    if(pDevice->geoInfo.dataWidth == NAND_16BIT_DATA) nand_addr.ColAddr >>= 1;
    nlld_status = nlld_readPartialPage(pDevice, nand_addr, pageInfo->byteCount, dataBuf);
    if(nlld_status != NLLD_SUCCESS)
    {
       return FLASH_FAIL;
    }
    #endif

    if(FLASH_CheckEmpty(dataBuf, pageInfo->byteCount) != FLASH_PASS)
    {
       
       #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
       #if defined(NAND_LLD)
       if(pDevice->geoInfo.totalBlocks > (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
       {
          addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset + SPARE_ECC_OFFSET);
       }else{
          addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), pageInfo->vPage.pageOffset + SPARE_ECC_OFFSET);
       }
       #endif
       #else
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset + SPARE_ECC_OFFSET);
       #endif 

       #if(MANAGED_REGIONS == true)
       managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
       baseAddr = managedInfo.baseAddr;
       addr = managedInfo.deviceOffsetBytes;
       #endif

       #if defined(HSSIMU)
       hssim_Read_B((uint16_t *)baseAddr, addr, spareBuf, MAX_SPARE_INFO_ECC_SIZE);

       #elif defined(NAND_LLD)
       nand_addr.ColAddr = addr % pDevice->geoInfo.pageSize;
       if(pDevice->geoInfo.dataWidth == NAND_16BIT_DATA) nand_addr.ColAddr >>= 1;
       nlld_status = nlld_randomDataOutput(pDevice, nand_addr.ColAddr, spareBuf, MAX_SPARE_INFO_ECC_SIZE);
       if(nlld_status != NLLD_SUCCESS)
       {
          return FLASH_FAIL;
       }
       #endif

       #if (FTL_HAMMING_ECC == true)
       nlld_calEcc(&dataBuf[0], 16, eccCodes);
       nlld_status = nlld_correctUseEcc(&dataBuf[0], 16, &spareBuf[0], eccCodes, &correctLoc, 1);
       if (nlld_status == NLLD_ECC_ERROR_UNCORRECTABLE)
       {
          DBG_Printf("FLASH_RamPageReadSpareData: ERROR ECC nlld_status %d\n", nlld_status, 0);
          return FLASH_ECC_FAIL;
       }
       #else  /* NLLD_BCH_ECC */
       for (eccOffset = 0; eccOffset < TOTAL_LENGTH_BYTE; eccOffset++)
       {
          if (eccOffset < pageInfo->byteCount) /* SPARE_INFO_SIZE */
          {
             eccData[eccOffset] = dataBuf[eccOffset];
          }
          else
          {
             eccData[eccOffset] = EMPTY_BYTE;
          } 
       }
       memcpy(&eccData[DATA_LENGTH_BYTE], &spareBuf[0], PARA_LENGTH_BYTE);
       nlld_status = nlld_bch_decode(eccData, &errNum, ECC_MODE_FIX);
       if (nlld_status == NLLD_BCH_ERROR_NOT_CORRECTABLE)
       {
          DBG_Printf("FLASH_ReadData: ERROR ECC nlld_status %d\n", nlld_status, 0);
          return FLASH_ECC_FAIL;
       }
       memcpy(&dataBuf[0], &eccData[0], 16);
       #endif  /* #if (FTL_HAMMING_ECC == true) */
    }
    #endif

    return FLASH_PASS;
}

//------ FLASH_RamPageWriteCMD -----
FLASH_STATUS FLASH_RamPageWriteCMD(FLASH_PAGE_INFO *pageInfo, uint8_t *dataBuf)
{
    FLASH_STATUS status = FLASH_PASS;                  /*4*/

    #if(DEBUG_FLASH_ANNOUNCE == 1)
    DBG_Printf("FLASH_RamPageWriteCMD: devID = %d, ", pageInfo->devID, 0);
    DBG_Printf("pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
    DBG_Printf("pageOffset = %d, ", pageInfo->vPage.pageOffset, 0);
    DBG_Printf("byteCount = %d, ", pageInfo->byteCount, 0);
    DBG_Printf("dataBuf = 0x%p\n", (uint32_t)dataBuf, 0);
    #endif  // #if(DEBUG_FLASH_ANNOUNCE == 1)

    #if(DEBUG_FLASH_CHECK_PARAM == 1)
    if((pageInfo->vPage.vPageAddr >= (NUMBER_OF_PAGES_PER_EBLOCK * NUMBER_OF_ERASE_BLOCKS)) ||
       (pageInfo->vPage.pageOffset >= NUMBER_OF_BYTES_PER_PAGE))
    {
       //DBG_Printf("FLASH_RamPageWriteCMD: ERROR: pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
       //DBG_Printf("pageOffset = %d\n", pageInfo->vPage.pageOffset, 0);
       return FLASH_PARAM;
    }
    if((pageInfo->vPage.pageOffset % NUMBER_OF_BYTES_PER_SECTOR) > 0)
    {
       //DBG_Printf("FLASH_RamPageWriteCMD: ERROR: pageOffset should be multiple of NUMBER_OF_BYTES_PER_SECTOR\n", 0, 0);
       return FLASH_PARAM;
    }
    if((pageInfo->byteCount % NUMBER_OF_BYTES_PER_SECTOR) > 0)
    {
       //DBG_Printf("FLASH_RamPageWriteCMD: ERROR: byteCount should be multiple of NUMBER_OF_BYTES_PER_SECTOR\n", 0, 0);
       return FLASH_PARAM;
    }
    if(dataBuf == NULL)
    {
       //DBG_Printf("FLASH_RamPageWriteCMD: ERROR: dataBuf should be non-zero\n", 0, 0);
       return FLASH_PARAM;
    }
    #endif  // #if(DEBUG_FLASH_CHECK_PARAM == 1)

    status = FLASH_WriteData(pageInfo, dataBuf, NAND_PAGE_PROGRAM_MAIN);
    if(status != FLASH_PASS)
    {
       return status;
    }

    return FLASH_PASS;
}

//------ FLASH_RamPageWriteFFPad -----
FLASH_STATUS FLASH_RamPageWriteFFPad(FLASH_PAGE_INFO *pageInfo, uint8_t *dataBuf)
{
    return FLASH_PASS;
}

//------ FLASH_PageCommit -----
FLASH_STATUS FLASH_PageCommit(uint8_t devID)
{
    return FLASH_PASS;
}

//------ FLASH_RamPageWriteDataBlock -----
FLASH_STATUS FLASH_RamPageWriteDataBlock(FLASH_PAGE_INFO *pageInfo, uint8_t *dataBuf)
{
    FLASH_STATUS status = FLASH_PASS;                  /*4*/

    #if(DEBUG_FLASH_ANNOUNCE == 1)
    DBG_Printf("FLASH_RamPageWriteDataBlock: devID = %d, ", pageInfo->devID, 0);
    DBG_Printf("pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
    DBG_Printf("pageOffset = %d, ", pageInfo->vPage.pageOffset, 0);
    DBG_Printf("byteCount = %d, ", pageInfo->byteCount, 0);
    DBG_Printf("dataBuf = 0x%p\n", (uint32_t)dataBuf, 0);
    #endif  // #if(DEBUG_FLASH_ANNOUNCE == 1)

    #if(DEBUG_FLASH_CHECK_PARAM == 1)
    if((pageInfo->vPage.vPageAddr >= (NUMBER_OF_PAGES_PER_EBLOCK * NUMBER_OF_ERASE_BLOCKS)) ||
       (pageInfo->vPage.pageOffset >= NUMBER_OF_BYTES_PER_PAGE))
    {
       //DBG_Printf("FLASH_RamPageWriteDataBlock: ERROR: pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
       //DBG_Printf("pageOffset = %d\n", pageInfo->vPage.pageOffset, 0);
       return FLASH_PARAM;
    }
    #if (CACHE_RAM_BD_MODULE == false || FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    if((pageInfo->vPage.pageOffset % NUMBER_OF_BYTES_PER_SECTOR) > 0)
    {
       //DBG_Printf("FLASH_RamPageWriteDataBlock: ERROR: pageOffset should be multiple of NUMBER_OF_BYTES_PER_SECTOR\n", 0, 0);
       return FLASH_PARAM;
    }
    #else
    if ((pageInfo->vPage.pageOffset % FLUSH_RAM_TABLE_SIZE) > 0)
    {
       DBG_Printf("FLASH_RamPageWriteDataBlock: ERROR: pageOffset should be multiple of FLUSH_RAM_TABLE_SIZE\n", 0, 0);
       return FLASH_PARAM;
    }
    #endif

    if(dataBuf == NULL)
    {
       //DBG_Printf("FLASH_RamPageWriteDataBlock: ERROR: dataBuf should be non-zero\n", 0, 0);
       return FLASH_PARAM;
    }
    #endif  // #if(DEBUG_FLASH_CHECK_PARAM == 1)

    status = FLASH_WriteData(pageInfo, dataBuf, NAND_PAGE_PROGRAM_CONFIRM);
    if(status != FLASH_PASS)
    {
       return status;
    }

    return FLASH_PASS;
}

//------ FLASH_RamPageWriteMetaData -----
FLASH_STATUS FLASH_RamPageWriteMetaData(FLASH_PAGE_INFO *pageInfo, uint8_t *dataBuf)
{
    FLASH_STATUS status = FLASH_PASS;                  /*4*/

    #if(DEBUG_FLASH_ANNOUNCE == 1)
    DBG_Printf("FLASH_RamPageWriteMetaData: devID = %d, ", pageInfo->devID, 0);
    DBG_Printf("pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
    DBG_Printf("pageOffset = %d, ", pageInfo->vPage.pageOffset, 0);
    DBG_Printf("byteCount = %d, ", pageInfo->byteCount, 0);
    DBG_Printf("dataBuf = 0x%p\n", (uint32_t)dataBuf, 0);
    #endif  // #if(DEBUG_FLASH_ANNOUNCE == 1)

    #if(DEBUG_FLASH_CHECK_PARAM == 1)
    if((pageInfo->vPage.vPageAddr >= (NUMBER_OF_PAGES_PER_EBLOCK * NUMBER_OF_ERASE_BLOCKS)) ||
       (pageInfo->vPage.pageOffset >= NUMBER_OF_BYTES_PER_PAGE))
    {
       //DBG_Printf("FLASH_RamPageWriteMetaData: ERROR: pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
       //DBG_Printf("pageOffset = %d\n", pageInfo->vPage.pageOffset, 0);
       return FLASH_PARAM;
    }
    if(dataBuf == NULL)
    {
       //DBG_Printf("FLASH_RamPageWriteMetaData: ERROR: dataBuf should be non-zero\n", 0, 0);
       return FLASH_PARAM;
    }

    #if(DEBUG_FLASH_CHECK_BOUNDARY == 1)
    if((pageInfo->vPage.pageOffset % FLASH_BYTES_PER_WORD) > 0)
    {
       DBG_Printf("FLASH_RamPageWriteMetaData: ERROR: pageOffset is NOT word boundary\n", 0, 0);
       return FLASH_PARAM;
    }
    if((pageInfo->byteCount % FLASH_BYTES_PER_WORD) > 0)
    {
       DBG_Printf("FLASH_RamPageWriteMetaData: ERROR: byteCount is NOT word boundary\n", 0, 0);
       return FLASH_PARAM;
    }
    #endif  // #if(DEBUG_FLASH_CHECK_BOUNDARY == 1)
    #endif  // #if(DEBUG_FLASH_CHECK_PARAM == 1)

    status = FLASH_WriteData(pageInfo, dataBuf, NAND_PAGE_PROGRAM_CONFIRM);
    if(status != FLASH_PASS)
    {
       return status;
    }

    return FLASH_PASS;
}

//------ FLASH_RamPageWriteSpareData -----
FLASH_STATUS FLASH_RamPageWriteSpareData(FLASH_PAGE_INFO *pageInfo, uint8_t *dataBuf)
{
    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    uint32_t addr = 0;                                   /*4*/
    uint32_t count = 0;                                  /*4*/
    #if defined(NAND_LLD)
    nand_lld *pDevice = NULL;                          /*4*/
    nand_lld_status nlld_status = NLLD_SUCCESS;        /*4*/
    #endif
    #if (FTL_BCH_ECC == true)
    uint8_t eccData[TOTAL_LENGTH_BYTE];                  /*524*/
    uint16_t eccOffset;                                  /*2*/
    #endif
    #endif
    #if(MANAGED_REGIONS == true)
    MANAGED_REGIONS_INFO_STRUCT managedInfo;
    FTL_STATUS managedStatus = FTL_ERR_PASS;
    #endif

    #if(DEBUG_FLASH_ANNOUNCE == 1)
    DBG_Printf("FLASH_RamPageWriteSpareData: devID = %d, ", pageInfo->devID, 0);
    DBG_Printf("pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
    DBG_Printf("pageOffset = %d, ", pageInfo->vPage.pageOffset, 0);
    DBG_Printf("byteCount = %d, ", pageInfo->byteCount, 0);
    DBG_Printf("dataBuf = 0x%p\n", (uint32_t)dataBuf, 0);
    #endif  // #if(DEBUG_FLASH_ANNOUNCE == 1)

    #if(DEBUG_FLASH_CHECK_PARAM == 1)
    if((pageInfo->vPage.vPageAddr >= (NUMBER_OF_PAGES_PER_EBLOCK * NUMBER_OF_ERASE_BLOCKS)) ||
       (pageInfo->vPage.pageOffset >= NUMBER_OF_BYTES_PER_PAGE))
    {
       //DBG_Printf("FLASH_RamPageWriteSpareData: ERROR: pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
       //DBG_Printf("pageOffset = %d\n", pageInfo->vPage.pageOffset, 0);
       return FLASH_PARAM;
    }
    if(pageInfo->vPage.pageOffset < VIRTUAL_PAGE_SIZE)
    {
       //DBG_Printf("FLASH_RamPageWriteSpareData: ERROR: pageOffset should be spare area\n", 0, 0);
       return FLASH_PARAM;
    }
    if(pageInfo->byteCount > 16)
    {
       //DBG_Printf("FLASH_RamPageWriteSpareData: ERROR: byteCount should be less than 16 bytes or equal to 16 bytes\n", 0, 0);
       return FLASH_PARAM;
    }
    if(dataBuf == NULL)
    {
       //DBG_Printf("FLASH_RamPageWriteSpareData: ERROR: dataBuf should be non-zero\n", 0, 0);
       return FLASH_PARAM;
    }

    #if(DEBUG_FLASH_CHECK_BOUNDARY == 1)
    if((pageInfo->vPage.pageOffset % FLASH_BYTES_PER_WORD) > 0)
    {
       DBG_Printf("FLASH_RamPageWriteSpareData: ERROR: pageOffset is NOT word boundary\n", 0, 0);
       return FLASH_PARAM;
    }
    if((pageInfo->byteCount % FLASH_BYTES_PER_WORD) > 0)
    {
       DBG_Printf("FLASH_RamPageWriteSpareData: ERROR: byteCount is NOT word boundary\n", 0, 0);
       return FLASH_PARAM;
    }
    #endif  // #if(DEBUG_FLASH_CHECK_BOUNDARY == 1)
    #endif  // #if(DEBUG_FLASH_CHECK_PARAM == 1)

    #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    baseAddr = FLASH_DeviceBase(pageInfo->devID);
    if(baseAddr == NO_DEVICE)
    {
       return FLASH_PARAM;
    }

    addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset + SPARE_AREA_OFFSET);
    #if(MANAGED_REGIONS == true)
    managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
    baseAddr = managedInfo.baseAddr;
    addr = managedInfo.deviceOffsetBytes;
    #endif
    #if defined(HSSIMU)
    hssim_Write_B((uint16_t *)baseAddr, addr, dataBuf, pageInfo->byteCount);

    #elif defined(NAND_LLD)
    pDevice = (nand_lld *)baseAddr;
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
    if(pDevice->geoInfo.totalBlocks <= (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
    {
       pDevice = (nand_lld *)pNandLLDDev1;
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), pageInfo->vPage.pageOffset + SPARE_AREA_OFFSET);
    }
    #endif
    addr = addr % pDevice->geoInfo.pageSize; // Get ColAddr
    if(pDevice->geoInfo.dataWidth == NAND_16BIT_DATA) addr >>= 1;
    nlld_status = nlld_randomDataInput(pDevice, addr, dataBuf, pageInfo->byteCount);
    if(nlld_status != NLLD_SUCCESS)
    {
       return FLASH_PARAM;
    }
    #endif

    for(count = 0; count < MAX_ECC_SIZE; count++)
    {
       spareBuf[count] = 0xFF;
    }

    #if (FTL_HAMMING_ECC == true)
    nlld_calEcc(&dataBuf[0], 16, &spareBuf[0]);
    #else  /* NLLD_BCH_ECC */
    for (eccOffset = 0; eccOffset < TOTAL_LENGTH_BYTE; eccOffset++)
    {
       if (eccOffset < pageInfo->byteCount) /* SPARE_INFO_SIZE */
       {
          eccData[eccOffset] = dataBuf[eccOffset];
       }
       else
       {
          eccData[eccOffset] = EMPTY_BYTE;
       }
    }
    nlld_bch_encode(eccData);
    memcpy(&spareBuf[0], &eccData[DATA_LENGTH_BYTE], PARA_LENGTH_BYTE);
    #endif  /* #if (FTL_HAMMING_ECC == true) */
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
    #if defined(NAND_LLD)
    if(pDevice->geoInfo.totalBlocks > (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
    {
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset + SPARE_ECC_OFFSET);
    }else{
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), pageInfo->vPage.pageOffset + SPARE_ECC_OFFSET);
    }
    #endif
    #else
    addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset + SPARE_ECC_OFFSET);
    #endif
    #if(MANAGED_REGIONS == true)
    managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
    baseAddr = managedInfo.baseAddr;
    addr = managedInfo.deviceOffsetBytes;
    #endif

    #if defined(HSSIMU)
    hssim_Write_B((uint16_t *)baseAddr, addr, spareBuf, MAX_SPARE_INFO_ECC_SIZE);

    #elif defined(NAND_LLD)
    addr = addr % pDevice->geoInfo.pageSize; // Get ColAddr
    if(pDevice->geoInfo.dataWidth == NAND_16BIT_DATA) addr >>= 1;
    nlld_status = nlld_randomDataInput(pDevice, addr, spareBuf, MAX_SPARE_INFO_ECC_SIZE);
    if(nlld_status != NLLD_SUCCESS)
    {
       return FLASH_PARAM;
    }
    nlld_status = nlld_programConfirm(pDevice, NLLD_API_OP);
    if(nlld_status != NLLD_SUCCESS)
    {
       return FLASH_PROGRAM_FAIL;
    }
    #endif
    #endif

    return FLASH_PASS;
}

//------ FLASH_Erase -----------
FLASH_STATUS FLASH_Erase(FLASH_PAGE_INFO *pageInfo)
{
    uint32_t addr = 0;                                   /*4*/
    #if defined(HSSIMU)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    #elif defined(__LLD)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    DEVSTATUS dev_status = DEV_NOT_BUSY;               /*4*/
    #elif defined(__SLLD)
    SLLD_STATUS    slld_status = SLLD_OK;                 /*4*/
    DEVSTATUS dev_status = dev_not_busy;               /*4*/
    #elif defined(LINUX_MTD)
    int ret = 0;                                       /*4*/
    struct erase_info erase;
    #elif defined(NAND_LLD)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    nand_lld *pDevice = NULL;                          /*4*/
    nand_lld_status nlld_status = NLLD_SUCCESS;        /*4*/
    #endif
    #if(MANAGED_REGIONS == true)
    MANAGED_REGIONS_INFO_STRUCT managedInfo;
    FTL_STATUS managedStatus = FTL_ERR_PASS;
    #endif

    #if(DEBUG_FLASH_ANNOUNCE == 1)
    DBG_Printf("FLASH_Erase: devID = %d, ", pageInfo->devID, 0);
    DBG_Printf("pageAddr = 0x%X\n", pageInfo->vPage.vPageAddr, 0);
    #endif  // #if(DEBUG_FLASH_ANNOUNCE == 1)

    #if(DEBUG_FLASH_CHECK_PARAM == 1)
    if((pageInfo->vPage.vPageAddr >= (NUMBER_OF_PAGES_PER_EBLOCK * NUMBER_OF_ERASE_BLOCKS)) ||
       (pageInfo->vPage.pageOffset >= NUMBER_OF_BYTES_PER_PAGE))
    {
       //DBG_Printf("FLASH_Erase: ERROR: pageAddr = 0x%X, ", pageInfo->vPage.vPageAddr, 0);
       //DBG_Printf("pageOffset = %d\n", pageInfo->vPage.pageOffset, 0);
       return FLASH_PARAM;
    }
    #endif  // #if(DEBUG_FLASH_CHECK_PARAM == 1)

    #if(FTL_DEBUG_FLASH_USAGE == true)
    DBGEraseCount[pageInfo->devID][(pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)]++;
    #endif

    #if defined(HSSIMU) || defined(__LLD) || defined(NAND_LLD)
    baseAddr = FLASH_DeviceBase(pageInfo->devID);
    if(baseAddr == NO_DEVICE)
    {
       return FLASH_PARAM;
    }
    #endif

    addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset);
    #if(MANAGED_REGIONS == true)
    managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
    baseAddr = managedInfo.baseAddr;
    addr = managedInfo.deviceOffsetBytes;
    #endif

    #if defined(HSSIMU)
    hssim_Erase((uint16_t *)baseAddr, (addr/FLASH_BYTES_PER_WORD), ((NUMBER_OF_BYTES_PER_PAGE * NUMBER_OF_PAGES_PER_EBLOCK)/FLASH_BYTES_PER_WORD));

    #elif defined(__LLD)
    dev_status = lld_SectorEraseOp((FLASHDATA *)baseAddr, (addr/LLD_BYTES_PER_OP));
    if(dev_status != DEV_NOT_BUSY)
    {
       DBG_Printf("FLASH_Erase ERROR: dev_status %d\n", dev_status, 0);
       return FLASH_ERASE_FAIL;
    }

    #elif defined(__SLLD)
    slld_status = slld_SEOp(addr, &dev_status);
    if(slld_status != SLLD_OK)
    {
       DBG_Printf("FLASH_Erase ERROR: slld_status %d\n", slld_status, 0);
       return FLASH_ERASE_FAIL;
    }

    #elif defined(LINUX_MTD)
    erase.mtd = dev->mtd;
    erase.callback = NULL;
    erase.addr = addr;
    erase.len = EBLOCK_SIZE;
    erase.priv = 0;
    ret = (*(dev->mtd->erase))(dev->mtd, &erase);
    if(ret != 0)
    {
       DBG_Printf("FLASH_Erase: ERROR mtd->erase %d, ", ret, 0);
       DBG_Printf("addr %d, ", addr, 0);
       DBG_Printf("state %d\n", erase.state, 0);
       return FLASH_ERASE_FAIL;
    }
    else
    { 
       if(erase.state != MTD_ERASE_DONE)
       {
          DBG_Printf("FLASH_Erase: mtd->erase is not done! state %d\n", erase.state, 0);
          return FLASH_ERASE_FAIL;
       }
    }

    #elif defined(NAND_LLD)
    pDevice = (nand_lld *)baseAddr;
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
    if(pDevice->geoInfo.totalBlocks <= (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
    {
       pDevice = (nand_lld *)pNandLLDDev1;
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), pageInfo->vPage.pageOffset);
    }
    #endif
    addr = addr / pDevice->geoInfo.pageSize; // Get RowAddr
    nlld_status = nlld_eraseBlock(pDevice, NLLD_API_OP, addr);
    if(nlld_status != NLLD_SUCCESS)
    {
       DBG_Printf("FLASH_Erase ERROR: nlld_status %d\n", nlld_status, 0);
       return FLASH_ERASE_FAIL;
    }
    #endif

    return FLASH_PASS;
}

//------ FLASH_PageLoadForMove -----
FLASH_STATUS FLASH_PageLoadForMove(FLASH_PAGE_INFO *pageInfo)
{
    return FLASH_PASS;
}

//------ FLASH_PageCommitForMove -----
FLASH_STATUS FLASH_PageCommitForMove(FLASH_PAGE_INFO *pageInfo)
{
    return FLASH_PASS;
}

//------ FLASH_SuspendErase -----
FLASH_STATUS FLASH_SuspendErase(FLASH_PAGE_INFO *pageInfo)
{
    return FLASH_PASS;
}

//------ FLASH_ResumeErase -----
FLASH_STATUS FLASH_ResumeErase(FLASH_PAGE_INFO *pageInfo)
{
    return FLASH_PASS;
}

//------ FLASH_CheckReady -----
FLASH_STATUS FLASH_CheckReady(FLASH_PAGE_INFO *pageInfo)
{
    return FLASH_PASS;
}

//------ FLASH_WaitForFlashXferComplete ----
FLASH_STATUS FLASH_WaitForFlashXferComplete(void)
{
    return FLASH_PASS;
}

//------ FLASH_WaitForFlashNotBusy -----------
FLASH_STATUS FLASH_WaitForFlashNotBusy(uint8_t devID)
{
    return FLASH_PASS;
}

//------ FLASH_ReadStatus -----------
FLASH_STATUS FLASH_ReadStatus(FLASH_PAGE_INFO *pageInfo)
{
    return FLASH_PASS;
}

//------------------------------
FLASH_STATUS FLASH_CheckEmpty(uint8_t *dataBuf, uint32_t size)
{
    uint16_t * ptr16 = NULL;                           /*4*/
    uint32_t * ptr32 = NULL;                           /*4*/
    uint32_t count = 0;                                  /*4*/
    uint32_t num = 0;                                    /*4*/

    if((uint32_t)dataBuf & 0x01)
    {
       for(count = 0; count < size; count++)
       {
          if(dataBuf[count] != EMPTY_BYTE)
          {
             return FLASH_FAIL;
          }
       }
    }
    else if((uint32_t)dataBuf & 0x02)
    {
       ptr16 = (uint16_t *)dataBuf;
       num = size / 2;
       for(count = 0; count < num; count++)
       {
          if(ptr16[count] != EMPTY_WORD)
          {
             return FLASH_FAIL;
          }
       }
    }
    else
    {
       ptr32 = (uint32_t *)dataBuf;
       num = size / 4;
       for(count = 0; count < num; count++)
       {
          if(ptr32[count] != EMPTY_DWORD)
          {
             return FLASH_FAIL;
          }
       }
    }
    return FLASH_PASS;
}

//------------------------------
FLASH_STATUS FLASH_CheckDefectEBlock(FLASH_PAGE_INFO *pageInfo)
{
    #if defined(NAND_LLD)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    uint32_t addr = 0;                                   /*4*/
    nand_lld *pDevice = NULL;                          /*4*/
    nand_lld_status nlld_status = NLLD_SUCCESS;        /*4*/
    #if(MANAGED_REGIONS == true)
    MANAGED_REGIONS_INFO_STRUCT managedInfo;
    FTL_STATUS managedStatus = FTL_ERR_PASS;
    #endif

    baseAddr = FLASH_DeviceBase(pageInfo->devID);
    if(baseAddr == NO_DEVICE)
    {
       return FLASH_PARAM;
    }

    addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset);
    #if(MANAGED_REGIONS == true)
    managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
    baseAddr = managedInfo.baseAddr;
    addr = managedInfo.deviceOffsetBytes;
    #endif

    pDevice = (nand_lld *)baseAddr;
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
    if(pDevice->geoInfo.totalBlocks <= (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
    {
       pDevice = (nand_lld *)pNandLLDDev1;
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), pageInfo->vPage.pageOffset);
    }
    #endif
    if(pDevice->geoInfo.pageSize != 0)
    {
       addr /= pDevice->geoInfo.pageSize;
    }
    nlld_status = nlld_checkBadBlock(pDevice, addr);
    if(nlld_status != NLLD_SUCCESS)
    {
       DBG_Printf("FLASH_CheckDefectEBlock : addr 0x%x\n", addr, 0);
       return FLASH_FAIL;
    }
    #endif

    #if defined(HSSIMU)
    #ifdef NAND_IMAGE_TOOL  // for NAND Image Tool
    uint32_t count;
    uint16_t eBlockAddr = 0;
    uint32_t baseAddr = NO_DEVICE;
    uint32_t addr = 0;


    // Get Physical EBlock Address
    eBlockAddr = (uint16_t)(pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK);

    // check IBB EBlock
    for(count = 0;count < g_bb_count;count++)
    {
        if(g_eBlockNumBox[count] == eBlockAddr)
        {
            baseAddr = FLASH_DeviceBase(pageInfo->devID);
            if(baseAddr == NO_DEVICE)
            {
               return FLASH_PARAM;
            }
            #if (FTL_ENABLE_NAND_SECOND_CHIP == true)
            #ifdef defined(NAND_LLD)
            if(pDevice->geoInfo.totalBlocks > (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
            {
               addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset);
            }else{
               addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), pageInfo->vPage.pageOffset);
            }
            #endif
            #else
            addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset);
            #endif
            #if(MANAGED_REGIONS == true)
            managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
            baseAddr = managedInfo.baseAddr;
            addr = managedInfo.deviceOffsetBytes;
            #endif
            hssim_Erase((uint16_t *)baseAddr, (addr/FLASH_BYTES_PER_WORD), ((NUMBER_OF_BYTES_PER_PAGE * NUMBER_OF_PAGES_PER_EBLOCK)/FLASH_BYTES_PER_WORD));
            return FLASH_FAIL;
        }
    }
    #endif
    #endif

    return FLASH_PASS;
}

//------------------------------
FLASH_STATUS FLASH_MarkDefectEBlock(FLASH_PAGE_INFO *pageInfo)
{
    #if defined(NAND_LLD)
    uint32_t baseAddr = NO_DEVICE;                       /*4*/
    uint8_t retryCount = 3;                              /*1*/
    nand_lld *pDevice = NULL;                          /*4*/
    uint32_t addr = 0;                                   /*4*/
    nand_lld_status nlld_status = NLLD_SUCCESS;        /*4*/
    uint8_t dataBuf[SPARE_SIZE];
    uint8_t count;
    dev_address nand_addr;
    #if(MANAGED_REGIONS == true)
    MANAGED_REGIONS_INFO_STRUCT managedInfo;
    FTL_STATUS managedStatus = FTL_ERR_PASS;
    #endif

    #if (FTL_CHECK_BAD_BLOCK_LIMIT == true)
    gBBCount[pageInfo->devID]++;
    if(gBBCount[pageInfo->devID] > gBBDevLimit[pageInfo->devID])
    {
       DBG_Printf("[Warning] bad block is more than %d", gBBDevLimit[pageInfo->devID], 0);
       DBG_Printf(" in dev%d. \n", pageInfo->devID, 0);
    }
    #endif

    baseAddr = FLASH_DeviceBase(pageInfo->devID);
    if(baseAddr == NO_DEVICE)
    {
       return FLASH_PARAM;
    }

    addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr, pageInfo->vPage.pageOffset);
    #if(MANAGED_REGIONS == true)
    managedStatus = GetManagedRegionsInfo(addr, &managedInfo);
    baseAddr = managedInfo.baseAddr;
    addr = managedInfo.deviceOffsetBytes;
    #endif

    for(count = 0; count < SPARE_SIZE; count++)
    {
       dataBuf[count] = 0;
    }

    pDevice = (nand_lld *)baseAddr;
    #if (FTL_ENABLE_NAND_SECOND_CHIP == true) 
    if(pDevice->geoInfo.totalBlocks <= (FTL_START_EBLOCK + (pageInfo->vPage.vPageAddr / NUMBER_OF_PAGES_PER_EBLOCK)))
    {
       pDevice = (nand_lld *)pNandLLDDev1;
       addr = FLASH_GetAddress(pageInfo->vPage.vPageAddr - (NUMBER_OF_PAGES_PER_EBLOCK * pDevice->geoInfo.totalBlocks), pageInfo->vPage.pageOffset);
    }
    #endif

    while(retryCount)
    {
       nand_addr.ColAddr = addr % pDevice->geoInfo.pageSize;
       nand_addr.RowAddr = addr / pDevice->geoInfo.pageSize;
       if(pDevice->geoInfo.dataWidth == NAND_16BIT_DATA) nand_addr.ColAddr >>= 1;
       nlld_status = nlld_programPage(pDevice, NLLD_API_OP, PROGRAM_SPARE, nand_addr, dataBuf);
       if(nlld_status == NLLD_SUCCESS)
       {
          break;
       }
       retryCount--;
    }
    if(nlld_status != NLLD_SUCCESS)
    {
       DBG_Printf("FLASH_MarkDefectEBlock ERROR: nlld_status %d\n", nlld_status, 0);
       return FLASH_FAIL;
    }
    #endif

    return FLASH_PASS;
}

//------ FLASH_GetReadyDeviceID -----
FLASH_STATUS FLASH_GetReadyDeviceID(uint8_t *bitmask)
{
    *bitmask = 1 << 0;
    return FLASH_PASS;
}

//------ FLASH_GetXferDoneDeviceID ----
FLASH_STATUS FLASH_GetXferDoneDeviceID(uint8_t *devID)
{
    *devID = 0;
    return FLASH_PASS;
}

//-------- FLASH_GetSectorReady --------
FLASH_STATUS FLASH_GetSectorReady(uint8_t *sysRAM, uint8_t *sysECCRAM, uint8_t dataDirection)
{
    return FLASH_PASS;
}

//-------- FLASH_IndicateSectorDone ---------
FLASH_STATUS FLASH_IndicateSectorDone(uint8_t *sysRAM, uint8_t *sysECCRAM, uint8_t dataDirection)
{
    return FLASH_PASS;
}

#if(FTL_DEBUG_FLASH_USAGE == true)
FTL_STATUS DBG_GetPhyDBGEraseCount(uint8_t devID, uint16_t phyEBNum, uint32_t *count)
{
    if((devID >= NUMBER_OF_DEVICES) || (phyEBNum >= NUMBER_OF_ERASE_BLOCKS))
    {
       *count = 0;
       return FTL_ERR_FAIL;
    }

    *count = DBGEraseCount[devID][phyEBNum];
    return FTL_ERR_PASS;
}
#endif
