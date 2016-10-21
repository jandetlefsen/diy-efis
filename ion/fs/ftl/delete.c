#include "common.h"
#include "if_in.h"
#include "if_ex.h"

//-------------------------------------------

FTL_STATUS FTL_DeviceObjectsDelete(uint32_t LBA, uint32_t NB, uint32_t * sectorsDeleted)
  {
#if (FTL_ENABLE_DELETE_API == true)
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t totalPages = 0; /*4*/
  uint32_t currentPageCount = 0; /*4*/
  uint32_t currentLogicalPageNum = 0; /*4*/
  ADDRESS_STRUCT endPage = {0, 0, 0}; /*6*/
  ADDRESS_STRUCT startPage = {0, 0, 0}; /*6*/
  ADDRESS_STRUCT currentPage = {0, 0, 0}; /*6*/
  uint16_t currentLogicalEBNum = 0; /*2*/
  uint16_t logicalPageOffset = 0; /*2*/
  bool isFullPage = false; /*1*/
  uint16_t tempPPA = EMPTY_WORD; /*1*/
  uint8_t numSectors = 0; /*1*/

#if(FTL_DELETE_WITH_GC == true)
  uint16_t logicalEBNum[NUM_DEVICES]; /*8*/
  uint16_t logicalGCEBNum = EMPTY_WORD; /*2*/
  uint16_t freedUpPages = EMPTY_WORD; /*2*/
  uint16_t freePageIndex = EMPTY_WORD; /*2*/
#endif  // #if(FTL_DELETE_WITH_GC == true)

#if(FTL_EBLOCK_CHAINING == true)
  uint16_t chainEBNum = 0; /*2*/
#endif  // #if(FTL_EBLOCK_CHAINING == true)

#if(FTL_SUPER_SYS_EBLOCK == true)
#if(FTL_DEFECT_MANAGEMENT == true)
  uint8_t devID = 0; /*1*/
  uint16_t sanityCounter = 0;
#endif
#endif  // #if(FTL_SUPER_SYS_EBLOCK == true)

#if (DEBUG_FTL_API_ANNOUNCE == 1)
  DBG_Printf("FTL_DeviceObjectsDelete: LBA = %d, ", LBA, 0);
  DBG_Printf("NB = %d, \n", NB, 0);

#endif  // #if (DEBUG_FTL_API_ANNOUNCE == 1)
#if (DEBUG_ENABLE_LOGGING == true)
  if((status = DEBUG_InsertLog(LBA, NB, DEBUG_LOG_DELETE)) != FTL_ERR_PASS)
    {
    return (status);
    }
#endif

  if((status = FTL_CheckRange(LBA, NB)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckPointer(sectorsDeleted)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckMount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }

#if (FTL_SUPER_SYS_EBLOCK == true)
#if(FTL_DEFECT_MANAGEMENT == true)
  for(devID = 0; devID < NUMBER_OF_DEVICES; devID++)
    {
    while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
      {
      if((status = FTL_CheckForSuperSysEBLogSpace(devID, SYS_EBLOCK_INFO_CHANGED)) != FTL_ERR_PASS)
        {
        return status;
        }
      if((status = FTL_CreateSuperSysEBLog(devID, SYS_EBLOCK_INFO_CHANGED)) == FTL_ERR_PASS)
        {
        break;
        }
      if(status != FTL_ERR_SUPER_WRITE_02)
        {
        return status;
        }
      sanityCounter++;
      }
    if(sanityCounter >= MAX_BAD_BLOCK_SANITY_TRIES)
      {
      return status;
      }
    }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

  if(NB > 0)
    {
    if((status = GetPageSpread(LBA, NB, &startPage, &totalPages, &endPage)) != FTL_ERR_PASS)
      {
      FTL_ClearMTLockBit();
      return status;
      }
    (*sectorsDeleted) = 0;
    currentPage = startPage;

    for(currentPageCount = 0; currentPageCount < totalPages; currentPageCount++)
      {
      currentLogicalPageNum = currentPage.logicalPageNum;
      if((status = GetLogicalEBNum(currentLogicalPageNum, &currentLogicalEBNum)) != FTL_ERR_PASS)
        {
        FTL_ClearMTLockBit();
        return status;
        }
      if((status = GetLogicalPageOffset(currentLogicalPageNum, &logicalPageOffset)) != FTL_ERR_PASS)
        {
        FTL_ClearMTLockBit();
        return status;
        }

      /* start page */
      if(currentPageCount == 0)
        {
        /* single page transfer */
        if(currentPageCount == (totalPages - 1))
          {
          if((currentPage.pageOffset == 0) && (endPage.pageOffset == 0))
            {
            isFullPage = true;
            }
          else
            {
            /*Don't delete the index because this is NOT a full page*/
            isFullPage = false;
            }
          numSectors = (uint8_t) NB;
          }
          /* multiple pages transfer */
        else
          {
          if(currentPage.pageOffset == 0)
            {
            isFullPage = true;
            }
          else
            {
            /*Don't delete the index because this is NOT a full page*/
            isFullPage = false;
            }
          numSectors = NUMBER_OF_SECTORS_PER_PAGE - currentPage.pageOffset;
          }

        if(isFullPage == false)
          {
          if(HitDeleteInfo(currentPage.devID, currentLogicalPageNum) == FTL_ERR_PASS)
            {
            if((status = UpdateDeleteInfo(currentPage.pageOffset, numSectors)) != FTL_ERR_PASS)
              {
              return status;
              }
            if(GetDeleteInfoNumSectors() == NUMBER_OF_SECTORS_PER_PAGE)
              {
              isFullPage = true;
              }
            }
          }

        if(isFullPage == true)
          {
          if((status = ClearDeleteInfo()) != FTL_ERR_PASS)
            {
            return status;
            }
          }
        else
          {
          if(currentPageCount == (totalPages - 1))
            {
            if(HitDeleteInfo(currentPage.devID, currentLogicalPageNum) != FTL_ERR_PASS)
              {
              if((status = ClearDeleteInfo()) != FTL_ERR_PASS)
                {
                return status;
                }
              status = InitDeleteInfo(currentPage.devID, currentLogicalPageNum,
                                      currentPage.pageOffset, numSectors);
              if(status != FTL_ERR_PASS)
                {
                return status;
                }
              }
            }
          else
            {
            if((status = ClearDeleteInfo()) != FTL_ERR_PASS)
              {
              return status;
              }
            }
          }
        }
        /* end page */
      else if(currentPageCount == (totalPages - 1))
        {
        if(endPage.pageOffset == 0)
          {
          isFullPage = true;
          }
        else
          {
          /*Don't delete the index because this is NOT a full page*/
          isFullPage = false;

          status = InitDeleteInfo(currentPage.devID, currentLogicalPageNum,
                                  currentPage.pageOffset, endPage.pageOffset);
          if(status != FTL_ERR_PASS)
            {
            return status;
            }
          }
        }
        /* middle page */
      else
        {
        isFullPage = true;
        }

      if(isFullPage == true)
        {
#if (CACHE_RAM_BD_MODULE == true)
        if(FTL_ERR_PASS != (status = CACHE_LoadEB(0, currentLogicalEBNum, CACHE_WRITE_TYPE)))
          {
          return status;
          }
#endif
        tempPPA = GetPPASlot(currentPage.devID, currentLogicalEBNum, logicalPageOffset);
        if(tempPPA != EMPTY_INVALID)
          {

#if(FTL_EBLOCK_CHAINING == true)
          if(tempPPA == CHAIN_INVALID)
            {
            chainEBNum = GetChainLogicalEBNum(currentPage.devID, currentLogicalEBNum);
            if(chainEBNum == EMPTY_WORD)
              {
              FTL_ClearMTLockBit();
              return FTL_ERR_ECHAIN_GC_DEL;
              }
            else
              {
#if (CACHE_RAM_BD_MODULE == true)
              if(FTL_ERR_PASS != (status = CACHE_LoadEB(currentPage.devID, chainEBNum, CACHE_WRITE_TYPE)))
                {
                return status;
                }
#endif
              SetPPASlot(currentPage.devID, currentLogicalEBNum, logicalPageOffset, EMPTY_INVALID);
              MarkPPAMappingTableEntryDirty(currentPage.devID, currentLogicalEBNum, logicalPageOffset);
              UpdatePageTableInfo(currentPage.devID, chainEBNum,
                                  logicalPageOffset, EMPTY_INVALID, BLOCK_INFO_STALE_PAGE);
              }
            }
          else
            {
#if (CACHE_RAM_BD_MODULE == true)
            if(FTL_ERR_PASS != (status = CACHE_LoadEB(currentPage.devID, currentLogicalEBNum, CACHE_WRITE_TYPE)))
              {
              return status;
              }
#endif
            UpdatePageTableInfo(currentPage.devID, currentLogicalEBNum,
                                logicalPageOffset, EMPTY_INVALID, BLOCK_INFO_STALE_PAGE);
            }

#else  // #if(FTL_EBLOCK_CHAINING == true)
          UpdatePageTableInfo(currentPage.devID, currentLogicalEBNum,
                              logicalPageOffset, EMPTY_INVALID, BLOCK_INFO_STALE_PAGE);
#endif  // #else  // #if(FTL_EBLOCK_CHAINING == true)

          (*sectorsDeleted) += NUM_SECTORS_PER_PAGE;
          }

#if(FTL_RPB_CACHE == true)
        if(currentLogicalPageNum == GetRPBCacheLogicalPageAddr(currentPage.devID))
          {
          ClearRPBCache(currentPage.devID);
          }
#endif  // #if(FTL_RPB_CACHE == true)
        }
      if((status = IncPageAddr(&currentPage)) != FTL_ERR_PASS)
        {
        FTL_ClearMTLockBit();
        return status;
        }
      }

#if(FTL_DELETE_WITH_GC == true)
    /*erase eblks*/
    for(currentPageCount = 0; currentPageCount < NUM_DEVICES; currentPageCount++)
      {
      logicalEBNum[currentPageCount] = EMPTY_WORD;
      }
    currentPage = startPage;
    for(currentPageCount = 0; currentPageCount < totalPages; currentPageCount++)
      {
      GetLogicalEBNum(currentPage.logicalPageNum, &currentLogicalEBNum);
      if(currentLogicalEBNum == logicalEBNum[currentPage.devID])
        {
        /*ForcedGC have already been done on currentLogicalEBNum*/
        }
      else
        {
        logicalEBNum[currentPage.devID] = currentLogicalEBNum;
        logicalGCEBNum = EMPTY_WORD;

#if (CACHE_RAM_BD_MODULE == true)
        if(FTL_ERR_PASS != (status = CACHE_LoadEB(currentPage.devID, currentLogicalEBNum, CACHE_WRITE_TYPE)))
          {
          return status;
          }
#endif

#if(FTL_EBLOCK_CHAINING == true)
        chainEBNum = GetChainLogicalEBNum(currentPage.devID, currentLogicalEBNum);
        if(chainEBNum != EMPTY_WORD)
          {
#if (CACHE_RAM_BD_MODULE == true)
          if(FTL_ERR_PASS != (status = CACHE_LoadEB(currentPage.devID, chainEBNum, CACHE_WRITE_TYPE)))
            {
            return status;
            }
#endif
          if(GetNumFreePages(currentPage.devID, chainEBNum) < Delete_GC_Threshold)
            {
            logicalGCEBNum = currentLogicalEBNum;
            }
          }
        else
          {
          if(GetNumFreePages(currentPage.devID, currentLogicalEBNum) < Delete_GC_Threshold)
            {
            logicalGCEBNum = currentLogicalEBNum;
            }
          }

#else  // #if(FTL_EBLOCK_CHAINING == true)
        if(GetNumFreePages(currentPage.devID, currentLogicalEBNum) < Delete_GC_Threshold)
          {
          logicalGCEBNum = currentLogicalEBNum;
          }
#endif  // #else  // #if(FTL_EBLOCK_CHAINING == true)

        if(logicalGCEBNum != EMPTY_WORD)
          {
          FTL_DeleteFlag = true;
          freedUpPages = EMPTY_WORD;
          freePageIndex = EMPTY_WORD;
          status = ClearGC_Info();
          if(FTL_ERR_PASS != status)
            {
            FTL_ClearMTLockBit();
            return status;
            }

#if (CACHE_RAM_BD_MODULE == true)
          if(FTL_ERR_PASS != (status = CACHE_LoadEB(currentPage.devID, logicalGCEBNum, CACHE_WRITE_TYPE)))
            {
            return status;
            }
#endif

#if(FTL_DEFECT_MANAGEMENT == true)
          status = InternalForcedGCWithBBManagement(currentPage.devID, logicalGCEBNum, &freedUpPages, &freePageIndex, false);
          if(status != FTL_ERR_PASS)
            {
            FTL_ClearMTLockBit();
            return status;
            }
          if(GetTransLogEBFailedBadBlockInfo() == true)
            {
            if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS)
              {
              FTL_ClearMTLockBit();
              return status;
              }
            }
          ClearTransLogEBBadBlockInfo();

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
          status = FTL_InternalForcedGC(currentPage.devID, logicalGCEBNum, &freedUpPages, &freePageIndex, false);
          if(status != FTL_ERR_PASS)
            {
            FTL_ClearMTLockBit();
            return status;
            }
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

          FTL_DeleteFlag = false;
          }
        }
      if((status = IncPageAddr(&currentPage)) != FTL_ERR_PASS)
        {
        FTL_ClearMTLockBit();
        return status;
        }
      }
#endif  // #if(FTL_DELETE_WITH_GC == true)

    }
  FTL_ClearMTLockBit();
#endif  // #if (FTL_ENABLE_DELETE_API == true)

  FTL_UpdatedFlag = UPDATED_DONE;
  return FTL_ERR_PASS;
  }
