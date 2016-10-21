#include "def.h"
#include "common.h"

//----------------------------

FTL_STATUS FTL_GetCapacity(FTL_CAPACITY * capacity)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

  if((status = FTL_CheckPointer(capacity)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckMount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }
  capacity->totalSize = NUM_DATA_EBLOCKS * NUMBER_OF_PAGES_PER_EBLOCK *
    NUMBER_OF_SECTORS_PER_PAGE * NUM_DEVICES * SECTOR_SIZE; //In Bytes
  capacity->numDBlocks = NUM_DATA_EBLOCKS * NUMBER_OF_PAGES_PER_EBLOCK *
    NUMBER_OF_SECTORS_PER_PAGE * NUM_DEVICES;
  capacity->numEBlocks = NUM_EBLOCKS_PER_DEVICE * NUM_DEVICES;
  capacity->eBlockSizeBytes = EBLOCK_SIZE;
  capacity->eBlockSizePages = NUM_PAGES_PER_EBLOCK;
  capacity->pageSizeDBlocks = NUM_SECTORS_PER_PAGE;
  capacity->pageSizeBytes = NUM_SECTORS_PER_PAGE * SECTOR_SIZE;
  capacity->numBlocks = capacity->numDBlocks;
  capacity->blockSize = capacity->eBlockSizeBytes; //NUM_EBLOCKS_PER_DEVICE * NUM_DEVICES;   // is equivalent to numEBlocks OBSOLETE
  capacity->pageSize = capacity->pageSizeBytes; //NUM_SECTORS_PER_PAGE * SECTOR_SIZE;     // is equivalent to pageSizeBytes OBSOLETE
  capacity->busWidth = 16; // hardcoded for now
  capacity->numDevices = NUM_DEVICES;
  FTL_ClearMTLockBit();
  return FTL_ERR_PASS;
  }

//--------------------------------

FTL_STATUS FTL_GetUtilizationInfo(uint32_t DeviceID, uint32_t * Free, uint32_t * Used, uint32_t * Dirty)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t numValidTemp = EMPTY_WORD; /*2*/
  uint16_t numInvalidTemp = EMPTY_WORD; /*2*/
  uint16_t numFreeTemp = EMPTY_WORD; /*2*/
  uint16_t numUsedTemp = EMPTY_WORD; /*2*/
  uint16_t logicalEBNum = 0; /*2*/

  if((status = FTL_CheckDevID((uint8_t) DeviceID)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckPointer(Free)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckPointer(Used)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckPointer(Dirty)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckMount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }
  for(logicalEBNum = 0; logicalEBNum < NUM_DATA_EBLOCKS; logicalEBNum++)
    {
#if (CACHE_RAM_BD_MODULE == true)
    if(FTL_ERR_PASS != (status = CACHE_LoadEB((FTL_DEV) DeviceID, logicalEBNum, CACHE_READ_TYPE)))
      {
      return status;
      }
#endif
    *Used = 0;
    *Dirty = 0;
    *Free = 0;
    GetNumValidUsedPages((FTL_DEV) DeviceID, logicalEBNum, &numUsedTemp, &numValidTemp);
    numInvalidTemp = numUsedTemp - numValidTemp;
    numFreeTemp = NUM_PAGES_PER_EBLOCK - numUsedTemp;
    *Used += numValidTemp;
    *Dirty += numInvalidTemp;
    *Free += numFreeTemp;
    }
  FTL_ClearMTLockBit();
  return FTL_ERR_PASS;
  }

//-------------------------------

FTL_STATUS FTL_GetEraseCycles(uint32_t DeviceID, uint32_t * high_block_number, uint32_t * high_erase_count,
                              uint32_t * low_block_number, uint32_t * low_erase_count)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t eraseCount = EMPTY_DWORD; /*4*/
  uint16_t tempCount = EMPTY_WORD; /*2*/
  uint32_t highestEraseCount = EMPTY_DWORD; /*4*/
  uint16_t highestEraseEBNum = EMPTY_WORD; /*2*/
  uint32_t lowestEraseCount = 0; /*4*/
  uint16_t lowestEraseEBNum = 0; /*2*/

  if((status = FTL_CheckDevID((uint8_t) DeviceID)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckPointer(high_block_number)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckPointer(high_erase_count)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckPointer(low_block_number)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckPointer(low_erase_count)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckMount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }
  for(tempCount = 0; tempCount < NUM_DATA_EBLOCKS; tempCount++)
    {
#if (CACHE_RAM_BD_MODULE == true)
    if(FTL_ERR_PASS != (status = CACHE_LoadEB((FTL_DEV) DeviceID, tempCount, CACHE_READ_TYPE)))
      {
      return status;
      }
#endif

    eraseCount = GetTrueEraseCount((FTL_DEV) DeviceID, tempCount);
    if(eraseCount < highestEraseCount)
      {
      highestEraseCount = eraseCount;
      highestEraseEBNum = tempCount;
      }
    if(eraseCount > lowestEraseCount)
      {
      lowestEraseCount = eraseCount;
      lowestEraseEBNum = tempCount;
      }
    }
  *high_block_number = highestEraseEBNum;
  *high_erase_count = highestEraseCount;
  *low_block_number = lowestEraseEBNum;
  *low_erase_count = lowestEraseCount;
  FTL_ClearMTLockBit();
  return FTL_ERR_PASS;
  }

//--------------------------------

FTL_STATUS TST_GetDevConfig(FTL_DEV_CONFIG* devConfig)
  {
  devConfig->dataEBlkStart = 0;
  devConfig->dataEBlkEnd = NUM_DATA_EBLOCKS - 1;
  devConfig->reserveEBlkStart = NUM_DATA_EBLOCKS;
  devConfig->reserveEBlkEnd = NUMBER_OF_ERASE_BLOCKS - 1;
  devConfig->sysEBlkStart = NUM_DATA_EBLOCKS;
  devConfig->sysEBlkEnd = NUMBER_OF_ERASE_BLOCKS - 1;
  devConfig->numDevices = NUM_DEVICES;
  devConfig->sectorsPerPage = NUM_SECTORS_PER_PAGE;
  devConfig->dataPagesPerEBlock = NUM_PAGES_PER_EBLOCK;
  devConfig->sysPagesPerEBlock = 0;

#if (FTL_EBLOCK_CHAINING == true)
  devConfig->eblockChainingEnabled = 1;

#else  // #if (FTL_EBLOCK_CHAINING == true)
  devConfig->eblockChainingEnabled = 0;
#endif  // #else  // #if (FTL_EBLOCK_CHAINING == true)

  return FTL_ERR_PASS;
  }

//-------------------------------

FTL_STATUS TST_GetPhysicalEBEraseCount(FTL_DEV devID, uint16_t phyEBNum, uint32_t * eraseCount)
  {
#if (CACHE_RAM_BD_MODULE == true)
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
#endif
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/

  *eraseCount = EMPTY_WORD;
  for(logicalEBNum = 0; logicalEBNum < NUM_EBLOCKS_PER_DEVICE; logicalEBNum++)
    {
#if (CACHE_RAM_BD_MODULE == true)
    if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, logicalEBNum, CACHE_READ_TYPE)))
      {
      return status;
      }
#endif
    if(FTL_ERR_PASS == TABLE_CheckUsedSysEB(devID, logicalEBNum))
      {
      continue;
      }
    if(GetPhysicalEBlockAddr(devID, logicalEBNum) == phyEBNum)
      {
      *eraseCount = GetTrueEraseCount(devID, logicalEBNum);
      break;
      }
    }
  return FTL_ERR_PASS;
  }

//------------------------------------

FTL_STATUS TST_GetLogicalEBEraseCount(FTL_DEV devID, uint16_t logicalEBNum, uint16_t * physicalEBNum, uint32_t * eraseCount)
  {
#if (CACHE_RAM_BD_MODULE == true)
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, logicalEBNum, CACHE_READ_TYPE)))
    {
    return status;
    }
#endif
  *physicalEBNum = GetPhysicalEBlockAddr(devID, logicalEBNum);
  *eraseCount = GetTrueEraseCount(devID, logicalEBNum);
  return FTL_ERR_PASS;
  ;
  }

//-------------------------------

FTL_STATUS TST_GetAverageEraseCount(FTL_DEV devID, uint16_t Start_Logical_eBlk,
                                    uint16_t End_Logical_eBlk, uint16_t * avgEraseCount)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t totalEraseCount = 0; /*4*/
  uint32_t totalEraseBlocks = 0; /*4*/
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/
  uint16_t physicalEBNum = EMPTY_WORD; /*2*/
  uint32_t eraseCount = 0; /*2*/
  uint16_t start = 0; /*2*/
  uint16_t end = 0; /*2*/

  totalEraseCount = 0;
  totalEraseBlocks = 0;
  start = Start_Logical_eBlk;
  end = End_Logical_eBlk;
  if(EMPTY_WORD == start)
    {
    start = 0;
    }
  if(EMPTY_WORD == end)
    {
    end = NUMBER_OF_ERASE_BLOCKS;
    }
  for(logicalEBNum = start; logicalEBNum < end; logicalEBNum++)
    {
#if (CACHE_RAM_BD_MODULE == true)
    if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, logicalEBNum, CACHE_READ_TYPE)))
      {
      return status;
      }
#endif
    status = TST_GetLogicalEBEraseCount(devID, logicalEBNum, &physicalEBNum, &eraseCount);
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
    totalEraseCount += (uint32_t) eraseCount;
    totalEraseBlocks++;
    }
  *avgEraseCount = (uint16_t) (totalEraseCount / totalEraseBlocks);
  return FTL_ERR_PASS;
  }

//--------------------------------------------

FTL_STATUS TST_CheckEraseCount(FTL_DEV devID, uint16_t Start_Logical_eBlk, uint16_t End_Logical_eBlk, uint16_t expected_value)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t blkCount = 0; /*2*/
  uint16_t phyEBNum = EMPTY_WORD; /*2*/
  uint32_t eraseCount = EMPTY_DWORD; /*2*/

  if(End_Logical_eBlk == EMPTY_WORD)
    {
    End_Logical_eBlk = Start_Logical_eBlk;
    }
  for(blkCount = Start_Logical_eBlk; blkCount <= End_Logical_eBlk; blkCount++)
    {
    if((status = TST_GetLogicalEBEraseCount(devID, blkCount, &phyEBNum, &eraseCount)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(eraseCount != expected_value)
      {
      //DBG_Printf("ERROR- TST_CheckEraseCount: logBlk = 0x%X, ", blkCount, 0);
      //DBG_Printf("phyBlk = 0x%X, ", phyEBNum, 0);
      //DBG_Printf("expectedEC = %d, ", expected_value, 0);
      //DBG_Printf("actualEC = %d\n", eraseCount, 0);
      return FTL_ERR_TST_ERASE_CNT_ERR;
      }
    }
  return FTL_ERR_PASS;
  }

//---------------------------------------

FTL_STATUS TST_LBAToPBA(uint32_t LBA, PBA_STRUCT_PTR pbaStruct, bool verifyFlash)
  {
  FLASH_PAGE_INFO pageInfo = {0, 0,
    {0, 0}}; /*11*/
  uint16_t phyPageIndex = EMPTY_WORD; /*2*/
  uint16_t dataByte = 0; /*2*/
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/
  uint16_t phyEBlockAddr = EMPTY_WORD; /*2*/
  uint16_t pageOffset = EMPTY_WORD; /*2*/
  uint32_t logicalPageAddr = EMPTY_DWORD; /*4*/

  pbaStruct->devID = EMPTY_BYTE;
  pbaStruct->PBA = EMPTY_DWORD;
  pbaStruct->PPA = EMPTY_DWORD;
  GetDevNum(LBA, &pbaStruct->devID);
  GetPageNum(LBA, (uint32_t *) & logicalPageAddr);
  GetSectorNum(LBA, (uint8_t *) & pageOffset);
  GetLogicalEBNum(logicalPageAddr, (uint16_t *) & logicalEBNum);
  phyPageIndex = GetPPASlot(pbaStruct->devID, logicalEBNum, (uint16_t) logicalPageAddr);
  if(phyPageIndex == EMPTY_INVALID)
    {
    return FTL_ERR_TST_PAGE_NOT_FOUND;
    }
  phyEBlockAddr = GetPhysicalEBlockAddr(pbaStruct->devID, logicalEBNum);
  pbaStruct->PPA = CalcPhyPageAddrFromPageOffset(phyEBlockAddr, phyPageIndex);
  pbaStruct->PBA = pbaStruct->PPA + pageOffset;
  // Verify the LBA 
  if(verifyFlash == true)
    {
    pageInfo.devID = pbaStruct->devID;
    pageInfo.byteCount = SECTOR_SIZE;
    pageInfo.vPage.vPageAddr = pbaStruct->PPA;
    pageInfo.vPage.pageOffset = pageOffset * SECTOR_SIZE;
    if((FLASH_RamPageReadDataBlock(&pageInfo,
                                   (uint8_t *) & pseudoRPB[pageInfo.devID][pageInfo.vPage.pageOffset])) != FLASH_PASS)
      {
      return FTL_ERR_FLASH_READ_01;
      }
    for(dataByte = 0; dataByte < SECTOR_SIZE; dataByte++)
      {
      if(pseudoRPB[pageInfo.devID][pageInfo.vPage.pageOffset + dataByte] != EMPTY_BYTE)
        {
        return FTL_ERR_PASS;
        }
      }
    return FTL_ERR_TST_LBA_MISMATCH;
    }
  return FTL_ERR_PASS;
  }

//--------------- TST_ReadDefectList -------------------------------

FTL_STATUS TST_ReadDefectList(FTL_DEFECT* buffer)
  {
  // No defects in NOR Flash
  buffer[0].devID = EMPTY_WORD;
  buffer[0].phyEBNum = EMPTY_WORD;
  return FTL_ERR_PASS;
  }

FTL_STATUS TST_WriteDefectList(FTL_DEFECT* buffer, uint16_t replace)
  {
  FTL_DEFECT defect = {0, 0}; /*4*/

  // No defects in NOR Flash
  defect.devID = buffer[0].devID;
  defect.phyEBNum = buffer[0].phyEBNum | replace;
  return FTL_ERR_PASS;
  }


//------ TST_ResetFlash -----
/*This function will reset the flash device immediately without waiting for any pending operations to complete.*/

/*This function will not wait for the device to become ready.It will return immediately.*/
FLASH_STATUS TST_ResetFlash(uint32_t devId)
  {
  // Stop any Transfer in Progress in HW specific fashion
  return FLASH_PASS;
  }
