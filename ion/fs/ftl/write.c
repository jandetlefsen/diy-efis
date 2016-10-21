#include "common.h"
#include "if_in.h"
#include "if_ex.h"

#include <string.h>

FTL_STATUS FTL_DeviceObjectsWrite(uint8_t * buffer, uint32_t LBA, uint32_t NB, uint32_t * bytesDone)
  {
  uint32_t numEntries = 0; /*4*/
  uint32_t result = FTL_ERR_PASS; /*4*/
  uint16_t currentEntry = 0; /*2*/
  uint32_t remainingEntries = 0; /*4*/
  uint32_t currentLBA_range = 0; /*4*/
  uint32_t currentNBs = 0; /*4*/
  uint32_t NBs_left = 0; /*4*/
#if (FTL_DEFECT_MANAGEMENT == true  || CACHE_RAM_BD_MODULE == true)
  uint32_t eblockBoundary = 0;
#if (FTL_DEFECT_MANAGEMENT == true)
  uint16_t writeCount = 0;
#endif
#endif
#if (FTL_DEFECT_MANAGEMENT == true)
  FTL_STATUS badBlockStatus = FTL_ERR_PASS;
  uint8_t * bbBufferPtr;
#endif
#if(FTL_SUPER_SYS_EBLOCK == true)
#if(FTL_DEFECT_MANAGEMENT == true)
  uint8_t devID = 0; /*1*/
  uint16_t sanityCounter = 0;
#endif
#endif  // #if(FTL_SUPER_SYS_EBLOCK == true)
  uint32_t previousBufferValue = 0; /*4*/
#if (DEBUG_ENABLE_LOGGING == true)
  if((result = DEBUG_InsertLog(LBA, NB, DEBUG_LOG_WRITE)) != FTL_ERR_PASS)
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

#if(FTL_DEFECT_MANAGEMENT == true)
  ClearTransLogEBBadBlockInfo();
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

  *bytesDone = 0;

#if (FTL_SUPER_SYS_EBLOCK == true)
#if(FTL_DEFECT_MANAGEMENT == true)
  for(devID = 0; devID < NUMBER_OF_DEVICES; devID++)
    {
    while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
      {
      if((result = FTL_CheckForSuperSysEBLogSpace(devID, SYS_EBLOCK_INFO_CHANGED)) != FTL_ERR_PASS)
        {
        return result;
        }
      if((result = FTL_CreateSuperSysEBLog(devID, SYS_EBLOCK_INFO_CHANGED)) == FTL_ERR_PASS)
        {
        break;
        }
      if(result != FTL_ERR_SUPER_WRITE_02)
        {
        return result;
        }
      sanityCounter++;
      }
    }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if(FTL_RPB_CACHE == true)
  result = RPBCacheForWrite(&buffer, &LBA, &NB, bytesDone);
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
#if (FTL_DEFECT_MANAGEMENT == true || CACHE_RAM_BD_MODULE == true)
    eblockBoundary = NUM_SECTORS_PER_EBLOCK - (currentLBA_range % NUM_SECTORS_PER_EBLOCK);
    if(currentNBs > eblockBoundary)
      {
      currentNBs = eblockBoundary;
      }
#if (FTL_DEFECT_MANAGEMENT == true)
    bbBufferPtr = buffer;
#endif
    do
      {
#endif

#if (CACHE_RAM_BD_MODULE == true)
      if(FTL_ERR_PASS != (result = CACHE_LoadEB((uint8_t) ((currentLBA_range) & DEVICE_BIT_MAP) >> DEVICE_BIT_SHIFT, (uint16_t) ((currentLBA_range) / NUM_SECTORS_PER_EBLOCK), CACHE_WRITE_TYPE)))
        {
        return result;
        }
#endif

      do
        {
        result = FTL_BuildTransferMapForWriteBlocking(currentLBA_range, currentNBs, &numEntries);
        if(FTL_ERR_PASS != result)
          {
#if (FTL_DEFECT_MANAGEMENT == true)
          badBlockStatus = TranslateBadBlockError(result);
#endif
          if(FTL_ERR_DATA_GC_NEEDED == result)
            {
            // There are no free areas to write to we must sit and wait for GC to create space
            uint32_t GC_result = FTL_ERR_PASS; //create some very temporary variables to accommodate GC
            uint16_t temp = EMPTY_WORD;
            uint16_t temp2 = EMPTY_WORD;

#if (CACHE_RAM_BD_MODULE == true)
            if(FTL_ERR_PASS != (GC_result = CACHE_LoadEB(GC_Info.devID, GC_Info.logicalEBlock, CACHE_WRITE_TYPE)))
              {
              return GC_result;
              }
#endif

#if(FTL_DEFECT_MANAGEMENT == true)
            GC_result = InternalForcedGCWithBBManagement(EMPTY_BYTE, EMPTY_WORD, &temp, &temp2, false);

#else
            GC_result = FTL_InternalForcedGC(EMPTY_BYTE, EMPTY_WORD, &temp, &temp2, false);
#endif

            if(GC_result != FTL_ERR_PASS)
              {
              FTL_ClearMTLockBit();
              return GC_result;
              }

#if (CACHE_RAM_BD_MODULE == true)

            if(FTL_ERR_PASS != (GC_result = CACHE_LoadEB((uint8_t) ((currentLBA_range) & DEVICE_BIT_MAP) >> DEVICE_BIT_SHIFT, (uint16_t) (currentLBA_range / NUM_SECTORS_PER_EBLOCK), CACHE_READ_TYPE)))
              {
              return GC_result;
              }
#endif
            }
#if (FTL_DEFECT_MANAGEMENT == true)
          else if(badBlockStatus == FTL_ERR_BAD_BLOCK_SOURCE)
            {
            if(writeCount >= MAX_BAD_BLOCK_SANITY_TRIES)
              {
              FTL_ClearMTLockBit();
              return result;
              }
            writeCount++;
            badBlockStatus = BB_ManageBadBlockErrorForChainErase(); // space check erase failure. so all flash changes will be in teh reserve pool
            if(badBlockStatus != FTL_ERR_PASS)
              {
              FTL_ClearMTLockBit();
              return result;
              }
            }
          else if(badBlockStatus == FTL_ERR_LOG_WR)
            {
            // should not come into here.
            return FTL_ERR_FAIL;
            /*
            if(writeCount >= MAX_BAD_BLOCK_SANITY_TRIES)
            {
                FTL_ClearMTLockBit();
                return result; 
            }
            writeCount++;
            badBlockStatus = BB_ManageBadBlockErrorForGCLog(); // space check erase failure. so all flash changes will be in teh reserve pool
            if(badBlockStatus !=FTL_ERR_PASS)
            {
                FTL_ClearMTLockBit();
                return result;
            }
            // change error code
            result = FTL_ERR_FAIL;
             */
            }
#endif
          else
            {
            FTL_ClearMTLockBit();
            return (result); //exit the transfer
            }
          }
        }
      while(FTL_ERR_PASS != result);

      //end of building transfer map while loop
      //All gc's needed for current transfer have been done
      //Transfer Map is now built data is now in from HOST in the location pointed at by the buffer pointer
      for(currentEntry = 0; currentEntry < numEntries; currentEntry++)
        {
        previousBufferValue = (uint32_t) (buffer);
        if((result = FTL_TransferPageForWrite(&buffer, &remainingEntries)) != FTL_ERR_PASS)
          {
#if (FTL_DEFECT_MANAGEMENT == true)
          badBlockStatus = TranslateBadBlockError(result);
          if(badBlockStatus == FTL_ERR_LOG_WR)
            {
            if(writeCount >= MAX_BAD_BLOCK_SANITY_TRIES)
              {
              FTL_ClearMTLockBit();
              return result;
              }
            writeCount++;
            badBlockStatus = BB_ManageBadBlockErrorForGCLog(); // space check erase failure. so all flash changes will be in teh reserve pool
            if(badBlockStatus != FTL_ERR_PASS)
              {
              FTL_ClearMTLockBit();
              return result;
              }
            buffer = bbBufferPtr;
            }
          else if(badBlockStatus == FTL_ERR_BAD_BLOCK_SOURCE)
            {
            if(writeCount >= MAX_BAD_BLOCK_SANITY_TRIES)
              {
              FTL_ClearMTLockBit();
              return result;
              }
            writeCount++;
            badBlockStatus = BB_ManageBadBlockErrorForSource(); // space check erase failure. so all flash changes will be in teh reserve pool
            if(badBlockStatus != FTL_ERR_PASS)
              {
              FTL_ClearMTLockBit();
              return result;
              }
            buffer = bbBufferPtr;
            }
          else
#endif
            {
            FTL_ClearMTLockBit();
            return (result); //exit the transfer
            }
          break; // break the 'for', go to the 'while'
          }
        *bytesDone += ((uint32_t) (buffer)) - previousBufferValue;
        }

#if(FTL_DEFECT_MANAGEMENT == true || CACHE_RAM_BD_MODULE == true)
#if (FTL_DEFECT_MANAGEMENT == true)
      ClearBadBlockInfo();
#endif
      }
    while(result != FTL_ERR_PASS);
#endif

#if(FTL_DEFECT_MANAGEMENT == true)
    if(GetTransLogEBFailedBadBlockInfo() == true)
      {
      if((result = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS)
        {
        FTL_ClearMTLockBit();
        return (result);
        }
      }
    ClearTransLogEBBadBlockInfo();
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

    } //end of one transfer map at a time (xfer maps are limited in size check xyz for current size)
#if (FTL_STATIC_WEAR_LEVELING == true)
  result = FTL_StaticWearLevelData();
  if(result != FTL_ERR_PASS)
    {
    return (result);
    }
#endif
  FTL_ClearMTLockBit();
  FTL_UpdatedFlag = UPDATED_DONE;
  return (FTL_ERR_PASS); //If we made it to this location then we have transfered all the pages without error
  }

//-------------------------------------------------------------------------------

FTL_STATUS FTL_BuildTransferMapForWriteBlocking(uint32_t LBA, uint32_t NB,
                                                uint32_t * resultingEntries)
  {
  uint8_t pageLoopCount = NUM_SECTORS_PER_PAGE; /*1*/
  uint16_t logicalPageOffset = 0; /*2*/
  uint16_t logicalEBNum = 0; /*2*/
  uint16_t logicalMergeEB = EMPTY_WORD; /*2*/
  uint32_t logicalMergePage = EMPTY_DWORD; /*4*/
  uint32_t status = FTL_ERR_PASS; /*2*/
  uint16_t phyEBNum = 0; /*2*/
  uint32_t totalPages = 0; /*4*/
  uint32_t currentPageCount = 0; /*4*/
  uint32_t phyPageAddr = EMPTY_DWORD; /*4*/
  uint32_t mergePage = EMPTY_DWORD; /*4*/
  uint16_t freePageIndex = EMPTY_WORD; /*2*/
  uint32_t currentLBA = 0; /*4*/
  ADDRESS_STRUCT endPage = {0, 0, 0}; /*6*/
  ADDRESS_STRUCT startPage = {0, 0, 0}; /*6*/
  ADDRESS_STRUCT currentPage = {0, 0, 0}; /*6*/
  uint16_t tempPhyEBNum = 0; /*2*/
  uint16_t tempPhyEBNumMerge = 0; /*2*/

#if(FTL_EBLOCK_CHAINING == true)
  uint16_t chainEBNum = EMPTY_WORD; /*2*/
  CHAIN_INFO chainInfo = {0, 0, 0, 0, 0}; /*10*/
  uint16_t chainPPA = EMPTY_WORD; /*2*/
  uint16_t orginalEBSpace = 0; /*2*/
  uint16_t prevLogEB = EMPTY_WORD; /*2*/
#endif  // #if(FTL_EBLOCK_CHAINING == true)

#if (DEBUG_FTL_API_ANNOUNCE == 1)
  DBG_Printf("FTL_BuildTransferMapForWriteBlocking: LBA = 0x%X, ", LBA, 0);
  DBG_Printf("NB = %d\n", NB, 0);
#endif  // #if DEBUG_GC_ANNOUNCE

  if((status = TRANS_ClearTransMap()) != FTL_ERR_PASS)
    {
    return status;
    }
#if(FTL_DEFECT_MANAGEMENT == true)
  ClearBadBlockInfo();
#endif
  if((status = GetPageSpread(LBA, NB, &startPage, &totalPages, &endPage)) != FTL_ERR_PASS)
    {
    return status;
    }
  SetTransferEB(&startPage, &endPage);
  if((status = FTL_CheckForFreePages(&startPage, &endPage, totalPages)) != FTL_ERR_PASS)
    {
    return status;
    }

#if(FTL_DEFECT_MANAGEMENT == true)
  if((badBlockInfo.devID == EMPTY_BYTE) && (badBlockInfo.sourceLogicalEBNum == EMPTY_WORD))
    {
    /* EB Chaining is not happened */
    /* With defect Management turned on, the writes will span a write, so storing it once is enough.
    also cannot store it the 'for' loop below, since the table info is will be updated with each interation
     */
    if((status = GetLogicalEBNum(startPage.logicalPageNum, &logicalEBNum)) != FTL_ERR_PASS)
      {
      return status;
      }
#if(FTL_EBLOCK_CHAINING == true)
    // check if chain is present, if it is, then switch Source to chain to block.
    chainEBNum = GetChainLogicalEBNum(currentPage.devID, logicalEBNum);
    if(chainEBNum != EMPTY_WORD)
      { // not chained, lets chain it and update spaceCheck

      StoreSourceBadBlockInfo(startPage.devID, chainEBNum, FTL_BAD_BLOCK_WRITING);
      StoreTargetBadBlockInfo(startPage.devID, logicalEBNum, FTL_BAD_BLOCK_WRITING);
      }
    else
#endif
      {
      StoreSourceBadBlockInfo(startPage.devID, logicalEBNum, FTL_BAD_BLOCK_WRITING);
      // target info is not needed for tranferring pages, so that it should be cleared.
      ClearTargetBadBlockInfo();
      }
    }
#endif       

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
#if DEBUG_CHECK_TABLES
    status = DBG_CheckPPAandBitMap(currentPage.devID, logicalEBNum);
    if(FTL_ERR_PASS != status)
      {
      return status;
      }
#endif  // #if DBG_CHECK_TABLES

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
    if((status = GetPhyPageAddr(/*IN*/&currentPage, phyEBNum, logicalEBNum,
                                &phyPageAddr)) != FTL_ERR_PASS)
      {
      return status;
      }

#if(FTL_EBLOCK_CHAINING == true)
    chainEBNum = GetChainLogicalEBNum(currentPage.devID, logicalEBNum);
    if(chainEBNum != EMPTY_WORD)
      {
      if(prevLogEB != logicalEBNum) /*gaurenteed to pass once, since prevLogEB is set to EMPTY_WORD*/
        {
        orginalEBSpace = NUM_PAGES_PER_EBLOCK - GetNumFreePages(currentPage.devID, logicalEBNum);
        orginalEBSpace++;
        prevLogEB = logicalEBNum;
        }
      else
        {
        orginalEBSpace++;
        }
      if(orginalEBSpace > (NUM_PAGES_PER_EBLOCK - GC_THRESHOLD))
        {
        chainInfo.isChained = true;
        chainInfo.logChainToEB = chainEBNum;
        chainInfo.phyChainToEB = GetChainPhyEBNum(currentPage.devID, logicalEBNum);
        }
      else
        {
        chainInfo.isChained = false;
        }
      // this is a chained EB, lets check teh phyPageAddr
      if(phyPageAddr == PAGE_CHAINED)
        {
        chainInfo.isChained = true;
        chainInfo.logChainToEB = chainEBNum;
        chainInfo.phyChainToEB = GetChainPhyEBNum(currentPage.devID, logicalEBNum);
        if((status = GetPhyPageAddr(/*IN*/&currentPage, chainInfo.phyChainToEB, chainInfo.logChainToEB,
                                    &(chainInfo.phyPageAddr))) != FTL_ERR_PASS)
          {
          return status;
          }
        phyPageAddr = chainInfo.phyPageAddr;
        }
      }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

    /*need this regardless*/

#if(FTL_EBLOCK_CHAINING == true)
    if(chainInfo.isChained == true)
      {
      freePageIndex = GetFreePageIndex(currentPage.devID, chainInfo.logChainToEB);

#if (ENABLE_EB_ERASED_BIT == true)
      SetEBErased(currentPage.devID, chainInfo.logChainToEB, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

      }
    else
#endif  // #if(FTL_EBLOCK_CHAINING == true)

      {
      freePageIndex = GetFreePageIndex(currentPage.devID, logicalEBNum);

#if (ENABLE_EB_ERASED_BIT == true)
      SetEBErased(currentPage.devID, logicalEBNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

      }
    if(phyPageAddr == EMPTY_DWORD)
      { /*page not found*/

#if(FTL_EBLOCK_CHAINING == true)
      if(chainInfo.isChained == true)
        {
        phyPageAddr = CalcPhyPageAddrFromPageOffset(chainInfo.phyChainToEB, freePageIndex);
        }
      else
#endif  // #if(FTL_EBLOCK_CHAINING == true)

        {
        phyPageAddr = CalcPhyPageAddrFromPageOffset(phyEBNum, freePageIndex);
        }
      if(GC_Info.devID != EMPTY_BYTE)
        //this is the merge case, the page will not be found in the new EB since GC will not copy
        //the page over
        {
        if((GC_Info.startMerge.logicalPageNum == currentPage.logicalPageNum)
           && (GC_Info.startMerge.DevID == currentPage.devID)
           && (GC_Info.startMerge.logicalEBNum == logicalEBNum))
          {
          mergePage = GC_Info.startMerge.phyPageNum;
          if((status = ClearMergeGC_Info(currentPage.devID, logicalEBNum, currentPage.logicalPageNum)) != FTL_ERR_PASS)
            {
            return status;
            }
          }
        else
          {
          if((GC_Info.endMerge.logicalPageNum == currentPage.logicalPageNum)
             && (GC_Info.endMerge.DevID == currentPage.devID)
             && (GC_Info.endMerge.logicalEBNum == logicalEBNum))
            {
            mergePage = GC_Info.endMerge.phyPageNum;
            if((status = ClearMergeGC_Info(currentPage.devID, logicalEBNum, currentPage.logicalPageNum)) != FTL_ERR_PASS)
              {
              return status;
              }
            }
          }
        }
      }
    else
      {
      /*found a page*/
      mergePage = phyPageAddr;

#if(FTL_EBLOCK_CHAINING == true)
      if(chainInfo.isChained == true)
        {
        phyPageAddr = CalcPhyPageAddrFromPageOffset(chainInfo.phyChainToEB, freePageIndex);
        }
      else
#endif  // #if(FTL_EBLOCK_CHAINING == true)

        {
        phyPageAddr = CalcPhyPageAddrFromPageOffset(phyEBNum, freePageIndex);
        }
      }

#if(FTL_EBLOCK_CHAINING == true)
    if((status = UpdateTransferMap(currentLBA, &currentPage, &endPage, &startPage,
                                   totalPages, phyPageAddr, mergePage, true, chainInfo.isChained)) != FTL_ERR_PASS)

#else  // #if(FTL_EBLOCK_CHAINING == true)
    if((status = UpdateTransferMap(currentLBA, &currentPage, &endPage, &startPage,
                                   totalPages, phyPageAddr, mergePage, true, false)) != FTL_ERR_PASS)
#endif  // #else  // #if(FTL_EBLOCK_CHAINING == true)

      {
      return status;
      }
    if(mergePage != EMPTY_DWORD)// mark the merge page as stale
      {
      // make stale only if the pages are in the same eblock, if in diffent eblock, GC has already marked this a EMPTY
      // and the BuildTransferMapForWriteBlocking may have marked it as used.
      GetLogicalEBNum(phyPageAddr, &tempPhyEBNum);
      GetLogicalEBNum(mergePage, &tempPhyEBNumMerge);
      if(tempPhyEBNum == tempPhyEBNumMerge)
        {
        if((status = GetPageNum(currentLBA, &logicalMergePage)) != FTL_ERR_PASS) // the LBA should mathc
          {
          return status;
          }
        if((status = GetLogicalEBNum(logicalMergePage, &logicalMergeEB)) != FTL_ERR_PASS)
          {
          return status;
          }

#if(FTL_EBLOCK_CHAINING == true)
        if(chainInfo.isChained == true)
          { /*mark the new page valid*/
          UpdatePageTableInfo(currentPage.devID, chainInfo.logChainToEB, EMPTY_INVALID,
                              GetIndexFromPhyPage(mergePage), BLOCK_INFO_STALE_PAGE);
          }
        else
#endif  // #if(FTL_EBLOCK_CHAINING == true)

          {
          UpdatePageTableInfo(currentPage.devID, logicalMergeEB, EMPTY_INVALID,
                              GetIndexFromPhyPage(mergePage), BLOCK_INFO_STALE_PAGE);
          }
        }
      }
    currentLBA = currentLBA + (pageLoopCount - currentPage.pageOffset);
    /*Mark the current page as being used*/
    if((status = GetLogicalPageOffset(currentPage.logicalPageNum, &logicalPageOffset)) != FTL_ERR_PASS)
      {
      return status;
      }

#if(FTL_EBLOCK_CHAINING == true)
    if(chainInfo.isChained == true)
      { /*mark the new page valid*/
      UpdatePageTableInfo(currentPage.devID, chainInfo.logChainToEB, logicalPageOffset,
                          freePageIndex, BLOCK_INFO_VALID_PAGE);
      // have to mark the old page CHAIN_INVALID, 
      //1) when merge page is pointing into the old eblock, and 
      //2) even when writing a full page, have to mark the logical slot os CHAIN_INVALID
#if (NUMBER_OF_PAGES_PER_EBLOCK < 256)
      chainPPA = GetPPASlot(currentPage.devID, logicalEBNum, (uint8_t) logicalPageOffset);
#else
      chainPPA = GetPPASlot(currentPage.devID, logicalEBNum, (uint16_t) logicalPageOffset);
#endif
      if(chainPPA != CHAIN_INVALID)
        {
        UpdatePageTableInfo(currentPage.devID, logicalEBNum, logicalPageOffset,
                            CHAIN_INVALID, BLOCK_INFO_STALE_PAGE);
        }
      }
    else
#endif  // #if(FTL_EBLOCK_CHAINING == true)

      {
      UpdatePageTableInfo(currentPage.devID, logicalEBNum, logicalPageOffset,
                          freePageIndex, BLOCK_INFO_VALID_PAGE);
      }
    /*temp use of the free Page index*/
    mergePage = EMPTY_DWORD; /*if mergePage was used set it back to empty, checking and then setting will take more time*/
    /*Done writing the page, lets see if we need to garbage collect*/
    if((status = IncPageAddr(&currentPage)) != FTL_ERR_PASS)
      {
      return status;
      }

#if DEBUG_CHECK_TABLES
    status = DBG_CheckPPAandBitMap(currentPage.devID, logicalEBNum);
    if(FTL_ERR_PASS != status)
      {
      return status;
      }
#endif  // #if DBG_CHECK_TABLES

    } //end of for
  *resultingEntries = currentPageCount;
  return FTL_ERR_PASS;
  }

//------------------------------------------------------------------------

FTL_STATUS FTL_TransferPageForWrite(uint8_t * *byteBuffer, uint32_t * remainingEntries) /*4,4*/
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t currentTransfer = 0; /*2*/
  uint16_t startIndex = 0; /*2*/
  uint16_t endIndex = 0; /*2*/
  uint8_t numSectors = 0; /*1*/
  FTL_DEV deviceNum = 0; /*1*/
  uint8_t dstStartSector = 0; /*1*/
  uint16_t count = 0; /*2*/
  uint32_t currentLBA = 0; /*4*/
  uint32_t physicalPage = 0; /*4*/
  uint32_t mergePage = EMPTY_DWORD; /*4*/
  uint8_t * dataBuf = *byteBuffer; /*4*/
  FLASH_PAGE_INFO flashPageInfo = {0, 0,
    {0, 0}}; /*11*/
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  uint8_t sectorCount = 0; /*1*/
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  SPARE_INFO spareInfo; /*16*/
  SPARE_LOG_ENTRY spareLog; /*16*/
#if (SPANSCRC32 == true)
  uint32_t crc32 = 0; /*4*/
#endif
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

  if((status = FTL_GetNextTransferMapEntry(&currentTransfer, &startIndex, &endIndex)) != FTL_ERR_PASS)
    {
    return status;
    }
  dstStartSector = GetTMStartSector(currentTransfer);
  numSectors = GetTMNumSectors(currentTransfer);
  currentLBA = GetTMStartLBA(currentTransfer);
  deviceNum = GetTMDevID(currentTransfer);
  physicalPage = GetTMPhyPage(currentTransfer);
  mergePage = GetTMMergePage(currentTransfer);

#if (DEBUG_FTL_API_ANNOUNCE == 1)
  DBG_Printf("FTL_TransferPageForWrite: LBA = 0x%X, ", currentLBA, 0);
  DBG_Printf("deviceNum = %d, ", deviceNum, 0);
  DBG_Printf("phyPage = 0x%X,\n  ", physicalPage, 0);
  DBG_Printf("startSec = %d, ", dstStartSector, 0);
  DBG_Printf("numSecs = %d, ", numSectors, 0);
  DBG_Printf("mergePage = 0x%X\n", mergePage, 0);
#endif  // #if (DEBUG_FTL_API_ANNOUNCE == 1)

  if(currentTransfer == 0) /*write the log info for the first transfer*/
    {
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    /*Write the type A log info in first*/
    /*get a log info pointer*/
    if((status = GetNextLogEntryLocation(deviceNum, &flashPageInfo)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = FTL_WriteLogInfo(&flashPageInfo, (uint8_t *) & TransLogEntry.entryA)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = FTL_ClearA()) != FTL_ERR_PASS)
      {
      return status;
      }
    /*Write the type B log info in second*/
    if(TranslogBEntries > TEMP_B_ENTRIES)
      {
      return FTL_ERR_LOG_TOO_MANY_TYPE_B;
      }
    for(count = 0; count < TranslogBEntries; count++)
      {
      if((status = GetNextLogEntryLocation(deviceNum, &flashPageInfo)) != FTL_ERR_PASS)
        {
        return status;
        }
      if((status = FTL_WriteLogInfo(&flashPageInfo, (uint8_t *)&(TransLogEntry.entryB[count]))) != FTL_ERR_PASS)
        {
        return status;
        }
      if((status = FTL_ClearB(count)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
    TranslogBEntries = 0; /*done writing*/
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  if((status = GetPageNum(currentLBA, &spareInfo.logicalPageAddr)) != FTL_ERR_PASS)
    {
    return status;
    }
  for(count = 0; count < SPARE_INFO_RESERVED; count++)
    {
    spareInfo.reserved[count] = EMPTY_BYTE;
    }
  spareInfo.pad = EMPTY_WORD;
  spareLog.type = SPARE_LOG_TYPE;
  if((status = GetLogicalEBNum(spareInfo.logicalPageAddr, &spareLog.logicalEBNum)) != FTL_ERR_PASS)
    {
    return status;
    }
  for(count = 0; count < SPARE_LOG_ENTRY_RESERVED; count++)
    {
    spareLog.reserved[count] = EMPTY_BYTE;
    }
  if(writeLogFlag == false)
    {
    /*Write the sparelog*/
    if((status = GetNextLogEntryLocation(deviceNum, &flashPageInfo)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = FTL_WriteLogInfo(&flashPageInfo, (uint8_t *) & spareLog)) != FTL_ERR_PASS)
      {
      return status;
      }
    }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

  if(numSectors == NUM_SECTORS_PER_PAGE)
    {
    flashPageInfo.devID = deviceNum;
    flashPageInfo.vPage.vPageAddr = physicalPage;

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    flashPageInfo.byteCount = SECTOR_SIZE;
    for(sectorCount = 0; sectorCount < NUM_SECTORS_PER_PAGE; sectorCount++, dataBuf += SECTOR_SIZE)
      {
      if(FLASH_CheckEmpty(dataBuf, SECTOR_SIZE) == FLASH_PASS)
        {
        continue;
        }
      flashPageInfo.vPage.pageOffset = sectorCount * SECTOR_SIZE;
      if(FLASH_RamPageWriteDataBlock(&flashPageInfo, dataBuf) != FLASH_PASS)
        {
        return FTL_ERR_FLASH_WRITE_04;
        }
      }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    flashPageInfo.byteCount = VIRTUAL_PAGE_SIZE;
    flashPageInfo.vPage.pageOffset = 0;
    if((FLASH_RamPageWriteCMD(&flashPageInfo, dataBuf)) != FLASH_PASS)
      {
      return FTL_ERR_FLASH_WRITE_04;
      }
    flashPageInfo.byteCount = SPARE_INFO_SIZE;
    flashPageInfo.vPage.pageOffset = VIRTUAL_PAGE_SIZE;
#if (SPANSCRC32 == true)
    CalcCalculateCRC(dataBuf, &crc32);
    spareInfo.crc32 = crc32;
#endif
    if((status = FTL_WriteSpareInfo(&flashPageInfo, &spareInfo)) != FTL_ERR_PASS)
      {
      return status;
      }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

    }
    //TRANSFER PARTIAL PAGE CASE
  else
    {
    flashPageInfo.devID = deviceNum;
    /*first read the old page in if needed*/
    if(mergePage != EMPTY_DWORD)
      {
      flashPageInfo.byteCount = VIRTUAL_PAGE_SIZE;
      flashPageInfo.vPage.vPageAddr = mergePage;
      flashPageInfo.vPage.pageOffset = 0;
      if(FLASH_RamPageReadDataBlock(&flashPageInfo, &pseudoRPB[deviceNum][0]) != FLASH_PASS)
        {
        return FTL_ERR_TRANS_WR_FULL;
        }

      memcpy(&(pseudoRPB[deviceNum][SECTOR_SIZE * dstStartSector]), dataBuf, (numSectors * SECTOR_SIZE));

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
      flashPageInfo.vPage.vPageAddr = physicalPage;
      flashPageInfo.byteCount = SECTOR_SIZE;
      for(sectorCount = 0; sectorCount < NUM_SECTORS_PER_PAGE; sectorCount++)
        {
        if(FLASH_CheckEmpty(&pseudoRPB[deviceNum][sectorCount * SECTOR_SIZE], SECTOR_SIZE) == FLASH_PASS)
          {
          continue;
          }
        flashPageInfo.vPage.pageOffset = sectorCount * SECTOR_SIZE;
        if(FLASH_RamPageWriteDataBlock(&flashPageInfo, &pseudoRPB[deviceNum][sectorCount * SECTOR_SIZE]) != FLASH_PASS)
          {
          return FTL_ERR_FLASH_COMMIT_02;
          }
        }
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
      flashPageInfo.vPage.vPageAddr = physicalPage;
      flashPageInfo.vPage.pageOffset = 0;
      flashPageInfo.byteCount = VIRTUAL_PAGE_SIZE;
#if (SPANSCRC32 == true)
      CalcCalculateCRC(&pseudoRPB[deviceNum][0], &crc32);
      spareInfo.crc32 = crc32;
#endif
      if((FLASH_RamPageWriteCMD(&flashPageInfo, &pseudoRPB[deviceNum][0])) != FLASH_PASS)
        {
        return FTL_ERR_FLASH_COMMIT_02;
        }
      flashPageInfo.byteCount = SPARE_INFO_SIZE;
      flashPageInfo.vPage.pageOffset = VIRTUAL_PAGE_SIZE;
      if((status = FTL_WriteSpareInfo(&flashPageInfo, &spareInfo)) != FTL_ERR_PASS)
        {
        return status;
        }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

      }
    else
      {
      flashPageInfo.byteCount = numSectors * SECTOR_SIZE;
      flashPageInfo.vPage.vPageAddr = physicalPage;
      flashPageInfo.vPage.pageOffset = dstStartSector * SECTOR_SIZE;

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
      if(FLASH_RamPageWriteDataBlock(&flashPageInfo, dataBuf) != FLASH_PASS)
        {
        return FTL_ERR_FLASH_WRITE_17;
        }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
      // calculate CRC
#if (SPANSCRC32 == true)
      Init_PseudoRPB();
      memcpy(&(pseudoRPB[deviceNum][SECTOR_SIZE * dstStartSector]), dataBuf, (numSectors * SECTOR_SIZE));
      CalcCalculateCRC(&pseudoRPB[deviceNum][0], &crc32);
      spareInfo.crc32 = crc32;
#endif
      if((FLASH_RamPageWriteCMD(&flashPageInfo, dataBuf)) != FLASH_PASS)
        {
        return FTL_ERR_FLASH_WRITE_17;
        }
      flashPageInfo.byteCount = SPARE_INFO_SIZE;
      flashPageInfo.vPage.pageOffset = VIRTUAL_PAGE_SIZE;
      if((status = FTL_WriteSpareInfo(&flashPageInfo, &spareInfo)) != FTL_ERR_PASS)
        {
        return status;
        }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

      }
    }

  (*byteBuffer) += (numSectors * SECTOR_SIZE);
  *remainingEntries = endIndex - startIndex;
  if(0 == *remainingEntries)
    {
    ClearTransferEB();
    FTL_ClearGCSave(CLEAR_GC_SAVE_RUNTIME_MODE);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    /*Write the type C log info in last*/
    if((status = GetNextLogEntryLocation(deviceNum, &flashPageInfo)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = FTL_WriteLogInfo(&flashPageInfo, (uint8_t *) & TransLogEntry.entryC)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = FTL_ClearC(count)) != FTL_ERR_PASS)
      {
      return status;
      }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    if(GCMoveArrayNotEmpty == true)
      {
      /*Write the sparelog*/
      if((status = GetNextLogEntryLocation(deviceNum, &flashPageInfo)) != FTL_ERR_PASS)
        {
        return status;
        }
      if((status = FTL_WriteLogInfo(&flashPageInfo, (uint8_t *) & spareLog)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

    /*Clear GC_Info*/
    if(GC_Info.devID != EMPTY_BYTE)
      {
      if((status = ClearGC_Info()) != FTL_ERR_PASS)
        {
        return status;
        }
      }

    status = ClearGCPageBitMap();
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
    }
  return FTL_ERR_PASS;
  }

//----------------------------------------------------------------

uint16_t FTL_GetNumLogEntries(ADDRESS_STRUCT_PTR startPage, uint32_t totalPages)
  {
  int16_t numEB = 0; /*2*/
  uint16_t currentPageCount = 0; /*2*/
  uint16_t logicalEBNum = 0; /*2*/
  uint16_t spaceCheckEBNum = 0; /*2*/
  uint16_t spaceCheck[2] = {0, 0}; /*4*/
  uint16_t requestEBNum[2] = {0, 0}; /*4*/
  FTL_DEV requestDevID[2] = {0, 0}; /*2*/
  uint16_t logicalEblockChange = EMPTY_WORD; /*2*/
  uint16_t numLogEntries = 0; /*2*/
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  ADDRESS_STRUCT currentPage = {0, 0, 0}; /*6*/
#if(FTL_EBLOCK_CHAINING == true)
  uint16_t chainEBNum = 0; /*2*/
#endif

  for(numEB = 0; numEB < 2; numEB++)
    {
    spaceCheck[numEB] = 0;
    requestEBNum[numEB] = EMPTY_WORD;
    }

  numEB = -1;
  logicalEblockChange = EMPTY_WORD;
  currentPage = *startPage;

  for(currentPageCount = 0; currentPageCount < totalPages; currentPageCount++)
    {
    if((status = GetLogicalEBNum(currentPage.logicalPageNum, &logicalEBNum)) != FTL_ERR_PASS)
      {
      return EMPTY_WORD;
      }

    spaceCheckEBNum = logicalEBNum;
#if(FTL_EBLOCK_CHAINING == true)
    chainEBNum = GetChainLogicalEBNum(currentPage.devID, logicalEBNum);
    if(chainEBNum != EMPTY_WORD)
      {
      spaceCheckEBNum = chainEBNum;
      }
#endif // #if(FTL_EBLOCK_CHAINING == true)

    if(logicalEblockChange == spaceCheckEBNum)
      {
      spaceCheck[numEB]++;
      }
    else
      {
      numEB++;
      spaceCheck[numEB] = NUM_PAGES_PER_EBLOCK - GetNumFreePages(currentPage.devID, spaceCheckEBNum);
      spaceCheck[numEB]++;
      logicalEblockChange = spaceCheckEBNum;
      requestEBNum[numEB] = logicalEBNum;
      requestDevID[numEB] = currentPage.devID;
      }

    if((status = IncPageAddr(&currentPage)) != FTL_ERR_PASS)
      {
      return EMPTY_WORD;
      }
    }

  if(numEB >= 2)
    {
    //DBG_Printf("num of EB is out of range %d\n", numEB, 0);
    return EMPTY_WORD;
    }

  /* check for trans log entry */
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  numLogEntries = CalcNumLogEntries(totalPages);
  if(EMPTY_WORD == numLogEntries)
    {
    return EMPTY_WORD;
    }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  numLogEntries = 1;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

  for(logicalEBNum = 0; logicalEBNum <= numEB; logicalEBNum++)
    {
    if(spaceCheck[logicalEBNum] > NUM_PAGES_PER_EBLOCK - GC_THRESHOLD)
      {
#if(FTL_EBLOCK_CHAINING == true)
      chainEBNum = GetChainLogicalEBNum(requestDevID[logicalEBNum], requestEBNum[logicalEBNum]);
      if(EMPTY_WORD == chainEBNum)
        {
        /* for chain log entry */
        numLogEntries++;
        }
#endif
      /* for GC log entry */
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
      numLogEntries += (NUM_GC_TYPE_B + 1);

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
      numLogEntries += 2;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

#if(FTL_ENABLE_UNUSED_EB_SWAP  == true)
      /* for EB swap entry */
      numLogEntries++;
#if(FTL_EBLOCK_CHAINING == true)
      numLogEntries++;
#endif  // #if(FTL_EBLOCK_CHAINING == true)
#endif  // #if(FTL_ENABLE_UNUSED_EB_SWAP  == true)

      }
    }
  return numLogEntries;
  }

//----------------------------------------------------------------

FTL_STATUS FTL_CheckForFreePages(ADDRESS_STRUCT_PTR startPage, ADDRESS_STRUCT_PTR endPage, uint32_t totalPages)
  {
  /*needed for space check*/
  ADDRESS_STRUCT currentPage = {0, 0, 0}; /*6*/
  ADDRESS_STRUCT tempPage = {0, 0, 0}; /*6*/
  uint16_t currentPageCount = 0; /*2*/
  uint16_t spaceCheck[NUM_DEVICES]; /*2*/
  uint16_t logicalEblockChange[NUM_DEVICES]; /*2*/
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/
  uint16_t logLogicalEBNum = EMPTY_WORD; /*2*/
  uint16_t logPhyEBAddr = EMPTY_WORD; /*2*/
  uint32_t key = 0; /*4*/
  uint32_t phyPage = 0; /*4*/
  uint16_t templogicalEBNum = EMPTY_WORD; /*2*/
  uint16_t numLogEntries = 0; /*2*/
  uint16_t currentLogIndex = 0; /*2*/

#if(FTL_EBLOCK_CHAINING == true)
  uint16_t chainEBNum = 0; /*2*/
  uint16_t chainEBLogNum = EMPTY_WORD; /*2*/
  uint16_t chainEBPhyNum = EMPTY_WORD; /*2*/
  uint16_t chainTrack = EMPTY_WORD; /*2*/
  CHAIN_INFO chainInfo = {0, 0, 0, 0, 0}; /*10*/
#endif

  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t pageOffset = 0; /*2*/
  GC_MERGE_TEMP_STRUCT startMerge = {0, 0}; /*2*/
  GC_MERGE_TEMP_STRUCT endMerge = {0, 0}; /*2*/

  /*Check to see we have space in the flash. Basically run through a simulated write operation*/
  for(currentPageCount = 0; currentPageCount < NUM_DEVICES; currentPageCount++)
    {
    spaceCheck[currentPageCount] = 0;
    logicalEblockChange[currentPageCount] = EMPTY_WORD;
    }
  currentPage = *startPage;
  /*check for log space*/
  numLogEntries = FTL_GetNumLogEntries(startPage, totalPages);
  if(EMPTY_WORD == numLogEntries)
    {
    return FTL_ERR_LOG_TOO_MANY_ENTRIES;
    }
  key = GetTransLogEBCounter(currentPage.devID);
  if((status = TABLE_TransLogEBGetLatest(currentPage.devID, &logLogicalEBNum,
                                         &logPhyEBAddr, key)) != FTL_ERR_PASS)
    {
    return status;
    }
  currentLogIndex = GetFreePageIndex(currentPage.devID, logLogicalEBNum);
  if((currentLogIndex + numLogEntries) > NUM_LOG_ENTRIES_PER_EBLOCK)
    {
#if(FTL_DEFECT_MANAGEMENT == true)
    if(GetTransLogEBArrayCount(currentPage.devID) < (NUM_TRANSACTLOG_EBLOCKS - LOG_EBLOCK_RETRIES))

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    if(GetTransLogEBArrayCount(currentPage.devID) < NUM_TRANSACTLOG_EBLOCKS)
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)
      {
      if((status = CreateNextTransLogEBlock(currentPage.devID, logLogicalEBNum)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
    else
      {
      if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
    }
  /*end log space check*/
  for(currentPageCount = 0; currentPageCount < totalPages; currentPageCount++)
    {
    if((status = GetLogicalEBNum(currentPage.logicalPageNum, &logicalEBNum)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(spaceCheck[currentPage.devID] == 0)
      {
      spaceCheck[currentPage.devID] = NUM_PAGES_PER_EBLOCK - GetNumFreePages(currentPage.devID, logicalEBNum);
      spaceCheck[currentPage.devID]++;
      logicalEblockChange[currentPage.devID] = logicalEBNum;
      }
    else
      {
      if(logicalEblockChange[currentPage.devID] == logicalEBNum)
        {
        spaceCheck[currentPage.devID]++;
        }
      else
        {
        spaceCheck[currentPage.devID] = NUM_PAGES_PER_EBLOCK - GetNumFreePages(currentPage.devID, logicalEBNum);
        spaceCheck[currentPage.devID]++;
        logicalEblockChange[currentPage.devID] = logicalEBNum;
        }
      }

#if(FTL_EBLOCK_CHAINING == true)
    if(spaceCheck[currentPage.devID] > NUM_PAGES_PER_EBLOCK - GC_THRESHOLD)
      {

#if DEBUG_SPACE_CHECK
      DBG_Printf("FTL_CheckForFreePages: EBChain spaceCheck = 0x%X\n", spaceCheck[currentPage.devID], 0);
#endif  // DEBUG_SPACE_CHECK

      // see if the EB has been chained, if not lets chain it
      if(chainTrack == logicalEBNum)
        {
        //already been here, the chain is full as well, so do nothing, just fall through to the next spaceCheck if
        }
      else
        {
        chainEBNum = GetChainLogicalEBNum(currentPage.devID, logicalEBNum);
        if(chainEBNum == EMPTY_WORD)
          { // not chained, lets chain it and update spaceCheck

#if DEBUG_SPACE_CHECK
          DBG_Printf("FTL_CheckForFreePages: make chain\n", 0, 0);
#endif  // DEBUG_SPACE_CHECK

          chainEBLogNum = logicalEBNum;
          chainEBPhyNum = GetPhysicalEBlockAddr(currentPage.devID, logicalEBNum);
          if((status = FTL_SwapDataReserveEBlock(currentPage.devID, EMPTY_WORD,
                                                 &chainEBPhyNum, &chainEBLogNum, false, false)) != FTL_ERR_PASS)
            {
            if(status == FTL_ERR_ECHAIN_GC_NEEDED)
              {
              // no more EBs left to chain, have to do a GC, so lets do a GC to free up a chain EB.
              // find the EB with the longest chain.  

#if DEBUG_SPACE_CHECK
              DBG_Printf("FTL_CheckForFreePages: No EBs left to chain\n", 0, 0);
#endif  // DEBUG_SPACE_CHECK

              GC_Info.devID = currentPage.devID;
              GC_Info.logicalEBlock = GetChainWithLowestVaildUsedRatio(currentPage.devID);
              if(GC_Info.logicalEBlock != EMPTY_WORD)
                {
                if((status = ClearGCPageBitMap()) != FTL_ERR_PASS)
                  {
                  return status;
                  }
                return FTL_ERR_DATA_GC_NEEDED;
                }
              else
                {
                if((status = ClearGC_Info()) != FTL_ERR_PASS)
                  {
                  return status;
                  }
                }
              }
            else
              {
              return status;
              }
            }
          else
            {

#if DEBUG_SPACE_CHECK
            DBG_Printf("FTL_CheckForFreePages: Setup Chain logical EB 0x%X, ", logicalEBNum, 0);
            DBG_Printf("to logical EB 0x%X\n", chainEBLogNum, 0);
#endif  // DEBUG_SPACE_CHECK

            spaceCheck[currentPage.devID] = NUM_PAGES_PER_EBLOCK -
              GetNumFreePages(currentPage.devID, chainEBLogNum);
            spaceCheck[currentPage.devID]++;
            chainTrack = logicalEBNum;
            }
          }
        else
          { // just update space check for the chain EB
          spaceCheck[currentPage.devID] = NUM_PAGES_PER_EBLOCK -
            GetNumFreePages(currentPage.devID, chainEBNum);
          spaceCheck[currentPage.devID]++;
          chainTrack = logicalEBNum;

#if DEBUG_SPACE_CHECK
          DBG_Printf("FTL_CheckForFreePages: switch to chain EB 0x%X, ", chainEBNum, 0);
          DBG_Printf("new spaceCheck =  0x%X\n", spaceCheck[currentPage.devID], 0);
#endif  // DEBUG_SPACE_CHECK

          }
        }
      }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

    if(spaceCheck[currentPage.devID] > NUM_PAGES_PER_EBLOCK - GC_THRESHOLD)
      {
      // Data EBlock has overflowed

#if DEBUG_SPACE_CHECK
      DBG_Printf("FTL_CheckForFreePages: Overflow spaceCheck = 0x%X\n", spaceCheck[currentPage.devID], 0);
#endif  // DEBUG_SPACE_CHECK

      /*store the GC info now, will be needed in data GC later*/
      GC_Info.devID = currentPage.devID;
      GC_Info.logicalEBlock = logicalEBNum;
      /*Start SRA Addding numsectors update*/
      tempPage = currentPage;
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
          tempPage = currentPage;
          }
        }
      if((status = GetLogicalEBNum(startPage->logicalPageNum, &templogicalEBNum)) != FTL_ERR_PASS)
        {
        return status;
        }
      if((currentPage.devID == startPage->devID) && (logicalEBNum == templogicalEBNum))
        { /*this is the start page, and it lies in the EB to be GC'ed, so have to move it too*/
        if((totalPages != 1) || (endPage->pageOffset == 0))
          { /*multi page transfer*/
          startMerge.numSectors = NUM_SECTORS_PER_PAGE - startPage->pageOffset;
          }
        else
          {
          /*single page transfer*/
          startMerge.numSectors = endPage->pageOffset - startPage->pageOffset;
          }
        if(startMerge.numSectors != NUM_SECTORS_PER_PAGE)
          { /*numSectors is not full page in size, then we have to merge, so update*/
          GC_Info.startMerge.logicalPageNum = startPage->logicalPageNum;
          GC_Info.startMerge.DevID = currentPage.devID;
          GC_Info.startMerge.logicalEBNum = logicalEBNum;
          status = GetLogicalPageOffset(GC_Info.startMerge.logicalPageNum, &pageOffset);
          if(FTL_ERR_PASS != status)
            {
            return status;
            }
          currentPage.logicalPageNum = GC_Info.startMerge.logicalPageNum;
          currentPage.pageOffset = (uint8_t) pageOffset;
          status = GetPhyPageAddr(&currentPage, GetPhysicalEBlockAddr(currentPage.devID, logicalEBNum),
                                  logicalEBNum, &phyPage);
          if(FTL_ERR_PASS != status)
            {
            return status;
            }

#if(FTL_EBLOCK_CHAINING == true)
          chainEBNum = GetChainLogicalEBNum(currentPage.devID, logicalEBNum);
          if(chainEBNum != EMPTY_WORD)
            {

#if DEBUG_SPACE_CHECK
            DBG_Printf("FTL_CheckForFreePages: (1) Merge logical EB 0x%X, ", logicalEBNum, 0);
            DBG_Printf("and chain EB 0x%X\n", chainEBNum, 0);
#endif  // DEBUG_SPACE_CHECK

            chainInfo.isChained = true;
            chainInfo.logChainToEB = chainEBNum;
            chainInfo.phyChainToEB = GetChainPhyEBNum(currentPage.devID, logicalEBNum);
            // this is a chained EB, lets check teh phyPageAddr
            if(phyPage == PAGE_CHAINED)
              {
              if((status = GetPhyPageAddr(/*IN*/&currentPage, chainInfo.phyChainToEB, chainInfo.logChainToEB,
                                          &(chainInfo.phyPageAddr))) != FTL_ERR_PASS)
                {
                return status;
                }
              phyPage = chainInfo.phyPageAddr;
              }
            }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

          GC_Info.startMerge.phyPageNum = phyPage;
          }
        }
      if((status = GetLogicalEBNum(endPage->logicalPageNum, &templogicalEBNum)) != FTL_ERR_PASS)
        {
        return status;
        }
      if((tempPage.devID == endPage->devID) &&
         (logicalEBNum == templogicalEBNum) && (totalPages > 1)) /*totalPage > 1 will make end
                page be updated only on multipage updates, as the start page takes care of singel page updates*/
        { /*This is the end page, and it lies in the EB to be GC'ed. So have to move it too*/
        if(endPage->pageOffset == 0)
          {
          endMerge.numSectors = NUM_SECTORS_PER_PAGE;
          }
        else
          {
          if(totalPages != 1)
            { /*multi page transfer*/
            endMerge.numSectors = endPage->pageOffset;
            }
          else
            {
            /*single page transfer*/
            endMerge.numSectors = endPage->pageOffset - startPage->pageOffset;
            }
          }
        if(endMerge.numSectors != NUM_SECTORS_PER_PAGE)
          { /*numSectors is not full page in size, then we have to merge, so update*/
          GC_Info.endMerge.logicalPageNum = endPage->logicalPageNum;
          GC_Info.endMerge.DevID = endPage->devID;
          GC_Info.endMerge.logicalEBNum = logicalEBNum;
          status = GetLogicalPageOffset(GC_Info.endMerge.logicalPageNum, &pageOffset);
          if(FTL_ERR_PASS != status)
            {
            return status;
            }
          currentPage.logicalPageNum = GC_Info.endMerge.logicalPageNum;
          currentPage.pageOffset = (uint8_t) pageOffset;
          status = GetPhyPageAddr(&currentPage, GetPhysicalEBlockAddr(currentPage.devID, logicalEBNum),
                                  logicalEBNum, &phyPage);
          if(FTL_ERR_PASS != status)
            {
            return status;
            }

#if(FTL_EBLOCK_CHAINING == true)
          chainEBNum = GetChainLogicalEBNum(currentPage.devID, logicalEBNum);
          if(chainEBNum != EMPTY_WORD)
            {

#if DEBUG_SPACE_CHECK
            DBG_Printf("FTL_CheckForFreePages: (2) Merge logical EB 0x%X, ", logicalEBNum, 0);
            DBG_Printf("and 0x%X\n", chainEBNum, 0);
#endif  // DEBUG_SPACE_CHECK

            chainInfo.isChained = true;
            chainInfo.logChainToEB = chainEBNum;
            chainInfo.phyChainToEB = GetChainPhyEBNum(currentPage.devID, logicalEBNum);
            // this is a chained EB, lets check teh phyPageAddr
            if(phyPage == PAGE_CHAINED)
              {
              if((status = GetPhyPageAddr(/*IN*/&currentPage, chainInfo.phyChainToEB, chainInfo.logChainToEB,
                                          &(chainInfo.phyPageAddr))) != FTL_ERR_PASS)
                {
                return status;
                }
              phyPage = chainInfo.phyPageAddr;
              }
            }
#endif // #if(FTL_EBLOCK_CHAINING == true)

          GC_Info.endMerge.phyPageNum = phyPage;
          }
        }
      // Clear GC Bit Map
      if((status = ClearGCPageBitMap()) != FTL_ERR_PASS)
        {
        return status;
        }
      // Check for pages being added or replaced
      currentPage = *startPage;
      for(currentPageCount = 0; currentPageCount < totalPages; currentPageCount++)
        {
        // only check for pages in the device selected for GC
        if(currentPage.devID == GC_Info.devID)
          {
          // only check for pages in the selected EBlock
          if((status = GetLogicalEBNum(currentPage.logicalPageNum, &logicalEBNum)) != FTL_ERR_PASS)
            {
            return status;
            }
          if(logicalEBNum == GC_Info.logicalEBlock)
            {
            // Calculate offset within EBlock of page being replaced or added
            if((status = GetLogicalPageOffset(currentPage.logicalPageNum, &pageOffset)) != FTL_ERR_PASS)
              {
              return status;
              }

#if(FTL_DEFECT_MANAGEMENT == true)
            status = GetPhyPageAddr(&currentPage, GetPhysicalEBlockAddr(currentPage.devID, logicalEBNum),
                                    logicalEBNum, &phyPage);
            if(FTL_ERR_PASS != status)
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
              if(phyPage == PAGE_CHAINED)
                {
                if((status = GetPhyPageAddr(/*IN*/&currentPage, chainInfo.phyChainToEB, chainInfo.logChainToEB,
                                            &(chainInfo.phyPageAddr))) != FTL_ERR_PASS)
                  {
                  return status;
                  }
                phyPage = chainInfo.phyPageAddr;
                }
              }
#endif // #if(FTL_EBLOCK_CHAINING == true)

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
            phyPage = EMPTY_DWORD;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

            // Set bit of logical page
            if((status = SetPageMoved(pageOffset, phyPage)) != FTL_ERR_PASS)
              {
              return status;
              }
            }
          }
        if((status = IncPageAddr(&currentPage)) != FTL_ERR_PASS)
          {
          return status;
          }
        }
      return FTL_ERR_DATA_GC_NEEDED;
      } // end:     if(spaceCheck[currentPage->devID] > NUM_PAGES_PER_EBLOCK - GC_THRESHOLD)
    if((status = IncPageAddr(&currentPage)) != FTL_ERR_PASS)
      {
      return status;
      }
    } //end of for
  return FTL_ERR_PASS;
  }
