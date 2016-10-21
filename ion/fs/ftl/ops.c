#include "lowlevel.h"
#include "common.h"

#include <string.h>

//------------------------------------------------------------------------------
/* This function will do the transfer map, and also the page address cache, when implimented */

/* isPreloaded and phyPage are needed for when the transfer map and address cache are implimented*/
FTL_STATUS FTL_BackgroundTask(void) /**/
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

  if((status = FTL_CheckMount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }

  FTL_ClearMTLockBit();
  return FTL_ERR_PASS;
  }

#if(FTL_REDUNDANT_LOG_ENTRIES == true)
//-----------------------------

FTL_STATUS FTL_WriteLogInfo(FLASH_PAGE_INFO_PTR flashPagePtr, uint8_t * logPtr)
  {
  uint16_t * logEntry = (uint16_t *) logPtr; /*4*/
  uint8_t redundantLog[LOG_ENTRY_SIZE * 2]; /*32*/

  logEntry[LOG_ENTRY_CHECK_WORD] = CalcCheckWord(&logEntry[LOG_ENTRY_DATA_START], LOG_ENTRY_DATA_WORDS);
  memcpy(&redundantLog[0], logPtr, LOG_ENTRY_SIZE);
  memcpy(&redundantLog[LOG_ENTRY_SIZE], logPtr, LOG_ENTRY_SIZE);
  flashPagePtr->byteCount += LOG_ENTRY_SIZE;
  if(FLASH_RamPageWriteMetaData(flashPagePtr, &redundantLog[0]) != FLASH_PASS)
    {
    return FTL_ERR_LOG_WR;
    }
  flashPagePtr->byteCount -= LOG_ENTRY_SIZE;
  return FTL_ERR_PASS;
  }

//----------------------------------

FTL_STATUS FTL_WriteFlushInfo(FLASH_PAGE_INFO_PTR flashPagePtr, SYS_EBLOCK_FLUSH_INFO_PTR flushInfoPtr)
  {
  uint16_t * flushInfoEntry = (uint16_t *) flushInfoPtr; /*4*/
  uint8_t redundantInfo[FLUSH_INFO_SIZE * 2]; /*32*/

  flushInfoEntry[FLUSH_INFO_CHECK_WORD] = CalcCheckWord(&flushInfoEntry[FLUSH_INFO_DATA_START], FLUSH_INFO_DATA_WORDS);
  memcpy(&redundantInfo[0], (uint8_t *) flushInfoEntry, FLUSH_INFO_SIZE);
  memcpy(&redundantInfo[FLUSH_INFO_SIZE], (uint8_t *) flushInfoEntry, FLUSH_INFO_SIZE);
  flashPagePtr->byteCount += FLUSH_INFO_SIZE;
  if(FLASH_RamPageWriteMetaData(flashPagePtr, &redundantInfo[0]) != FLASH_PASS)
    {
    return FTL_ERR_FLASH_WRITE_08;
    }
  flashPagePtr->byteCount -= FLUSH_INFO_SIZE;
  return FTL_ERR_PASS;
  }

#if (FTL_SUPER_SYS_EBLOCK == true)
//-----------------------------

FTL_STATUS FTL_WriteSuperInfo(FLASH_PAGE_INFO_PTR flashPagePtr, uint8_t * logPtr)
  {
  uint16_t * logEntry = (uint16_t *) logPtr; /*4*/
  uint8_t redundantLog[LOG_ENTRY_SIZE * 2]; /*32*/

  logEntry[LOG_ENTRY_CHECK_WORD] = CalcCheckWord(&logEntry[LOG_ENTRY_DATA_START], LOG_ENTRY_DATA_WORDS);
  memcpy(&redundantLog[0], logPtr, LOG_ENTRY_SIZE);
  memcpy(&redundantLog[LOG_ENTRY_SIZE], logPtr, LOG_ENTRY_SIZE);
  flashPagePtr->byteCount += LOG_ENTRY_SIZE;
  if(FLASH_RamPageWriteMetaData(flashPagePtr, &redundantLog[0]) != FLASH_PASS)
    {
    return FTL_ERR_SUPER_WRITE_01;
    }
  flashPagePtr->byteCount -= LOG_ENTRY_SIZE;
  return FTL_ERR_PASS;
  }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

//---------------------------------------

FTL_STATUS FTL_WriteSysEBlockInfo(FLASH_PAGE_INFO_PTR flashPagePtr, SYS_EBLOCK_INFO_PTR sysInfoPtr)
  {
  uint16_t * localPtr = (uint16_t *) sysInfoPtr; /*4*/
  uint8_t redundantInfo[SYS_INFO_SIZE * 2]; /*32*/

  // Write sector once
  localPtr[SYS_INFO_CHECK_WORD] = CalcCheckWord(&localPtr[SYS_INFO_DATA_START], SYS_INFO_DATA_WORDS);
  memcpy(&redundantInfo[0], (uint8_t *) localPtr, SYS_INFO_SIZE);
  memcpy(&redundantInfo[SYS_INFO_SIZE], (uint8_t *) localPtr, SYS_INFO_SIZE);
  flashPagePtr->byteCount += SYS_INFO_SIZE;
  if((FLASH_RamPageWriteMetaData(flashPagePtr, &redundantInfo[0])) != FLASH_PASS)
    {
    return FTL_ERR_FLASH_WRITE_12;
    }
  flashPagePtr->byteCount -= SYS_INFO_SIZE;
  return FTL_ERR_PASS;
  }

#else  // #if(FTL_REDUNDANT_LOG_ENTRIES == true)
//----------------------------------

FTL_STATUS FTL_WriteLogInfo(FLASH_PAGE_INFO_PTR flashPagePtr, uint8_t * logPtr)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t * logEntry = (uint16_t *) logPtr; /*4*/

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  uint32_t byteCount = 0; /*4*/
  uint32_t count = 0; /*4*/
  FLASH_STATUS flashStatus = FLASH_PASS;
  uint8_t storeArray[SECTOR_SIZE]; /*1*/
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

  logEntry[LOG_ENTRY_CHECK_WORD] = CalcCheckWord(&logEntry[LOG_ENTRY_DATA_START], LOG_ENTRY_DATA_WORDS);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  if(FLASH_RamPageWriteMetaData(flashPagePtr, (uint8_t *) logEntry) != FLASH_PASS)
    {
    return FTL_ERR_LOG_WR;
    }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  memcpy(storeArray, logPtr, (uint16_t) flashPagePtr->byteCount);
  for(count = flashPagePtr->byteCount; count < SECTOR_SIZE; count++)
    {
    storeArray[count] = EMPTY_BYTE;
    }
  byteCount = flashPagePtr->byteCount;
  flashPagePtr->byteCount = SECTOR_SIZE;
  flashStatus = FLASH_RamPageWriteDataBlock(flashPagePtr, storeArray);
  if(flashStatus != FLASH_PASS)
    {
#if(FTL_DEFECT_MANAGEMENT == true)
    if(flashStatus == FLASH_PARAM)
      {
      return FTL_ERR_FAIL;
      }
    if((status = TransLogEBFailure(flashPagePtr, storeArray)) != FTL_ERR_PASS)
      {
      return status;
      }

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    return FTL_ERR_LOG_WR;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

    }
  flashPagePtr->byteCount = byteCount;

  if(writeLogFlag == false)
    {
    writeLogFlag = true;
    }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

  return FTL_ERR_PASS;
  }

//----------------------------------

FTL_STATUS FTL_WriteFlushInfo(FLASH_PAGE_INFO_PTR flashPagePtr, SYS_EBLOCK_FLUSH_INFO_PTR flushInfoPtr)
  {
  uint16_t * flushInfoEntry = (uint16_t *) flushInfoPtr; /*4*/

  flushInfoEntry[FLUSH_INFO_CHECK_WORD] = CalcCheckWord(&flushInfoEntry[FLUSH_INFO_DATA_START], FLUSH_INFO_DATA_WORDS);
  if(FLASH_RamPageWriteMetaData(flashPagePtr, (uint8_t *) flushInfoEntry) != FLASH_PASS)
    {
    return FTL_ERR_FLASH_WRITE_08;
    }
  return FTL_ERR_PASS;
  }

#if (FTL_SUPER_SYS_EBLOCK == true)
#if (FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
//----------------------------------

FTL_STATUS InitPackedSuperInfo(void)
  {
  uint32_t count = 0; /*4*/

  for(count = 0; count < SECTOR_SIZE; count++)
    {
    packedSuperInfo[count] = EMPTY_BYTE;
    }
  return FTL_ERR_PASS;
  }

//----------------------------------

FTL_STATUS CopyPackedSuperInfo(uint16_t offset, uint8_t * logPtr)
  {
  uint16_t * logEntry = (uint16_t *) logPtr; /*4*/

  logEntry[LOG_ENTRY_CHECK_WORD] = CalcCheckWord(&logEntry[LOG_ENTRY_DATA_START], LOG_ENTRY_DATA_WORDS);
  memcpy(&packedSuperInfo[(offset * LOG_ENTRY_SIZE)], logPtr, LOG_ENTRY_SIZE);
  return FTL_ERR_PASS;
  }

//----------------------------------

FTL_STATUS WritePackedSuperInfo(FLASH_PAGE_INFO_PTR flashPagePtr)
  {
  uint32_t byteCount = 0; /*4*/
  FLASH_STATUS flashStatus = FLASH_PASS;

  byteCount = flashPagePtr->byteCount;
  flashPagePtr->byteCount = SECTOR_SIZE;
  if((flashStatus = FLASH_RamPageWriteDataBlock(flashPagePtr, &packedSuperInfo[0])) != FLASH_PASS)
    {
#if(FTL_DEFECT_MANAGEMENT == true)
    if(flashStatus == FLASH_PARAM)
      {
      return FTL_ERR_FAIL;
      }
    if(FLASH_MarkDefectEBlock(flashPagePtr) != FLASH_PASS)
      {
      // do nothing, just try to mark bad, even if it fails we move on.
      }
    return FTL_ERR_SUPER_WRITE_02;
#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    return FTL_ERR_SUPER_WRITE_02;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

    }
  flashPagePtr->byteCount = byteCount;
  return FTL_ERR_PASS;
  }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

//-----------------------------

FTL_STATUS FTL_WriteSuperInfo(FLASH_PAGE_INFO_PTR flashPagePtr, uint8_t * logPtr)
  {
  uint16_t * logEntry = (uint16_t *) logPtr; /*4*/

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  uint32_t byteCount = 0; /*4*/
  uint32_t count = 0; /*4*/
  FLASH_STATUS flashStatus = FLASH_PASS;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

  logEntry[LOG_ENTRY_CHECK_WORD] = CalcCheckWord(&logEntry[LOG_ENTRY_DATA_START], LOG_ENTRY_DATA_WORDS);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  if(FLASH_RamPageWriteMetaData(flashPagePtr, (uint8_t *) logEntry) != FLASH_PASS)
    {
    return FTL_ERR_SUPER_WRITE_05;
    }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  memcpy(&pseudoRPB[flashPagePtr->devID][0], logPtr, (uint16_t) flashPagePtr->byteCount);
  for(count = flashPagePtr->byteCount; count < SECTOR_SIZE; count++)
    {
    pseudoRPB[flashPagePtr->devID][count] = EMPTY_BYTE;
    }
  byteCount = flashPagePtr->byteCount;
  flashPagePtr->byteCount = SECTOR_SIZE;
  flashStatus = FLASH_RamPageWriteDataBlock(flashPagePtr, &pseudoRPB[flashPagePtr->devID][0]);
  if(flashStatus != FLASH_PASS)
    {
#if(FTL_DEFECT_MANAGEMENT == true)
    if(flashStatus == FLASH_PARAM)
      {
      return FTL_ERR_FAIL;
      }
    if(FLASH_MarkDefectEBlock(flashPagePtr) != FLASH_PASS)
      {
      // do nothing, just try to mark bad, even if it fails we move on.
      }
    return FTL_ERR_SUPER_WRITE_03;
#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    return FTL_ERR_SUPER_WRITE_03;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

    }
  flashPagePtr->byteCount = byteCount;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

  return FTL_ERR_PASS;
  }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)


//---------------------------------------

FTL_STATUS FTL_WriteSysEBlockInfo(FLASH_PAGE_INFO_PTR flashPagePtr, SYS_EBLOCK_INFO_PTR sysInfoPtr)
  {
  uint16_t * localPtr = (uint16_t *) sysInfoPtr; /*4*/

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  uint32_t byteCount = 0; /*4*/
  uint32_t count = 0; /*4*/
  FLASH_STATUS flashStatus = FLASH_PASS;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

  // Write sector once
  localPtr[SYS_INFO_CHECK_WORD] = CalcCheckWord(&localPtr[SYS_INFO_DATA_START], SYS_INFO_DATA_WORDS);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  if((FLASH_RamPageWriteMetaData(flashPagePtr, (uint8_t *) localPtr)) != FLASH_PASS)
    {
    return FTL_ERR_FLASH_WRITE_12;
    }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  memcpy(&pseudoRPB[flashPagePtr->devID][0], (uint8_t *) sysInfoPtr, (uint16_t) flashPagePtr->byteCount);
  for(count = flashPagePtr->byteCount; count < SECTOR_SIZE; count++)
    {
    pseudoRPB[flashPagePtr->devID][count] = EMPTY_BYTE;
    }
  byteCount = flashPagePtr->byteCount;
  flashPagePtr->byteCount = SECTOR_SIZE;
  flashStatus = FLASH_RamPageWriteDataBlock(flashPagePtr, &pseudoRPB[flashPagePtr->devID][0]);
  if(flashStatus != FLASH_PASS)
    {
    if(flashStatus == FLASH_PARAM)
      {
      return FTL_ERR_FAIL;
      }
    return FTL_ERR_FLASH_WRITE_12;
    }
  flashPagePtr->byteCount = byteCount;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

  return FTL_ERR_PASS;
  }
#endif  // #else  // #if(FTL_REDUNDANT_LOG_ENTRIES == true)

//---------------------------------

FTL_STATUS FTL_EraseAllTransLogBlocksOp(FTL_DEV devID)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint8_t tempCount = 0x0; /*1*/
  uint16_t logicalAddr = 0x0; /*2*/
  uint16_t phyEBAddr = 0x0; /*2*/
  uint16_t eBlockNum = 0x0; /*2*/
  uint32_t latestIncNumber = 0x0; /*4*/
  uint16_t nextLogicalEBAddr = 0; /*2*/
  uint16_t nextPhyEBAddr = 0; /*2*/
  FLASH_PAGE_INFO flashPageInfo = {0, 0,
    {0, 0}}; /*11*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  SYS_EBLOCK_INFO_PTR tempSysPtr = NULL; /*4*/
  FLASH_STATUS flashStatus = FLASH_PASS;

#if (ENABLE_EB_ERASED_BIT == true)
  uint8_t eraseStatus = false; /*1*/
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

#if(FTL_DEFECT_MANAGEMENT == true)
  uint8_t checkBBMark = false; /*1*/
#endif

  if((status = FTL_FindEmptyTransLogEBlock(devID, &nextLogicalEBAddr, &nextPhyEBAddr)) != FTL_ERR_PASS)
    {
    return status;
    }

#if (FTL_SUPER_SYS_EBLOCK == true)
  if((false == SuperEBInfo[devID].checkLost) && (false == SuperEBInfo[devID].checkSuperPF) && (false == SuperEBInfo[devID].checkSysPF))
    {
    if((status = FTL_CreateSuperSysEBLog(devID, SYS_EBLOCK_INFO_CHANGED)) != FTL_ERR_PASS)
      {
      return status;
      }
    }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if (ENABLE_EB_ERASED_BIT == true)
  eraseStatus = GetEBErased(devID, nextLogicalEBAddr);

  if(false == eraseStatus)
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
    {

    if((status = FTL_EraseOp(devID, nextLogicalEBAddr)) != FTL_ERR_PASS)
      {

#if(FTL_DEFECT_MANAGEMENT == true)
      if(status == FTL_ERR_FAIL)
        {
        return status;
        }
      SetBadEBlockStatus(devID, nextLogicalEBAddr, true);
      flashPageInfo.devID = devID;
      flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(nextPhyEBAddr, 0);
      flashPageInfo.vPage.pageOffset = 0;
      flashPageInfo.byteCount = 0;
      if(FLASH_MarkDefectEBlock(&flashPageInfo) != FLASH_PASS)
        {
        // do nothing, just try to mark bad, even if it fails we move on.
        }
      return FTL_ERR_LOG_NEW_EBLOCK_FAIL;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
      return status;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

      }
    }
  flashPageInfo.devID = devID;
  flashPageInfo.vPage.pageOffset = 0;
  flashPageInfo.byteCount = sizeof (SYS_EBLOCK_INFO);
  flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(nextPhyEBAddr, 0);
  //construct the system info for the Log Eblock with new key
  sysEBlockInfo.phyAddrThisEBlock = nextPhyEBAddr;
  sysEBlockInfo.incNumber = (GetTransLogEBCounter(devID) + 1);
  sysEBlockInfo.type = SYS_EBLOCK_INFO_LOG;
  sysEBlockInfo.checkVersion = EMPTY_WORD;
  sysEBlockInfo.oldSysBlock = EMPTY_WORD;
  sysEBlockInfo.fullFlushSig = EMPTY_WORD;
  for(tempCount = 0; tempCount < sizeof (sysEBlockInfo.reserved); tempCount++)
    {
    sysEBlockInfo.reserved[tempCount] = EMPTY_BYTE;
    }
  if((status = FTL_WriteSysEBlockInfo(&flashPageInfo, &sysEBlockInfo)) != FTL_ERR_PASS)
    {

#if(FTL_DEFECT_MANAGEMENT == true)
    if(status == FTL_ERR_FAIL)
      {
      return status;
      }
    SetBadEBlockStatus(devID, nextLogicalEBAddr, true);
    if(FLASH_MarkDefectEBlock(&flashPageInfo) != FLASH_PASS)
      {
      // do nothing, just try to mark bad, even if it fails we move on.
      }
    return FTL_ERR_LOG_NEW_EBLOCK_FAIL;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    return status;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

    }
#if (ENABLE_EB_ERASED_BIT == true)
  SetEBErased(devID, nextLogicalEBAddr, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
  // Initialize EBlock Info entries
  SetDirtyCount(devID, nextLogicalEBAddr, 0);
  SetGCOrFreePageNum(devID, nextLogicalEBAddr, 1);
  MarkEBlockMappingTableEntryDirty(devID, nextLogicalEBAddr);
  SetTransLogEBCounter(devID, sysEBlockInfo.incNumber);
  // This is required to overwrite the old entry.
  for(eBlockNum = 0; eBlockNum < NUM_TRANSACTLOG_EBLOCKS; eBlockNum++)
    {
    if((status = TABLE_GetTransLogEntry(devID, eBlockNum, &logicalAddr, &phyEBAddr, &latestIncNumber)) != FTL_ERR_PASS)
      {
      return status; // trying to excess outside table.
      }
    if((logicalAddr == EMPTY_WORD) && (phyEBAddr == EMPTY_WORD) && (latestIncNumber == EMPTY_DWORD))
      {
      break; // no more entries in table
      }
#if(FTL_DEFECT_MANAGEMENT == false)
    if((status = TABLE_InsertReservedEB(devID, logicalAddr)) != FTL_ERR_PASS)
      {
      return status;
      }
#endif
    //write  OLD_SYS_BLOCK_SIGNATURE
    sysEBlockInfo.oldSysBlock = OLD_SYS_BLOCK_SIGNATURE;
    flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBAddr, 0);
    flashPageInfo.vPage.pageOffset = (uint16_t) ((uint32_t)&(tempSysPtr->oldSysBlock));
    flashPageInfo.byteCount = OLD_SYS_BLOCK_SIGNATURE_SIZE;
    //     flashPageInfo.byteCount = sizeof(sysEBlockInfo.oldSysBlock);
    flashStatus = FLASH_RamPageWriteMetaData(&flashPageInfo, (uint8_t *) & sysEBlockInfo.oldSysBlock);
    if(flashStatus != FLASH_PASS)
      {

#if(FTL_DEFECT_MANAGEMENT == true)
      if(flashStatus == FLASH_PARAM)
        {
        return FTL_ERR_FLASH_WRITE_09;
        }
      SetBadEBlockStatus(devID, logicalAddr, true);
      // just try to mark bad, even if it fails we move on.
      FLASH_MarkDefectEBlock(&flashPageInfo);

      checkBBMark = true;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
      return FTL_ERR_FLASH_WRITE_09;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

      }
#if(FTL_DEFECT_MANAGEMENT == true)
    if((status = TABLE_InsertReservedEB(devID, logicalAddr)) != FTL_ERR_PASS)
      {
      return status;
      }
#endif
    }
  if((status = TABLE_TransEBClear(devID)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = TABLE_TransLogEBInsert(devID, nextLogicalEBAddr, nextPhyEBAddr, sysEBlockInfo.incNumber)) != FTL_ERR_PASS)
    {
    return status;
    }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  writeLogFlag = false;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

#if(FTL_DEFECT_MANAGEMENT == true)
  if(true == checkBBMark)
    {
    return FTL_ERR_MARKBB_COMMIT;
    }
#endif

  return FTL_ERR_PASS;
  }


#if (FTL_SUPER_SYS_EBLOCK == true)
//------------------------------

FTL_STATUS GetNextSuperSysEBEntryLocation(FTL_DEV devID, FLASH_PAGE_INFO_PTR pageInfoPtr, uint16_t * entryIndexPtr) /*  1,  4*/
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t latestIncNumber = EMPTY_DWORD; /*4*/
  uint16_t phyEBlockAddr = EMPTY_WORD; /*2*/
  uint16_t logicalBlockNum = EMPTY_WORD; /*2*/
  uint16_t entryIndex = EMPTY_WORD; /*2*/
  uint16_t byteOffset = 0; /*2*/

  // Initialize variables
  pageInfoPtr->devID = devID;
  pageInfoPtr->byteCount = LOG_ENTRY_SIZE;
  latestIncNumber = GetSuperSysEBCounter(devID);
  status = TABLE_SuperSysEBGetLatest(devID, &logicalBlockNum, &phyEBlockAddr, latestIncNumber);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }
  // Get offset to free entry
  entryIndex = GetFreePageIndex(devID, logicalBlockNum);
  if(entryIndex < NUM_LOG_ENTRIES_PER_EBLOCK)
    {
    // Latest EBlock has room for more
    byteOffset = entryIndex * LOG_ENTRY_DELTA;
    pageInfoPtr->vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddr, entryIndex);
    pageInfoPtr->vPage.pageOffset = byteOffset % VIRTUAL_PAGE_SIZE;
    SetGCOrFreePageNum(devID, logicalBlockNum, entryIndex + 1);
    *entryIndexPtr = entryIndex;
    MarkEBlockMappingTableEntryDirty(devID, logicalBlockNum);

    //#if(FTL_DEFECT_MANAGEMENT == true)
    //SetTransLogEBNumBadBlockInfo(logicalBlockNum);
    //#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

    }
  else
    {
    return FTL_ERR_SUPER_CANNOT_FIND_NEXT_ENTRY;
    }

  return FTL_ERR_PASS;
  }

//---------------------------------

FTL_STATUS CreateNextSuperSystemEBlockOp(FTL_DEV devID)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint8_t tempCount = 0x0; /*1*/
  uint16_t logicalAddr = 0x0; /*2*/
  uint16_t phyEBAddr = 0x0; /*2*/
  uint16_t eBlockNum = 0x0; /*2*/
  uint32_t latestIncNumber = 0x0; /*4*/
  uint16_t nextLogicalEBAddr = 0; /*2*/
  uint16_t nextPhyEBAddr = 0; /*2*/
  FLASH_PAGE_INFO flashPageInfo = {0, 0,
    {0, 0}}; /*11*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  SYS_EBLOCK_INFO_PTR tempSysPtr = NULL; /*4*/
  FLASH_STATUS flashStatus = FLASH_PASS; /*1*/

  uint16_t findDataEBNum;
  uint16_t findSysEBNum;

#if (ENABLE_EB_ERASED_BIT == true)
  uint8_t eraseStatus = false; /*1*/
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

#if (FTL_DEFECT_MANAGEMENT == true)
  uint16_t sanityCounter = 0; /*2*/
  uint16_t findTries = 0; /*2*/
  uint8_t checkBBMark = false; /*1*/
#endif  // #if (FTL_DEFECT_MANAGEMENT == true)

#if (FTL_DEFECT_MANAGEMENT == true)
  do
    {
    findTries++;
    if(findTries > 2)
      {
      return status;
      }
#endif

    if((status = FTL_FindEmptySuperSysEBlock(devID, &nextLogicalEBAddr, &nextPhyEBAddr)) != FTL_ERR_PASS)
      {
      FTL_FindAllAreaSuperSysEBlock(devID, &findDataEBNum, &findSysEBNum);
      if(1 <= findSysEBNum)
        {
        gProtectForSuperSysEBFlag = true;
#if(FTL_DEFECT_MANAGEMENT == true)
        sanityCounter = 0;
        while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
          {
          if((status = Flush_GC(devID)) != FTL_ERR_FLUSH_FLUSH_GC_FAIL)
            {
            break;
            }
          sanityCounter++;
          }
        if(status != FTL_ERR_PASS)
          {
          return status;
          }
#else
        if((status = Flush_GC(devID)) != FTL_ERR_PASS)
          {
          return status;
          }
#endif
        gProtectForSuperSysEBFlag = false;
        if((status = FTL_FindEmptySuperSysEBlock(devID, &nextLogicalEBAddr, &nextPhyEBAddr)) != FTL_ERR_PASS)
          {
#if (FTL_DEFECT_MANAGEMENT == true)
          continue;
#else
          return status;
#endif
          }
        }
      else if(1 <= findDataEBNum)
        {

        if((status = DataGCForSuperSysEB()) != FTL_ERR_PASS)
          {
          return status;
          }
        if((status = FTL_FindEmptySuperSysEBlock(devID, &nextLogicalEBAddr, &nextPhyEBAddr)) != FTL_ERR_PASS)
          {
#if (FTL_DEFECT_MANAGEMENT == true)
          continue;
#else
          return status;
#endif
          }
        }
      }
#if (FTL_DEFECT_MANAGEMENT == true)
    }
  while(status != FTL_ERR_PASS);
#endif


#if (ENABLE_EB_ERASED_BIT == true)
  eraseStatus = GetEBErased(devID, nextLogicalEBAddr);

  if(false == eraseStatus)
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
    {

    if((status = FTL_EraseOp(devID, nextLogicalEBAddr)) != FTL_ERR_PASS)
      {

#if(FTL_DEFECT_MANAGEMENT == true)
      if(status == FTL_ERR_FAIL)
        {
        return status;
        }
      SetBadEBlockStatus(devID, nextLogicalEBAddr, true);
      flashPageInfo.devID = devID;
      flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(nextPhyEBAddr, 0);
      flashPageInfo.vPage.pageOffset = 0;
      flashPageInfo.byteCount = 0;
      if(FLASH_MarkDefectEBlock(&flashPageInfo) != FLASH_PASS)
        {
        // do nothing, just try to mark bad, even if it fails we move on.
        }
      return FTL_ERR_SUPER_LOG_NEW_EBLOCK_FAIL_01;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
      return status;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

      }
    }
  flashPageInfo.devID = devID;
  flashPageInfo.vPage.pageOffset = 0;
  flashPageInfo.byteCount = sizeof (SYS_EBLOCK_INFO);
  flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(nextPhyEBAddr, 0);
  //construct the system info for the Log Eblock with new key
  sysEBlockInfo.phyAddrThisEBlock = nextPhyEBAddr;
  sysEBlockInfo.incNumber = (GetSuperSysEBCounter(devID) + 1);
  sysEBlockInfo.type = SYS_EBLOCK_INFO_SUPER;
  sysEBlockInfo.checkVersion = EMPTY_WORD;
  sysEBlockInfo.oldSysBlock = EMPTY_WORD;
  sysEBlockInfo.fullFlushSig = EMPTY_WORD;
  for(tempCount = 0; tempCount < sizeof (sysEBlockInfo.reserved); tempCount++)
    {
    sysEBlockInfo.reserved[tempCount] = EMPTY_BYTE;
    }
  if((status = FTL_WriteSysEBlockInfo(&flashPageInfo, &sysEBlockInfo)) != FTL_ERR_PASS)
    {

#if(FTL_DEFECT_MANAGEMENT == true)
    if(status == FTL_ERR_FAIL)
      {
      return status;
      }
    SetBadEBlockStatus(devID, nextLogicalEBAddr, true);
    if(FLASH_MarkDefectEBlock(&flashPageInfo) != FLASH_PASS)
      {
      // do nothing, just try to mark bad, even if it fails we move on.
      }
    return FTL_ERR_SUPER_LOG_NEW_EBLOCK_FAIL_02;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    return status;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

    }
#if (ENABLE_EB_ERASED_BIT == true)
  SetEBErased(devID, nextLogicalEBAddr, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
  // Initialize EBlock Info entries
  SetDirtyCount(devID, nextLogicalEBAddr, 0);
  SetGCOrFreePageNum(devID, nextLogicalEBAddr, 1);
  MarkEBlockMappingTableEntryDirty(devID, nextLogicalEBAddr);
  SetSuperSysEBCounter(devID, sysEBlockInfo.incNumber);
  // This is required to overwrite the old entry.
  for(eBlockNum = 0; eBlockNum < NUM_SUPER_SYS_EBLOCKS; eBlockNum++)
    {
    if((status = TABLE_GetSuperSysEBEntry(devID, eBlockNum, &logicalAddr, &phyEBAddr, &latestIncNumber)) != FTL_ERR_PASS)
      {
      return status; // trying to excess outside table.
      }
    if((logicalAddr == EMPTY_WORD) && (phyEBAddr == EMPTY_WORD) && (latestIncNumber == EMPTY_DWORD))
      {
      break; // no more entries in table
      }
#if(FTL_DEFECT_MANAGEMENT == false)
    if((status = TABLE_InsertReservedEB(devID, logicalAddr)) != FTL_ERR_PASS)
      {
      return status;
      }
#endif
    //write  OLD_SYS_BLOCK_SIGNATURE
    sysEBlockInfo.oldSysBlock = OLD_SYS_BLOCK_SIGNATURE;
    flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBAddr, 0);
    flashPageInfo.vPage.pageOffset = (uint16_t) ((uint32_t)&(tempSysPtr->oldSysBlock));
    flashPageInfo.byteCount = OLD_SYS_BLOCK_SIGNATURE_SIZE;
    flashStatus = FLASH_RamPageWriteMetaData(&flashPageInfo, (uint8_t *) & sysEBlockInfo.oldSysBlock);
    if(flashStatus != FLASH_PASS)
      {

#if(FTL_DEFECT_MANAGEMENT == true)
      if(FLASH_PARAM == flashStatus)
        {
        return FTL_ERR_SUPER_FLASH_WRITE_01;
        }
      SetBadEBlockStatus(devID, logicalAddr, true);
      // just try to mark bad, even if it fails we move on.
      FLASH_MarkDefectEBlock(&flashPageInfo);
      checkBBMark = true;
#else  // #if(FTL_DEFECT_MANAGEMENT == true)
      return FTL_ERR_SUPER_FLASH_WRITE_01;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

      }
#if(FTL_DEFECT_MANAGEMENT == true)
    if(false == checkBBMark)
      {
      if((status = TABLE_InsertReservedEB(devID, logicalAddr)) != FTL_ERR_PASS)
        {
        return status;
        }
      checkBBMark = false;
      }
#endif
    }
  if((status = TABLE_SuperSysEBClear(devID)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = TABLE_SuperSysEBInsert(devID, nextLogicalEBAddr, nextPhyEBAddr, sysEBlockInfo.incNumber)) != FTL_ERR_PASS)
    {
    return status;
    }

  return FTL_ERR_PASS;
  }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

FTL_STATUS FTL_RemoveOldTransLogBlocks(FTL_DEV devID, uint32_t logEBCounter)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t eBlockCount = 0; /*2*/
  uint16_t blockNum = EMPTY_WORD; /*2*/
  uint16_t logicalEBAddr = EMPTY_WORD; /*2*/
  uint16_t physicalEBAddr = EMPTY_WORD; /*2*/
  uint32_t key = EMPTY_DWORD; /*4*/

  for(eBlockCount = 0; eBlockCount < NUM_TRANSACTLOG_EBLOCKS; eBlockCount++)
    {
    if((status = TABLE_GetTransLogEntry(devID, eBlockCount, &logicalEBAddr, &physicalEBAddr, &key)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(key == EMPTY_DWORD)
      {
      break;
      }
    if(key <= logEBCounter)
      {
      blockNum = eBlockCount;
      }
    }
  if(blockNum != EMPTY_WORD)
    {
    if((status = TABLE_TransLogEBRemove(devID, blockNum)) != FTL_ERR_PASS)
      {
      return status;
      }
    }

  return FTL_ERR_PASS;
  }

///----------------------------------

FTL_STATUS FTL_EraseOp(FTL_DEV devID, uint16_t logicalEBNum)
  {
  FTL_STATUS status = FTL_ERR_PASS;
#if (CACHE_RAM_BD_MODULE == true)
#if (FTL_STATIC_WEAR_LEVELING == true)
  uint32_t eraseCount = 0;
  uint16_t lowLogicalEBNum = 0;
  uint32_t lowestCount = 0;
#endif
#endif

  status = FTL_EraseOpNoDirty(devID, logicalEBNum);
  if(status != FTL_ERR_PASS)
    {
    return status;
    }
  IncEraseCount(devID, logicalEBNum);

#if (CACHE_RAM_BD_MODULE == true)
#if (FTL_STATIC_WEAR_LEVELING == true)
  eraseCount = GetTrueEraseCount(devID, logicalEBNum);
  status = SetSaveStaticWL(devID, logicalEBNum, eraseCount);
  if(status != FTL_ERR_PASS)
    {
    return status;
    }
  status = GetSaveStaticWL(devID, &lowLogicalEBNum, &lowestCount, CACHE_WL_LOW);
  if(status != FTL_ERR_PASS)
    {
    return status;
    }
  if(lowLogicalEBNum == logicalEBNum)
    {
    status = ClearSaveStaticWL(devID, logicalEBNum, eraseCount, CACHE_WL_LOW);
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
    }
#endif
#endif
#if (ENABLE_EB_ERASED_BIT == true)
  SetEBErased(devID, logicalEBNum, true);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
  MarkEBlockMappingTableEntryDirty(devID, logicalEBNum);
  return FTL_ERR_PASS;
  }

FTL_STATUS FTL_EraseOpNoDirty(FTL_DEV devID, uint16_t logicalEBNum)
  {
  FLASH_PAGE_INFO pageInfo = {0, 0,
    {0, 0}}; /*11*/
  uint16_t phyEBNum = 0; /*2*/
  FLASH_STATUS flashStatus = FLASH_PASS;

  phyEBNum = GetPhysicalEBlockAddr(devID, logicalEBNum);
  pageInfo.devID = devID;
  pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyEBNum, 0);
  pageInfo.vPage.pageOffset = 0;
  pageInfo.byteCount = 0;

  flashStatus = FLASH_Erase(&pageInfo);
  if(flashStatus != FLASH_PASS)
    {
    if(flashStatus == FLASH_PARAM)
      {
      return FTL_ERR_FAIL;
      }
    return FTL_ERR_FLASH_ERASE_01;
    }
  return FTL_ERR_PASS;
  }
//---------------------------------

uint32_t GetTotalDirtyBitCnt(FTL_DEV devID)
  {
  uint16_t secCnt = 0; /*2*/
  uint32_t dirtyBitTotalCnt = 0; /*4*/

#if (CACHE_RAM_BD_MODULE == true)
  for(secCnt = 0; secCnt < NUM_EBLOCK_MAP_INDEX; secCnt++)
    {
    if(true == CACHE_IsDirtyIndex(devID, secCnt))
      {
      dirtyBitTotalCnt++; // EB
      dirtyBitTotalCnt = dirtyBitTotalCnt + PPA_CACHE_TABLE_OFFSET; // PPA 
      }
    }
#else
  for(secCnt = 0; secCnt < BITS_EBLOCK_DIRTY_BITMAP_DEV_TABLE; secCnt++)
    {
    if(IsEBlockMappingTableSectorDirty(devID, secCnt))
      {
      dirtyBitTotalCnt++;
      }
    }
  for(secCnt = 0; secCnt < BITS_PPA_DIRTY_BITMAP_DEV_TABLE; secCnt++)
    {
    if(IsPPAMappingTableSectorDirty(devID, secCnt))
      {
      dirtyBitTotalCnt++;
      }
    }
#endif
  return dirtyBitTotalCnt;
  }

//----------------------------------

FTL_STATUS CheckFlushSpace(FTL_DEV devID)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t phyEBlockAddr = 0x0; /*2*/
  uint16_t freeIndex = 0x0; /*2*/
  uint16_t logicalEBlockNum = 0x0; /*2*/
  uint32_t latestIncNumber = 0x0; /*4*/
  uint32_t dirtyBitTotalCnt = 0x0; /*4*/

#if(FTL_DEFECT_MANAGEMENT == true)
  uint16_t sanityCounter = 0; /*2*/
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

  latestIncNumber = GetFlushEBCounter(devID);
  if((status = TABLE_FlushEBGetLatest(devID, &logicalEBlockNum, &phyEBlockAddr, latestIncNumber)) != FTL_ERR_PASS)
    {
    return status;
    }
  freeIndex = GetFreePageIndex(devID, logicalEBlockNum);
  MarkEBlockMappingTableEntryDirty(devID, logicalEBlockNum);
  // Get the total dirty bits
  dirtyBitTotalCnt = GetTotalDirtyBitCnt(devID);
  /* (MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK-1) is used instead of (MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK). */
#if(CACHE_RAM_BD_MODULE == false)
  if(dirtyBitTotalCnt >= (MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK - 1))
#else
  if(dirtyBitTotalCnt >= (MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK - (1 + PPA_CACHE_TABLE_OFFSET)))
#endif
    {
    return FTL_ERR_FLUSH_GC_NEEDED;
    }
  if((freeIndex + dirtyBitTotalCnt) >= MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK)
    {
    if(TABLE_GetReservedEBlockNum(devID) <= MIN_SYSTEM_RESERVE_EBLOCKS)
      {
      return FTL_ERR_FLUSH_GC_NEEDED;
      }
    if(GetFlushLogEBArrayCount(devID) >= NUM_FLUSH_LOG_EBLOCKS)
      {
      return FTL_ERR_FLUSH_GC_NEEDED;
      }
    SetGCOrFreePageNum(devID, logicalEBlockNum, MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK);

#if(FTL_DEFECT_MANAGEMENT == true)
    while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
      {
      if((status = TABLE_GetReservedEB(devID, &logicalEBlockNum, false)) != FTL_ERR_PASS)
        {
        return status;
        }
      if((status = CreateNextFlushEntryLocation(devID, logicalEBlockNum)) != FTL_ERR_FLUSH_NEXT_EBLOCK_FAIL)
        {
        break;
        }
      sanityCounter++;
      }
    if(status != FTL_ERR_PASS)
      {
      return status;
      }

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    if((status = TABLE_GetReservedEB(devID, &logicalEBlockNum, false)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = CreateNextFlushEntryLocation(devID, logicalEBlockNum)) != FTL_ERR_PASS)
      {
      return status;
      }
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

    }
  return FTL_ERR_PASS;
  }

FTL_STATUS GetLogEntryLocation(FTL_DEV devID, LOG_ENTRY_LOC_PTR nextLoc)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/
  uint16_t physicalEBNum = EMPTY_WORD; /*2*/
  uint32_t key = 0; /*4*/
  uint16_t count = 0; /*2*/
  uint16_t logEBAddr = EMPTY_WORD; /*2*/
  uint16_t phyEBAddr = EMPTY_WORD; /*2*/
  uint32_t latestIncNumber = 0; /*4*/
  uint16_t eBlockNum = EMPTY_WORD; /*2*/
  uint16_t entryIndex = EMPTY_WORD; /*2*/

  key = GetTransLogEBCounter(devID);
  if((status = TABLE_TransLogEBGetLatest(devID, &logicalEBNum, &physicalEBNum, key)) != FTL_ERR_PASS)
    {
    return status;
    }
  for(count = 0; count < NUM_TRANSACTLOG_EBLOCKS; count++)
    {
    if((status = TABLE_GetTransLogEntry(devID, count, &logEBAddr, &phyEBAddr, &latestIncNumber)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((logEBAddr == EMPTY_WORD) && (phyEBAddr == EMPTY_WORD) && (latestIncNumber == EMPTY_DWORD))
      {
      break;
      }
    if((logEBAddr == logicalEBNum) && (phyEBAddr == physicalEBNum) && (latestIncNumber == key))
      {
      eBlockNum = count;
      break;
      }
    }
  if(eBlockNum != EMPTY_WORD)
    {
    entryIndex = GetFreePageIndex(devID, logicalEBNum);
    nextLoc->eBlockNum = eBlockNum;
    nextLoc->entryIndex = entryIndex;
    }
  else
    {
    return FTL_ERR_LOG_NO_FOUND_EBLOCK;
    }
  return FTL_ERR_PASS;
  }

//-----------------------------------

FTL_STATUS TABLE_FlushDevice(FTL_DEV devID, uint8_t flushMode)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint8_t numRamTables = 0x2; /*1*/
#if (CACHE_RAM_BD_MODULE == false)
  uint8_t bitMapData = 0x0; /*1*/
#endif
  uint8_t flushTypeCnt = 0x0; /*1*/
#if (CACHE_RAM_BD_MODULE == false)
  uint32_t dataByteCnt = 0x0; /*4*/
#endif
  uint16_t logicalBlockNum = 0x0; /*2*/
#if (CACHE_RAM_BD_MODULE == false)
  uint32_t totalRamTableBytes = 0x0; /*4*/
  uint16_t bitMapCounter = 0x0; /*2*/
#endif
  uint32_t dirtyBitTotalCnt = 0x0; /*4*/
#if (CACHE_RAM_BD_MODULE == false)
  uint32_t dirtyBitCounter = 0x0; /*4*/
  uint16_t tempCount = 0; /*2*/
  uint8_t * dirtyBitMapPtr = NULL; /*4*/
  uint8_t * ramMappingTablePtr = NULL; /*4*/
#endif
  FLASH_PAGE_INFO flushStructPageInfo = {0, 0,
    {0, 0}}; /*11*/
  FLASH_PAGE_INFO flushRAMTablePageInfo = {0, 0,
    {0, 0}}; /*11*/
#if (CACHE_RAM_BD_MODULE == false)
  SYS_EBLOCK_FLUSH_INFO sysEBlockFlushInfo; /*16*/
  uint8_t * srcPtr = NULL; /*4*/
  uint32_t tempSize = 0; /*4*/
  uint16_t maxTableOffset = 0; /*2*/
  LOG_ENTRY_LOC nextLoc = {0, 0}; /*4*/
#endif

#if (CACHE_RAM_BD_MODULE == true)
  CACHE_INFO_EBMCACHE ebmCacheInfo = {0, 0, 0, 0};
  CACHE_INFO_EBLOCK_PPAMAP eBlockPPAMapInfo = {0, 0};
  CACHE_INFO_RAMMAP ramMapInfo = {0, 0, 0, 0};
  uint16_t indexCount = 0;
  uint16_t blockNum = 0;
  uint16_t totalBlock = 0;
  uint16_t logicalEBNumTmp = 0;
  uint16_t phyEBlockAddrTmp = 0;
  uint32_t key = 0;
  uint16_t saveIndex[NUM_FLUSH_LOG_EBLOCKS];
  uint8_t skipFlag = false;
  //    uint16_t freeIndex = 0;
  uint16_t latestLogicalEBNum = 0;
  uint16_t currentLogicalEBNum = 0;
  uint32_t keyTmp = 0;
#endif

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
#if (CACHE_RAM_BD_MODULE == false)
  uint16_t * flushInfoEntry = NULL; /*4*/
  FLASH_STATUS flashStatus = FLASH_PASS;
#endif
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

#if (CACHE_RAM_BD_MODULE == false)
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  for(tempCount = 0; tempCount < sizeof (sysEBlockFlushInfo.reserved); tempCount++)
    {
    sysEBlockFlushInfo.reserved[tempCount] = EMPTY_BYTE;
    }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  sysEBlockFlushInfo.tableCheckWord = EMPTY_WORD;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
#endif

#if (CACHE_RAM_BD_MODULE == true)
  // For Flush_GC case
  key = GetFlushEBCounter(devID);
  if((status = TABLE_FlushEBGetLatest(devID, &latestLogicalEBNum, &phyEBlockAddrTmp, key)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = GetNextFlushEntryLocation(devID, &flushStructPageInfo,
                                         &flushRAMTablePageInfo, &currentLogicalEBNum)) != FTL_ERR_PASS)
    {
    return status;
    }
  if(latestLogicalEBNum != currentLogicalEBNum)
    {
    // set dirty mark
    skipFlag = false;
    for(blockNum = 0; blockNum < NUM_FLUSH_LOG_EBLOCKS; blockNum++)
      {
      TABLE_GetFlushLogEntry(devID, blockNum, &logicalEBNumTmp, &phyEBlockAddrTmp, &keyTmp);
      if(EMPTY_WORD == logicalEBNumTmp)
        {
        break;
        }
      if(logicalEBNumTmp == currentLogicalEBNum)
        {
        key = keyTmp;
        skipFlag = true;
        }
      if((true == skipFlag) && (keyTmp >= key))
        {
        MarkEBlockMappingTableEntryDirty(devID, logicalEBNumTmp);
        }
      }
    }

  dirtyBitTotalCnt = gCounterDirty; // Save dirty count
  if(gCounterDirty == 0)
    {
    return FTL_ERR_PASS;
    }

  totalBlock = 0;
  for(blockNum = 0; blockNum < NUM_FLUSH_LOG_EBLOCKS; blockNum++)
    {
    TABLE_GetFlushLogEntry(devID, blockNum, &logicalEBNumTmp, &phyEBlockAddrTmp, &key);
    if(EMPTY_WORD == logicalEBNumTmp)
      {
      continue;
      }
    if(FTL_ERR_PASS != (status = CACHE_GetRamMap(devID, logicalEBNumTmp, &ramMapInfo)))
      {
      return status;
      }
    if(true == ramMapInfo.presentEBM)
      {
      skipFlag = false;
      for(indexCount = 0; indexCount < totalBlock; indexCount++)
        {
        if(saveIndex[indexCount] == ramMapInfo.ebmCacheIndex)
          {
          skipFlag = true;
          break;
          }
        }
      if(true == skipFlag)
        {
        continue;
        }
      saveIndex[totalBlock] = ramMapInfo.ebmCacheIndex; // Save flush eb
      totalBlock++;
      }
    }


  for(flushTypeCnt = 0; flushTypeCnt < numRamTables; flushTypeCnt++)
    {
    gCounterDirty = (uint16_t) dirtyBitTotalCnt;
    for(indexCount = 0; indexCount < NUM_EBLOCK_MAP_INDEX; indexCount++)
      {
      if(flushTypeCnt == 1)
        {
        skipFlag = false;
        for(blockNum = 0; blockNum < totalBlock; blockNum++)
          {
          if(saveIndex[blockNum] == indexCount)
            {
            skipFlag = true;
            break;
            }
          }
        if(true == skipFlag)
          {
          continue;
          }
        }
      if(FTL_ERR_PASS != (status = CACHE_GetEBMCache(devID, indexCount, &ebmCacheInfo)))
        {
        return status;
        }

      if(CACHE_DIRTY == ebmCacheInfo.cacheStatus)
        {

        if((status = GetNextFlushEntryLocation(devID, &flushStructPageInfo,
                                               &flushRAMTablePageInfo, &logicalBlockNum)) != FTL_ERR_PASS)
          {
          return status; // go Flush GC.
          }

#if (ENABLE_EB_ERASED_BIT == true)
        SetEBErased(devID, logicalBlockNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

        // update position
        eBlockPPAMapInfo.entryIndex = (uint16_t) GetGCNum(devID, logicalBlockNum);
        TABLE_GetFlushLogCacheEntry(devID, GetPhysicalEBlockAddr(devID, logicalBlockNum), &eBlockPPAMapInfo.flashLogEBArrayCount);

        if(flushTypeCnt == 1)
          {
          // Flush EBM table
          if(FTL_ERR_PASS != (status = CACHE_CacheToFlash(devID, indexCount, eBlockPPAMapInfo, CACHE_EBLOCKMAP, flushMode)))
            {
            return status;
            }

          }
        else
          {

          // Flush PPA table
          if(FTL_ERR_PASS != (status = CACHE_CacheToFlash(devID, indexCount, eBlockPPAMapInfo, CACHE_PPAMAP, flushMode)))
            {
            return status;
            }
          }
        }
      }
    }


  for(blockNum = 0; blockNum < totalBlock; blockNum++)
    {

    if(FTL_ERR_PASS != (status = CACHE_GetEBMCache(devID, saveIndex[blockNum], &ebmCacheInfo)))
      {
      return status;
      }

    if(CACHE_DIRTY == ebmCacheInfo.cacheStatus)
      {

      // Flush EBM table
      if((status = GetNextFlushEntryLocation(devID, &flushStructPageInfo,
                                             &flushRAMTablePageInfo, &logicalBlockNum)) != FTL_ERR_PASS)
        {
        return status; // go Flush GC.
        }

#if (ENABLE_EB_ERASED_BIT == true)
      SetEBErased(devID, logicalBlockNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

      // update position
      eBlockPPAMapInfo.entryIndex = (uint16_t) GetGCNum(devID, logicalBlockNum);
      TABLE_GetFlushLogCacheEntry(devID, GetPhysicalEBlockAddr(devID, logicalBlockNum), &eBlockPPAMapInfo.flashLogEBArrayCount);

      if(FTL_ERR_PASS != (status = CACHE_CacheToFlash(devID, saveIndex[blockNum], eBlockPPAMapInfo, CACHE_EBLOCKMAP, flushMode)))
        {
        return status;
        }
      }
    }

  // clear dirty
  if(FTL_ERR_PASS != (status = CACHE_CleanAllDirtyIndex(devID)))
    {
    return status;
    }

  // For Flush_GC case
  key = GetFlushEBCounter(devID);
  if((status = TABLE_FlushEBGetLatest(devID, &latestLogicalEBNum, &phyEBlockAddrTmp, key)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = GetNextFlushEntryLocation(devID, &flushStructPageInfo,
                                         &flushRAMTablePageInfo, &currentLogicalEBNum)) != FTL_ERR_PASS)
    {
    return status;
    }
  if(latestLogicalEBNum != currentLogicalEBNum)
    {
    SetGCOrFreePageNum(devID, currentLogicalEBNum, MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK);
    }
#else
  sysEBlockFlushInfo.logIncNum = EMPTY_DWORD;
  dirtyBitTotalCnt = GetTotalDirtyBitCnt(devID); // needed to findout the end_point
  if(dirtyBitTotalCnt == 0)
    {
    return FTL_ERR_PASS;
    }
  for(flushTypeCnt = 0; flushTypeCnt < numRamTables; flushTypeCnt++)
    {
    dataByteCnt = 0x0;
    if(flushTypeCnt == 1)
      {
      ramMappingTablePtr = (uint8_t *) (&EBlockMappingTable[devID][0]);
      dirtyBitMapPtr = (uint8_t *) (&EBlockMappingTableDirtyBitMap[devID][0]);
      totalRamTableBytes = sizeof (EBlockMappingTable[devID]);
      maxTableOffset = (BITS_EBLOCK_DIRTY_BITMAP_DEV_TABLE - 1);
      sysEBlockFlushInfo.type = EBLOCK_MAP_TABLE_FLUSH;
      }
    else if(flushTypeCnt == 0)
      {
      ramMappingTablePtr = (uint8_t *) (&PPAMappingTable[devID]);
      dirtyBitMapPtr = (uint8_t *) (&PPAMappingTableDirtyBitMap[devID][0]);
      totalRamTableBytes = sizeof (PPAMappingTable[devID]);
      maxTableOffset = (BITS_PPA_DIRTY_BITMAP_DEV_TABLE - 1);
      sysEBlockFlushInfo.type = PPA_MAP_TABLE_FLUSH;
      }
    for(dataByteCnt = 0, bitMapCounter = 0; dataByteCnt < totalRamTableBytes;
        dataByteCnt += FLUSH_RAM_TABLE_SIZE, bitMapCounter++)
      {
      // If we already wrote all the dirty entries, just return.
      if(dirtyBitTotalCnt == 0)
        {
        return FTL_ERR_PASS;
        }
      bitMapData = GetBitMapField(dirtyBitMapPtr, bitMapCounter, 1);
      if(bitMapData == DIRTY_BIT)
        {
        dirtyBitCounter++;
        if((status = GetNextFlushEntryLocation(devID, &flushStructPageInfo,
                                               &flushRAMTablePageInfo, &logicalBlockNum)) != FTL_ERR_PASS)
          {
          return status; // Do GC.
          }

        // Inc free page
        IncGCOrFreePageNum(devID, logicalBlockNum);
        // Mark dirty for the flush entry
        MarkEBlockMappingTableEntryDirty(devID, logicalBlockNum);

#if (ENABLE_EB_ERASED_BIT == true)
        SetEBErased(devID, logicalBlockNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

        // clean the dirty bit...
        SetBitMapField(dirtyBitMapPtr, bitMapCounter, 1, CLEAN_BIT);
        //dirtyBitTotalCnt = GetTotalDirtyBitCnt(devID); // can be implemented more efficiently...
        dirtyBitTotalCnt--;
        // type, TableOffset, endPoint, erase started, reserve  @Time T1
        // sysEBlockFlushInfo.type & sysEBlockFlushInfo.tableOffset already set above
        // Mark end point if its last entry
        if((dirtyBitTotalCnt == 0) && (flushTypeCnt == 1))
          {
          if(flushMode == FLUSH_SHUTDOWN_MODE)
            {
            if((status = GetLogEntryLocation(devID, &nextLoc)) != FTL_ERR_PASS)
              {
              return status;
              }
            sysEBlockFlushInfo.eBlockNumLoc = nextLoc.eBlockNum;
            sysEBlockFlushInfo.entryIndexLoc = nextLoc.entryIndex;
            }
          else
            {
            sysEBlockFlushInfo.eBlockNumLoc = EMPTY_WORD;
            sysEBlockFlushInfo.entryIndexLoc = EMPTY_WORD;
            }
          sysEBlockFlushInfo.endPoint = END_POINT_SIGNATURE;
          sysEBlockFlushInfo.logIncNum = GetTransLogEBCounter(devID);
          }
        else
          {
          sysEBlockFlushInfo.eBlockNumLoc = EMPTY_WORD;
          sysEBlockFlushInfo.entryIndexLoc = EMPTY_WORD;
          sysEBlockFlushInfo.endPoint = EMPTY_BYTE;
          }
        if(flushTypeCnt == 1)
          {
          // flush adjusted free page
          if((status = AdjustFlushEBlockFreePage(devID, logicalBlockNum, bitMapCounter)) != FTL_ERR_PASS)
            {
            return status;
            }
          }
        // Flush EBlockMappingTable data - 512 bits/ 200 bytes
        sysEBlockFlushInfo.tableOffset = (uint16_t) (dataByteCnt / FLUSH_RAM_TABLE_SIZE);
        tempSize = totalRamTableBytes - dataByteCnt;

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
        if((sysEBlockFlushInfo.tableOffset == maxTableOffset) && (tempSize < SECTOR_SIZE))
          {
          srcPtr = (uint8_t *) (ramMappingTablePtr + (sysEBlockFlushInfo.tableOffset * SECTOR_SIZE));
          memcpy(&pseudoRPB[devID][0], srcPtr, (uint16_t) tempSize);
          for(tempCount = (uint16_t) tempSize; tempCount < SECTOR_SIZE; tempCount++)
            {
            pseudoRPB[devID][tempCount] = EMPTY_BYTE;
            }
          if(FLASH_RamPageWriteDataBlock(&flushRAMTablePageInfo, &pseudoRPB[devID][0]) != FLASH_PASS)
            {
            return FTL_ERR_FLASH_WRITE_03;
            }
          }
        else
          {
          if(FLASH_RamPageWriteDataBlock(&flushRAMTablePageInfo, (uint8_t *) (ramMappingTablePtr
                                                                              + (sysEBlockFlushInfo.tableOffset * SECTOR_SIZE))) != FLASH_PASS)
            {
            return FTL_ERR_FLASH_WRITE_05;
            }
          }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
        memcpy(&pseudoRPB[devID][0], (uint8_t *) & sysEBlockFlushInfo, FLUSH_INFO_SIZE);
        srcPtr = (uint8_t *) (ramMappingTablePtr + (sysEBlockFlushInfo.tableOffset * FLUSH_RAM_TABLE_SIZE));
        if((sysEBlockFlushInfo.tableOffset == maxTableOffset) && (tempSize < FLUSH_RAM_TABLE_SIZE))
          {
          memcpy(&pseudoRPB[devID][FLUSH_INFO_SIZE], srcPtr, (uint16_t) tempSize);
          for(tempCount = (uint16_t) (tempSize + FLUSH_INFO_SIZE); tempCount < FLUSH_RAM_TABLE_SIZE; tempCount++)
            {
            pseudoRPB[devID][tempCount] = EMPTY_BYTE;
            }
          }
        else
          {
          memcpy(&pseudoRPB[devID][FLUSH_INFO_SIZE], srcPtr, FLUSH_RAM_TABLE_SIZE);
          }
        flushInfoEntry = (uint16_t *) & pseudoRPB[devID][0];
        flushInfoEntry[FLUSH_INFO_TABLE_CHECK_WORD] = CalcCheckWord(&flushInfoEntry[FLUSH_INFO_TABLE_START], (FLUSH_RAM_TABLE_SIZE / 2));
        flushInfoEntry[FLUSH_INFO_CHECK_WORD] = CalcCheckWord(&flushInfoEntry[FLUSH_INFO_DATA_START], FLUSH_INFO_DATA_WORDS);
        flashStatus = FLASH_RamPageWriteDataBlock(&flushRAMTablePageInfo, &pseudoRPB[devID][0]);
        if(flashStatus != FLASH_PASS)
          {

#if(FTL_DEFECT_MANAGEMENT == true)
          if(flashStatus == FLASH_PARAM)
            {
            return FTL_ERR_FLASH_WRITE_10;
            }
          SetBadEBlockStatus(devID, logicalBlockNum, true);
          if(FLASH_MarkDefectEBlock(&flushRAMTablePageInfo) != FLASH_PASS)
            {
            // do nothing, just try to mark bad, even if it fails we move on.
            }
          if(flushTypeCnt == 1)
            {
            // restore free page if adjusted
            if((status = RestoreFlushEBlockFreePage(devID, logicalBlockNum)) != FTL_ERR_PASS)
              {
              return status;
              }
            }
          return FTL_ERR_FLUSH_FLUSH_FAIL;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
          return FTL_ERR_FLASH_WRITE_10;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

          }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

        if(flushTypeCnt == 1)
          {
          // restore free page if adjusted
          if((status = RestoreFlushEBlockFreePage(devID, logicalBlockNum)) != FTL_ERR_PASS)
            {
            return status;
            }
          }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
        // Write END SIGNATURE, @Time T2
        if((status = FTL_WriteFlushInfo(&flushStructPageInfo, &sysEBlockFlushInfo)) != FTL_ERR_PASS)
          {
          return status;
          }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

        }
      }
    }

#endif // #if (CACHE_RAM_BD_MODULE == true)

  return FTL_ERR_PASS;
  }

//---------------------------------------

FTL_STATUS UpdateEBOrderingTable(FTL_DEV devID, uint16_t startEB, uint16_t * formatCount)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FLASH_STATUS flash_status = FLASH_PASS; /*4*/
  uint16_t eBlockCount = 0; /*2*/
  FLASH_PAGE_INFO flashPage = {0, 0,
    {0, 0}}; /*11*/
  uint16_t phyEBlockAddr = EMPTY_WORD; /*2*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  uint32_t key = EMPTY_DWORD; /*4*/
  uint16_t countEB = EMPTY_WORD; /*2*/
  uint16_t phyEBlockAddrOld = EMPTY_WORD; /*2*/
  SYS_EBLOCK_INFO_PTR sysTempPtr = NULL; /*4*/
  uint32_t prevKey = EMPTY_DWORD; /*4*/
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/
  uint16_t phyEBlockAddrTmp = EMPTY_WORD; /*2*/
  uint16_t logicalEBNumTmp = EMPTY_WORD; /*2*/
  uint8_t swap = false; /*1*/
  uint16_t logEBlockAddrOld = EMPTY_WORD; /*2*/
#if(FTL_DEFECT_MANAGEMENT == true)
  uint16_t saveLogicalAddr = EMPTY_WORD; /*2*/
#endif
  uint16_t tempFormatCount = 0; /*2*/

  flashPage.devID = devID;
  for(eBlockCount = startEB; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
    {
#if (CACHE_RAM_BD_MODULE == true)
    if(FTL_START_EBLOCK == startEB)
      {
      if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, eBlockCount, CACHE_INIT_TYPE)))
        {
        return status;
        }
      }
#endif

    phyEBlockAddr = GetPhysicalEBlockAddr(devID, eBlockCount);
    flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddr, 0);
    flashPage.vPage.pageOffset = 0;

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    flashPage.byteCount = sizeof (sysEBlockInfo);
    flash_status = FLASH_RamPageReadMetaData(&flashPage, (uint8_t *) (&sysEBlockInfo));

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    flashPage.byteCount = SECTOR_SIZE;
    flash_status = FLASH_RamPageReadDataBlock(&flashPage, &pseudoRPB[devID][0]);
    memcpy((uint8_t *) (&sysEBlockInfo), &pseudoRPB[devID][0], SYS_INFO_SIZE);
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

    if(flash_status != FLASH_PASS)
      {

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
      if(flash_status == FLASH_ECC_FAIL)
        {
#if (ENABLE_EB_ERASED_BIT == true)
        SetEBErased(devID, eBlockCount, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

        if(SYSTEM_START_EBLOCK > eBlockCount)
          {
          continue;
          }

        (*formatCount)++;

#if(FTL_DEFECT_MANAGEMENT == true)
        if(FLASH_CheckDefectEBlock(&flashPage) != FLASH_PASS)
          {
          continue;
          }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

#if( FTL_EBLOCK_CHAINING == true)
        if(EMPTY_WORD != GetChainLogicalEBNum(devID, eBlockCount))
          {
          continue;
          }
#endif  // #if( FTL_EBLOCK_CHAINING == true)

        if(FTL_ERR_PASS == TABLE_CheckUsedSysEB(devID, eBlockCount))
          {
          continue;
          }

        if((status = TABLE_InsertReservedEB(devID, eBlockCount)) != FTL_ERR_PASS)
          {
          return status;
          }
        continue;
        }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

      return FTL_ERR_FLASH_READ_08;
      }
    if((false == VerifyCheckWord((uint16_t *) & sysEBlockInfo.type, SYS_INFO_DATA_WORDS, sysEBlockInfo.checkWord)) &&
       (sysEBlockInfo.oldSysBlock != OLD_SYS_BLOCK_SIGNATURE) && (sysEBlockInfo.phyAddrThisEBlock == phyEBlockAddr))
      {
      if(sysEBlockInfo.type == SYS_EBLOCK_INFO_FLUSH)
        {
#if (ENABLE_EB_ERASED_BIT == true)
        SetEBErased(devID, eBlockCount, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

        // Insert the entry, if fails check if this has higher incNum and insert that instead
        if((status = TABLE_FlushEBInsert(devID, eBlockCount, phyEBlockAddr, sysEBlockInfo.incNumber)) == FTL_ERR_FLUSH_NO_ENTRIES)
          {
          swap = false;
          for(countEB = 0; countEB < NUM_FLUSH_LOG_EBLOCKS; countEB++)
            {
            if((status = TABLE_GetFlushLogEntry(devID, countEB, &logicalEBNumTmp, &phyEBlockAddrTmp, &key)) != FTL_ERR_PASS)
              {
              return status;
              }
            if(key < sysEBlockInfo.incNumber)
              {
              swap = true;
              }
            }
          if(swap == true)
            {
            // This will clear the smallest entry
            if((status = TABLE_FlushEBGetNext(devID, &logEBlockAddrOld, &phyEBlockAddrOld, NULL)) != FTL_ERR_PASS)
              {
              return status;
              }
#if(FTL_DEFECT_MANAGEMENT == false)
            if((status = TABLE_InsertReservedEB(devID, logEBlockAddrOld)) != FTL_ERR_PASS)
              {
              return status;
              }
#else
            saveLogicalAddr = logEBlockAddrOld;
#endif
            // Space is now avaiable, Insert the higher entry
            if((status = TABLE_FlushEBInsert(devID, eBlockCount, phyEBlockAddr, sysEBlockInfo.incNumber)) != FTL_ERR_PASS)
              {
              return status;
              }
            flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddrOld, 0);
            }
          else
            {
#if(FTL_DEFECT_MANAGEMENT == false)
            if((status = TABLE_InsertReservedEB(devID, eBlockCount)) != FTL_ERR_PASS)
              {
              return status;
              }
#else
            saveLogicalAddr = eBlockCount;
#endif
            flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddr, 0);
            }
          // Mark old flush log eblock
          sysEBlockInfo.oldSysBlock = OLD_SYS_BLOCK_SIGNATURE;
          flashPage.devID = devID;
          flashPage.vPage.pageOffset = (uint16_t) ((uint32_t) (&sysTempPtr->oldSysBlock));
          flashPage.byteCount = OLD_SYS_BLOCK_SIGNATURE_SIZE;
          //              flashPage.byteCount = sizeof(sysEBlockInfo.oldSysBlock);
          flash_status = FLASH_RamPageWriteMetaData(&flashPage, (uint8_t *) & sysEBlockInfo.oldSysBlock);
          if(flash_status != FLASH_PASS)
            {

#if(FTL_DEFECT_MANAGEMENT == true)
            if(flash_status == FLASH_PARAM)
              {
              return FTL_ERR_FLASH_WRITE_07;
              }
            SetBadEBlockStatus(devID, saveLogicalAddr, true);
            // just try to mark bad, even if it fails we move on.
            FLASH_MarkDefectEBlock(&flashPage);

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
            return FTL_ERR_FLASH_WRITE_07;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

            }
#if(FTL_DEFECT_MANAGEMENT == true)
          if((status = TABLE_InsertReservedEB(devID, saveLogicalAddr)) != FTL_ERR_PASS)
            {
            return status;
            }
#endif
          }
        else if(status != FTL_ERR_PASS)
          {
          return status;
          }
        if(sysEBlockInfo.incNumber > GetFlushEBCounter(devID))
          {
          SetFlushLogEBCounter(devID, sysEBlockInfo.incNumber);
          }
        if(SYSTEM_START_EBLOCK > eBlockCount)
          {
          tempFormatCount++;
          }
        continue;
        }
      if(sysEBlockInfo.type == SYS_EBLOCK_INFO_LOG)
        {
#if (ENABLE_EB_ERASED_BIT == true)
        SetEBErased(devID, eBlockCount, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

        if((status = TABLE_TransLogEBInsert(devID, eBlockCount, phyEBlockAddr, sysEBlockInfo.incNumber)) == FTL_ERR_LOG_INSERT)
          {
          swap = false;
          for(countEB = 0; countEB < NUM_TRANSACTLOG_EBLOCKS; countEB++)
            {
            if((status = TABLE_GetTransLogEntry(devID, countEB, &logicalEBNumTmp, &phyEBlockAddrTmp, &key)) != FTL_ERR_PASS)
              {
              return status;
              }
            if(key < sysEBlockInfo.incNumber)
              {
              swap = true;
              }
            }
          if(swap == true)
            {
            // This will clear the smallest entry
            if((status = TABLE_TransLogEBGetNext(devID, &logEBlockAddrOld, &phyEBlockAddrOld, NULL)) != FTL_ERR_PASS)
              {
              return status;
              }
#if(FTL_DEFECT_MANAGEMENT == false)
            if((status = TABLE_InsertReservedEB(devID, logEBlockAddrOld)) != FTL_ERR_PASS)
              {
              return status;
              }
#else
            saveLogicalAddr = logEBlockAddrOld;
#endif
            // Space is now avaiable, Insert the higher entry
            if((status = TABLE_TransLogEBInsert(devID, eBlockCount, phyEBlockAddr, sysEBlockInfo.incNumber)) != FTL_ERR_PASS)
              {
              return status;
              }
            flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddrOld, 0);
            }
          else
            {

#if(FTL_DEFECT_MANAGEMENT == false)
            if((status = TABLE_InsertReservedEB(devID, eBlockCount)) != FTL_ERR_PASS)
              {
              return status;
              }
#else
            saveLogicalAddr = eBlockCount;
#endif
            flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddr, 0);
            }
          // Mark old flush log eblock
          sysEBlockInfo.oldSysBlock = OLD_SYS_BLOCK_SIGNATURE;
          flashPage.devID = devID;
          flashPage.vPage.pageOffset = (uint16_t) ((uint32_t) (&sysTempPtr->oldSysBlock));
          flashPage.byteCount = OLD_SYS_BLOCK_SIGNATURE_SIZE;
          //              flashPage.byteCount = sizeof(sysEBlockInfo.oldSysBlock);
          flash_status = FLASH_RamPageWriteMetaData(&flashPage, (uint8_t *) & sysEBlockInfo.oldSysBlock);
          if(flash_status != FLASH_PASS)
            {

#if(FTL_DEFECT_MANAGEMENT == true)
            if(flash_status == FLASH_PARAM)
              {
              return FTL_ERR_FLASH_WRITE_06;
              }
            SetBadEBlockStatus(devID, saveLogicalAddr, true);
            // just try to mark bad, even if it fails we move on.
            FLASH_MarkDefectEBlock(&flashPage);

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
            return FTL_ERR_FLASH_WRITE_06;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

            }
#if(FTL_DEFECT_MANAGEMENT == true)
          if((status = TABLE_InsertReservedEB(devID, saveLogicalAddr)) != FTL_ERR_PASS)
            {
            return status;
            }
#endif

          }
        else if(status != FTL_ERR_PASS)
          {
          return status;
          }
        if(sysEBlockInfo.incNumber > GetTransLogEBCounter(devID))
          {
          SetTransLogEBCounter(devID, sysEBlockInfo.incNumber);
          }
        if(SYSTEM_START_EBLOCK > eBlockCount)
          {
          tempFormatCount++;
          }
        continue;
        }
      if(SYSTEM_START_EBLOCK > eBlockCount)
        {
        continue;
        }
      (*formatCount)++;

#if (FTL_SUPER_SYS_EBLOCK == true)
      if(FTL_ERR_PASS == TABLE_CheckUsedSuperEB(devID, eBlockCount))
        {
        continue;
        }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

      if((status = TABLE_InsertReservedEB(devID, eBlockCount)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
    else
      {

      if(SYSTEM_START_EBLOCK > eBlockCount)
        {
        continue;
        }

      (*formatCount)++;

#if (ENABLE_EB_ERASED_BIT == true) // For Filed Update, correct format compatibility between current version(1.2.11) and new version(1.2.12).
      if((sysEBlockInfo.oldSysBlock == OLD_SYS_BLOCK_SIGNATURE) && (true == GetEBErased(devID, eBlockCount)) && (SYSTEM_START_EBLOCK == startEB))
        {
        SetEBErased(devID, eBlockCount, false);
        }
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

#if(FTL_DEFECT_MANAGEMENT == true)
      if(FLASH_CheckDefectEBlock(&flashPage) != FLASH_PASS)
        {
        continue;
        }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

#if( FTL_EBLOCK_CHAINING == true)
      if(EMPTY_WORD != GetChainLogicalEBNum(devID, eBlockCount))
        {
        continue;
        }
#endif

#if (FTL_SUPER_SYS_EBLOCK == true)
      if(FTL_ERR_PASS == TABLE_CheckUsedSuperEB(devID, eBlockCount))
        {
        continue;
        }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

      if((status = TABLE_InsertReservedEB(devID, eBlockCount)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
    }
  /* check sanity */
  for(eBlockCount = 0; eBlockCount < NUM_FLUSH_LOG_EBLOCKS; eBlockCount++)
    {
    if((status = TABLE_GetFlushLogEntry(devID, eBlockCount, &logicalEBNum, &phyEBlockAddr, &key)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(key == EMPTY_DWORD)
      {
      break;
      }
    if(eBlockCount > 0)
      {
      if(key > (prevKey + 1))
        {
        if((status = TABLE_FlushEBRemove(devID, eBlockCount)) != FTL_ERR_PASS)
          {
          return status;
          }
        SetFlushLogEBCounter(devID, prevKey);
        }
      }
    prevKey = key;
    }
  (*formatCount) = (*formatCount) - tempFormatCount;

  return FTL_ERR_PASS;
  }

//-----------------------------------

FTL_STATUS TABLE_Flush(uint8_t flushMode)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint8_t devID = 0; /*1*/
  uint8_t flushGCFlag = false; /*1*/

#if(FTL_DEFECT_MANAGEMENT == true)
  uint8_t flushFail = false; /*1*/
  uint16_t sanityCounter = 0; /*2*/
  uint8_t checkBBMark = false; /*1*/
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

  for(devID = 0; devID < NUM_DEVICES; devID++)
    {
    if(((status = CheckFlushSpace(devID)) == FTL_ERR_FLUSH_GC_NEEDED) || (flushMode == FLUSH_GC_MODE))
      {

#if(FTL_DEFECT_MANAGEMENT == false)
      if((status = Flush_GC(devID)) != FTL_ERR_PASS)
        {
        return status;
        }
#endif  // #if(FTL_DEFECT_MANAGEMENT == false)

      flushGCFlag = true;
      }
    else if(status != FTL_ERR_PASS)
      {
      return status;
      }
    // This does not check for avaiable free space...
    if(flushGCFlag == false) //already done a full flush, dont flush again.
      {
      if((status = TABLE_FlushDevice(devID, flushMode)) != FTL_ERR_PASS)
        {

#if(FTL_DEFECT_MANAGEMENT == true)
        if((status == FTL_ERR_FLUSH_FLUSH_FAIL) || (status == FTL_ERR_FLUSH_NEXT_ENTRY))
          {
          flushFail = true;
          }
        else
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

          {
          return status;
          }
        }
      if(flushMode == FLUSH_NORMAL_MODE)
        {
        // Erase the log entries, only if its not empty...
#if(FTL_DEFECT_MANAGEMENT == true)
        if(flushFail == false)
          {
          while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
            {
            status = FTL_EraseAllTransLogBlocksOp(devID);
            if(status == FTL_ERR_MARKBB_COMMIT)
              {
              checkBBMark = true;
              status = FTL_ERR_PASS;
              }
            if(status != FTL_ERR_LOG_NEW_EBLOCK_FAIL)
              {
              break;
              }
            checkBBMark = true;
            sanityCounter++;
            }
          if(status != FTL_ERR_PASS)
            {
            return status;
            }
          if(true == checkBBMark)
            {
            flushFail = true;
            }
          }

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
        if((status = FTL_EraseAllTransLogBlocksOp(devID)) != FTL_ERR_PASS)
          {
          return status;
          }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

        }
      }

#if(FTL_DEFECT_MANAGEMENT == true)
    if((flushGCFlag == true) || (flushFail == true))
      {
      sanityCounter = 0;
      while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
        {
        if((status = Flush_GC(devID)) != FTL_ERR_FLUSH_FLUSH_GC_FAIL)
          {
          break;
          }
        sanityCounter++;
        }
      if(status != FTL_ERR_PASS)
        {
        return status;
        }
      }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

    }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  writeLogFlag = false;
#endif

#if(FTL_DEFECT_MANAGEMENT == true)
  FTL_ClearGCSave(CLEAR_GC_SAVE_RUNTIME_MODE);
#endif // #if(FTL_DEFECT_MANAGEMENT == true)

  return FTL_ERR_PASS;
  }

//--------------------------------------

FTL_STATUS TABLE_LoadFlushTable()
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FLASH_STATUS flash_status = FLASH_PASS; /*4*/
  uint8_t eBlockCnt = 0x0; /*1*/
  uint8_t devID = 0x0; /*1*/
  uint8_t ramTablesUpdated = false; /*1*/
  uint8_t logEblockFound = false; /*1*/
  uint16_t logicalAddr = 0x0; /*2*/
  uint16_t phyAddr = 0x0; /*2*/
  uint32_t key = 0x0; /*4*/
  uint16_t freePageIndex = 0x0; /*2*/
  uint16_t formatCount = 0x0; /*2*/
  uint16_t validFreePageIndex = 1; /*2*/
  uint32_t transLogEBCounter = 0; /*4*/
  uint32_t logIndexFound = EMPTY_DWORD; /*4*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  SYS_EBLOCK_FLUSH_INFO sysEBlockFlushInfo; /*16*/
  FLASH_PAGE_INFO flashPage = {0, 0,
    {0, 0}}; /*11*/
  FLASH_PAGE_INFO flushStructPageInfo = {0, 0,
    {0, 0}}; /*11*/
  FLASH_PAGE_INFO flushRAMTablePageInfo = {0, 0,
    {0, 0}}; /*11*/
#if (CACHE_RAM_BD_MODULE == false)
  uint8_t * EBlockMappingTablePtr = NULL; /*4*/
  uint8_t * PPAMappingTablePtr = NULL; /*4*/
#endif
  LOG_ENTRY_LOC shutdownLoc = {EMPTY_WORD, EMPTY_WORD}; /*4*/
  uint16_t eBlockCount = 0x0; /*2*/

  KEY_TABLE_ENTRY tempArray[NUM_DEVICES][NUM_TRANSACTLOG_EBLOCKS];
  uint16_t tempCount = 0x0; /*1*/
  uint16_t tempBlockCount = 0x0; /*2*/
  uint8_t tempFound = false; /*1*/
  uint32_t tempEBCounter = 0x0; /*4*/

#if (FTL_SUPER_SYS_EBLOCK == true)
  SYS_EBLOCK_INFO_PTR tempSysPtr = NULL; /*4*/
  uint16_t phyEBAddr = 0x0; /*2*/
  uint16_t eBlockNum = 0x0; /*2*/
  uint32_t latestIncNumber = 0x0; /*4*/
  uint8_t checkFlag = false; /*1*/
  KEY_TABLE_ENTRY tempSuperArray[NUM_DEVICES][NUM_SUPER_SYS_EBLOCKS];
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  uint8_t flushEblockFailed = false; /*1*/
  uint16_t checkLoopPhyEB = EMPTY_WORD; /*1*/
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

#if(FTL_DEFECT_MANAGEMENT == true)
  uint16_t sanityCounter = 0; /*2*/
  uint8_t checkBBMark = false; /*1*/
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  uint16_t eccErrSector = EMPTY_WORD;
  uint16_t flashReadCount = 0x0;
#endif

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
#if (CACHE_RAM_BD_MODULE == false)
  uint16_t bitMapCounter = 0;
#endif
#endif

#if (CACHE_RAM_BD_MODULE == true)
  KEY_TABLE_ENTRY tempFlushArray[NUM_DEVICES][NUM_FLUSH_LOG_EBLOCKS];
  CACHE_INFO_EBLOCK_PPAMAP eBlockPPAMapInfo = {0, 0};
  uint16_t logicalEBNum = 0x0;
#if (CACHE_DYNAMIC_ALLOCATION == false)
  uint16_t index[NUM_EBLOCK_MAP_INDEX * NUMBER_OF_DEVICES];
#else
  uint16_t * index;
#endif
  uint16_t indexCount = 0;
  uint16_t indexMaxCount = 0;
  uint8_t eBlockCnt2 = 0;
  uint8_t pfFlushFlag = false;
#if (FTL_STATIC_WEAR_LEVELING == true)
#if (CACHE_RAM_BD_MODULE == false)
  uint16_t ebCount = 0;
  uint32_t sector = 0;
  uint32_t sector2 = 0;
  uint16_t offset = 0;
#endif
  uint32_t maxValue = 0;
  uint32_t eraseCount = 0;
#endif
#endif


#if (CACHE_RAM_BD_MODULE == true)
#if (CACHE_DYNAMIC_ALLOCATION == true)
  index = (uint16_t *) malloc(sizeof (uint16_t) * (NUM_EBLOCK_MAP_INDEX * NUMBER_OF_DEVICES));
  if(NULL == index)
    {
    return FTL_ERR_FAIL;
    }
#endif
#endif

  for(devID = 0; devID < NUM_DEVICES; devID++)
    {

#if (FTL_SUPER_SYS_EBLOCK == true)
    checkFlag = false;
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    checkLoopPhyEB = EMPTY_WORD;
    do
      {
      if(EMPTY_WORD != checkLoopPhyEB) // take back a EBlock/PPAMappingTable.
        {
#if (CACHE_RAM_BD_MODULE == false)
        for(eBlockCount = 0; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
          {
          TABLE_ClearMappingTable(devID, eBlockCount, eBlockCount, ERASE_STATUS_GET_DWORD_MASK | 1);
          TABLE_ClearPPATable(devID, eBlockCount);
          }
        for(bitMapCounter = 0; bitMapCounter < EBLOCK_DIRTY_BITMAP_DEV_TABLE_SIZE; bitMapCounter++)
          {
          EBlockMappingTableDirtyBitMap[devID][bitMapCounter] = CLEAN_BIT;
          }
#else
        // don't need this. clear array after routine
#endif
        flushEblockFailed = false;
        }
#endif

#if (CACHE_RAM_BD_MODULE == true)

      for(eBlockCnt = 0; eBlockCnt < NUM_FLUSH_LOG_EBLOCKS; eBlockCnt++)
        {
        tempFlushArray[devID][eBlockCnt].logicalEBNum = EMPTY_WORD;
        tempFlushArray[devID][eBlockCnt].phyAddr = EMPTY_WORD;
        tempFlushArray[devID][eBlockCnt].key = EMPTY_DWORD;
        tempFlushArray[devID][eBlockCnt].cacheNum = EMPTY_BYTE;
        }

      indexCount = 0;
      if((status = CACHE_ClearAll()) != FTL_ERR_PASS) // clear cache table
        {
        return status;
        }

#else
      EBlockMappingTablePtr = (uint8_t *) (&EBlockMappingTable[devID][0]);
      PPAMappingTablePtr = (uint8_t *) (&PPAMappingTable[devID][0][0]);
#endif

      for(eBlockCnt = 0; eBlockCnt < NUM_FLUSH_LOG_EBLOCKS; eBlockCnt++)
        {
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
        if((status = TABLE_FlushEBGetNext(devID, &logicalAddr, &phyAddr, &key)) == FTL_ERR_FLUSH_NO_EBLOCKS)
          {
          break;
          }
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
        if((status = TABLE_GetFlushLogEntry(devID, eBlockCnt, &logicalAddr, &phyAddr, &key)) != FTL_ERR_PASS)
          {
          return status;
          }
        if(EMPTY_WORD == logicalAddr && EMPTY_WORD == phyAddr && EMPTY_DWORD == key)
          {
          status = FTL_ERR_FLUSH_NO_EBLOCKS;
          break;
          }
#endif

#if (CACHE_RAM_BD_MODULE == true)
        tempFlushArray[devID][eBlockCnt].logicalEBNum = logicalAddr;
        tempFlushArray[devID][eBlockCnt].phyAddr = phyAddr;
        tempFlushArray[devID][eBlockCnt].key = key;
#endif

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
        for(freePageIndex = 1, validFreePageIndex = 0; freePageIndex < MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK; freePageIndex++)
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
        for(freePageIndex = 0, validFreePageIndex = 0; freePageIndex < MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK; freePageIndex++)
#endif
          {
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
          if((freePageIndex % NUMBER_OF_SECTORS_PER_PAGE) == 0)
            {
            eccErrSector = EMPTY_WORD;
#endif
            if((status = GetFlushLoc(devID, phyAddr, freePageIndex, &flushStructPageInfo, &flushRAMTablePageInfo)) != FTL_ERR_PASS)
              {
              return status;
              }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
            flash_status = FLASH_RamPageReadMetaData(&flushStructPageInfo, (uint8_t *) & sysEBlockFlushInfo);

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
            flushRAMTablePageInfo.byteCount = VIRTUAL_PAGE_SIZE; // Set 2048 byte size
            flash_status = FLASH_RamPageReadDataBlock(&flushRAMTablePageInfo, &pseudoRPB[devID][0]);
            memcpy((uint8_t *) & sysEBlockFlushInfo, &pseudoRPB[devID][0], FLUSH_INFO_SIZE);
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

            if(flash_status != FLASH_PASS)
              {

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
              // power failure check...
              if(flash_status == FLASH_ECC_FAIL)
                {
                // power failure occurred, invoke Flush_GC
                flushEblockFailed = true;

                for(flashReadCount = 0; flashReadCount < NUMBER_OF_SECTORS_PER_PAGE; flashReadCount++)
                  {
                  if((status = GetFlushLoc(devID, phyAddr, (freePageIndex + flashReadCount), &flushStructPageInfo, &flushRAMTablePageInfo)) != FTL_ERR_PASS)
                    {
                    return status;
                    }

                  flash_status = FLASH_RamPageReadDataBlock(&flushRAMTablePageInfo, &pseudoRPB[devID][(flashReadCount * SECTOR_SIZE)]);
                  if(flash_status != FLASH_PASS)
                    {
                    if(flash_status == FLASH_ECC_FAIL)
                      {
                      if(EMPTY_WORD == eccErrSector)
                        {
                        eccErrSector = flashReadCount;
                        }
                      }
                    else
                      {
                      return FTL_ERR_FLASH_READ_07;
                      }
                    }
                  }
                }
              else
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

                {
                return FTL_ERR_FLASH_READ_07;
                }
              }
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
            }
          else
            {
            memcpy((uint8_t *) & pseudoRPB[devID][0], (uint8_t *) & pseudoRPB[devID][((freePageIndex % NUMBER_OF_SECTORS_PER_PAGE) * SECTOR_SIZE)], SECTOR_SIZE);
            memcpy((uint8_t *) & sysEBlockFlushInfo, &pseudoRPB[devID][0], FLUSH_INFO_SIZE);
            } // if((freePageIndex % NUMBER_OF_SECTORS_PER_PAGE) == 0)

          if(EMPTY_WORD != eccErrSector && ((freePageIndex % NUMBER_OF_SECTORS_PER_PAGE) == eccErrSector))
            {
            // ECC error
            break;
            }
          // Skip Sys info
          if(0 == freePageIndex)
            {
            continue;
            }
#endif
          // power failure check...
          if(true == VerifyCheckWord((uint16_t *) & sysEBlockFlushInfo.type,
                                     FLUSH_INFO_DATA_WORDS, sysEBlockFlushInfo.checkWord))
            {
            // No more entries..
            break;
            }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
          if((status = VerifyRamTable((uint16_t *) & pseudoRPB[devID][0])) != FTL_ERR_PASS)
            {
            // power failure occurred, invoke Flush_GC
            flushEblockFailed = true;
            break;
            }


          if(phyAddr != checkLoopPhyEB)
            {
            if(sysEBlockFlushInfo.type == EBLOCK_MAP_TABLE_FLUSH)
              {

#if (CACHE_RAM_BD_MODULE == true)
              eBlockPPAMapInfo.entryIndex = freePageIndex;
              eBlockPPAMapInfo.flashLogEBArrayCount = eBlockCnt;
              index[indexCount + (devID * NUMBER_OF_DEVICES)] = sysEBlockFlushInfo.tableOffset;
              indexCount++;
              if(indexCount >= NUM_EBLOCK_MAP_INDEX)
                {
                indexMaxCount = indexCount;
                indexCount = 0;
                }
              if(FTL_ERR_PASS != (status = CACHE_SetEBlockAndPPAMap(devID, sysEBlockFlushInfo.tableOffset, &eBlockPPAMapInfo, CACHE_EBLOCKMAP)))
                {
                return status;
                }

#else
              if((status = LoadRamTable(&flushRAMTablePageInfo, EBlockMappingTablePtr,
                                        sysEBlockFlushInfo.tableOffset, sizeof (EBlockMappingTable[devID]))) != FTL_ERR_PASS)
                {
                return status;
                }
#endif
              }
            else if(sysEBlockFlushInfo.type == PPA_MAP_TABLE_FLUSH)
              {

#if (CACHE_RAM_BD_MODULE == true)
              eBlockPPAMapInfo.entryIndex = freePageIndex;
              eBlockPPAMapInfo.flashLogEBArrayCount = eBlockCnt;
              if(FTL_ERR_PASS != (status = CACHE_SetEBlockAndPPAMap(devID, sysEBlockFlushInfo.tableOffset, &eBlockPPAMapInfo, CACHE_PPAMAP)))
                {
                return status;
                }
#else
              if((status = LoadRamTable(&flushRAMTablePageInfo, PPAMappingTablePtr,
                                        sysEBlockFlushInfo.tableOffset, sizeof (PPAMappingTable[devID]))) != FTL_ERR_PASS)
                {
                return status;
                }
#endif
              }
            }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

          if(sysEBlockFlushInfo.endPoint == END_POINT_SIGNATURE)
            {
            validFreePageIndex = freePageIndex;
            logIndexFound = sysEBlockFlushInfo.logIncNum;
            shutdownLoc.eBlockNum = sysEBlockFlushInfo.eBlockNumLoc;
            shutdownLoc.entryIndex = sysEBlockFlushInfo.entryIndexLoc;
            }
          }
        if((validFreePageIndex == 0) && (freePageIndex == MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK))
          {
          flashPage.devID = devID;
          flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyAddr, 0);
          flashPage.vPage.pageOffset = 0;


#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
          flashPage.byteCount = sizeof (sysEBlockInfo);
          flash_status = FLASH_RamPageReadMetaData(&flashPage, (uint8_t *) (&sysEBlockInfo));

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
          flashPage.byteCount = SECTOR_SIZE;
          flash_status = FLASH_RamPageReadDataBlock(&flashPage, &pseudoRPB[devID][0]);
          memcpy((uint8_t *) & sysEBlockInfo, &pseudoRPB[devID][0], SYS_INFO_SIZE);
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

          if(flash_status != FLASH_PASS)
            {
            return FTL_ERR_FLUSH_READ;
            }
          if(sysEBlockInfo.fullFlushSig == FULL_FLUSH_SIGNATURE)
            {
            validFreePageIndex = freePageIndex;
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
            // 2nd loop skips
            continue;
#endif
            }
          }
#if (CACHE_RAM_BD_MODULE == true)
        if((freePageIndex - 1) != validFreePageIndex)
          {
          pfFlushFlag = true;
          }
#endif
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
        if((freePageIndex - 1) == validFreePageIndex)
          {
          // 2nd loop skips
          continue;
          }
        if(((freePageIndex - 1) != validFreePageIndex) && (EMPTY_WORD == checkLoopPhyEB))
          {
          // go to 2nd loop
          checkLoopPhyEB = phyAddr;
          break;
          }
#endif

        for(freePageIndex = 1; freePageIndex <= validFreePageIndex; freePageIndex++)
          {
            if((status = GetFlushLoc(devID, phyAddr, freePageIndex, &flushStructPageInfo, &flushRAMTablePageInfo)) != FTL_ERR_PASS)
              {
              return status;
              }

            flash_status = FLASH_RamPageReadMetaData(&flushStructPageInfo, (uint8_t *) & sysEBlockFlushInfo);

            if(flash_status != FLASH_PASS)
              {
              return FTL_ERR_FLASH_READ_06;
              }

          // power failure check...
          if(true == VerifyCheckWord((uint16_t *) & sysEBlockFlushInfo.type, FLUSH_INFO_DATA_WORDS, sysEBlockFlushInfo.checkWord))
            {
            // No more entries..
            break;
            }


          if(sysEBlockFlushInfo.type == EBLOCK_MAP_TABLE_FLUSH)
            {


#if (CACHE_RAM_BD_MODULE == true)
            eBlockPPAMapInfo.entryIndex = freePageIndex;
            eBlockPPAMapInfo.flashLogEBArrayCount = eBlockCnt;
            index[indexCount + (devID * NUMBER_OF_DEVICES)] = sysEBlockFlushInfo.tableOffset;
            indexCount++;
            if(indexCount >= NUM_EBLOCK_MAP_INDEX)
              {
              indexMaxCount = indexCount;
              indexCount = 0;
              }
            if(FTL_ERR_PASS != (status = CACHE_SetEBlockAndPPAMap(devID, sysEBlockFlushInfo.tableOffset, &eBlockPPAMapInfo, CACHE_EBLOCKMAP)))
              {
              return status;
              }

#else
            if((status = LoadRamTable(&flushRAMTablePageInfo, EBlockMappingTablePtr,
                                      sysEBlockFlushInfo.tableOffset, sizeof (EBlockMappingTable[devID]))) != FTL_ERR_PASS)
              {
              return status;
              }
#endif
            }
          else if(sysEBlockFlushInfo.type == PPA_MAP_TABLE_FLUSH)
            {

#if (CACHE_RAM_BD_MODULE == true)
            eBlockPPAMapInfo.entryIndex = freePageIndex;
            eBlockPPAMapInfo.flashLogEBArrayCount = eBlockCnt;
            if(FTL_ERR_PASS != (status = CACHE_SetEBlockAndPPAMap(devID, sysEBlockFlushInfo.tableOffset, &eBlockPPAMapInfo, CACHE_PPAMAP)))
              {
              return status;
              }
#else
            if((status = LoadRamTable(&flushRAMTablePageInfo, PPAMappingTablePtr,
                                      sysEBlockFlushInfo.tableOffset, sizeof (PPAMappingTable[devID]))) != FTL_ERR_PASS)
              {
              return status;
              }
#endif
            }
          }
        }

#if (CACHE_RAM_BD_MODULE == true)

    // Set Flush Array
    if(0 == indexMaxCount)
      {
      indexMaxCount = indexCount;
      }

    for(eBlockCnt = 0; eBlockCnt < NUM_FLUSH_LOG_EBLOCKS; eBlockCnt++)
      {
      logicalAddr = tempFlushArray[devID][eBlockCnt].logicalEBNum;
      phyAddr = tempFlushArray[devID][eBlockCnt].phyAddr;
      key = tempFlushArray[devID][eBlockCnt].key;
      if(EMPTY_WORD == logicalAddr)
        {
        continue;
        }
      if((status = TABLE_FlushEBInsert(devID, logicalAddr, phyAddr, key)) != FTL_ERR_PASS)
        {
        return status;
        }
      }

    // Set Reserved and System EB
    for(logicalEBNum = NUM_DATA_EBLOCKS; logicalEBNum < NUMBER_OF_ERASE_BLOCKS; logicalEBNum++)
      {
      if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, logicalEBNum, CACHE_INIT_TYPE)))
        {
        return status;
        }
      }

    TABLE_FlushEBClear(devID);
#endif

#if (FTL_SUPER_SYS_EBLOCK == true)
    if(((shutdownLoc.eBlockNum == EMPTY_WORD) && (shutdownLoc.entryIndex == EMPTY_WORD)) || (true == SuperEBInfo[devID].checkLost)) // For Power Failure Case.
#else  // #if(FTL_SUPER_SYS_EBLOCK == true)
    if((shutdownLoc.eBlockNum == EMPTY_WORD) && (shutdownLoc.entryIndex == EMPTY_WORD)) // For Power Failure Case.
#endif
      {
      for(eBlockCount = 0; eBlockCount < NUM_TRANSACTLOG_EBLOCKS; eBlockCount++)
        {
        tempArray[devID][eBlockCount].logicalEBNum = EMPTY_WORD;
        tempArray[devID][eBlockCount].phyAddr = EMPTY_WORD;
        tempArray[devID][eBlockCount].key = EMPTY_DWORD;
        }
      for(eBlockCount = 0; eBlockCount < NUM_TRANSACTLOG_EBLOCKS; eBlockCount++)
        {
        if((status = TABLE_GetTransLogEntry(devID, eBlockCount, &logicalAddr, &phyAddr, &key)) != FTL_ERR_PASS)
          {
          break; // trying to excess outside table.
          }
        if((logicalAddr == EMPTY_WORD) && (phyAddr == EMPTY_WORD) && (key == EMPTY_DWORD))
          {
          break; // no more entries in table
          }
        tempArray[devID][eBlockCount].logicalEBNum = logicalAddr;
        tempArray[devID][eBlockCount].phyAddr = phyAddr;
        tempArray[devID][eBlockCount].key = key;
        }
      tempCount = GetTransLogEBArrayCount(devID);
      tempEBCounter = GetTransLogEBCounter(devID);
      }

    // clear and update the flush Tables entries
    if((status = TABLE_InitEBOrderingTable(devID)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = UpdateEBOrderingTable(devID, SYSTEM_START_EBLOCK, (uint16_t *) & formatCount)) != FTL_ERR_PASS)
      {
      return status;
      }

#if (CACHE_RAM_BD_MODULE == true)
    for(eBlockCnt = 0; eBlockCnt < NUM_FLUSH_LOG_EBLOCKS; eBlockCnt++)
      {
      if((status = TABLE_FlushEBGetNext(devID, &logicalAddr, &phyAddr, &key)) == FTL_ERR_FLUSH_NO_EBLOCKS)
        {
        break;
        }
      for(eBlockCnt2 = 0; eBlockCnt2 < NUM_FLUSH_LOG_EBLOCKS; eBlockCnt2++)
        {
        if(phyAddr == tempFlushArray[devID][eBlockCnt2].phyAddr)
          {
          tempFlushArray[devID][eBlockCnt2].logicalEBNum = logicalAddr;
          }
        }
      }


    for(eBlockCnt = 0; eBlockCnt < NUM_FLUSH_LOG_EBLOCKS; eBlockCnt++)
      {
      logicalAddr = tempFlushArray[devID][eBlockCnt].logicalEBNum;
      phyAddr = tempFlushArray[devID][eBlockCnt].phyAddr;
      key = tempFlushArray[devID][eBlockCnt].key;
      if(EMPTY_WORD == logicalAddr)
        {
        continue;
        }
      if((status = TABLE_FlushEBInsert(devID, logicalAddr, phyAddr, key)) != FTL_ERR_PASS)
        {
        return status;
        }
      }

#if (FTL_STATIC_WEAR_LEVELING == true)
    for(logicalEBNum = 0; logicalEBNum < NUMBER_OF_ERASE_BLOCKS; logicalEBNum++)
      {
      if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, logicalEBNum, CACHE_INIT_TYPE)))
        {
        return status;
        }
      eraseCount = GetTrueEraseCount(devID, logicalEBNum);

      if(ERASE_STATUS_CLEAR_DWORD_MASK == eraseCount)
        {
        continue;
        }

      status = SetSaveStaticWL(devID, logicalEBNum, eraseCount);
      if(status != FTL_ERR_PASS)
        {
        return status;
        }

      if(GetGCNum(devID, logicalEBNum) > maxValue && logicalEBNum < NUM_DATA_EBLOCKS)
        {
        maxValue = GetGCNum(devID, logicalEBNum);
        }
      }
    // Set to greater than the largest value used so far
    GCNum[devID] = maxValue + 1;

#endif // #if (FTL_STATIC_WEAR_LEVELING == true)
    for(indexCount = 0; indexCount < indexMaxCount; indexCount++)
      {
      if((((index[indexCount + (devID * NUM_EBLOCK_MAP_INDEX)] * FLUSH_RAM_TABLE_SIZE) % ((EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD))) == 0))
        {
        logicalEBNum = (uint16_t) ((index[indexCount + (devID * NUM_EBLOCK_MAP_INDEX)] * FLUSH_RAM_TABLE_SIZE) / (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD));
        }
      else
        {
        logicalEBNum = (uint16_t) ((index[indexCount + (devID * NUM_EBLOCK_MAP_INDEX)] * FLUSH_RAM_TABLE_SIZE) / (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) + 1;
        }
      if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, logicalEBNum, CACHE_INIT_TYPE)))
        {
        return status;
        }
      }
#endif // #if (CACHE_RAM_BD_MODULE == true)

#if (FTL_SUPER_SYS_EBLOCK == true)
    for(eBlockNum = 0; eBlockNum < NUM_SUPER_SYS_EBLOCKS; eBlockNum++)
      {
      tempSuperArray[devID][eBlockNum].logicalEBNum = EMPTY_WORD;
      tempSuperArray[devID][eBlockNum].phyAddr = EMPTY_WORD;
      tempSuperArray[devID][eBlockNum].key = EMPTY_DWORD;
      }
    for(eBlockNum = 0; eBlockNum < NUM_SUPER_SYS_EBLOCKS; eBlockNum++)
      {
      if((status = TABLE_GetSuperSysEBEntry(devID, eBlockNum, &logicalAddr, &phyEBAddr, &latestIncNumber)) != FTL_ERR_PASS)
        {
        break; // trying to excess outside table.
        }
      if((logicalAddr == EMPTY_WORD) && (phyEBAddr == EMPTY_WORD) && (latestIncNumber == EMPTY_DWORD))
        {
        break; // no more entries in table
        }
      tempSuperArray[devID][eBlockNum].logicalEBNum = logicalAddr;
      tempSuperArray[devID][eBlockNum].phyAddr = phyEBAddr;
      tempSuperArray[devID][eBlockNum].key = latestIncNumber;
      }
    if((status = TABLE_SuperSysEBClear(devID)) != FTL_ERR_PASS)
      {
      return status;
      }
    for(eBlockNum = 0; eBlockNum < NUM_SUPER_SYS_EBLOCKS; eBlockNum++)
      {
      phyEBAddr = tempSuperArray[devID][eBlockNum].phyAddr;
      latestIncNumber = tempSuperArray[devID][eBlockNum].key;
      logicalAddr = GetLogicalEBlockAddr(devID, phyEBAddr);
      if((status = TABLE_SuperSysEBInsert(devID, logicalAddr, phyEBAddr, latestIncNumber)) != FTL_ERR_PASS)
        {
        if((status = TABLE_SuperSysEBClear(devID)) != FTL_ERR_PASS)
          {
          // skip
          }
        if((status = TABLE_SuperSysEBInsert(devID, EMPTY_WORD, EMPTY_WORD, EMPTY_DWORD)) != FTL_ERR_PASS)
          {
          //skip
          }
        SuperEBInfo[devID].checkSuperPF = true;
        }
      }
    TABLE_ClearReservedEB(devID);
    for(eBlockCount = SYSTEM_START_EBLOCK; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
      {
#if( FTL_EBLOCK_CHAINING == true)
      if(EMPTY_WORD != GetChainLogicalEBNum(devID, eBlockCount))
        {
#if (ENABLE_EB_ERASED_BIT == true)
        SetEBErased(devID, eBlockCount, false);
#endif // #if (ENABLE_EB_ERASED_BIT == true)
        continue;
        }
#endif  // #if( FTL_EBLOCK_CHAINING == true)

      if(FTL_ERR_PASS == TABLE_CheckUsedSysEB(devID, eBlockCount))
        {
#if (ENABLE_EB_ERASED_BIT == true)
        SetEBErased(devID, eBlockCount, false);
#endif // #if (ENABLE_EB_ERASED_BIT == true)
        continue;
        }

      if((status = TABLE_InsertReservedEB(devID, eBlockCount)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
#endif

#if (FTL_SUPER_SYS_EBLOCK == true)
    if(((shutdownLoc.eBlockNum == EMPTY_WORD) && (shutdownLoc.entryIndex == EMPTY_WORD)) || (true == SuperEBInfo[devID].checkLost)) // For Power Failure case.
#else  // #if(FTL_SUPER_SYS_EBLOCK == true)
    if((shutdownLoc.eBlockNum == EMPTY_WORD) && (shutdownLoc.entryIndex == EMPTY_WORD)) // For Power Failure case.
#endif
      {
      tempFound = false;
      for(eBlockCount = 0; eBlockCount < NUM_TRANSACTLOG_EBLOCKS; eBlockCount++) // sanity check
        {
        if((status = TABLE_GetTransLogEntry(devID, eBlockCount, &logicalAddr, &phyAddr, &key)) != FTL_ERR_PASS)
          {
          break; // trying to excess outside table.
          }
        if((logicalAddr == EMPTY_WORD) && (phyAddr == EMPTY_WORD) && (key == EMPTY_DWORD))
          {
          break; // no more entries in table
          }
        tempFound = false;

        for(tempBlockCount = 0; tempBlockCount < tempCount; tempBlockCount++)
          {
          if(tempArray[devID][tempBlockCount].phyAddr == phyAddr)
            {
            tempFound = true;
            }
          }
        if(false == tempFound)
          {
          break;
          }
        }

      if(tempCount != GetTransLogEBArrayCount(devID)) // Check an array count
        {
        tempFound = false;
        }
      if(false == tempFound) // Restructuring TransEB 
        {
        if((status = TABLE_TransEBClear(devID)) != FTL_ERR_PASS)
          {
          return status;
          }
        for(eBlockCount = 0; eBlockCount < NUM_TRANSACTLOG_EBLOCKS; eBlockCount++)
          {
          phyAddr = tempArray[devID][eBlockCount].phyAddr;
          if(EMPTY_WORD == phyAddr)
            {
            continue;
            }
          logicalAddr = GetLogicalEBlockAddr(devID, phyAddr);
          if(EMPTY_WORD == logicalAddr)
            {
            continue;
            }
          key = tempArray[devID][eBlockCount].key;
          if((status = TABLE_TransLogEBInsert(devID, logicalAddr, phyAddr, key)) != FTL_ERR_PASS)
            {
            return status;
            }
          }
        SetTransLogEBCounter(devID, tempEBCounter);

        TABLE_ClearReservedEB(devID);
        for(eBlockCount = SYSTEM_START_EBLOCK; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
          {
#if (ENABLE_EB_ERASED_BIT == true)
          SetEBErased(devID, eBlockCount, false);
#endif // #if (ENABLE_EB_ERASED_BIT == true)

#if( FTL_EBLOCK_CHAINING == true)
          if(EMPTY_WORD != GetChainLogicalEBNum(devID, eBlockCount))
            {
            continue;
            }
#endif  // #if( FTL_EBLOCK_CHAINING == true)

          if(FTL_ERR_PASS == TABLE_CheckUsedSysEB(devID, eBlockCount))
            {
            continue;
            }

          if((status = TABLE_InsertReservedEB(devID, eBlockCount)) != FTL_ERR_PASS)
            {
            return status;
            }
          }
        }
      }

    if(logIndexFound != EMPTY_DWORD)
      {

      transLogEBCounter = GetTransLogEBCounter(devID);
      if(logIndexFound == transLogEBCounter)
        { // get rid of teh log entries, the current flush is already the latest one, dont need this 

        if((shutdownLoc.eBlockNum == EMPTY_WORD) && (shutdownLoc.entryIndex == EMPTY_WORD))
          {

#if (ENABLE_EB_ERASED_BIT == true)
          for(eBlockCount = SYSTEM_START_EBLOCK; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
            {
            SetEBErased(devID, eBlockCount, false);
            }
#endif // #if (ENABLE_EB_ERASED_BIT == true)


#if (FTL_SUPER_SYS_EBLOCK == true)
          SuperEBInfo[devID].checkSysPF = true;

          // for Power Failure Caes
          if(true == SuperEBInfo[devID].checkLost || true == SuperEBInfo[devID].checkSuperPF || true == SuperEBInfo[devID].checkSysPF)
            {
            for(eBlockNum = 0; eBlockNum < NUM_SUPER_SYS_EBLOCKS; eBlockNum++)
              {
              if((status = TABLE_GetSuperSysEBEntry(devID, eBlockNum, &logicalAddr, &phyEBAddr, &latestIncNumber)) != FTL_ERR_PASS)
                {
                break; // trying to excess outside table.
                }
              if((logicalAddr == EMPTY_WORD) && (phyEBAddr == EMPTY_WORD) && (latestIncNumber == EMPTY_DWORD))
                {
                break; // no more entries in table
                }
              //write  OLD_SYS_BLOCK_SIGNATURE
              sysEBlockInfo.oldSysBlock = OLD_SYS_BLOCK_SIGNATURE;
              flashPage.devID = devID;
              flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBAddr, 0);
              flashPage.vPage.pageOffset = (uint16_t) ((uint32_t)&(tempSysPtr->oldSysBlock));
              flashPage.byteCount = OLD_SYS_BLOCK_SIGNATURE_SIZE;
              if((flash_status = FLASH_RamPageWriteMetaData(&flashPage, (uint8_t *) & sysEBlockInfo.oldSysBlock)) != FLASH_PASS)
                {
#if(FTL_DEFECT_MANAGEMENT == true)
                if(FLASH_PARAM == flash_status)
                  {
                  return FTL_ERR_SUPER_FLASH_WRITE_02;
                  }
                SetBadEBlockStatus(devID, logicalAddr, true);

                if(FLASH_MarkDefectEBlock(&flashPage) != FLASH_PASS)
                  {
                  // do nothing, just try to mark bad, even if it fails we move on.
                  }
#else
                return FTL_ERR_SUPER_FLASH_WRITE_02;
#endif
                }
              else
                {
                if((status = TABLE_InsertReservedEB(devID, eBlockNum)) != FTL_ERR_PASS)
                  {
                  return status;
                  }
                }
              }
            if(false == checkFlag)
              {
              TABLE_SuperSysEBClear(devID);
              TABLE_SuperSysEBInsert(devID, EMPTY_WORD, EMPTY_WORD, EMPTY_DWORD); // for next Super System EB
              checkFlag = true;
              }
            }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)


#if(FTL_DEFECT_MANAGEMENT == true)
          while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
            {
            status = FTL_EraseAllTransLogBlocksOp(devID);
            if(status == FTL_ERR_MARKBB_COMMIT)
              {
              checkBBMark = true;
              status = FTL_ERR_PASS;
              }
            if(status != FTL_ERR_LOG_NEW_EBLOCK_FAIL)
              {
              break;
              }
            checkBBMark = true;
            sanityCounter++;
            }
          if(status != FTL_ERR_PASS)
            {
            return status;
            }

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
          if((status = FTL_EraseAllTransLogBlocksOp(devID)) != FTL_ERR_PASS)
            {
            return status;
            }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

          }
        }
      else if(logIndexFound < transLogEBCounter)
        {
        if((shutdownLoc.eBlockNum == EMPTY_WORD) && (shutdownLoc.entryIndex == EMPTY_WORD))
          {
          if((status = FTL_RemoveOldTransLogBlocks(devID, logIndexFound)) != FTL_ERR_PASS)
            {
            return status;
            }

          TABLE_ClearReservedEB(devID);
          for(eBlockCount = SYSTEM_START_EBLOCK; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
            {
#if (ENABLE_EB_ERASED_BIT == true)
            SetEBErased(devID, eBlockCount, false);
#endif // #if (ENABLE_EB_ERASED_BIT == true)

#if( FTL_EBLOCK_CHAINING == true)
            if(EMPTY_WORD != GetChainLogicalEBNum(devID, eBlockCount))
              {
              continue;
              }
#endif  // #if( FTL_EBLOCK_CHAINING == true)

            if(FTL_ERR_PASS == TABLE_CheckUsedSysEB(devID, eBlockCount))
              {
              continue;
              }

            if((status = TABLE_InsertReservedEB(devID, eBlockCount)) != FTL_ERR_PASS)
              {
              return status;
              }
            }
          }
        }
      else
        {
        return FTL_ERR_LOG_EB_COUNTER;
        }
      }
    // Go and read Log entries, (if any)
    if((status = GetTransLogsSetRAMTables(devID, &shutdownLoc, &ramTablesUpdated, &logEblockFound)) != FTL_ERR_PASS)
      {
      return status;
      }

#if (CACHE_RAM_BD_MODULE == true)
    if((ramTablesUpdated == true) || (logEblockFound == true) || (pfFlushFlag == true))
#else
    if((ramTablesUpdated == true) || (logEblockFound == true))
#endif

      {

      for(eBlockCount = 0; eBlockCount < NUM_TRANSACTLOG_EBLOCKS; eBlockCount++)
        {
        tempArray[devID][eBlockCount].logicalEBNum = EMPTY_WORD;
        tempArray[devID][eBlockCount].phyAddr = EMPTY_WORD;
        tempArray[devID][eBlockCount].key = EMPTY_DWORD;
        }
      for(eBlockCount = 0; eBlockCount < NUM_TRANSACTLOG_EBLOCKS; eBlockCount++)
        {
        if((status = TABLE_GetTransLogEntry(devID, eBlockCount, &logicalAddr, &phyAddr, &key)) != FTL_ERR_PASS)
          {
          break; // trying to excess outside table.
          }
        if((logicalAddr == EMPTY_WORD) && (phyAddr == EMPTY_WORD) && (key == EMPTY_DWORD))
          {
          break; // no more entries in table
          }

        logicalAddr = GetLogicalEBlockAddr(devID, phyAddr);
        tempArray[devID][eBlockCount].logicalEBNum = logicalAddr;
        tempArray[devID][eBlockCount].phyAddr = phyAddr;
        tempArray[devID][eBlockCount].key = key;
        }
      if((status = TABLE_TransEBClear(devID)) != FTL_ERR_PASS)
        {
        return status;
        }
      for(eBlockCount = 0; eBlockCount < NUM_TRANSACTLOG_EBLOCKS; eBlockCount++)
        {
        logicalAddr = tempArray[devID][eBlockCount].logicalEBNum;
        phyAddr = tempArray[devID][eBlockCount].phyAddr;
        key = tempArray[devID][eBlockCount].key;
        if((logicalAddr == EMPTY_WORD) && (phyAddr == EMPTY_WORD) && (key == EMPTY_DWORD))
          {
          break;
          }
        if((status = TABLE_TransLogEBInsert(devID, logicalAddr, phyAddr, key)) != FTL_ERR_PASS)
          {
          return status;
          }
        }


      TABLE_ClearReservedEB(devID);
      for(eBlockCount = SYSTEM_START_EBLOCK; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
        {
#if (ENABLE_EB_ERASED_BIT == true)
        SetEBErased(devID, eBlockCount, false);
#endif // #if (ENABLE_EB_ERASED_BIT == true)

#if( FTL_EBLOCK_CHAINING == true)
        if(EMPTY_WORD != GetChainLogicalEBNum(devID, eBlockCount))
          {
          continue;
          }
#endif  // #if( FTL_EBLOCK_CHAINING == true)

        if(FTL_ERR_PASS == TABLE_CheckUsedSysEB(devID, eBlockCount))
          {
          continue;
          }

        if((status = TABLE_InsertReservedEB(devID, eBlockCount)) != FTL_ERR_PASS)
          {
          return status;
          }
        }

#if (FTL_SUPER_SYS_EBLOCK == true)
      SuperEBInfo[devID].checkSysPF = true;

      // for Power Failure Caes
      if(true == SuperEBInfo[devID].checkLost || true == SuperEBInfo[devID].checkSuperPF || true == SuperEBInfo[devID].checkSysPF)
        {
        for(eBlockNum = 0; eBlockNum < NUM_SUPER_SYS_EBLOCKS; eBlockNum++)
          {
          if((status = TABLE_GetSuperSysEBEntry(devID, eBlockNum, &logicalAddr, &phyEBAddr, &latestIncNumber)) != FTL_ERR_PASS)
            {
            break; // trying to excess outside table.
            }
          if((logicalAddr == EMPTY_WORD) && (phyEBAddr == EMPTY_WORD) && (latestIncNumber == EMPTY_DWORD))
            {
            break; // no more entries in table
            }
          //write  OLD_SYS_BLOCK_SIGNATURE
          sysEBlockInfo.oldSysBlock = OLD_SYS_BLOCK_SIGNATURE;
          flashPage.devID = devID;
          flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBAddr, 0);
          flashPage.vPage.pageOffset = (uint16_t) ((uint32_t)&(tempSysPtr->oldSysBlock));
          flashPage.byteCount = OLD_SYS_BLOCK_SIGNATURE_SIZE;
          if((flash_status = FLASH_RamPageWriteMetaData(&flashPage, (uint8_t *) & sysEBlockInfo.oldSysBlock)) != FLASH_PASS)
            {
#if(FTL_DEFECT_MANAGEMENT == true)
            if(FLASH_PARAM == flash_status)
              {
              return FTL_ERR_SUPER_FLASH_WRITE_03;
              }
            SetBadEBlockStatus(devID, logicalAddr, true);

            if(FLASH_MarkDefectEBlock(&flashPage) != FLASH_PASS)
              {
              // do nothing, just try to mark bad, even if it fails we move on.
              }
#else
            return FTL_ERR_SUPER_FLASH_WRITE_03;
#endif
            }
          else
            {
            if((status = TABLE_InsertReservedEB(devID, logicalAddr)) != FTL_ERR_PASS)
              {
              return status;
              }
            }
          }
        if(false == checkFlag)
          {
          TABLE_SuperSysEBClear(devID);
          TABLE_SuperSysEBInsert(devID, EMPTY_WORD, EMPTY_WORD, EMPTY_DWORD); // for next Super System EB
          }
        }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

      // Flush after load to mark a stable point.
      if((status = TABLE_Flush(FLUSH_GC_MODE)) != FTL_ERR_PASS)
        {
        return status;
        }

#if (CACHE_RAM_BD_MODULE == true)
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
      eBlockCount = gTargetPftEBForNand;
      if(EMPTY_WORD != eBlockCount)
        {
        if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, eBlockCount, CACHE_INIT_TYPE)))
          {
          return status;
          }
        if(EMPTY_WORD != GetChainLogicalEBNum(devID, eBlockCount))
          {
          if(eBlockCount >= NUM_DATA_EBLOCKS)
            {
            eBlockCount = GetChainLogicalEBNum(devID, eBlockCount);
            }
          }
        if(eBlockCount < NUM_DATA_EBLOCKS)
          {
          if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, eBlockCount, CACHE_INIT_TYPE)))
            {
            return status;
            }

#if(FTL_DEFECT_MANAGEMENT == true)
          status = InternalForcedGCWithBBManagement(devID, eBlockCount, &validFreePageIndex, &freePageIndex, true);

#else
          status = FTL_InternalForcedGC(devID, eBlockCount, &validFreePageIndex, &freePageIndex, true);
#endif

          if(status != FTL_ERR_PASS)
            {
            return status;
            }
          }

        }
      gTargetPftEBForNand = EMPTY_WORD;
      if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS)
        {
        return status;
        }
#endif
#endif

      shutdownLoc.eBlockNum = EMPTY_WORD;
      shutdownLoc.entryIndex = EMPTY_WORD;
      }
    if((status = ResetIndexValue(devID, &shutdownLoc)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(formatCount >= NUM_SYSTEM_EBLOCKS)
      {
      return FTL_ERR_FLUSH_TOO_MANY_BLOCKS;
      }
#if(FTL_DEFECT_MANAGEMENT == true)
    if(true == checkBBMark)
      {
      // Flush BB Mark
      if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
#endif
    }

  return FTL_ERR_PASS;
  }

//-------------------------------

FTL_STATUS GetTransLogsSetRAMTables(FTL_DEV devID, LOG_ENTRY_LOC_PTR startLoc, uint8_t * ramTablesUpdated,
                                    uint8_t * logEblockFound)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FLASH_STATUS flash_status = FLASH_PASS; /*4*/
  uint16_t entryIndex = 0x0; /*2*/
  uint16_t byteOffset = 0x0; /*2*/
  uint16_t logicalAddr = 0x0; /*2*/
  uint16_t phyEBAddr = 0x0; /*2*/
  uint16_t eBlockNum = 0x0; /*2*/
  uint16_t logEntry[LOG_ENTRY_SIZE / 2]; /*16*/
  uint16_t logType = EMPTY_WORD; /*2*/
  uint16_t prevLogType = EMPTY_WORD; /*2*/
  uint32_t latestIncNumber = 0x0; /*4*/
  uint8_t skipFlag = false; /*1*/
  uint16_t savedEntryIndex = 0x0; /*2*/
  uint16_t savedEBlockNum = 0x0; /*2*/
  uint8_t GCLogTypeB = EMPTY_BYTE; /*1*/

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  uint16_t logEBNum = 0x0; /*2*/
  uint16_t phyEBOffset = 0x0; /*2*/
  uint8_t entryBCnt = 0x0; /*1*/
  uint8_t pageLocEntryCnt = 0x0; /*1*/
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

#if DEBUG_LOG_ENTRIES
  uint8_t pageCnt = 0; /*1*/
#endif  // #if DEBUG_LOG_ENTRIES

#if DEBUG_EARLY_BAD_ENTRY
  uint16_t badEntry = false; /*2*/
#endif  // #if DEBUG_EARLY_BAD_ENTRY

  FLASH_PAGE_INFO flashPageInfo = {0, 0,
    {0, 0}}; /*11*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  GC_LOG_ENTRY gcLog;
  TRANS_LOG_ENTRY_A_PTR getLogType; /*16*/

#if(FTL_EBLOCK_CHAINING == true)
  CHAIN_LOG_ENTRY chainLog; /*16*/
#endif  // #if(FTL_EBLOCK_CHAINING == true)

#if (FTL_ENABLE_UNUSED_EB_SWAP == true)
  EBSWAP_LOG_ENTRY ebSwapLog; /*16*/
#endif  // #if (FTL_ENABLE_UNUSED_EB_SWAP == true)

#if(FTL_UNLINK_GC == true)
  uint8_t foundUnlinkLog = false; /*1*/
  uint8_t unlinkLogTypeB = 0; /*1*/
  uint16_t count = 0; /*2*/
  uint16_t pageOffset = 0; /*2*/
  uint16_t freePageIndex = 0; /*2*/
  FREE_BIT_MAP_TYPE bitMap = 0; /*1*/
  uint8_t pageBitMap[GC_MOVE_BITMAP];
  UNLINK_LOG_ENTRY unlinkLog;
#endif  // #if(FTL_UNLINK_GC == true)

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  uint8_t transLogFlag = false; /*1*/
  SPARE_LOG_ENTRY spareLog; /*16*/
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

  *logEblockFound = false;
  for(eBlockNum = 0; eBlockNum < NUM_TRANSACTLOG_EBLOCKS; eBlockNum++)
    {
    if((status = TABLE_GetTransLogEntry(devID, eBlockNum, &logicalAddr, &phyEBAddr, &latestIncNumber)) != FTL_ERR_PASS)
      {
      return status; // trying to excess outside table.
      }
    if((logicalAddr == EMPTY_WORD) && (phyEBAddr == EMPTY_WORD) && (latestIncNumber == EMPTY_DWORD))
      {
      break; // no more entries in table
      }
    if((startLoc->eBlockNum != EMPTY_WORD) && (startLoc->entryIndex != EMPTY_WORD))
      {
      if(eBlockNum < startLoc->eBlockNum)
        {
        continue;
        }
      }

    flashPageInfo.devID = devID;
    flashPageInfo.vPage.pageOffset = 0;
    flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBAddr, 0);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    flashPageInfo.byteCount = sizeof (SYS_EBLOCK_INFO);
    flash_status = FLASH_RamPageReadMetaData(&flashPageInfo, (uint8_t *) (&sysEBlockInfo));

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    flashPageInfo.byteCount = SECTOR_SIZE;
    flash_status = FLASH_RamPageReadDataBlock(&flashPageInfo, &pseudoRPB[devID][0]);
    memcpy((uint8_t *) (&sysEBlockInfo), &pseudoRPB[devID][0], SYS_INFO_SIZE);
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

    if(flash_status != FLASH_PASS)
      {
      return FTL_ERR_FLASH_READ_05;
      }
    // Only if this valid Trans block
    if((false == VerifyCheckWord((uint16_t *) & sysEBlockInfo.type,
                                 SYS_INFO_DATA_WORDS, sysEBlockInfo.checkWord)) &&
       (sysEBlockInfo.oldSysBlock != OLD_SYS_BLOCK_SIGNATURE) &&
       (sysEBlockInfo.type == SYS_EBLOCK_INFO_LOG))
      {

#if DEBUG_EARLY_BAD_ENTRY
      badEntry = false;
#endif  // #if DEBUG_EARLY_BAD_ENTRY

      // go through all the entries in the block
      for(entryIndex = 1; entryIndex < NUM_LOG_ENTRIES_PER_EBLOCK; entryIndex++)
        {
        if((startLoc->eBlockNum != EMPTY_WORD) && (startLoc->entryIndex != EMPTY_WORD))
          {
          if((eBlockNum == startLoc->eBlockNum) && (entryIndex < startLoc->entryIndex))
            {
            continue;
            }
          }
        byteOffset = entryIndex * LOG_ENTRY_DELTA;
        flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBAddr, entryIndex);
        flashPageInfo.vPage.pageOffset = byteOffset % VIRTUAL_PAGE_SIZE;

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
        flashPageInfo.byteCount = sizeof (logEntry);
        flash_status = FLASH_RamPageReadMetaData(&flashPageInfo, (uint8_t *) & logEntry[0]);

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
        flashPageInfo.byteCount = SECTOR_SIZE;
        flash_status = FLASH_RamPageReadDataBlock(&flashPageInfo, &pseudoRPB[devID][0]);
        memcpy((uint8_t *) & logEntry[0], &pseudoRPB[devID][0], LOG_ENTRY_SIZE);
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

        if(flash_status != FLASH_PASS)
          {

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
          // power failure check...
          if(flash_status == FLASH_ECC_FAIL)
            {
            // power failure occurred, invoke Flush_GC
            *logEblockFound = true;
            break;
            }
          else
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

            {
            return FTL_ERR_FLASH_READ_04;
            }
          }
        if(*logEblockFound == false)
          {
          if(FLASH_CheckEmpty((uint8_t *) & logEntry[0], LOG_ENTRY_SIZE) != FLASH_PASS)
            {
            *logEblockFound = true;
            }
          }

#if DEBUG_EARLY_BAD_ENTRY
        if(true == badEntry)
          break;
#endif  // #if DEBUG_EARLY_BAD_ENTRY

        if(VerifyCheckWord(&logEntry[LOG_ENTRY_DATA_START],
                           LOG_ENTRY_DATA_WORDS, logEntry[LOG_ENTRY_CHECK_WORD]))
          {

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
#if(FTL_REDUNDANT_LOG_ENTRIES == true)
          // Primary Copy is bad; Check Redundant Copy
          flashPageInfo.vPage.pageOffset += LOG_ENTRY_SIZE;
          if((FLASH_RamPageReadMetaData(&flashPageInfo, (uint8_t *) & logEntry[0])) != FLASH_PASS)
            {
            return FTL_ERR_FLASH_READ_03;
            }
          flashPageInfo.vPage.pageOffset -= LOG_ENTRY_SIZE;
          if(VerifyCheckWord(&logEntry[LOG_ENTRY_DATA_START],
                             LOG_ENTRY_DATA_WORDS, logEntry[LOG_ENTRY_CHECK_WORD]))
#endif  // #if(FTL_REDUNDANT_LOG_ENTRIES == true)
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

            {
            // Log Entry is bad

#if DEBUG_EARLY_BAD_ENTRY     
            badEntry = true;
            // Get one more entry
            continue;

#else  // #if DEBUG_EARLY_BAD_ENTRY
            // Don't check any more entries
            break;
#endif  // #else  // #if DEBUG_EARLY_BAD_ENTRY

            }

          }
        getLogType = (TRANS_LOG_ENTRY_A_PTR) logEntry;
        logType = getLogType->type; // get LogType
        if((uint8_t) logType == EMPTY_BYTE)
          {
          break;
          }

#if(FTL_UNLINK_GC == true)
        if(foundUnlinkLog == true)
          {
          if((status = UpdateRAMTablesUsingUnlinkLogs(devID, &unlinkLog)) != FTL_ERR_PASS)
            {
            return status;
            }
          *ramTablesUpdated = true;
          foundUnlinkLog = false;
          }
#endif  // #if(FTL_UNLINK_GC == true)

        switch(logType)
          {
            // ........Trans Logs...........
          case TRANS_LOG_TYPE_A:
          {
            TranslogBEntries = 0;
            if(prevLogType != EMPTY_WORD) // correct seq. check.
              {
              return FTL_ERR_LOG_TYPE_A_SEQUENCE;
              }
            prevLogType = TRANS_LOG_TYPE_A;
            memcpy((uint8_t *) & TransLogEntry.entryA, (uint8_t *) & logEntry[0], sizeof (logEntry));

#if (CACHE_RAM_BD_MODULE == true)
            if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, (uint16_t) ((TransLogEntry.entryA.LBA) / NUM_SECTORS_PER_EBLOCK), CACHE_INIT_TYPE)))
              {
              return status;
              }
#endif
            break;
          }
          case TRANS_LOG_TYPE_B:
          {
            if(prevLogType != TRANS_LOG_TYPE_A && prevLogType != TRANS_LOG_TYPE_B) // correct seq. check.
              {
              return FTL_ERR_LOG_TYPE_B_SEQUENCE;
              }
            prevLogType = TRANS_LOG_TYPE_B;
            memcpy((uint8_t *) & TransLogEntry.entryB[TranslogBEntries], (uint8_t *) & logEntry[0], sizeof (logEntry));

            TranslogBEntries++;
            break;

          }
          case TRANS_LOG_TYPE_C:
          {
            if((prevLogType == TRANS_LOG_TYPE_A) || (prevLogType == TRANS_LOG_TYPE_B)) // correct seq. check.
              {
              prevLogType = EMPTY_WORD;
              }
            else
              {
              return FTL_ERR_LOG_TYPE_C_SEQUENCE;
              }
            memcpy((uint8_t *) & TransLogEntry.entryC, (uint8_t *) & logEntry[0], sizeof (logEntry));

            if(true == skipFlag)
              {
              skipFlag = false;
              // Back up to the saved point
              entryIndex = savedEntryIndex;
              eBlockNum = savedEBlockNum;
              prevLogType = GC_TYPE_A;
              // Don't process this yet
              break;
              }
            if((status = UpdateRAMTablesUsingTransLogs(devID)) != FTL_ERR_PASS)
              {
              return status;
              }
            *ramTablesUpdated = true;
            break;

          }
            // .........GC Logs...........
          case GC_TYPE_A:
          {
            if(prevLogType != EMPTY_WORD) // correct seq. check.
              {
              return FTL_ERR_LOG_GC_A_SEQUENCE;
              }
            prevLogType = GC_TYPE_A;
            if(false == skipFlag)
              {
              memcpy((uint8_t *) & gcLog.partA, (uint8_t *) & logEntry[0], sizeof (logEntry));
              }

#if (CACHE_RAM_BD_MODULE == true)
            if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, gcLog.partA.logicalEBAddr, CACHE_INIT_TYPE)))
              {
              return status;
              }
#endif

#if (ENABLE_EB_ERASED_BIT == true)
            SetEBErased(devID, gcLog.partA.reservedEBAddr, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

            if(true == gcLog.partA.holdForMerge)
              {
              if(false == skipFlag)
                  {
                  // Stop processing until a TRANS_TYPE_C is found,
                  //   then starting processing again with the next entry
                  skipFlag = true;
                  savedEntryIndex = entryIndex;
                  savedEBlockNum = eBlockNum;
                  }
              }
            break;
          }
          case GC_TYPE_B:
          {
            // correct seq. check.
            if(prevLogType == GC_TYPE_A)
              {
              GCLogTypeB = 0;
              }

            else if(prevLogType == GC_TYPE_B)
              {
              GCLogTypeB++;
              }
            else
              {
              return FTL_ERR_LOG_GC_B_SEQUENCE;
              }

#if (NUM_GC_TYPE_B > 1)
            if(GCLogTypeB < (NUM_GC_TYPE_B - 1))
              {
              prevLogType = GC_TYPE_B;
              }
            else
              {
              prevLogType = EMPTY_WORD;
              }
#else
            prevLogType = EMPTY_WORD;
#endif

            memcpy((uint8_t *) & gcLog.partB[GCLogTypeB], (uint8_t *) & logEntry[0], sizeof (logEntry));


            if(GCLogTypeB == (NUM_GC_TYPE_B - 1))
              {
              if(false == skipFlag)
                {
                if((status = UpdateRAMTablesUsingGCLogs(devID, &gcLog)) != FTL_ERR_PASS)
                  {
                  return status;
                  }
                *ramTablesUpdated = true;
                }
              }
            break;
          }

#if(FTL_EBLOCK_CHAINING == true)
            // ........Chain Logs..........
          case CHAIN_LOG_TYPE:
          {
            if(prevLogType != EMPTY_WORD) // correct seq. check.
              {
              return FTL_ERR_LOG_CHAIN_SEQUENCE;
              }
            memcpy((uint8_t *) & chainLog, (uint8_t *) & logEntry[0], sizeof (logEntry));

#if (CACHE_RAM_BD_MODULE == true)
            if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, chainLog.logicalFrom, CACHE_INIT_TYPE)))
              {
              return status;
              }

            if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, chainLog.logicalTo, CACHE_INIT_TYPE)))
              {
              return status;
              }
#endif

            if(false == skipFlag)
              {
              if((status = UpdateRAMTablesUsingChainLogs(devID, &chainLog)) != FTL_ERR_PASS)
                {
                return status;
                }
              *ramTablesUpdated = true;
              }
            break;
          }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

#if (FTL_ENABLE_UNUSED_EB_SWAP == true)
            // ........EBSwap Logs..........
          case EBSWAP_LOG_TYPE:
          {
            if(prevLogType != EMPTY_WORD) // correct seq. check.
              {
              return FTL_ERR_LOG_EBSWAP_SEQUENCE;
              }
            memcpy((uint8_t *) & ebSwapLog, (uint8_t *) & logEntry[0], sizeof (logEntry));
            if(false == skipFlag)
              {
              if((status = UpdateRAMTablesUsingEBSwapLogs(devID, &ebSwapLog)) != FTL_ERR_PASS)
                {
                return status;
                }
              *ramTablesUpdated = true;
              }
            break;
          }
#endif  // #if (FTL_ENABLE_UNUSED_EB_SWAP == true)

#if(FTL_UNLINK_GC == true)
            // ........UnlinkGC Logs..........
          case UNLINK_LOG_TYPE_A1:
          {
            if(prevLogType != EMPTY_WORD) // correct seq. check.
              {
              return FTL_ERR_LOG_UNLINK_A1_SEQUENCE;
              }
            memcpy((uint8_t *) & unlinkLog.partA, (uint8_t *) & logEntry[0], sizeof (logEntry));

#if (CACHE_RAM_BD_MODULE == true)
            if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, unlinkLog.partA.fromLogicalEBAddr, CACHE_INIT_TYPE)))
              {
              return status;
              }
            if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, unlinkLog.partA.toLogicalEBAddr, CACHE_INIT_TYPE)))
              {
              return status;
              }
#endif

            if(false == skipFlag)
              {
              if((status = UpdateRAMTablesUsingUnlinkLogs(devID, &unlinkLog)) != FTL_ERR_PASS)
                {
                return status;
                }
              *ramTablesUpdated = true;
              }
            break;
          }
          case UNLINK_LOG_TYPE_A2:
          {
            if(prevLogType != EMPTY_WORD) // correct seq. check.
              {
              return FTL_ERR_LOG_UNLINK_A2_SEQUENCE;
              }
            prevLogType = UNLINK_LOG_TYPE_A2;
            memcpy((uint8_t *) & unlinkLog.partA, (uint8_t *) & logEntry[0], sizeof (logEntry));

#if (CACHE_RAM_BD_MODULE == true)
            if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, unlinkLog.partA.fromLogicalEBAddr, CACHE_INIT_TYPE)))
              {
              return status;
              }
            if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, unlinkLog.partA.toLogicalEBAddr, CACHE_INIT_TYPE)))
              {
              return status;
              }
#endif
            break;
          }
          case UNLINK_LOG_TYPE_B:
          {
            // correct seq. check.
            if(prevLogType == UNLINK_LOG_TYPE_A2)
              {
              unlinkLogTypeB = 0;
              }
            else if(prevLogType == UNLINK_LOG_TYPE_B)
              {
              unlinkLogTypeB++;
              }
            else
              {
              return FTL_ERR_LOG_UNLINK_B_SEQUENCE;
              }

#if (NUM_UNLINK_TYPE_B > 1)
            if(unlinkLogTypeB < (NUM_UNLINK_TYPE_B - 1))
              {
              prevLogType = UNLINK_LOG_TYPE_B;
              }
            else
              {
              prevLogType = EMPTY_WORD;
              }
#else
            prevLogType = EMPTY_WORD;
#endif

            memcpy((uint8_t *) & unlinkLog.partB[unlinkLogTypeB], (uint8_t *) & logEntry[0], sizeof (logEntry));

            if(unlinkLogTypeB == (NUM_UNLINK_TYPE_B - 1))
              {
              if(false == skipFlag)
                {
                foundUnlinkLog = true;
                }
              }
            break;

          }
#endif  // #if(FTL_UNLINK_GC == true)

          default:
          {
            break;
          }
          } // switch
        } // entryIndex loop
      } // valid trans block check
    } // eBlockNum loop

  if((prevLogType == TRANS_LOG_TYPE_A) || (prevLogType == TRANS_LOG_TYPE_B))
    {
    // Previous write operation did not complete
    // Mark all pages listed as stale so they will not be re-used
    // Trans Entry A 
    for(pageLocEntryCnt = 0; pageLocEntryCnt < NUM_ENTRIES_TYPE_A; pageLocEntryCnt++)
      {
      logEBNum = TransLogEntry.entryA.pageLoc[pageLocEntryCnt].logEBNum;
      phyEBOffset = TransLogEntry.entryA.pageLoc[pageLocEntryCnt].phyEBOffset;
      if(logEBNum == EMPTY_WORD)
        {
        break; // No more Type A entries
        }

#if( FTL_EBLOCK_CHAINING == true)
      if(CHAIN_FLAG == (CHAIN_FLAG & phyEBOffset))
        {
        // Page is in chained-to EBlock
        logEBNum = GetChainLogicalEBNum(devID, logEBNum);
        phyEBOffset = phyEBOffset & ~CHAIN_FLAG;
        }
#endif  // #if( FTL_EBLOCK_CHAINING == true)

      // If chain-to EBlock was never assigned, do not update table
      if(FTL_ERR_PASS != TABLE_CheckUsedSysEB(devID, logEBNum))
        {
#if (ENABLE_EB_ERASED_BIT == true)
        SetEBErased(devID, logEBNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

        // if page was valid before the transaction, it is still valid when the transaction is canceled
        if(BLOCK_INFO_VALID_PAGE != GetEBlockMapFreeBitIndex(devID, logEBNum, phyEBOffset))
          {
          UpdatePageTableInfo(devID, logEBNum, EMPTY_INVALID, phyEBOffset, BLOCK_INFO_STALE_PAGE);
          }
        }
      }
    // Trans Entry B
    for(entryBCnt = 0; entryBCnt < TranslogBEntries; entryBCnt++)
      {
      for(pageLocEntryCnt = 0; pageLocEntryCnt < NUM_ENTRIES_TYPE_B; pageLocEntryCnt++)
        {
        logEBNum = TransLogEntry.entryB[entryBCnt].pageLoc[pageLocEntryCnt].logEBNum;
        phyEBOffset = TransLogEntry.entryB[entryBCnt].pageLoc[pageLocEntryCnt].phyEBOffset;
        if(logEBNum == EMPTY_WORD)
          {
          break; // No more Type B entries
          }

#if( FTL_EBLOCK_CHAINING == true)
        if(CHAIN_FLAG == (CHAIN_FLAG & phyEBOffset))
          {
          // Page is in chained-to EBlock
          logEBNum = GetChainLogicalEBNum(devID, logEBNum);
          phyEBOffset = phyEBOffset & ~CHAIN_FLAG;
          }
#endif  // #if( FTL_EBLOCK_CHAINING == true)

        // If chain-to EBlock was never assigned, do not update table
        if(FTL_ERR_PASS != TABLE_CheckUsedSysEB(devID, logEBNum))
          {
#if (ENABLE_EB_ERASED_BIT == true)
          SetEBErased(devID, logEBNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

          // if page was valid before the transaction, it is still valid when the transaction is canceled
          if(BLOCK_INFO_VALID_PAGE != GetEBlockMapFreeBitIndex(devID, logEBNum, phyEBOffset))
            {
            UpdatePageTableInfo(devID, logEBNum, EMPTY_INVALID, phyEBOffset, BLOCK_INFO_STALE_PAGE);
            }
          }
        }
      }
    }

#if(FTL_UNLINK_GC == true)
  if(foundUnlinkLog == true)
    {
    for(count = 0; count < NUM_UNLINK_TYPE_B; count++)
      {
      memcpy(&pageBitMap[count * NUM_ENTRIES_UNLINK_TYPE_B], &unlinkLog.partB[count].pageMovedBitMap[0], NUM_ENTRIES_UNLINK_TYPE_B);
      }

    for(pageOffset = 0; pageOffset < NUM_PAGES_PER_EBLOCK; pageOffset++)
      {
      bitMap = GetBitMapField(&pageBitMap[0], pageOffset, 1);
      if(bitMap == GC_MOVED_PAGE)
        {
        freePageIndex = GetFreePageIndex(devID, unlinkLog.partA.toLogicalEBAddr);
        UpdatePageTableInfo(devID, unlinkLog.partA.toLogicalEBAddr, EMPTY_INVALID, freePageIndex, BLOCK_INFO_STALE_PAGE);
        }
      }
    foundUnlinkLog = false;
    }
#endif  // #if(FTL_UNLINK_GC == true)

  return FTL_ERR_PASS;
  }

//------------------------------

FTL_STATUS ProcessPageLoc(FTL_DEV devID, LOG_PHY_PAGE_LOCATPTR pageLocPtr,
                          uint32_t pageAddress)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t logicalEBNum = 0; /*2*/
  uint16_t logicalPageOffset = 0; /*2*/
  uint16_t phyPageOffset = 0; /*2*/

#if(FTL_EBLOCK_CHAINING == true)
  uint16_t chainLogEBNum = EMPTY_WORD; /*2*/
#endif  //  #if(FTL_EBLOCK_CHAINING == true)

  logicalEBNum = pageLocPtr->logEBNum;

#if(FTL_EBLOCK_CHAINING == true)
  chainLogEBNum = GetChainLogicalEBNum(devID, logicalEBNum);
#endif  //  #if(FTL_EBLOCK_CHAINING == true)

  if((status = GetLogicalPageOffset(pageAddress, &logicalPageOffset)) != FTL_ERR_PASS)
    {
    return status;
    }
  // ...... Process Old Page .......
  // If this transfer follows another transfer, then the old page was a 
  //   merged-page that needs to be marked stale.
  // If this transfer follows a GC, then the GC was part of the same Write Op.
  //   The GC Log marked merged-from pages as erased.  Other PageLoc elements
  //   in this Transfer Log may have re-marked the old page to valid.     
  phyPageOffset = GetPPASlot(devID, logicalEBNum, logicalPageOffset);
  if(phyPageOffset != EMPTY_INVALID)
    {
    // Old page exists

#if(FTL_EBLOCK_CHAINING == true)
    if(EMPTY_WORD != chainLogEBNum)
      {
      // Block has been chained
      if(pageLocPtr->phyEBOffset & CHAIN_FLAG)
        {
        // New page is in chain-to block
        if(phyPageOffset != CHAIN_INVALID)
          {
          // Old page is in chained-from EBlock
          // Marked old page as chained
          UpdatePageTableInfo(devID, logicalEBNum, logicalPageOffset,
                              CHAIN_INVALID, BLOCK_INFO_STALE_PAGE);
          }
        else
          {
          // Old page is in chained-to EBlock
          // Mark old page as stale
          UpdatePageTableInfo(devID, chainLogEBNum, logicalPageOffset,
                              EMPTY_INVALID, BLOCK_INFO_STALE_PAGE);
          }
        }
      else
        {
        // New page is in chain-from block, therefore
        // Old page must also be in chained-from EBlock
        // Mark old page as stale
        UpdatePageTableInfo(devID, logicalEBNum, logicalPageOffset,
                            EMPTY_INVALID, BLOCK_INFO_STALE_PAGE);
        }
      }
    else
#endif  // #if(FTL_EBLOCK_CHAINING == true)

      {
      // Block has not been chained or Chaining is turned off
      // Mark old page as stale.
      UpdatePageTableInfo(devID, logicalEBNum, logicalPageOffset,
                          EMPTY_INVALID, BLOCK_INFO_STALE_PAGE);
      }
    }

#if(FTL_EBLOCK_CHAINING == true)
else
    {
    // Old page does not exist
    if(pageLocPtr->phyEBOffset & CHAIN_FLAG)
      {
      // New page is in chain-to block
      // Mark old location as chained
      UpdatePageTableInfo(devID, logicalEBNum, logicalPageOffset,
                          CHAIN_INVALID, BLOCK_INFO_STALE_PAGE);
      }
    }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  // .... Process New Page .....
  phyPageOffset = (uint16_t) (pageLocPtr->phyEBOffset & PPA_MASK);

#if(FTL_EBLOCK_CHAINING == true)
  if(EMPTY_WORD != chainLogEBNum)
    {
    // Block has been chained
    if(pageLocPtr->phyEBOffset & CHAIN_FLAG)
      {
      // New page is in chain-to block      
      UpdatePageTableInfo(devID, chainLogEBNum, logicalPageOffset,
                          phyPageOffset, BLOCK_INFO_VALID_PAGE);

#if (ENABLE_EB_ERASED_BIT == true)
      SetEBErased(devID, chainLogEBNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

      }
    else
      {
      // New page is in chain-from block
      UpdatePageTableInfo(devID, logicalEBNum, logicalPageOffset,
                          phyPageOffset, BLOCK_INFO_VALID_PAGE);

#if (ENABLE_EB_ERASED_BIT == true)
      SetEBErased(devID, logicalEBNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

      }
    }
  else
#endif  // #if(FTL_EBLOCK_CHAINING == true)

    {
    // Block has not been chained or chaining is turned off
    UpdatePageTableInfo(devID, logicalEBNum, logicalPageOffset,
                        phyPageOffset, BLOCK_INFO_VALID_PAGE);

#if (ENABLE_EB_ERASED_BIT == true)
    SetEBErased(devID, logicalEBNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

    }
  return FTL_ERR_PASS;
  }

//----------------------------------

FTL_STATUS UpdateRAMTablesUsingTransLogs(FTL_DEV devID)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint8_t entryBCnt = 0x0; /*1*/
  uint8_t pageLocEntryCnt = 0x0; /*1*/
  uint32_t pageAddress = 0x0; /*4*/

  LastTransLogLba = TransLogEntry.entryA.LBA;
  LastTransLogNpages = 0;
  if((status = GetPageNum(TransLogEntry.entryA.LBA, &pageAddress)) != FTL_ERR_PASS)
    {
    return status;
    }
  // Trans Entry A update...
  for(pageLocEntryCnt = 0; pageLocEntryCnt < NUM_ENTRIES_TYPE_A; pageLocEntryCnt++)
    {
    if(TransLogEntry.entryA.pageLoc[pageLocEntryCnt].logEBNum == EMPTY_WORD)
      {
      break; // No more Type A entries
      }
    status = ProcessPageLoc(devID, &TransLogEntry.entryA.pageLoc[pageLocEntryCnt],
                            pageAddress);
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
    LastTransLogNpages++;
    pageAddress++;
    }
  // Trans Entry B update...
  for(entryBCnt = 0; entryBCnt < TranslogBEntries; entryBCnt++)
    {
    for(pageLocEntryCnt = 0; pageLocEntryCnt < NUM_ENTRIES_TYPE_B; pageLocEntryCnt++)
      {
      if(TransLogEntry.entryB[entryBCnt].pageLoc[pageLocEntryCnt].logEBNum == EMPTY_WORD)
        {
        break; // because writting EntryB is optional..
        }
      status = ProcessPageLoc(devID, &TransLogEntry.entryB[entryBCnt].pageLoc[pageLocEntryCnt],
                              pageAddress);
      if(status != FTL_ERR_PASS)
        {
        return status;
        }
      LastTransLogNpages++;
      pageAddress++;
      }
    }
  return FTL_ERR_PASS;
  }

//-----------------------------

FTL_STATUS ProcessGCPageBitMap(FTL_DEV devID, uint16_t logicalEBNum, uint8_t * pageBitMap)
  {
  uint8_t bitMapData = 0x0; /*1*/
  uint16_t logicalPageOffset = EMPTY_WORD; /*2*/
  uint16_t phyPageOffset = EMPTY_WORD; /*2*/
  FREE_BIT_MAP_TYPE bitMap = 0; /*1*/

#if(FTL_EBLOCK_CHAINING == true)
  uint16_t chainEBNum = EMPTY_WORD; /*2*/
  uint16_t chainFreePageIndex = EMPTY_WORD; /*2*/
#endif  // #if(FTL_EBLOCK_CHAINING == true)

#if(FTL_EBLOCK_CHAINING == true)
  chainEBNum = GetChainLogicalEBNum(devID, logicalEBNum);
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  // Mark erased pages
  for(logicalPageOffset = 0; logicalPageOffset < NUM_PAGES_PER_EBLOCK; logicalPageOffset++)
    {
    bitMapData = GetBitMapField(&pageBitMap[0], logicalPageOffset, 1);
    if(bitMapData != GC_MOVED_PAGE)
      {
      //  Mark the page empty using the logical address
      //  The bit map includes the pages copied from either EBlock
      UpdatePageTableInfo(devID, logicalEBNum, logicalPageOffset,
                          EMPTY_INVALID, BLOCK_INFO_EMPTY_PAGE);
      // Don't clear the corresponding page in the chained-to EBlock
      }
    }

#if(FTL_EBLOCK_CHAINING == true)
  // clear the chain info
  if(chainEBNum != EMPTY_WORD)
    {
    ClearChainLink(devID, logicalEBNum, chainEBNum);
    }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  // Mark all stale pages as free
  for(phyPageOffset = 0; phyPageOffset < NUM_PAGES_PER_EBLOCK; phyPageOffset++)
    {
    bitMap = GetEBlockMapFreeBitIndex(devID, logicalEBNum, phyPageOffset);
    if(BLOCK_INFO_STALE_PAGE == bitMap)
      {
      // Note: PPA table should not include this page
      SetEBlockMapFreeBitIndex(devID, logicalEBNum, phyPageOffset, BLOCK_INFO_EMPTY_PAGE);
      }
    }

#if(FTL_EBLOCK_CHAINING == true)
  if(chainEBNum != EMPTY_WORD)
    {
    // Validate pages copied from the chained-to EBlock
    for(logicalPageOffset = 0; logicalPageOffset < NUM_PAGES_PER_EBLOCK; logicalPageOffset++)
      {
      bitMapData = GetBitMapField(&pageBitMap[0], logicalPageOffset, 1);
      if(bitMapData == GC_MOVED_PAGE)
        {
        // Page was copied
        if(EMPTY_INVALID != GetPPASlot(devID, chainEBNum, logicalPageOffset))
          {
          // Page came from chained-to EBlock
          // Find first free page
          chainFreePageIndex = GetFreePageIndex(devID, logicalEBNum);
          UpdatePageTableInfo(devID, logicalEBNum, logicalPageOffset,
                              chainFreePageIndex, BLOCK_INFO_VALID_PAGE);
          // Mark old page stale
          UpdatePageTableInfo(devID, chainEBNum, logicalPageOffset,
                              EMPTY_INVALID, BLOCK_INFO_STALE_PAGE);
          }
        }
      }
    }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  // Update dirty bit in Block_Info table
  MarkEBlockMappingTableEntryDirty(devID, logicalEBNum);
  return FTL_ERR_PASS;
  }

//-----------------------------

FTL_STATUS UpdateRAMTablesUsingGCLogs(FTL_DEV devID, GC_LOG_ENTRY_PTR ptrGCLog)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/
  uint16_t reservedEBNum = EMPTY_WORD; /*2*/
  uint16_t phyFromEBlock = EMPTY_WORD; /*2*/
  uint16_t phyToEBlock = EMPTY_WORD; /*2*/
  uint32_t oldToEraseCount = EMPTY_DWORD; /*2*/
  uint32_t oldFromEraseCount = EMPTY_DWORD; /*2*/
  uint8_t count = EMPTY_BYTE; /*1*/
  uint8_t pageBitMap[GC_MOVE_BITMAP];

#if(FTL_EBLOCK_CHAINING == true)
  uint16_t chainEBNum = EMPTY_WORD; /*2*/
#endif  // #if(FTL_EBLOCK_CHAINING == true)
#if (FTL_DEFECT_MANAGEMENT == true)
  uint8_t badEBlockFlag = false;
#endif
  logicalEBNum = ptrGCLog->partA.logicalEBAddr;
  reservedEBNum = ptrGCLog->partA.reservedEBAddr;
  phyFromEBlock = GetPhysicalEBlockAddr(devID, logicalEBNum);
  phyToEBlock = GetPhysicalEBlockAddr(devID, reservedEBNum);

#if(FTL_EBLOCK_CHAINING == true)
  chainEBNum = GetChainLogicalEBNum(devID, logicalEBNum);
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  for(count = 0; count < NUM_GC_TYPE_B; count++)
    {
    memcpy(&pageBitMap[count * NUM_ENTRIES_GC_TYPE_B], \
                  &ptrGCLog->partB[count].pageMovedBitMap[0], \
                  NUM_ENTRIES_GC_TYPE_B);
    }

  // Swap EBlocks
  oldToEraseCount = GetEraseCount(devID, reservedEBNum);
  oldFromEraseCount = GetEraseCount(devID, logicalEBNum);
  SetPhysicalEBlockAddr(devID, reservedEBNum, phyFromEBlock);
  SetPhysicalEBlockAddr(devID, logicalEBNum, phyToEBlock);
  SetEraseCount(devID, reservedEBNum, oldFromEraseCount);
  SetEraseCount(devID, logicalEBNum, oldToEraseCount);
#if (CACHE_RAM_BD_MODULE == true)
#if (FTL_STATIC_WEAR_LEVELING == true)
  oldFromEraseCount = GetTrueEraseCount(devID, reservedEBNum);
  status = SetSaveStaticWL(devID, reservedEBNum, oldFromEraseCount);
  if(status != FTL_ERR_PASS)
    {
    return status;
    }
  oldToEraseCount = GetTrueEraseCount(devID, logicalEBNum);
  status = SetSaveStaticWL(devID, logicalEBNum, oldToEraseCount);
  if(status != FTL_ERR_PASS)
    {
    return status;
    }
#endif
#endif
#if (FTL_DEFECT_MANAGEMENT == true)
  badEBlockFlag = GetBadEBlockStatus(devID, reservedEBNum);
  SetBadEBlockStatus(devID, reservedEBNum, GetBadEBlockStatus(devID, logicalEBNum));
  SetBadEBlockStatus(devID, logicalEBNum, badEBlockFlag);
#endif    
#if (ENABLE_EB_ERASED_BIT == true)
  if(GetEBErased(devID, logicalEBNum) == false)
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

    {
    IncEraseCount(devID, logicalEBNum);
    }

#if (ENABLE_EB_ERASED_BIT == true)
  SetEBErased(devID, logicalEBNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

  MarkEBlockMappingTableEntryDirty(devID, logicalEBNum);
  MarkEBlockMappingTableEntryDirty(devID, reservedEBNum);

  if((status = ProcessGCPageBitMap(devID, logicalEBNum, pageBitMap)) != FTL_ERR_PASS)
    {
    return status;
    }

  // Change stale pages to empty. 
  // Note: Chained pages are stale in the chained-from EBlock
  SetGCOrFreePageNum(devID, logicalEBNum, ptrGCLog->partA.GCNum);
  SetDirtyCount(devID, logicalEBNum, 0);
  MarkEBlockMappingTableEntryDirty(devID, logicalEBNum);
  return FTL_ERR_PASS;
  }

#if(FTL_EBLOCK_CHAINING == true)
//-----------------------------

FTL_STATUS UpdateRAMTablesUsingChainLogs(FTL_DEV devID, CHAIN_LOG_ENTRY_PTR chainLogPtr)
  {
  uint16_t logicalToEBNum = EMPTY_WORD; /*2*/
  uint16_t logicalFromEBNum = EMPTY_WORD; /*2*/

  logicalToEBNum = chainLogPtr->logicalTo;
  logicalFromEBNum = chainLogPtr->logicalFrom;

#if (ENABLE_EB_ERASED_BIT == true)
  if(GetEBErased(devID, logicalToEBNum) == false)
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

    {
    IncEraseCount(devID, logicalToEBNum);
    }
  // Clear the bit map and ppa tables of the chained-to EBlock
  SetDirtyCount(devID, logicalToEBNum, 0);
  TABLE_ClearFreeBitMap(devID, logicalToEBNum);
  TABLE_ClearPPATable(devID, logicalToEBNum);
  MarkPPAMappingTableEntryDirty(devID, logicalToEBNum, 0);

#if (ENABLE_EB_ERASED_BIT == true)
  SetEBErased(devID, logicalToEBNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

  SetChainLink(devID, logicalFromEBNum, logicalToEBNum,
               chainLogPtr->phyFrom, chainLogPtr->phyTo);
  MarkEBlockMappingTableEntryDirty(devID, logicalFromEBNum);
  MarkEBlockMappingTableEntryDirty(devID, logicalToEBNum);
  return FTL_ERR_PASS;
  }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

#if (FTL_ENABLE_UNUSED_EB_SWAP == true)
//-----------------------------

FTL_STATUS UpdateRAMTablesUsingEBSwapLogs(FTL_DEV devID, EBSWAP_LOG_ENTRY_PTR EBSwapLogPtr)
  {
  SwapUnusedEBlock(devID, EBSwapLogPtr->logicalDataEB, EBSwapLogPtr->logicalReservedEB);
  return FTL_ERR_PASS;
  }

//--------------------------------------

FTL_STATUS CreateSwapEBLog(FTL_DEV devID, uint16_t logicalDataEB, uint16_t logicalReservedEB)
  {
  EBSWAP_LOG_ENTRY ebSwapLog; /*16*/
  FLASH_PAGE_INFO flashPageInfo = {0, 0,
    {0, 0}}; /*11*/
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint8_t count = 0; /*1*/

  if((status = GetNextLogEntryLocation(devID, &flashPageInfo)) != FTL_ERR_PASS)
    {
    return status;
    }
  for(count = 0; count < EBSWAP_LOG_ENTRY_RESERVED; count++)
    {
    ebSwapLog.reserved[count] = EMPTY_BYTE;
    }
  ebSwapLog.type = EBSWAP_LOG_TYPE;
  ebSwapLog.logicalDataEB = logicalDataEB;
  ebSwapLog.logicalReservedEB = logicalReservedEB;
  if((status = FTL_WriteLogInfo(&flashPageInfo, (uint8_t *) & ebSwapLog)) != FTL_ERR_PASS)
    {
    return status;
    }
  return FTL_ERR_PASS;
  }
#endif  // #if (FTL_ENABLE_UNUSED_EB_SWAP == true)


#if (FTL_SUPER_SYS_EBLOCK == true)

FTL_STATUS FTL_FindSuperSysEB(FTL_DEV devID)
  {

  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FLASH_STATUS flash_status = FLASH_PASS; /*4*/
  uint16_t eBlockCount = 0; /*2*/
  FLASH_PAGE_INFO flashPage = {0, 0,
    {0, 0}}; /*11*/
  uint16_t phyEBlockAddr = EMPTY_WORD; /*2*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  uint32_t key = EMPTY_DWORD; /*4*/
  uint16_t countEB = EMPTY_WORD; /*2*/
  uint16_t phyEBlockAddrOld = EMPTY_WORD; /*2*/
  SYS_EBLOCK_INFO_PTR sysTempPtr = NULL; /*4*/
  uint32_t prevKey = EMPTY_DWORD; /*4*/
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/
  uint16_t phyEBlockAddrTmp = EMPTY_WORD; /*2*/
  uint16_t logicalEBNumTmp = EMPTY_WORD; /*2*/
  uint8_t swap = false; /*1*/
  uint16_t logEBlockAddrOld = EMPTY_WORD; /*2*/

  flashPage.devID = devID;
  for(eBlockCount = SUPER_SYS_START_EBLOCKS; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
    {
    phyEBlockAddr = GetPhysicalEBlockAddr(devID, eBlockCount);
    flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddr, 0);
    flashPage.vPage.pageOffset = 0;

    flashPage.byteCount = sizeof (sysEBlockInfo);
    flash_status = FLASH_RamPageReadMetaData(&flashPage, (uint8_t *) (&sysEBlockInfo));

    if(flash_status != FLASH_PASS)
      {
      return FTL_ERR_SUPER_READ_01;
      }
    if((false == VerifyCheckWord((uint16_t *) & sysEBlockInfo.type, SYS_INFO_DATA_WORDS, sysEBlockInfo.checkWord)) &&
       (sysEBlockInfo.oldSysBlock != OLD_SYS_BLOCK_SIGNATURE) && (sysEBlockInfo.phyAddrThisEBlock == phyEBlockAddr))
      {
      if(sysEBlockInfo.type == SYS_EBLOCK_INFO_SUPER)
        {
        // Insert the entry, if fails check if this has higher incNum and insert that instead
        if((status = TABLE_SuperSysEBInsert(devID, eBlockCount, phyEBlockAddr, sysEBlockInfo.incNumber)) == FTL_ERR_FLUSH_NO_ENTRIES)
          {
          swap = false;
          for(countEB = 0; countEB < NUM_SUPER_SYS_EBLOCKS; countEB++)
            {
            if((status = TABLE_GetSuperSysEBEntry(devID, countEB, &logicalEBNumTmp, &phyEBlockAddrTmp, &key)) != FTL_ERR_PASS)
              {
              return status;
              }
            if(key < sysEBlockInfo.incNumber)
              {
              swap = true;
              }
            }
          if(swap == true)
            {
            // This will clear the smallest entry
            if((status = TABLE_SuperSysEBGetNext(devID, &logEBlockAddrOld, &phyEBlockAddrOld, NULL)) != FTL_ERR_PASS)
              {
              return status;
              }
            // Space is now avaiable, Insert the higher entry
            if((status = TABLE_SuperSysEBInsert(devID, eBlockCount, phyEBlockAddr, sysEBlockInfo.incNumber)) != FTL_ERR_PASS)
              {
              return status;
              }
            flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddrOld, 0);
            }
          else
            {
            flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddr, 0);
            }
          // Mark old flush log eblock
          sysEBlockInfo.oldSysBlock = OLD_SYS_BLOCK_SIGNATURE;
          flashPage.devID = devID;
          flashPage.vPage.pageOffset = (uint16_t) ((uint32_t) (&sysTempPtr->oldSysBlock));
          flashPage.byteCount = OLD_SYS_BLOCK_SIGNATURE_SIZE;
          //              flashPage.byteCount = sizeof(sysEBlockInfo.oldSysBlock);
          if((FLASH_RamPageWriteMetaData(&flashPage, (uint8_t *) & sysEBlockInfo.oldSysBlock)) != FLASH_PASS)
            {

#if(FTL_DEFECT_MANAGEMENT == true)
            SetBadEBlockStatus(devID, logEBlockAddrOld, true);
            // just try to mark bad, even if it fails we move on.
            FLASH_MarkDefectEBlock(&flashPage);

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
            return FTL_ERR_SUPER_WRITE_04;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

            }
          }
        else if(status != FTL_ERR_PASS)
          {
          return status;
          }
        if(sysEBlockInfo.incNumber > GetSuperSysEBCounter(devID))
          {
          SetSuperSysEBCounter(devID, sysEBlockInfo.incNumber);
          }
        continue;
        }
      }
    }
  /* check sanity */
  for(eBlockCount = 0; eBlockCount < NUM_SUPER_SYS_EBLOCKS; eBlockCount++)
    {
    if((status = TABLE_GetSuperSysEBEntry(devID, eBlockCount, &logicalEBNum, &phyEBlockAddr, &key)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(key == EMPTY_DWORD)
      {
      break;
      }
    if(eBlockCount > 0)
      {
      if(key > (prevKey + 1))
        {
        if((status = TABLE_SuperSysEBRemove(devID, eBlockCount)) != FTL_ERR_PASS)
          {
          return status;
          }
        SetSuperSysEBCounter(devID, prevKey);
        }
      }
    prevKey = key;
    }
  return FTL_ERR_PASS;
  }


//-------------------------------

FTL_STATUS GetSuperSysInfoLogs(FTL_DEV devID, uint16_t * storePhySysEB, uint8_t * checkSuperPF)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FLASH_STATUS flash_status = FLASH_PASS; /*4*/
  uint16_t entryIndex = 0x0; /*2*/
  uint16_t byteOffset = 0x0; /*2*/
  uint16_t logicalAddr = 0x0; /*2*/
  uint16_t phyEBAddr = 0x0; /*2*/
  uint16_t eBlockNum = 0x0; /*2*/
  uint16_t logEntry[LOG_ENTRY_SIZE / 2]; /*16*/
  uint16_t logType = EMPTY_WORD; /*2*/
  uint32_t latestIncNumber = 0x0; /*4*/
  uint16_t left = 0x0; /*2*/
  uint16_t right = 0x0; /*2*/
  uint16_t mid = 0x0; /*2*/
  uint16_t tempEntryIndex = 0x0; /*2*/

#if DEBUG_EARLY_BAD_ENTRY
  uint16_t badEntry = false; /*2*/
#endif  // #if DEBUG_EARLY_BAD_ENTRY

  FLASH_PAGE_INFO flashPageInfo = {0, 0,
    {0, 0}}; /*11*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  SUPER_SYS_INFO superSysInfo; /*16*/

  uint16_t storeLog[MAX_NUM_SYS_EBLOCKS];
  uint16_t count = 0; /*2*/
  uint16_t tempCount = 0; /*2*/
  uint8_t endflag = false; /*1*/
  uint8_t checkFlag = false;
  SUPER_SYS_INFO_PTR getLogType; /*16*/

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  uint16_t packedCount = 0; /*2*/
  uint8_t firstFlag = false; /*1*/
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

  for(count = 0; count < MAX_NUM_SYS_EBLOCKS; count++)
    {
    storeLog[count] = EMPTY_WORD;
    }
  count = 0;

  for(eBlockNum = 0; eBlockNum < NUM_SUPER_SYS_EBLOCKS; eBlockNum++)
    {
    (*checkSuperPF) = false;
    endflag = false;
    if((status = TABLE_GetSuperSysEBEntry(devID, eBlockNum, &logicalAddr, &phyEBAddr, &latestIncNumber)) != FTL_ERR_PASS)
      {
      return status; // trying to excess outside table.
      }
    if((logicalAddr == EMPTY_WORD) && (phyEBAddr == EMPTY_WORD) && (latestIncNumber == EMPTY_DWORD))
      {
      break; // no more entries in table
      }

    flashPageInfo.devID = devID;
    flashPageInfo.vPage.pageOffset = 0;
    flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBAddr, 0);
    flashPageInfo.byteCount = sizeof (SYS_EBLOCK_INFO);
    flash_status = FLASH_RamPageReadMetaData(&flashPageInfo, (uint8_t *) (&sysEBlockInfo));

    if(flash_status != FLASH_PASS)
      {
      return FTL_ERR_FLASH_READ_05;
      }
    // Only if this valid Trans block
    if((false == VerifyCheckWord((uint16_t *) & sysEBlockInfo.type,
                                 SYS_INFO_DATA_WORDS, sysEBlockInfo.checkWord)) &&
       (sysEBlockInfo.oldSysBlock != OLD_SYS_BLOCK_SIGNATURE) &&
       (sysEBlockInfo.type == SYS_EBLOCK_INFO_SUPER))
      {

#if DEBUG_EARLY_BAD_ENTRY
      badEntry = false;
#endif  // #if DEBUG_EARLY_BAD_ENTRY

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
      (*checkSuperPF) = true;
#endif

      // Binary Search
      right = NUM_LOG_ENTRIES_PER_EBLOCK;
      left = 1;
      tempEntryIndex = 1;
      while(left <= right)
        {
        checkFlag = false;
        tempEntryIndex = (left + right) / 2; /* calc of middle key */
        mid = tempEntryIndex;
        byteOffset = tempEntryIndex * LOG_ENTRY_DELTA;
        flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBAddr, tempEntryIndex);
        flashPageInfo.vPage.pageOffset = byteOffset % VIRTUAL_PAGE_SIZE;
        flashPageInfo.byteCount = sizeof (logEntry);
        flash_status = FLASH_RamPageReadMetaData(&flashPageInfo, (uint8_t *) & logEntry[0]);

        if(flash_status != FLASH_PASS)
          {
          return FTL_ERR_SUPER_READ_04;
          }
        if(FLASH_CheckEmpty((uint8_t *) & logEntry[0], LOG_ENTRY_SIZE) != FLASH_PASS)
          {
          left = mid + 1; /* adjustment of left(start) key */
          }
        else
          {
          right = mid - 1; /* adjustment of right(end) key */
          }

        }

      if(0 == tempEntryIndex)
        {
        return FTL_ERR_FAIL;
        }

      memcpy((uint8_t *) & superSysInfo, (uint8_t *) & logEntry[0], sizeof (logEntry));

      if(SYS_EBLOCK_INFO_CHANGED == superSysInfo.type)
        {
        entryIndex = tempEntryIndex;
        }
      else
        {
        do
          {
          if(1 == tempEntryIndex)
            break;

          tempEntryIndex--;
          byteOffset = tempEntryIndex * LOG_ENTRY_DELTA;
          flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBAddr, tempEntryIndex);
          flashPageInfo.vPage.pageOffset = byteOffset % VIRTUAL_PAGE_SIZE;
          flashPageInfo.byteCount = sizeof (logEntry);
          flash_status = FLASH_RamPageReadMetaData(&flashPageInfo, (uint8_t *) & logEntry[0]);
          memcpy((uint8_t *) & superSysInfo, (uint8_t *) & logEntry[0], sizeof (logEntry));

          }
        while(SYS_EBLOCK_INFO_CHANGED != superSysInfo.type);

        entryIndex = tempEntryIndex;
        }

      // go through all the entries in the block
      for(entryIndex = tempEntryIndex; entryIndex < NUM_LOG_ENTRIES_PER_EBLOCK; entryIndex++)
        {

        byteOffset = entryIndex * LOG_ENTRY_DELTA;
        flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBAddr, entryIndex);
        flashPageInfo.vPage.pageOffset = byteOffset % VIRTUAL_PAGE_SIZE;
        flashPageInfo.byteCount = sizeof (logEntry);
        flash_status = FLASH_RamPageReadMetaData(&flashPageInfo, (uint8_t *) & logEntry[0]);

        if(flash_status != FLASH_PASS)
          {
          return FTL_ERR_SUPER_READ_05;
          }

#if DEBUG_EARLY_BAD_ENTRY
          if(true == badEntry)
            {
            if(EMPTY_WORD != logEntry[LOG_ENTRY_DATA_START])
              {
              DBG_Printf("GetSuperSysInfoLogs: Error: Bad Log Entry\n", 0, 0);
              DBG_Printf("   detected before end of Log, entryIndex = %d\n", entryIndex - 1, 0);
              }
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
            else
              {
              (*checkSuperPF) = false;
              }
#endif
            break;
            }
#endif  // #if DEBUG_EARLY_BAD_ENTRY

          if(VerifyCheckWord(&logEntry[LOG_ENTRY_DATA_START],
                             LOG_ENTRY_DATA_WORDS, logEntry[LOG_ENTRY_CHECK_WORD]))
            {
#if(FTL_REDUNDANT_LOG_ENTRIES == true)
            // Primary Copy is bad; Check Redundant Copy
            flashPageInfo.vPage.pageOffset += LOG_ENTRY_SIZE;
            if((FLASH_RamPageReadMetaData(&flashPageInfo, (uint8_t *) & logEntry[0])) != FLASH_PASS)
              {
              return FTL_ERR_SUPER_READ_02;
              }
            flashPageInfo.vPage.pageOffset -= LOG_ENTRY_SIZE;
            if(VerifyCheckWord(&logEntry[LOG_ENTRY_DATA_START],
                               LOG_ENTRY_DATA_WORDS, logEntry[LOG_ENTRY_CHECK_WORD]))
#endif  // #if(FTL_REDUNDANT_LOG_ENTRIES == true)

              {
              // Log Entry is bad

#if DEBUG_EARLY_BAD_ENTRY     
              badEntry = true;
              // Get one more entry
              continue;

#else  // #if DEBUG_EARLY_BAD_ENTRY
              // Don't check any more entries
              break;
#endif  // #else  // #if DEBUG_EARLY_BAD_ENTRY

              }

#if(FTL_REDUNDANT_LOG_ENTRIES == true)

#if DEBUG_REDUNDANT_MESG
            else
              {
              DBG_Printf("GetSuperSysInfoLogs: Use Redundant Log Entry: %d\n", entryIndex, 0);
              }
#endif  // #if DEBUG_REDUNDANT_MESG

#endif  // #if(FTL_REDUNDANT_LOG_ENTRIES == true)

            }
          getLogType = (SUPER_SYS_INFO_PTR) logEntry;
          logType = getLogType->type; // get LogType
          if((uint8_t) logType == EMPTY_BYTE)
            {
            (*checkSuperPF) = false;
            break;
            }

          switch(logType)
            {
              // ........SuperSysInfo...........
            case SYS_EBLOCK_INFO_SYSEB:
            {
              endflag = false;

              memcpy((uint8_t *) & superSysInfo, (uint8_t *) & logEntry[0], sizeof (logEntry));
              if(entryIndex == superSysInfo.EntryNumThisIndex)
                {
                for(tempCount = 0; tempCount < NUM_SYS_EB_ENTRY; tempCount++)
                  {
                  if(EMPTY_WORD != superSysInfo.PhyEBNum[tempCount])
                    {
                    storeLog[count] = superSysInfo.PhyEBNum[tempCount];
                    count++;
                    }
                  }
                }
              if(count > MAX_NUM_SYS_EBLOCKS)
                {
                return FTL_ERR_FAIL;
                }
              if(0 == superSysInfo.decNumber)
                {
                SuperEBInfo[devID].storeFreePage[eBlockNum] = entryIndex;
                endflag = true;
                }

              break;

            }
            case SYS_EBLOCK_INFO_CHANGED:
            {
              memcpy((uint8_t *) & superSysInfo, (uint8_t *) & logEntry[0], sizeof (logEntry));
              if(entryIndex == superSysInfo.EntryNumThisIndex)
                {
                for(tempCount = 0; tempCount < NUM_SYS_EB_ENTRY; tempCount++)
                  {
                  if(EMPTY_WORD != superSysInfo.PhyEBNum[tempCount])
                    {
                    break;
                    }
                  }
                if(NUM_SYS_EB_ENTRY == tempCount)
                  {
                  for(tempCount = 0; tempCount < MAX_NUM_SYS_EBLOCKS; tempCount++)
                    {
                    storeLog[tempCount] = EMPTY_WORD;
                    }
                  count = 0;
                  endflag = false;
                  }
                }
              break;

            }

            default:
            {
              break;
            }
            } // switch
        } // entryIndex loop
      } // valid trans block check
    } // eBlockNum loop

  if(false == endflag)
    {
    (*checkSuperPF) = true;
    return FTL_ERR_FAIL;
    }

  for(tempCount = 0; tempCount < MAX_NUM_SYS_EBLOCKS; tempCount++)
    {
    storePhySysEB[tempCount] = storeLog[tempCount];
    }

  return FTL_ERR_PASS;
  }

FTL_STATUS SetSysEBRamTable(FTL_DEV devID, uint16_t * storeSysEB, uint16_t * formatCount)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FLASH_STATUS flash_status = FLASH_PASS; /*4*/
  uint16_t eBlockCount = 0; /*2*/
  FLASH_PAGE_INFO flashPage = {0, 0,
    {0, 0}}; /*11*/
  uint16_t phyEBlockAddr = EMPTY_WORD; /*2*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  uint32_t key = EMPTY_DWORD; /*4*/
  uint32_t prevKey = EMPTY_DWORD; /*4*/
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/

  flashPage.devID = devID;
  for(eBlockCount = 0; eBlockCount < MAX_NUM_SYS_EBLOCKS; eBlockCount++)
    {
    if(EMPTY_WORD == storeSysEB[eBlockCount])
      {
      break;
      }
    phyEBlockAddr = storeSysEB[eBlockCount];
    flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddr, 0);
    flashPage.vPage.pageOffset = 0;
    flashPage.byteCount = sizeof (sysEBlockInfo);
    flash_status = FLASH_RamPageReadMetaData(&flashPage, (uint8_t *) (&sysEBlockInfo));

    if(flash_status != FLASH_PASS)
      {
      return FTL_ERR_SUPER_READ_03;
      }
    if((false == VerifyCheckWord((uint16_t *) & sysEBlockInfo.type, SYS_INFO_DATA_WORDS, sysEBlockInfo.checkWord)) &&
       (sysEBlockInfo.oldSysBlock != OLD_SYS_BLOCK_SIGNATURE) && (sysEBlockInfo.phyAddrThisEBlock == phyEBlockAddr))
      {
      if(sysEBlockInfo.type == SYS_EBLOCK_INFO_FLUSH)
        {
        // Insert the entry, if fails check if this has higher incNum and insert that instead
        if((status = TABLE_FlushEBInsert(devID, eBlockCount, phyEBlockAddr, sysEBlockInfo.incNumber)) == FTL_ERR_FLUSH_NO_ENTRIES)
          {
          return FTL_ERR_SUPER_FLUSH_NO_ROOM;
          }
        else if(status != FTL_ERR_PASS)
          {
          return status;
          }
        if(sysEBlockInfo.incNumber > GetFlushEBCounter(devID))
          {
          SetFlushLogEBCounter(devID, sysEBlockInfo.incNumber);
          }
        (*formatCount)++;
        continue;
        }
      if(sysEBlockInfo.type == SYS_EBLOCK_INFO_LOG)
        {
        if((status = TABLE_TransLogEBInsert(devID, eBlockCount, phyEBlockAddr, sysEBlockInfo.incNumber)) == FTL_ERR_LOG_INSERT)
          {
          return FTL_ERR_SUPER_LOG_NO_ROOM_01;
          }
        else if(status != FTL_ERR_PASS)
          {
          return status;
          }
        if(sysEBlockInfo.incNumber > GetTransLogEBCounter(devID))
          {
          SetTransLogEBCounter(devID, sysEBlockInfo.incNumber);
          }
        (*formatCount)++;
        continue;
        }
      }
    }
  /* check sanity */
  for(eBlockCount = 0; eBlockCount < NUM_FLUSH_LOG_EBLOCKS; eBlockCount++)
    {
    if((status = TABLE_GetFlushLogEntry(devID, eBlockCount, &logicalEBNum, &phyEBlockAddr, &key)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(key == EMPTY_DWORD)
      {
      break;
      }
    if(eBlockCount > 0)
      {
      if(key > (prevKey + 1))
        {
        if((status = TABLE_FlushEBRemove(devID, eBlockCount)) != FTL_ERR_PASS)
          {
          return status;
          }
        SetFlushLogEBCounter(devID, prevKey);
        }
      }
    prevKey = key;
    }
  return FTL_ERR_PASS;
  }

FTL_STATUS FTL_CheckForSuperSysEBLogSpace(FTL_DEV devID, uint8_t mode)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t latestIncNumber = EMPTY_DWORD; /*4*/
  uint16_t logLogicalEBNum = EMPTY_WORD; /*2*/
  uint16_t logPhyEBAddr = EMPTY_WORD; /*2*/
  uint16_t entryIndex = 0; /*2*/
  uint16_t numLogEntries = 0; /*2*/
  uint16_t total = 0; /*2*/
  uint16_t odd = 0; /*2*/
  uint16_t ebPhyAddr[MAX_NUM_SYS_EBLOCKS];
  uint16_t ebCount = 0; /*2*/

#if(FTL_DEFECT_MANAGEMENT == true)
  uint16_t sanityCounter = 0; /*2*/
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

  if(SYS_EBLOCK_INFO_CHANGED == mode)
    {
    if(true == SuperEBInfo[devID].checkChanged)
      {
      return FTL_ERR_PASS; // If changed, skip operation.
      }
    }
  else if(SYS_EBLOCK_INFO_SYSEB == mode)
    {
    if(false == SuperEBInfo[devID].checkChanged)
      {
      return FTL_ERR_PASS; // If not changed, skip operation.
      }
    }
  else
    {
    //DBG_Printf("FTL_CheckForSuperSysEBLogSpace: The mode is a paramter error\n", 0, 0);
    return FTL_ERR_SUPER_PARAM_03;
    }


  latestIncNumber = GetSuperSysEBCounter(devID);
  status = TABLE_SuperSysEBGetLatest(devID, &logLogicalEBNum, &logPhyEBAddr, latestIncNumber);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }

  // Get free index
  entryIndex = GetFreePageIndex(devID, logLogicalEBNum);

  if(SYS_EBLOCK_INFO_CHANGED == mode)
    {
    numLogEntries = 1;
    }
  else if(SYS_EBLOCK_INFO_SYSEB == mode)
    {

    // Get Physical Address of SystemEB/
    TABLE_GetPhySysEB(devID, &ebCount, ebPhyAddr);
    total = ebCount / NUM_SYS_EB_ENTRY;
    odd = ebCount % NUM_SYS_EB_ENTRY;
    if(0 < odd)
      {
      total++;
      }
    numLogEntries = total + 1; // add " + 1" for next SYS_EBLOCK_INFO_CHANGED
    }

#if(FTL_DEFECT_MANAGEMENT == true)
  if((entryIndex + numLogEntries) > NUM_LOG_ENTRIES_PER_EBLOCK || GetBadEBlockStatus(devID, logLogicalEBNum))
#else
  if((entryIndex + numLogEntries) > NUM_LOG_ENTRIES_PER_EBLOCK)
#endif
    {
#if(FTL_DEFECT_MANAGEMENT == true)
    while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
      {
      if((status = CreateNextSuperSystemEBlockOp(devID)) == FTL_ERR_PASS)
        {
        break;
        }
      if((FTL_ERR_SUPER_LOG_NEW_EBLOCK_FAIL_01 != status) && (FTL_ERR_SUPER_LOG_NEW_EBLOCK_FAIL_02 != status))
        {
        return status;
        }
      sanityCounter++;
      }
    if(sanityCounter >= MAX_BAD_BLOCK_SANITY_TRIES)
      {
      return status;
      }
#else
    if((status = CreateNextSuperSystemEBlockOp(devID)) != FTL_ERR_PASS)
      {
      return status;
      }
#endif
    }
  return status;
  }


//--------------------------------------

FTL_STATUS FTL_CreateSuperSysEBLog(FTL_DEV devID, uint8_t mode)
  {
  SUPER_SYS_INFO ebSuperLog; /*16*/
  FLASH_PAGE_INFO flashPageInfo = {0, 0,
    {0, 0}}; /*11*/
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t count = 0; /*2*/
  uint16_t entryCount = 0; /*2*/
  uint16_t ebCount = 0; /*2*/
  uint16_t ebPhyAddr[MAX_NUM_SYS_EBLOCKS];
  uint16_t total = 0; /*2*/
  uint16_t odd = 0; /*2*/
  uint16_t index = 0; /*2*/
  uint16_t entryIndex = 0; /*2*/

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  FTL_STATUS ram_status = FTL_ERR_PASS; /*4*/
  uint32_t latestIncNumber = EMPTY_DWORD; /*4*/
  uint16_t logLogicalEBNum = EMPTY_WORD; /*2*/
  uint16_t logPhyEBAddr = EMPTY_WORD; /*2*/
#endif

  if(SYS_EBLOCK_INFO_CHANGED == mode)
    {
    if(true == SuperEBInfo[devID].checkChanged)
      {
      return FTL_ERR_PASS; // If changed, skip operation.
      }
    }
  else if(SYS_EBLOCK_INFO_SYSEB == mode)
    {
    if(false == SuperEBInfo[devID].checkChanged)
      {
      return FTL_ERR_PASS; // If not changed, skip operation.
      }
    }
  else
    {
    return FTL_ERR_SUPER_PARAM_01;
    }

  if(SYS_EBLOCK_INFO_CHANGED == mode)
    {
    total = 1;
    }
  else if(SYS_EBLOCK_INFO_SYSEB == mode)
    {
    // Get Physical Address of SystemEB/
    TABLE_GetPhySysEB(devID, &ebCount, ebPhyAddr);
    total = ebCount / NUM_SYS_EB_ENTRY;
    odd = ebCount % NUM_SYS_EB_ENTRY;
    if(1 <= odd)
      {
      total++;
      }
    index = 0;
    }

  for(count = 0; count < total; count++)
    {
    if(SYS_EBLOCK_INFO_CHANGED == mode)
      {
      ebSuperLog.type = SYS_EBLOCK_INFO_CHANGED;
      ebSuperLog.decNumber = 0;
      for(entryCount = 0; entryCount < NUM_SYS_EB_ENTRY; entryCount++)
        {
        ebSuperLog.PhyEBNum[entryCount] = EMPTY_WORD;
        }

      }
    else if(SYS_EBLOCK_INFO_SYSEB == mode)
      {
      ebSuperLog.type = SYS_EBLOCK_INFO_SYSEB;
      ebSuperLog.decNumber = (uint8_t) ((total - count) - 1);
      for(entryCount = 0; entryCount < NUM_SYS_EB_ENTRY; entryCount++)
        {
        if(index < ebCount)
          {
          ebSuperLog.PhyEBNum[entryCount] = ebPhyAddr[index];
          index++;
          }
        else
          {
          ebSuperLog.PhyEBNum[entryCount] = EMPTY_WORD;
          }
        }
      }

    if((status = GetNextSuperSysEBEntryLocation(devID, &flashPageInfo, &entryIndex)) != FTL_ERR_PASS)
      {
      return status;
      }

    ebSuperLog.EntryNumThisIndex = entryIndex;
    if((status = FTL_WriteSuperInfo(&flashPageInfo, (uint8_t *) & ebSuperLog)) != FTL_ERR_PASS)
      {
      return status;
      }
    }

  SuperEBInfo[devID].checkChanged = true;

  return FTL_ERR_PASS;
  }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)


#if(FTL_UNLINK_GC == true)
//-----------------------------

FTL_STATUS ProcessUnlinkPageBitMap(FTL_DEV devID, uint16_t logFromEBNum, uint16_t logToEBNum, uint8_t * pageBitMap)
  {
  uint16_t pageOffset = 0; /*2*/
  uint16_t freePageIndex = 0; /*2*/
  FREE_BIT_MAP_TYPE bitMap = 0; /*1*/

  for(pageOffset = 0; pageOffset < NUM_PAGES_PER_EBLOCK; pageOffset++)
    {
    bitMap = GetBitMapField(&pageBitMap[0], pageOffset, 1);
    if(bitMap == GC_MOVED_PAGE)
      {
      freePageIndex = GetFreePageIndex(devID, logToEBNum);
      UpdatePageTableInfo(devID, logToEBNum, pageOffset, freePageIndex, BLOCK_INFO_VALID_PAGE);
      UpdatePageTableInfo(devID, logFromEBNum, pageOffset, EMPTY_INVALID, BLOCK_INFO_STALE_PAGE);
      }
    }
  return FTL_ERR_PASS;
  }

//-----------------------------

FTL_STATUS UpdateRAMTablesUsingUnlinkLogs(FTL_DEV devID, UNLINK_LOG_ENTRY_PTR ptrUnlinkLog)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t fromLogicalEBlock = EMPTY_WORD; /*2*/
  uint16_t toLogicalEBlock = EMPTY_WORD; /*2*/
  uint16_t count = 0; /*2*/
  uint8_t pageBitMap[GC_MOVE_BITMAP];

  fromLogicalEBlock = ptrUnlinkLog->partA.fromLogicalEBAddr;
  toLogicalEBlock = ptrUnlinkLog->partA.toLogicalEBAddr;

  if(ptrUnlinkLog->partA.type == UNLINK_LOG_TYPE_A2)
    {
    for(count = 0; count < NUM_UNLINK_TYPE_B; count++)
      {
      memcpy(&pageBitMap[count * NUM_ENTRIES_UNLINK_TYPE_B], &ptrUnlinkLog->partB[count].pageMovedBitMap[0], NUM_ENTRIES_UNLINK_TYPE_B);
      }

    if((status = ProcessUnlinkPageBitMap(devID, fromLogicalEBlock, toLogicalEBlock, &pageBitMap[0])) != FTL_ERR_PASS)
      {
      return status;
      }
    }

  if((status = UnlinkChain(devID, fromLogicalEBlock, toLogicalEBlock)) != FTL_ERR_PASS)
    {
    return status;
    }
  return FTL_ERR_PASS;
  }
#endif  // #if(FTL_UNLINK_GC == true)

//-----------------------------

FTL_STATUS LoadRamTable(FLASH_PAGE_INFO_PTR flashPage, uint8_t * ramTablePtr, uint16_t tableOffset, uint32_t devTableSize)
  {
  uint32_t dataBytes = 0; /*4*/
  uint8_t * destPtr = NULL; /*4*/

  dataBytes = devTableSize - (tableOffset * SECTOR_SIZE);
  // Aligned case
  if(dataBytes >= SECTOR_SIZE)
    {
    destPtr = (uint8_t *) ((uint8_t *) ramTablePtr + (tableOffset * SECTOR_SIZE));
    if((FLASH_RamPageReadDataBlock(flashPage, destPtr)) != FLASH_PASS)
      {
      return FTL_ERR_FLASH_READ_15;
      }
    }
  else // Not Aligned case
    {
    if((FLASH_RamPageReadDataBlock(flashPage, &pseudoRPB[flashPage->devID][0])) != FLASH_PASS)
      {
      return FTL_ERR_FLASH_READ_16;
      }
    destPtr = (uint8_t *) ((uint8_t *) ramTablePtr + (tableOffset * SECTOR_SIZE));
    memcpy(destPtr, &pseudoRPB[flashPage->devID][0], (uint16_t) dataBytes);
    }

  return FTL_ERR_PASS;
  }

//-----------------------

FTL_STATUS GetFlushLoc(FTL_DEV devID, uint16_t phyEBlockAddr, uint16_t freePageIndex, FLASH_PAGE_INFO_PTR flushInfoPtr, FLASH_PAGE_INFO_PTR flushRamTablePtr)
  {
  flushRamTablePtr->devID = devID;
  flushRamTablePtr->vPage.vPageAddr = CalcFlushRamTablePages(phyEBlockAddr, freePageIndex);
  flushRamTablePtr->vPage.pageOffset = CalcFlushRamTableOffset(freePageIndex);
  flushRamTablePtr->byteCount = SECTOR_SIZE;

#if (CACHE_RAM_BD_MODULE == true)
  flushRamTablePtr->byteCount = FLUSH_RAM_TABLE_SIZE;
#endif
  flushInfoPtr->devID = devID;
  flushInfoPtr->vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddr, freePageIndex);
  flushInfoPtr->vPage.pageOffset = (freePageIndex & ((VIRTUAL_PAGE_SIZE / BYTES_PER_CL) - 1)) * BYTES_PER_CL;
  flushInfoPtr->byteCount = sizeof (SYS_EBLOCK_FLUSH_INFO); // read/write word at a time.

  return FTL_ERR_PASS;
  }

//---------------------------

FTL_STATUS GetNextFlushEntryLocation(FTL_DEV devID, FLASH_PAGE_INFO_PTR flushInfoPtr, FLASH_PAGE_INFO_PTR flushRamTablePtr, uint16_t * logicalBlockNumPtr)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t phyEBlockAddr = EMPTY_WORD; /*2*/
  uint16_t entryIndex = EMPTY_WORD; /*2*/
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/
  uint32_t key = EMPTY_DWORD; /*4*/
  uint16_t eBlockCount = 0; /*2*/

  for(eBlockCount = 0; eBlockCount < NUM_FLUSH_LOG_EBLOCKS; eBlockCount++)
    {
    if((status = TABLE_GetFlushLogEntry(devID, eBlockCount, &logicalEBNum, &phyEBlockAddr, &key)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(key != EMPTY_DWORD)
      {
      entryIndex = GetFreePageIndex(devID, logicalEBNum);
      if(entryIndex < MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK)
        {
        break;
        }
      }
    }
  if(entryIndex >= MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK)
    {
    return FTL_ERR_FLUSH_NEXT_ENTRY;
    }
  *logicalBlockNumPtr = logicalEBNum;
  if((status = GetFlushLoc(devID, phyEBlockAddr, entryIndex, flushInfoPtr, flushRamTablePtr)) != FTL_ERR_PASS)
    {
    return status;
    }
  return FTL_ERR_PASS;
  }

//--------------------------

FTL_STATUS CreateNextFlushEntryLocation(FTL_DEV devID, uint16_t logicalBlockNum)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t phyEBlockAddr = 0x0; /*2*/
  uint16_t tempCount = 0x0; /*2*/
  uint32_t latestIncNumber = 0x0; /*4*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  FLASH_PAGE_INFO flushInfo = {0, 0,
    {0, 0}}; /*11*/

#if (ENABLE_EB_ERASED_BIT == true)
  uint8_t eraseStatus = false; /*1*/
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

#if (FTL_SUPER_SYS_EBLOCK == true)
  if((false == SuperEBInfo[devID].checkLost) && (false == SuperEBInfo[devID].checkSuperPF) && (false == SuperEBInfo[devID].checkSysPF))
    {
    if((status = FTL_CreateSuperSysEBLog(devID, SYS_EBLOCK_INFO_CHANGED)) != FTL_ERR_PASS)
      {
      return status;
      }
    }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if (ENABLE_EB_ERASED_BIT == true)
  eraseStatus = GetEBErased(devID, logicalBlockNum);

#if DEBUG_PRE_ERASED
  if(true == eraseStatus)
    {
    DBG_Printf("CreateNextFlushEntryLocation: EBlock 0x%X is already erased\n", logicalBlockNum, 0);
    }
#endif  // #if DEBUG_PRE_ERASED
  if(false == eraseStatus)
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
    {
    // Erase it before writting to make sure its empty...
    if((status = FTL_EraseOp(devID, logicalBlockNum)) != FTL_ERR_PASS)
      {

#if(FTL_DEFECT_MANAGEMENT == true)
      if(status == FTL_ERR_FAIL)
        {
        return status;
        }
      SetBadEBlockStatus(devID, logicalBlockNum, true);
      flushInfo.devID = devID;
      phyEBlockAddr = GetPhysicalEBlockAddr(devID, logicalBlockNum);
      flushInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyEBlockAddr, 0);
      flushInfo.vPage.pageOffset = 0;
      flushInfo.byteCount = 0;
      if(FLASH_MarkDefectEBlock(&flushInfo) != FLASH_PASS)
        {
        // do nothing, just try to mark bad, even if it fails we move on.
        }
      return FTL_ERR_FLUSH_NEXT_EBLOCK_FAIL;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
      return status;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

      }
    }
  flushInfo.devID = devID;
  flushInfo.byteCount = sizeof (SYS_EBLOCK_INFO);
  flushInfo.vPage.pageOffset = 0;
  phyEBlockAddr = GetPhysicalEBlockAddr(devID, logicalBlockNum);
  flushInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyEBlockAddr, 0);
  latestIncNumber = GetFlushEBCounter(devID) + 1;
  sysEBlockInfo.incNumber = latestIncNumber;
  sysEBlockInfo.phyAddrThisEBlock = phyEBlockAddr;
  sysEBlockInfo.type = SYS_EBLOCK_INFO_FLUSH;
  sysEBlockInfo.checkVersion = EMPTY_WORD;
  sysEBlockInfo.oldSysBlock = EMPTY_WORD;
  sysEBlockInfo.fullFlushSig = EMPTY_WORD;
  for(tempCount = 0; tempCount < NUM_SYS_RESERVED_BYTES; tempCount++)
    {
    sysEBlockInfo.reserved[tempCount] = EMPTY_BYTE;
    }
  if((status = FTL_WriteSysEBlockInfo(&flushInfo, &sysEBlockInfo)) != FTL_ERR_PASS)
    {

#if(FTL_DEFECT_MANAGEMENT == true)
    if(status == FTL_ERR_FAIL)
      {
      return status;
      }
    SetBadEBlockStatus(devID, logicalBlockNum, true);
    if(FLASH_MarkDefectEBlock(&flushInfo) != FLASH_PASS)
      {
      // do nothing, just try to mark bad, even if it fails we move on.
      }
    return FTL_ERR_FLUSH_NEXT_EBLOCK_FAIL;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    return status;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

    }
#if (ENABLE_EB_ERASED_BIT == true)
  SetEBErased(devID, logicalBlockNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
  SetFlushLogEBCounter(devID, latestIncNumber);
  SetGCOrFreePageNum(devID, logicalBlockNum, 1);
  MarkEBlockMappingTableEntryDirty(devID, logicalBlockNum);
  /*what if this is full*/
  if((status = TABLE_FlushEBInsert(devID, logicalBlockNum, phyEBlockAddr, latestIncNumber)) != FTL_ERR_PASS)
    {
    return status;
    }
  return FTL_ERR_PASS;
  }

//------------------------------

FTL_STATUS GetNextLogEntryLocation(FTL_DEV devID, FLASH_PAGE_INFO_PTR pageInfoPtr) /*  1,  4*/
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t latestIncNumber = EMPTY_DWORD; /*4*/
  uint16_t phyEBlockAddr = EMPTY_WORD; /*2*/
  uint16_t logicalBlockNum = EMPTY_WORD; /*2*/
  uint16_t entryIndex = EMPTY_WORD; /*2*/
  uint16_t byteOffset = 0; /*2*/

  // Initialize variables
  pageInfoPtr->devID = devID;
  pageInfoPtr->byteCount = LOG_ENTRY_SIZE;
  latestIncNumber = GetTransLogEBCounter(devID);
  status = TABLE_TransLogEBGetLatest(devID, &logicalBlockNum, &phyEBlockAddr, latestIncNumber);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }
  // Get offset to free entry
  entryIndex = GetFreePageIndex(devID, logicalBlockNum);
  if(entryIndex < NUM_LOG_ENTRIES_PER_EBLOCK)
    {
    // Latest EBlock has room for more
    byteOffset = entryIndex * LOG_ENTRY_DELTA;
    pageInfoPtr->vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddr, entryIndex);
    pageInfoPtr->vPage.pageOffset = byteOffset % VIRTUAL_PAGE_SIZE;
    SetGCOrFreePageNum(devID, logicalBlockNum, entryIndex + 1);
    MarkEBlockMappingTableEntryDirty(devID, logicalBlockNum);

#if(FTL_DEFECT_MANAGEMENT == true)
    SetTransLogEBNumBadBlockInfo(logicalBlockNum);
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

    }
  else
    {
    return FTL_ERR_CANNOT_FIND_NEXT_ENTRY;
    }

  return FTL_ERR_PASS;
  }

//--------------------------

FTL_STATUS CreateNextLogEntryLocation(FTL_DEV devID, uint16_t logicalBlockNum)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t phyEBlockAddr = 0x0; /*2*/
  uint16_t tempCount = 0x0; /*2*/
  uint32_t latestIncNumber = 0x0; /*4*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  FLASH_PAGE_INFO flushInfo = {0, 0,
    {0, 0}}; /*11*/

#if (ENABLE_EB_ERASED_BIT == true)
  uint8_t eraseStatus = false; /*1*/
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

#if (FTL_SUPER_SYS_EBLOCK == true)
  if((status = FTL_CreateSuperSysEBLog(devID, SYS_EBLOCK_INFO_CHANGED)) != FTL_ERR_PASS)
    {
    return status;
    }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if (ENABLE_EB_ERASED_BIT == true)
  eraseStatus = GetEBErased(devID, logicalBlockNum);

#if DEBUG_PRE_ERASED
  if(true == eraseStatus)
    {
    DBG_Printf("CreateNextLogEntryLocation: EBlock 0x%X is already erased\n", logicalBlockNum, 0);
    }
#endif  // #if DEBUG_PRE_ERASED
  if(false == eraseStatus)
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
    {

    // Erase it before writting to make sure its empty...
    if((status = FTL_EraseOp(devID, logicalBlockNum)) != FTL_ERR_PASS)
      {

#if(FTL_DEFECT_MANAGEMENT == true)
      if(status == FTL_ERR_FAIL)
        {
        return status;
        }
      SetBadEBlockStatus(devID, logicalBlockNum, true);
      flushInfo.devID = devID;
      phyEBlockAddr = GetPhysicalEBlockAddr(devID, logicalBlockNum);
      flushInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyEBlockAddr, 0);
      flushInfo.vPage.pageOffset = 0;
      flushInfo.byteCount = 0;
      if(FLASH_MarkDefectEBlock(&flushInfo) != FLASH_PASS)
        {
        // do nothing, just try to mark bad, even if it fails we move on.
        }
      return FTL_ERR_LOG_NEXT_EBLOCK_FAIL;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
      return status;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)
      }
    }
  flushInfo.devID = devID;
  flushInfo.byteCount = sizeof (SYS_EBLOCK_INFO);
  flushInfo.vPage.pageOffset = 0;
  phyEBlockAddr = GetPhysicalEBlockAddr(devID, logicalBlockNum);
  flushInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyEBlockAddr, 0);
  latestIncNumber = GetTransLogEBCounter(devID) + 1;
  sysEBlockInfo.phyAddrThisEBlock = phyEBlockAddr;
  sysEBlockInfo.incNumber = latestIncNumber;
  sysEBlockInfo.type = SYS_EBLOCK_INFO_LOG;
  sysEBlockInfo.checkVersion = EMPTY_WORD;
  sysEBlockInfo.oldSysBlock = EMPTY_WORD;
  sysEBlockInfo.fullFlushSig = EMPTY_WORD;
  for(tempCount = 0; tempCount < NUM_SYS_RESERVED_BYTES; tempCount++)
    {
    sysEBlockInfo.reserved[tempCount] = EMPTY_BYTE;
    }
  if((status = FTL_WriteSysEBlockInfo(&flushInfo, &sysEBlockInfo)) != FTL_ERR_PASS)
    {

#if(FTL_DEFECT_MANAGEMENT == true)
    if(status == FTL_ERR_FAIL)
      {
      return status;
      }
    SetBadEBlockStatus(devID, logicalBlockNum, true);
    if(FLASH_MarkDefectEBlock(&flushInfo) != FLASH_PASS)
      {
      // do nothing, just try to mark bad, even if it fails we move on.
      }
    return FTL_ERR_LOG_NEXT_EBLOCK_FAIL;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    return status;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

    }
#if (ENABLE_EB_ERASED_BIT == true)
  SetEBErased(devID, logicalBlockNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
  SetTransLogEBCounter(devID, latestIncNumber);
  SetDirtyCount(devID, logicalBlockNum, 0);
  SetGCOrFreePageNum(devID, logicalBlockNum, 1);
  MarkEBlockMappingTableEntryDirty(devID, logicalBlockNum);
  /*what if this is full*/
  if((status = TABLE_TransLogEBInsert(devID, logicalBlockNum, phyEBlockAddr, latestIncNumber)) != FTL_ERR_PASS)
    {
    return status;
    }
  return FTL_ERR_PASS;
  }

//--------------------------

FTL_STATUS CreateNextTransLogEBlock(FTL_DEV devID, uint16_t logicalBlockNum)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t nextLogicalEBlock = EMPTY_WORD; /*2*/
  uint16_t nextPhysicaEBlock = EMPTY_WORD; /*2*/

#if(FTL_DEFECT_MANAGEMENT == true)
  uint16_t sanityCounter = 0; /*2*/
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

  SetGCOrFreePageNum(devID, logicalBlockNum, NUM_LOG_ENTRIES_PER_EBLOCK);

#if(FTL_DEFECT_MANAGEMENT == true)
  while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
    {
    if((status = FTL_FindEmptyTransLogEBlock(devID, &nextLogicalEBlock, &nextPhysicaEBlock)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = CreateNextLogEntryLocation(devID, nextLogicalEBlock)) != FTL_ERR_LOG_NEXT_EBLOCK_FAIL)
      {
      break;
      }
    sanityCounter++;
    }
  if(status != FTL_ERR_PASS)
    {
    return status;
    }

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
  if((status = FTL_FindEmptyTransLogEBlock(devID, &nextLogicalEBlock, &nextPhysicaEBlock)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = CreateNextLogEntryLocation(devID, nextLogicalEBlock)) != FTL_ERR_PASS)
    {
    return status;
    }
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

  return FTL_ERR_PASS;
  }

//-----------------------------------------

FTL_STATUS TABLE_InitGcNum(void)
  {
  uint32_t maxValue = 0; /*4*/
  uint16_t logicalBlockNum = EMPTY_WORD; /*2*/
  FTL_DEV devID = 0; /*1*/

  for(devID = 0; devID < NUMBER_OF_DEVICES; devID++)
    {
    maxValue = 0;
    for(logicalBlockNum = 0; logicalBlockNum < NUM_DATA_EBLOCKS; logicalBlockNum++)
      {
      if(GetGCNum(devID, logicalBlockNum) > maxValue)
        {
        maxValue = GetGCNum(devID, logicalBlockNum);
        }
      }
    // Set to greater than the largest value used so far
    GCNum[devID] = maxValue + 1;
    }
  return FTL_ERR_PASS;
  }

//------------------------------

FTL_STATUS GetPhyPageAddr(ADDRESS_STRUCT_PTR currentPage, uint16_t phyEBNum,
                          uint16_t logEBNum, uint32_t * phyPage)
  {
  uint16_t pageIndex = 0; /*2*/
  uint16_t temp2 = 0; /*2*/

  pageIndex = GetIndexFromPhyPage(currentPage->logicalPageNum);
  temp2 = GetPPASlot(currentPage->devID, logEBNum, pageIndex);
  *phyPage = CalcPhyPageAddrFromPageOffset(phyEBNum, temp2);
  if(temp2 == CHAIN_INVALID)
    {
    *phyPage = PAGE_CHAINED;
    }
  return FTL_ERR_PASS;
  }

#if( FTL_EBLOCK_CHAINING == true)
//--------------------------

uint16_t GetLongestChain(FTL_DEV devID)
  {
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/
  uint16_t chainEB = 0; /*2*/
  uint16_t numPages = 0; /*2*/
  uint16_t numPagesTemp = 0; /*2*/
  uint16_t highEB = EMPTY_WORD; /*2*/

  for(logicalEBNum = NUM_DATA_EBLOCKS; logicalEBNum < NUM_EBLOCKS_PER_DEVICE; logicalEBNum++)
    {
    if(FTL_ERR_PASS == TABLE_CheckUsedSysEB(devID, logicalEBNum))
      {
      continue;
      }
    chainEB = GetChainLogicalEBNum(devID, logicalEBNum);
    if(chainEB != EMPTY_WORD)
      {
      numPagesTemp = NUM_PAGES_PER_EBLOCK - GetNumFreePages(devID, logicalEBNum);
      if(numPagesTemp > numPages)
        {
        numPages = numPagesTemp;
        highEB = chainEB;
        }
      }
    }
  return highEB;
  }
#endif  // #if( FTL_EBLOCK_CHAINING == true)

//------------------------

FTL_STATUS FTL_CheckMount_SetMTLockBit(void)
  {
#if(FTL_CHECK_ERRORS == true)
  FTL_STATUS status = FTL_ERR_PASS;

  if(mountStatus == 0)
    {
    return FTL_ERR_NOT_MOUNTED;
    }
  if((status = FTL_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }
#endif  // #if (FTL_CHECK_ERRORS == true)

  return FTL_ERR_PASS;
  }

//-----------------------

FTL_STATUS FTL_CheckUnmount_SetMTLockBit(void)
  {
#if(FTL_CHECK_ERRORS == true)
  FTL_STATUS status = FTL_ERR_PASS;

  if(mountStatus == 1)
    {
    return FTL_ERR_MOUNTED;
    }
  if((status = FTL_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }
#endif  // #if (FTL_CHECK_ERRORS == true)

  return FTL_ERR_PASS;
  }

//------------------------

FTL_STATUS FTL_SetMTLockBit(void)
  {
#if(FTL_CHECK_ERRORS == true)
  if(lockStatus == 0)
    {
    lockStatus = 1;
    }
  else
    {
    return FTL_ERR_LOCKED;
    }
#endif  // #if (FTL_CHECK_ERRORS == true)

  return FTL_ERR_PASS;
  }

//-------------------------

FTL_STATUS FTL_ClearMTLockBit(void)
  {
#if(FTL_CHECK_ERRORS == true)
  if(lockStatus == 1)
    {
    lockStatus = 0;
    }
  else
    {
    return FTL_ERR_UNLOCKED;
    }
#endif  // #if (FTL_CHECK_ERRORS == true)

  return FTL_ERR_PASS;
  }

//------------------------

void FTL_SetMountBit(void)
  {
#if(FTL_CHECK_ERRORS == true)
  mountStatus = 1;
#endif  // #if (FTL_CHECK_ERRORS == true)
  }

//---------------------------

void FTL_ClearMountBit(void)
  {
#if(FTL_CHECK_ERRORS == true)
  mountStatus = 0;
#endif  // #if (FTL_CHECK_ERRORS == true)
  }

//----------------------------

FTL_STATUS FTL_CheckDevID(uint8_t DevID)
  {
#if(FTL_CHECK_ERRORS == true)
  if(DevID >= NUM_DEVICES)
    {
    return FTL_ERR_ARG_DEVID;
    }
#endif  // #if (FTL_CHECK_ERRORS == true)

  return FTL_ERR_PASS;
  }

//------------------------------

FTL_STATUS FTL_CheckRange(uint32_t lba, uint32_t nb)
  {
#if(FTL_CHECK_ERRORS == true)
  if((lba >= MAX_NUMBER_LBA) || // Check for start too high
     (nb > MAX_NUMBER_LBA) || // Check for too many
     ((lba + nb) < lba) || // Check for addition overflow
     ((lba + nb) < nb) || // Check for addition overflow
     ((lba + nb - 1) >= MAX_NUMBER_LBA)) // Check for end too high
    {
    return FTL_ERR_OUT_OF_RANGE;
    }
#endif  // #if (FTL_CHECK_ERRORS == true)

  return FTL_ERR_PASS;
  }

//------------------------------

FTL_STATUS FTL_CheckPointer(void *Ptr)
  {
#if(FTL_CHECK_ERRORS == true)
  if(Ptr == NULL)
    {
    return FTL_ERR_ARG_PTR;
    }
#endif  // #if (FTL_CHECK_ERRORS == true)

  return FTL_ERR_PASS;
  }

//--------------------------------

FTL_STATUS ResetIndexValue(FTL_DEV devID, LOG_ENTRY_LOC_PTR startLoc)
  {
  uint32_t latestIncNumber = EMPTY_DWORD; /*4*/
  uint16_t logicalBlockNum = EMPTY_WORD; /*2*/
  uint16_t phyEBlockAddr = EMPTY_WORD; /*2*/
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t key = 0; /*4*/
  uint16_t count = 0; /*2*/
  uint16_t logEBAddr = EMPTY_WORD; /*2*/
  uint16_t phyEBAddr = EMPTY_WORD; /*2*/
  uint16_t eBlockNum = EMPTY_WORD; /*2*/

  latestIncNumber = GetTransLogEBCounter(devID);
  status = TABLE_TransLogEBGetLatest(devID, &logicalBlockNum, &phyEBlockAddr, latestIncNumber);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }
  if((startLoc->eBlockNum == EMPTY_WORD) && (startLoc->entryIndex == EMPTY_WORD))
    {
    SetGCOrFreePageNum(devID, logicalBlockNum, 1);
    }
  else
    {
    for(count = 0; count < NUM_TRANSACTLOG_EBLOCKS; count++)
      {
      if((status = TABLE_GetTransLogEntry(devID, count, &logEBAddr, &phyEBAddr, &key)) != FTL_ERR_PASS)
        {
        return status;
        }
      if((logEBAddr == EMPTY_WORD) && (phyEBAddr == EMPTY_WORD) && (key == EMPTY_DWORD))
        {
        break;
        }
      if((logEBAddr == logicalBlockNum) && (phyEBAddr == phyEBlockAddr) && (key == latestIncNumber))
        {
        eBlockNum = count;
        break;
        }
      }
    if(eBlockNum == startLoc->eBlockNum)
      {
      SetGCOrFreePageNum(devID, logicalBlockNum, startLoc->entryIndex);
      }
    else
      {
      return FTL_ERR_FLUSH_SHUTDOWN;
      }
    }
  return FTL_ERR_PASS;
  }

#if(FTL_CHECK_VERSION == true)

FTL_STATUS FTL_CheckVersion(void)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FLASH_STATUS flash_status = FLASH_PASS; /*4*/
  uint8_t devID = 0; /*1*/
  uint16_t eBlockCount = 0; /*2*/
  uint16_t checkVersion = EMPTY_WORD; /*2*/
  uint16_t logEBlockAddr = EMPTY_WORD; /*2*/
  uint16_t phyEBlockAddr = EMPTY_WORD; /*2*/
  uint32_t key = EMPTY_DWORD; /*4*/
  FLASH_PAGE_INFO flashPage = {0, 0,
    {0, 0}}; /*11*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/

  checkVersion = CalcCheckWord((uint16_t *) FTL_FLASH_IMAGE_VERSION, NUM_WORDS_OF_VERSION);
  for(eBlockCount = 0; eBlockCount < NUM_FLUSH_LOG_EBLOCKS; eBlockCount++)
    {
    if((status = TABLE_GetFlushLogEntry(devID, eBlockCount, &logEBlockAddr, &phyEBlockAddr, &key)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(key != EMPTY_DWORD)
      {
      flashPage.devID = devID;
      flashPage.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(phyEBlockAddr, 0);
      flashPage.vPage.pageOffset = 0;
      flashPage.byteCount = sizeof (sysEBlockInfo);
      flash_status = FLASH_RamPageReadMetaData(&flashPage, (uint8_t *) (&sysEBlockInfo));

      if(flash_status != FLASH_PASS)
        {
        return FTL_ERR_FLASH_READ_17;
        }

      if(sysEBlockInfo.checkVersion != EMPTY_WORD)
        {
        if(sysEBlockInfo.checkVersion == checkVersion)
          {
          return FTL_ERR_PASS;
          }
        else
          {
          return FTL_ERR_VERSMISMATCH;
          }
        }
      }
    }

  return FTL_ERR_VERSUNKNOWN;
  }
#endif // #if(FTL_CHECK_VERSION == true)

#if(FTL_DEFECT_MANAGEMENT == true)

FTL_STATUS TransLogEBFailure(FLASH_PAGE_INFO_PTR flashPagePtr, uint8_t * logPtr)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t logicalBlockNum = EMPTY_WORD; /*2*/
  uint32_t byteCount = 0; /*4*/
  uint16_t sanityCounter = 0; /*2*/
  FLASH_STATUS flashStatus = FLASH_PASS;

  byteCount = flashPagePtr->byteCount;
  SetTransLogEBFailedBadBlockInfo();
  logicalBlockNum = GetTransLogEBNumBadBlockInfo();
  SetBadEBlockStatus(flashPagePtr->devID, logicalBlockNum, true);
  if(FLASH_MarkDefectEBlock(flashPagePtr) != FLASH_PASS)
    {
    // do nothing, just try to mark bad, even if it fails we move on.
    }
  while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
    {
    if(GetTransLogEBArrayCount(flashPagePtr->devID) < NUM_TRANSACTLOG_EBLOCKS)
      {
      if((status = CreateNextTransLogEBlock(flashPagePtr->devID, logicalBlockNum)) != FTL_ERR_PASS)
        {
        return status;
        }
      /*rewrite the log*/
      if((status = GetNextLogEntryLocation(flashPagePtr->devID, flashPagePtr)) != FTL_ERR_PASS)
        {
        return status;
        }
      flashPagePtr->byteCount = byteCount;
      flashStatus = FLASH_RamPageWriteDataBlock(flashPagePtr, logPtr);
      if(flashStatus == FLASH_PASS)
        {
        status = FTL_ERR_PASS;
        break;
        }
      else
        {
        if(flashStatus == FLASH_PARAM)
          {
          return FTL_ERR_FAIL;
          }
        logicalBlockNum = GetTransLogEBNumBadBlockInfo();
        SetBadEBlockStatus(flashPagePtr->devID, logicalBlockNum, true);
        if(FLASH_MarkDefectEBlock(flashPagePtr) != FLASH_PASS)
          {
          // do nothing, just try to mark bad, even if it fails we move on.
          }
        sanityCounter++;
        }
      }
    else
      {
      return FTL_ERR_LOG_RECOVERY_EBLOCK;
      }
    }
  if(status != FTL_ERR_PASS)
    {
    return status;
    }
  return FTL_ERR_PASS;
  }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)
