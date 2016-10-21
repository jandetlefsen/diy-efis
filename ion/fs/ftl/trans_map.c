#include "def.h"
#include "calc.h"
#include "common.h"

#define FTL_TRANSFER_MAP_DEBUG  (0)
#define DEBUG_TRANSFER_LOG      (0)

//--------------------------------

FTL_STATUS UpdateTransferMap(uint32_t currentLBA, ADDRESS_STRUCT_PTR currentPage,
                             ADDRESS_STRUCT_PTR endPage, ADDRESS_STRUCT_PTR startPage, uint32_t totalPages,
                             uint32_t phyPage, uint32_t mergePage, uint8_t isWrite, uint8_t isChained)
  {
  ADDRESS_STRUCT tempPage = {0, 0, 0}; /*6*/
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

  if(TransferMapIndexEnd > NUM_TRANSFER_MAP_ENTRIES)
    {
    return FTL_ERR_TRANS_MAP_FULL;
    }
  SetTMDevID(TransferMapIndexEnd, currentPage->devID);
  SetTMStartLBA(TransferMapIndexEnd, currentLBA);
  SetTMPhyPage(TransferMapIndexEnd, phyPage);
  SetTMMergePage(TransferMapIndexEnd, mergePage);
  tempPage = *(currentPage);
  if(endPage->pageOffset == 0)
    {
    /*This is the case, where the page spread ends in the next page*/
    if((status = IncPageAddr(&tempPage)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((tempPage.devID != endPage->devID) || (tempPage.logicalPageNum != endPage->logicalPageNum))
      {
      /*not the last page, so reset the tempPage*/
      tempPage = *(currentPage);
      }
    }
  if((currentPage->devID == startPage->devID) && (currentPage->logicalPageNum == startPage->logicalPageNum))
    {
    /*this is the start page*/
    if((totalPages != 1) || (endPage->pageOffset == 0))
      {
      /*multi page transfer*/
      SetTMNumSectors(TransferMapIndexEnd, NUM_SECTORS_PER_PAGE - startPage->pageOffset);
      }
    else
      {
      /*single page transfer*/
      SetTMNumSectors(TransferMapIndexEnd, endPage->pageOffset - startPage->pageOffset);
      }
    SetTMStartSector(TransferMapIndexEnd, startPage->pageOffset);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    if(isWrite == true)
      {
      if((status = InsertEntryIntoLogEntry(TransferMapIndexEnd, phyPage, currentLBA,
                                           currentPage, isChained)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

    TransferMapIndexEnd++;
    return FTL_ERR_PASS;
    }
  if((tempPage.devID == endPage->devID) && (tempPage.logicalPageNum == endPage->logicalPageNum))
    {
    /*This is the end page*/
    if(endPage->pageOffset == 0)
      {
      SetTMNumSectors(TransferMapIndexEnd, NUM_SECTORS_PER_PAGE);
      }
    else
      {
      if(totalPages != 1)
        {
        /*multi page transfer*/
        SetTMNumSectors(TransferMapIndexEnd, endPage->pageOffset);
        }
      else
        {
        /*single page transfer*/
        SetTMNumSectors(TransferMapIndexEnd, endPage->pageOffset - startPage->pageOffset);
        }
      }
    SetTMStartSector(TransferMapIndexEnd, currentPage->pageOffset);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    if(isWrite == true)
      {
      if((status = InsertEntryIntoLogEntry(TransferMapIndexEnd, phyPage, currentLBA,
                                           currentPage, isChained)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

    TransferMapIndexEnd++;
    return FTL_ERR_PASS;
    }
  /*This is a middle page*/
  SetTMNumSectors(TransferMapIndexEnd, NUM_SECTORS_PER_PAGE);
  SetTMStartSector(TransferMapIndexEnd, 0);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  if(isWrite == true)
    {
    if((status = InsertEntryIntoLogEntry(TransferMapIndexEnd, phyPage, currentLBA,
                                         currentPage, isChained)) != FTL_ERR_PASS)
      {
      return status;
      }
    }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

  TransferMapIndexEnd++;
  return FTL_ERR_PASS;
  }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
//-----------------------------

FTL_STATUS InsertEntryIntoLogEntry(uint16_t index, uint32_t phyPageAddr,
                                   uint32_t currentLBA, ADDRESS_STRUCT_PTR currentPage, uint8_t isChained)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t tempValue = 0; /*2*/
  uint16_t tempIndex = 0; /*2*/
  uint16_t tempIndex2 = 0; /*2*/
  uint16_t tempIndex3 = 0; /*2*/
  uint16_t chainFlag = 0; /*2*/

  if(isChained)
    {
    chainFlag = CHAIN_FLAG;
    }
  if(index == 0)
    {
    status = FTL_ClearA();
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
    status = FTL_ClearC(0);
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
    TranslogBEntries = 0;
    TransLogEntry.entryA.LBA = currentLBA;
    TransLogEntry.entryC.GCNum = GCNum[currentPage->devID];

    if((status = GetPhysicalPageOffset(phyPageAddr, &tempValue)) != FTL_ERR_PASS)
      {
      return status;
      }
    TransLogEntry.entryA.pageLoc[index].phyEBOffset = tempValue | chainFlag;

    if((status = GetLogicalEBNum(currentPage->logicalPageNum, &tempValue)) != FTL_ERR_PASS)
      {
      return status;
      }
    TransLogEntry.entryA.pageLoc[index].logEBNum = tempValue;

    }
  else
    {
    if(index < NUM_ENTRIES_TYPE_A)
      {
      if((status = GetPhysicalPageOffset(phyPageAddr, &tempValue)) != FTL_ERR_PASS)
        {
        return status;
        }
      TransLogEntry.entryA.pageLoc[index].phyEBOffset = tempValue | chainFlag;

      if((status = GetLogicalEBNum(currentPage->logicalPageNum, &tempValue)) != FTL_ERR_PASS)
        {
        return status;
        }
      TransLogEntry.entryA.pageLoc[index].logEBNum = tempValue;

      }
    else
      {
      {
        /*TYPE B, fill it here*/
        tempIndex = index - NUM_ENTRIES_TYPE_A;
        tempIndex2 = tempIndex / NUM_ENTRIES_TYPE_B;
        tempIndex3 = tempIndex % NUM_ENTRIES_TYPE_B;
        if(tempIndex3 == 0)
          {
          status = FTL_ClearB(TranslogBEntries);
          if(status != FTL_ERR_PASS)
            {
            return status;
            }
          TranslogBEntries++;
          }
        if((status = GetPhysicalPageOffset(phyPageAddr, &tempValue)) != FTL_ERR_PASS)
          {
          return status;
          }
        TransLogEntry.entryB[tempIndex2].pageLoc[tempIndex3].phyEBOffset = tempValue | chainFlag;

        if((status = GetLogicalEBNum(currentPage->logicalPageNum, &tempValue)) != FTL_ERR_PASS)
          {
          return status;
          }
        TransLogEntry.entryB[tempIndex2].pageLoc[tempIndex3].logEBNum = tempValue;
      }
      TransLogEntry.entryC.seqNum = (uint8_t) (TranslogBEntries + 1);
      }
    }
  return status;
  }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

//------------------------------------

FTL_STATUS FTL_GetNextTransferMapEntry(uint16_t * nextEntryIndex,
                                       uint16_t * startIndex, uint16_t * endIndex)
  {
  if(TransferMapIndexEnd == TransferMapIndexStart)
    {
    return FTL_ERR_TRANS_MAP_EMPTY;
    }
  *nextEntryIndex = TransferMapIndexStart;
  /*Init this global, so ManageWrite() can access it later*/
  previousDevice = GetTMDevID(TransferMapIndexStart);
  /*should not need this again, clear it*/
  TransferMapIndexStart++;
#if(FTL_DEFECT_MANAGEMENT == false)
  if(TransferMapIndexEnd == TransferMapIndexStart)
    {
    TransferMapIndexEnd = TransferMapIndexStart = 0;
    }
#endif // #if(FTL_DEFECT_MANAGEMENT == false)
  *startIndex = TransferMapIndexStart;
  *endIndex = TransferMapIndexEnd;
  return FTL_ERR_PASS;
  }

uint16_t FTL_GetCurrentIndex(void)
  {
  return TransferMapIndexStart;
  }
//---------------------------

FTL_STATUS FTL_GetTransferMapEntry(uint16_t entryIndex, TRANS_MAP_ENTRY_PTR transferMapEntry)
  {
  transferMapEntry->devID = GetTMDevID(entryIndex);
  transferMapEntry->numSectors = GetTMNumSectors(entryIndex);
  transferMapEntry->phyPageAddr = GetTMPhyPage(entryIndex);
  transferMapEntry->startLBA = GetTMStartLBA(entryIndex);
  transferMapEntry->startSector = GetTMStartSector(entryIndex);
  transferMapEntry->logEBlockEntryIndex = GetTMLogInfo(entryIndex);
  transferMapEntry->mergePageForWrite = GetTMMergePage(entryIndex);
  return FTL_ERR_PASS;
  }

//----------------------

FTL_STATUS TRANS_ClearEntry(uint16_t index)
  {
  if(index > NUM_TRANSFER_MAP_ENTRIES)
    {
    return FTL_ERR_TRANS_NO_ENTRIES;
    }
  SetTMDevID(index, EMPTY_BYTE);
  SetTMNumSectors(index, EMPTY_BYTE);
  SetTMStartLBA(index, EMPTY_DWORD);
  SetTMPhyPage(index, EMPTY_DWORD);
  SetTMStartSector(index, EMPTY_BYTE);
  SetTMMergePage(index, EMPTY_DWORD);
  return FTL_ERR_PASS;
  }

//--------------------------------

FTL_STATUS TRANS_ClearTransMap(void)
  {
  uint16_t index = 0; /*2*/
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  // Modify to defect management because defect management needs transfermap
  if(TransferMapIndexEnd == TransferMapIndexStart)
    {
    TransferMapIndexEnd = TransferMapIndexStart = 0;
    return FTL_ERR_PASS;
    }
  else
    {
    for(index = 0; index < NUM_TRANSFER_MAP_ENTRIES; index++)
      {
      if((status = TRANS_ClearEntry(index)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
    TransferMapIndexEnd = TransferMapIndexStart = 0;
    }
  return FTL_ERR_PASS;
  }

//-------------------------------

FTL_STATUS TRANS_InitTransMap(void)
  {
  /*set the Transfer Map variables*/
  previousDevice = EMPTY_BYTE;
  TransferMapIndexEnd = TransferMapIndexStart = 0;
  /*end Transfer map stuff*/
  return FTL_ERR_PASS;
  }
