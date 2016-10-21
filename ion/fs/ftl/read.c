#include "def.h"
#include "calc.h"
#include "common.h"
#include "if_in.h"
#include "if_ex.h"

//--------------------------------------------------------------

FTL_STATUS FTL_DeviceObjectsRead(uint8_t * buffer, uint32_t LBA, uint32_t NB, uint32_t * bytesDone) /*4,4,4,4*/
  {
  uint32_t numEntries = 0; /*4*/
  uint32_t result = FTL_ERR_PASS; /*4*/
  uint16_t currentEntry = 0; /*2*/
  uint32_t remainingEntries = 0; /*4*/
  uint32_t currentLBA_range = 0; /*4*/
  uint32_t currentNBs = 0; /*4*/
  uint32_t NBs_left = 0; /*4*/
  uint32_t previousBufferValue = 0; /*4*/
#if (CACHE_RAM_BD_MODULE == true)
  uint32_t eblockBoundary = 0;
#endif
#if (DEBUG_ENABLE_LOGGING == true)
  if((result = DEBUG_InsertLog(LBA, NB, DEBUG_LOG_READ)) != FTL_ERR_PASS)
    {
    return (result);
    }
#endif
  if((result = FTL_CheckRange(LBA, NB)) != FTL_ERR_PASS)
    {
    return (result);
    }
  if((result = FTL_CheckPointer(buffer)) != FTL_ERR_PASS)
    {
    return (result);
    }
  if((result = FTL_CheckPointer(bytesDone)) != FTL_ERR_PASS)
    {
    return (result);
    }
  if((result = FTL_CheckMount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return (result);
    }

  if((result = ClearDeleteInfo()) != FTL_ERR_PASS)
    {
    return (result);
    }

  *bytesDone = 0;

#if(FTL_RPB_CACHE == true)
  result = RPBCacheForRead(&buffer, &LBA, &NB, bytesDone);
  if(FTL_ERR_PASS != result)
    {
    FTL_ClearMTLockBit();
    return (result);
    }
#endif  // #if(FTL_RPB_CACHE == true)

  for(currentLBA_range = LBA, NBs_left = NB; NBs_left > 0; currentLBA_range += currentNBs, NBs_left -= currentNBs)
    {
    if(NBs_left > NUM_SECTORS_PER_EBLOCK)
      {
      currentNBs = NUM_SECTORS_PER_EBLOCK;
      }
    else
      {
      currentNBs = NBs_left;
      }

#if (CACHE_RAM_BD_MODULE == true)
    eblockBoundary = NUM_SECTORS_PER_EBLOCK - (currentLBA_range % NUM_SECTORS_PER_EBLOCK);
    if(currentNBs > eblockBoundary)
      {
      currentNBs = eblockBoundary;
      }
#endif


#if (CACHE_RAM_BD_MODULE == true)
    if(FTL_ERR_PASS != (result = CACHE_LoadEB((uint8_t) ((currentLBA_range) & DEVICE_BIT_MAP) >> DEVICE_BIT_SHIFT, (uint16_t) (currentLBA_range / NUM_SECTORS_PER_EBLOCK), CACHE_READ_TYPE)))
      {
      return result;
      }
#endif

    result = FTL_BuildTransferMapForReadBlocking(currentLBA_range, currentNBs, &numEntries);
    if(FTL_ERR_PASS != result)
      {
      FTL_ClearMTLockBit();
      return (result); //exit the transfer
      }
    //Transfer Map is now built data is now in from HOST in the location pointed at by the bufffer pointer
    for(currentEntry = 0; currentEntry < numEntries; currentEntry++)
      {
      previousBufferValue = (uint32_t) (buffer);
      if((result = FTL_TransferPageForRead(&buffer, &remainingEntries)) != FTL_ERR_PASS)
        {
        FTL_ClearMTLockBit();
        return (result); //exit the transfer
        }
      *bytesDone += ((uint32_t) (buffer)) - previousBufferValue;
      } //end of inside for
    } //end of outside for

#if(FTL_RPB_CACHE == true)
  for(currentLBA_range = RPBCacheReadGroup.LBA, NBs_left = RPBCacheReadGroup.NB; NBs_left > 0; currentLBA_range += currentNBs, NBs_left -= currentNBs)
    {
    if(NBs_left > NUM_SECTORS_PER_EBLOCK)
      {
      currentNBs = NUM_SECTORS_PER_EBLOCK;
      }
    else
      {
      currentNBs = NBs_left;
      }
    result = FTL_BuildTransferMapForReadBlocking(currentLBA_range, currentNBs, &numEntries);
    if(FTL_ERR_PASS != result)
      {
      FTL_ClearMTLockBit();
      return (result); //exit the transfer
      }
    //Transfer Map is now built data is now in from HOST in the location pointed at by the bufffer pointer
    for(currentEntry = 0; currentEntry < numEntries; currentEntry++)
      {
      previousBufferValue = (uint32_t) (RPBCacheReadGroup.byteBuffer);
      if((result = FTL_TransferPageForRead(&RPBCacheReadGroup.byteBuffer, &remainingEntries)) != FTL_ERR_PASS)
        {
        FTL_ClearMTLockBit();
        return (result); //exit the transfer
        }
      *bytesDone += ((uint32_t) (RPBCacheReadGroup.byteBuffer)) - previousBufferValue;
      } //end of inside for
    } //end of outside for
#endif  // #if(FTL_RPB_CACHE == true)

  FTL_ClearMTLockBit();
  return (result); //If we made it to this location then we have transfered all the pages without error
  }

//--------------------------

FTL_STATUS FTL_BuildTransferMapForReadBlocking(uint32_t LBA, uint32_t NB,
                                               uint32_t * resultingEntries)
  {
  uint8_t pageLoopCount = NUM_SECTORS_PER_PAGE; /*1*/
  uint16_t logicalEBNum = 0; /*2*/
  uint32_t status = FTL_ERR_PASS; /*2*/
  uint16_t phyEBNum = 0; /*2*/
  uint32_t totalPages = 0; /*4*/
  uint32_t currentPageCount = 0; /*4*/
  uint32_t phyPageAddr = EMPTY_DWORD; /*4*/
  uint32_t mergePage = EMPTY_DWORD; /*4*/
  uint32_t currentLBA = 0; /*4*/
  ADDRESS_STRUCT endPage = {0, 0, 0}; /*6*/
  ADDRESS_STRUCT startPage = {0, 0, 0}; /*6*/
  ADDRESS_STRUCT currentPage = {0, 0, 0}; /*6*/

#if(FTL_EBLOCK_CHAINING == true)
  uint16_t chainEBNum = EMPTY_WORD; /*2*/
  CHAIN_INFO chainInfo = {0, 0, 0, 0, 0}; /*10*/
#endif  // #if(FTL_EBLOCK_CHAINING == true)

#if (DEBUG_FTL_API_ANNOUNCE == 1)
  DBG_Printf("FTL_BuildTransferMapForReadBlocking: LBA = 0x%X, ", LBA, 0);
  DBG_Printf("NB = %d\n", NB, 0);
#endif  // #if (DEBUG_FTL_API_ANNOUNCE == 1)

  if((status = TRANS_ClearTransMap()) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = GetPageSpread(LBA, NB, &startPage, &totalPages, &endPage)) != FTL_ERR_PASS)
    {
    return status;
    }
  currentPage = startPage;
  currentLBA = LBA;
  for(currentPageCount = 0; currentPageCount < totalPages; currentPageCount++)
    {
#if(FTL_EBLOCK_CHAINING == true)
    chainInfo.isChained = false;
#endif  // #if(FTL_EBLOCK_CHAINING == true)

    if((status = GetLogicalEBNum(currentPage.logicalPageNum, &logicalEBNum)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(currentPageCount == (totalPages - 1)) /*This will work even when there is a sub page to write, because totalPages-1 will equal 0*/
      {
      if(currentPageCount == 0)
        {
        currentPage.pageOffset = startPage.pageOffset;
        }
      else
        {
        currentPage.pageOffset = 0;
        }
      if(endPage.pageOffset == 0)
        {
        pageLoopCount = NUM_SECTORS_PER_PAGE;
        }
      else
        {
        pageLoopCount = endPage.pageOffset;
        }
      }
    /*If GC_Info has the data already, make sure phyPage GetPhyPage is not called*/
    phyEBNum = GetPhysicalEBlockAddr(currentPage.devID, logicalEBNum);
    if((status = GetPhyPageAddr(&currentPage, phyEBNum, logicalEBNum,
                                &phyPageAddr)) != FTL_ERR_PASS)
      {
      return status;
      }

#if(FTL_EBLOCK_CHAINING == true)
    chainEBNum = GetChainLogicalEBNum(currentPage.devID, logicalEBNum);
    if(chainEBNum != EMPTY_WORD)
      {
      chainInfo.isChained = true;
      chainInfo.logChainToEB = chainEBNum;
      chainInfo.phyChainToEB = GetChainPhyEBNum(currentPage.devID, logicalEBNum);
      // this is a chained EB, lets check teh phyPageAddr
      if(phyPageAddr == PAGE_CHAINED)
        {
        if((status = GetPhyPageAddr(&currentPage, chainInfo.phyChainToEB, chainInfo.logChainToEB,
                                    &(chainInfo.phyPageAddr))) != FTL_ERR_PASS)
          {
          return status;
          }
        phyPageAddr = chainInfo.phyPageAddr;
        }
      }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

    /*need this regardless*/
    if(phyPageAddr == EMPTY_DWORD)
      {
      /*we have a problem, page not found*/
      }
    if((status = UpdateTransferMap(currentLBA, &currentPage, &endPage, &startPage,
                                   totalPages, phyPageAddr, mergePage, false, false)) != FTL_ERR_PASS)
      {
      return status;
      }
    currentLBA = currentLBA + (pageLoopCount - currentPage.pageOffset);
    if((status = IncPageAddr(&currentPage)) != FTL_ERR_PASS)
      {
      return status;
      }
    } //end of for
  *resultingEntries = currentPageCount;
  return FTL_ERR_PASS;
  }

//--------------------------------------------------------------------------

FTL_STATUS FTL_TransferPageForRead(uint8_t * *byteBuffer, uint32_t * remainingEntries)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t currentTransfer = 0; /*2*/
  uint16_t startIndex = 0; /*2*/
  uint16_t endIndex = 0; /*2*/
  FTL_DEV deviceNum = 0; /*1*/
  uint8_t dstStartSector = 0; /*1*/
  uint8_t numSectors = 0; /*1*/
  uint32_t physicalPage = EMPTY_DWORD; /*4*/
  uint32_t count = 0; /*4*/
  FLASH_PAGE_INFO flashPageInfo = {0, 0,
    {0, 0,}}; /*11*/

  if((status = FTL_GetNextTransferMapEntry(&currentTransfer, &startIndex, &endIndex)) != FTL_ERR_PASS)
    {
    return status;
    }
  dstStartSector = GetTMStartSector(currentTransfer);
  numSectors = GetTMNumSectors(currentTransfer);
  deviceNum = GetTMDevID(currentTransfer);
  physicalPage = GetTMPhyPage(currentTransfer);

#if (DEBUG_FTL_API_ANNOUNCE == 1)
  DBG_Printf("FTL_TransferPageForRead: deviceNum=%d, ", deviceNum, 0);
  DBG_Printf("physicalPage=0x%x, ", physicalPage, 0);
  DBG_Printf("dstStartSector=%d, ", dstStartSector, 0);
  DBG_Printf("numSectors=%d\n", numSectors, 0);
#endif  // #if DEBUG_GC_ANNOUNCE

  /*Check to see if the page is aligned*/
  flashPageInfo.byteCount = numSectors * SECTOR_SIZE;
  flashPageInfo.devID = deviceNum;
  flashPageInfo.vPage.pageOffset = dstStartSector * SECTOR_SIZE;
  flashPageInfo.vPage.vPageAddr = physicalPage;
  if(physicalPage == EMPTY_DWORD)
    {
    /*just FF fill*/
    for(count = SECTOR_SIZE * numSectors; count; count--)
      {
#if(AUTOFILL_0xFF_UNMAPPED_SECTORS == true)
      *((*byteBuffer)++) = EMPTY_BYTE;
#else
      *((*byteBuffer)++) = 0;
#endif
      }
    }
  else
    {
    dstStartSector = 0;
    if((FLASH_RamPageReadDataBlock(&flashPageInfo, &((*byteBuffer)[SECTOR_SIZE * dstStartSector]))) != FLASH_PASS)
      {
      return FTL_ERR_FLASH_READ_02;
      }
    (*byteBuffer) += (numSectors * SECTOR_SIZE);
    }
  *remainingEntries = endIndex - startIndex;
  return FTL_ERR_PASS;
  }
