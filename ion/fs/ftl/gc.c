#include "def.h"
#include "calc.h"
#include "common.h"
#include "if_in.h"
#include <string.h>

#if(FTL_DEFECT_MANAGEMENT == true)
extern BAD_BLOCK_INFO badBlockInfo;
#endif
//-------------------------------------------------------------------------------------

FTL_STATUS ClearGC_Info(void)
  {
  GC_Info.devID = EMPTY_BYTE;
  GC_Info.endMerge.logicalPageNum = EMPTY_DWORD;
  GC_Info.endMerge.DevID = EMPTY_BYTE;
  GC_Info.endMerge.logicalEBNum = EMPTY_WORD;
  GC_Info.endMerge.phyPageNum = EMPTY_DWORD;
  GC_Info.startMerge.logicalPageNum = EMPTY_DWORD;
  GC_Info.startMerge.DevID = EMPTY_BYTE;
  GC_Info.startMerge.logicalEBNum = EMPTY_WORD;
  GC_Info.startMerge.phyPageNum = EMPTY_DWORD;
  GC_Info.logicalEBlock = EMPTY_WORD;
  return FTL_ERR_PASS;
  }

//-------------------------------------------------------------------------------------

FTL_STATUS ClearMergeGC_Info(FTL_DEV DevID, uint16_t logicalEBNum, uint32_t logicalPageNum)
  {
  if((GC_Info.endMerge.logicalPageNum == logicalPageNum)
     && (GC_Info.endMerge.DevID == DevID)
     && (GC_Info.endMerge.logicalEBNum == logicalEBNum))
    {
    GC_Info.endMerge.logicalPageNum = EMPTY_DWORD;
    GC_Info.endMerge.DevID = EMPTY_BYTE;
    GC_Info.endMerge.logicalEBNum = EMPTY_WORD;
    GC_Info.endMerge.phyPageNum = EMPTY_DWORD;
    }
  if((GC_Info.startMerge.logicalPageNum == logicalPageNum)
     && (GC_Info.startMerge.DevID == DevID)
     && (GC_Info.startMerge.logicalEBNum == logicalEBNum))
    {
    GC_Info.startMerge.logicalPageNum = EMPTY_DWORD;
    GC_Info.startMerge.DevID = EMPTY_BYTE;
    GC_Info.startMerge.logicalEBNum = EMPTY_WORD;
    GC_Info.startMerge.phyPageNum = EMPTY_DWORD;
    }
  return FTL_ERR_PASS;
  }

//----------------------------

FTL_STATUS ClearGCPageBitMap(void) /*0*/
  {
  uint16_t count = 0; /*2*/
  /*total stack bytes - 2*/
  for(count = 0; count < NUM_PAGES_PER_EBLOCK; count++)
    {
    GCMoveArray[count] = false;

#if(FTL_DEFECT_MANAGEMENT == true)
    badBlockPhyPageAddr[count] = EMPTY_DWORD;
#endif

    }
  GCMoveArrayNotEmpty = false;
  return FTL_ERR_PASS;
  }

//---------------------------

FTL_STATUS SetPageMoved(uint16_t pageAddress, uint32_t phyPageAddr) /*2*/
  {
  /*total stack bytes - 2*/

  if(pageAddress > NUM_PAGES_PER_EBLOCK)
    {
    return FTL_ERR_GC_SET_MOVED;
    }
  GCMoveArrayNotEmpty = true;
  GCMoveArray[pageAddress] = true;

#if(FTL_DEFECT_MANAGEMENT == true)
  badBlockPhyPageAddr[pageAddress] = phyPageAddr;
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

  return FTL_ERR_PASS;
  }

//--------------------------------------------------------------------------

FTL_STATUS IsPageMoved(uint32_t pageAddr, uint8_t * isMoved) /*4, 4*/
  {
  /*total stack bytes - 12 */

  if(pageAddr > NUM_PAGES_PER_EBLOCK)
    {
    return FTL_ERR_GC_IS_MOVED;
    }
  if(GCMoveArray[pageAddr] == false)
    {
    (*isMoved) = false;
    }
  else
    {
    (*isMoved) = true;
    }
  return FTL_ERR_PASS;
  }

//--------------------------------------------

FTL_STATUS FTL_SwapDataReserveEBlock(FTL_DEV devID, uint16_t logicalPageNum,
                                     uint16_t * ptrPhyReservedBlock, uint16_t * ptrLogicalReservedBlock, uint8_t WLflag, uint8_t badBlockFlagIn)
  {
  uint32_t tempEraseCount = 0; /*2*/
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

#if (FTL_ENABLE_UNUSED_EB_SWAP == true)
  uint8_t eraseStatus = true; /*1*/
#endif  // #if (FTL_ENABLE_UNUSED_EB_SWAP == true)

  uint16_t phyReservedEBlock = EMPTY_WORD; /*2*/
  uint16_t pickedEB = EMPTY_WORD; /*2*/
#if (FTL_DEFECT_MANAGEMENT == true)
  uint16_t freePageIndex = EMPTY_WORD;
  uint8_t badEBlockFlag = false; /*1*/
#if(FTL_EBLOCK_CHAINING == true)
  uint16_t chainEBNum = EMPTY_WORD;
  uint16_t phyChainEBNum = EMPTY_WORD;
#endif
#endif
#if(FTL_EBLOCK_CHAINING == true)
  CHAIN_LOG_ENTRY chainLogEntry; /*16*/
  FLASH_PAGE_INFO flashPageInfo = {0, 0,
    {0, 0}}; /*11*/
#endif  // #if(FTL_EBLOCK_CHAINING == true)
  uint8_t count = 0; /*1*/

  if((status = TABLE_GetReservedEB(devID, &pickedEB, WLflag)) != FTL_ERR_PASS)
    {
    return FTL_ERR_ECHAIN_SETUP_SANITY1;
    }

  phyReservedEBlock = GetPhysicalEBlockAddr(devID, pickedEB);

#if DEBUG_BLOCK_SELECT
  DBG_Printf("Swap: pickedEB = 0x%X, ", pickedEB, 0);
  DBG_Printf("phyReservedEBlock = 0x%X\n", phyReservedEBlock, 0);
#endif  // #if DEBUG_BLOCK_SELECT

  tempEraseCount = GetEraseCount(devID, pickedEB);
  if(logicalPageNum != EMPTY_WORD)
    {

#if(FTL_DEFECT_MANAGEMENT == true)
    if(badBlockFlagIn == false)
      {
      StoreSourceBadBlockInfo(devID, logicalPageNum, FTL_ERR_DATA_RESERVE);
      StoreTargetBadBlockInfo(devID, pickedEB, FTL_ERR_DATA_RESERVE);
      }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

    SetPhysicalEBlockAddr(devID, pickedEB, GetPhysicalEBlockAddr(devID, logicalPageNum));
    SetEraseCount(devID, pickedEB, GetEraseCount(devID, logicalPageNum));
    SetPhysicalEBlockAddr(devID, logicalPageNum, phyReservedEBlock);
    SetEraseCount(devID, logicalPageNum, tempEraseCount);
    MarkEBlockMappingTableEntryDirty(devID, pickedEB);
    MarkEBlockMappingTableEntryDirty(devID, logicalPageNum);
#if (CACHE_RAM_BD_MODULE == true)
#if (FTL_STATIC_WEAR_LEVELING == true)
    tempEraseCount = GetTrueEraseCount(devID, pickedEB);
    status = SetSaveStaticWL(devID, pickedEB, tempEraseCount);
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
    tempEraseCount = GetTrueEraseCount(devID, logicalPageNum);
    status = SetSaveStaticWL(devID, logicalPageNum, tempEraseCount);
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
#endif
#endif

#if (FTL_DEFECT_MANAGEMENT == true)
    badEBlockFlag = GetBadEBlockStatus(devID, logicalPageNum);
    SetBadEBlockStatus(devID, logicalPageNum, GetBadEBlockStatus(devID, pickedEB));
    SetBadEBlockStatus(devID, pickedEB, badEBlockFlag);
    if(badBlockFlagIn == true)
      {
#if(FTL_EBLOCK_CHAINING == true)
      chainEBNum = GetChainLogicalEBNum(devID, logicalPageNum);
      if(chainEBNum != EMPTY_WORD)
        {
        // its chained, the to informaiton of current logical EB, is valid, both logical and physical, the logical of the from EB is also valid, the physical of the from EB has to be updated.
        phyChainEBNum = GetChainPhyEBNum(devID, chainEBNum);
        SetChainPhyEBNum(devID, chainEBNum, phyReservedEBlock);
        MarkEBlockMappingTableEntryDirty(devID, chainEBNum);
        }
#endif
      *ptrLogicalReservedBlock = pickedEB;
      *ptrPhyReservedBlock = phyReservedEBlock;
      return status;
      }
#endif

    }
#if(FTL_EBLOCK_CHAINING == true)
else
    {
    if(((NUMBER_OF_SYSTEM_EBLOCKS - ((TABLE_GetReservedEBlockNum(devID) + 1) + gcSaveCount)) - TABLE_GetUsedSysEBCount(devID)) < NUM_CHAIN_EBLOCKS)
      {
#if(FTL_DEFECT_MANAGEMENT == true)
      if(badBlockFlagIn == false)
        {
        //check free page index
        freePageIndex = GetFreePageIndex(devID, *ptrLogicalReservedBlock);
        if(freePageIndex < NUM_PAGES_PER_EBLOCK)
          {
          StoreSourceBadBlockInfo(devID, *ptrLogicalReservedBlock, FTL_ERR_CHAIN_NOT_FULL_EB);
          StoreTargetBadBlockInfo(devID, pickedEB, FTL_ERR_CHAIN_NOT_FULL_EB);
          }
        else
          {
          StoreSourceBadBlockInfo(devID, pickedEB, FTL_ERR_CHAIN_FULL_EB);
          StoreTargetBadBlockInfo(devID, *ptrLogicalReservedBlock, FTL_ERR_CHAIN_FULL_EB);
          }
        }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

      // Empty Blocks are available for chaining
      // Erase "To" EBlock

#if (ENABLE_EB_ERASED_BIT == true)
      eraseStatus = GetEBErased(devID, pickedEB);

#if DEBUG_PRE_ERASED
      if(true == eraseStatus)
        {
        DBG_Printf("FTL_SwapDataReserveEBlock: EBlock 0x%X is already erased\n", pickedEB, 0);
        }
#endif  // #if DEBUG_PRE_ERASED
      if(false == eraseStatus)
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

        {
        status = FTL_EraseOp(devID, pickedEB);
        if(FTL_ERR_PASS != status)
          {
          if(FTL_ERR_FAIL == status)
            {
            return status;
            }
#if(FTL_DEFECT_MANAGEMENT == true)
          if(badBlockFlagIn == false)
            {
            StoreSourceBadBlockInfo(devID, pickedEB, FTL_ERR_CHAIN_FULL_EB);
            StoreTargetBadBlockInfo(devID, *ptrLogicalReservedBlock, FTL_ERR_CHAIN_FULL_EB);
            }
#endif   // #if(FTL_DEFECT_MANAGEMENT == true)
          return FTL_ERR_GC_ERASE1;
          }
        }
      // Clear the bit map and ppa tables of the chained-to EBlock
      SetDirtyCount(devID, pickedEB, 0);
      TABLE_ClearFreeBitMap(devID, pickedEB);
      TABLE_ClearPPATable(devID, pickedEB);
      MarkPPAMappingTableEntryDirty(devID, pickedEB, 0);
      MarkEBlockMappingTableEntryDirty(devID, pickedEB);
      SetChainLink(devID, *ptrLogicalReservedBlock, pickedEB,
                   *ptrPhyReservedBlock, phyReservedEBlock);
      /* CREATE A CHAIN LOG ENTRY */
      if((status = GetNextLogEntryLocation(devID, &flashPageInfo)) != FTL_ERR_PASS)
        {
        return status;
        }
      chainLogEntry.logicalFrom = *ptrLogicalReservedBlock;
      chainLogEntry.logicalTo = pickedEB;
      chainLogEntry.phyFrom = *ptrPhyReservedBlock;
      chainLogEntry.phyTo = phyReservedEBlock;
      for(count = 0; count < CHAIN_LOG_ENTRY_RESERVED; count++)
        {
        chainLogEntry.reserved[count] = EMPTY_BYTE;
        }
      chainLogEntry.type = CHAIN_LOG_TYPE;
      if((status = FTL_WriteLogInfo(&flashPageInfo, (uint8_t *) & chainLogEntry)) != FTL_ERR_PASS)
        {
        return status;
        }

#if DEBUG_GC_BLOCKS
      DBG_Printf("Chain Log: logFrom = 0x%X, ", chainLogEntry.logicalFrom, 0);
      DBG_Printf("logTo = 0x%X, ", chainLogEntry.logicalTo, 0);
      DBG_Printf("phyFrom = 0x%X, ", chainLogEntry.phyFrom, 0);
      DBG_Printf("phyTo = 0x%X\n", chainLogEntry.phyTo, 0);
#endif  // #if DEBUG_GC_BLOCKS

      }
    else
      {
#if DEBUG_BLOCK_SELECT
      DBG_Printf("  Not enough Reserved EBlocks to setup a chain; Must GC instead\n", 0, 0);
#endif  // #if DEBUG_BLOCK_SELECT

      if((status = TABLE_InsertReservedEB(devID, pickedEB)) != FTL_ERR_PASS)
        {
        return status;
        }

      return FTL_ERR_ECHAIN_GC_NEEDED;
      }
    }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  *ptrLogicalReservedBlock = pickedEB;
  *ptrPhyReservedBlock = phyReservedEBlock;
  return status;
  }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
//---------------------------------------

FTL_STATUS CopyPagesForDataGC(FTL_DEV devID, uint16_t logicalEBNum, uint16_t reserveEBNum, uint16_t * FreedUpPages, uint8_t * pageBitMap)
  {
  FLASH_STATUS flashStatus = FLASH_PASS; /*4*/
  uint16_t phyFromEBlock = EMPTY_WORD; /*2*/
  uint16_t phyToEBlock = EMPTY_WORD; /*2*/
  uint16_t logicalPageOffset = EMPTY_WORD; /*2*/
  FREE_BIT_MAP_TYPE bitMap = 0; /*1*/
  uint16_t phyPageOffset = EMPTY_WORD; /*2*/
  uint8_t isMoved = 0; /*1*/
  FLASH_PAGE_INFO pageInfo = {0, 0,
    {0, 0}}; /*11*/

#if(FTL_EBLOCK_CHAINING == true)
  uint16_t logChainToEB = EMPTY_WORD; /*2*/
  uint16_t phyChainToEB = EMPTY_WORD; /*2*/
  uint16_t chainFreePageIndex = EMPTY_WORD; /*2*/
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  phyFromEBlock = GetPhysicalEBlockAddr(devID, reserveEBNum);
  phyToEBlock = GetPhysicalEBlockAddr(devID, logicalEBNum);

#if(FTL_EBLOCK_CHAINING == true)
  logChainToEB = GetChainLogicalEBNum(devID, logicalEBNum);
  if(logChainToEB != EMPTY_WORD)
    {
    phyChainToEB = GetChainPhyEBNum(devID, logicalEBNum);
    }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  pageInfo.devID = devID;
  pageInfo.vPage.pageOffset = 0;
  pageInfo.byteCount = VIRTUAL_PAGE_SIZE;

  // Loop through bit map of "From" EBlock
  for(logicalPageOffset = 0; logicalPageOffset < NUM_PAGES_PER_EBLOCK; logicalPageOffset++)
    {
    isMoved = false;
    if(true == GCMoveArrayNotEmpty)
      {
      IsPageMoved(logicalPageOffset, &isMoved);
      }
    if(true == isMoved)
      {
      // This page is being rewritten - do not copy
      // Clear both the Bit Map in the Block Info and PPA Tables
      UpdatePageTableInfo(devID, logicalEBNum, logicalPageOffset, EMPTY_INVALID, BLOCK_INFO_EMPTY_PAGE);

#if(FTL_EBLOCK_CHAINING == true)
      if(logChainToEB != EMPTY_WORD)
        { /*clear the chain EB as well*/
        UpdatePageTableInfo(devID, logChainToEB, logicalPageOffset,
                            EMPTY_INVALID, BLOCK_INFO_EMPTY_PAGE);
        }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

      }
    else
      {
      // convert logical page to physical page
      phyPageOffset = GetPPASlot(devID, logicalEBNum, logicalPageOffset);
      if((EMPTY_INVALID != phyPageOffset) && (CHAIN_INVALID != phyPageOffset))
        {
        // Page must be valid - Copy Page
        pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyFromEBlock, phyPageOffset);
        flashStatus = FLASH_RamPageReadDataBlock(&pageInfo, &pseudoRPB[devID][0]);
        if(FLASH_PASS != flashStatus)
          {
          return FTL_ERR_GC_PAGE_LOAD1;
          }
        pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyToEBlock, phyPageOffset);
        flashStatus = FLASH_RamPageWriteDataBlock(&pageInfo, &pseudoRPB[devID][0]);
        if(FLASH_PASS != flashStatus)
          {
          return FTL_ERR_GC_PAGE_WR1;
          }
        // Update bit map in GC log entry
        SetBitMapField(&pageBitMap[0], logicalPageOffset, 1, GC_MOVED_PAGE);
        }
      }
    }

#if(FTL_EBLOCK_CHAINING == true)
  // clear the chain info
  if(logChainToEB != EMPTY_WORD)
    {
    ClearChainLink(devID, logicalEBNum, logChainToEB);
    }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  // Update Block_Info table
  // Erase Count was written by FTL_SwapDataReserveEBlock()
  SetGCOrFreePageNum(devID, logicalEBNum, GCNum[devID]++);
  SetDirtyCount(devID, logicalEBNum, 0);
  MarkEBlockMappingTableEntryDirty(devID, logicalEBNum);

  // Mark all stale pages as free
  for(phyPageOffset = 0; phyPageOffset < NUM_PAGES_PER_EBLOCK; phyPageOffset++)
    {
    bitMap = GetEBlockMapFreeBitIndex(devID, logicalEBNum, phyPageOffset);
    if(BLOCK_INFO_STALE_PAGE == bitMap)
      {
      *FreedUpPages = *FreedUpPages + 1;
      // Note: PPA table should not include this page
      SetEBlockMapFreeBitIndex(devID, logicalEBNum, phyPageOffset, BLOCK_INFO_EMPTY_PAGE);
      }
    }

#if(FTL_EBLOCK_CHAINING == true)
  /*let do it again for the chained pages*/
  for(logicalPageOffset = 0; logicalPageOffset < NUM_PAGES_PER_EBLOCK; logicalPageOffset++)
    {
    isMoved = false;
    if(true == GCMoveArrayNotEmpty)
      {
      IsPageMoved(logicalPageOffset, &isMoved);
      }
    if(true == isMoved)
      {
      // already done this no, need to do it again
      }
    else
      {
      // convert logical page to physical page
      phyPageOffset = GetPPASlot(devID, logicalEBNum, logicalPageOffset);
      if(CHAIN_INVALID == phyPageOffset)
        {
        // Page is in chained EB, get it from there
        phyPageOffset = GetPPASlot(devID, logChainToEB, logicalPageOffset);
        pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyChainToEB, phyPageOffset);
        *FreedUpPages = *FreedUpPages - 1;
        flashStatus = FLASH_RamPageReadDataBlock(&pageInfo, &pseudoRPB[devID][0]);
        if(FLASH_PASS != flashStatus)
          {
          return FTL_ERR_GC_PAGE_LOAD2;
          }
        /*get a index to write to*/
        chainFreePageIndex = GetFreePageIndex(devID, logicalEBNum);
        pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyToEBlock, chainFreePageIndex);
        flashStatus = FLASH_RamPageWriteDataBlock(&pageInfo, &pseudoRPB[devID][0]);
        if(FLASH_PASS != flashStatus)
          {
          return FTL_ERR_GC_PAGE_WR2;
          }
        UpdatePageTableInfo(devID, logicalEBNum, logicalPageOffset,
                            chainFreePageIndex, BLOCK_INFO_VALID_PAGE);
        /*clear the chained EB status*/
        UpdatePageTableInfo(devID, logChainToEB, logicalPageOffset,
                            EMPTY_INVALID, BLOCK_INFO_STALE_PAGE);
        // Update bit map in GC log entry
        SetBitMapField(&pageBitMap[0], logicalPageOffset, 1, GC_MOVED_PAGE);
        }
      }
    }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  // Update dirty bit in Block_Info table
  MarkEBlockMappingTableEntryDirty(devID, logicalEBNum);
  return FTL_ERR_PASS;
  }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
//---------------------------------------

FTL_STATUS CopyPagesForDataGC(FTL_DEV devID, uint16_t logicalEBNum, uint16_t reserveEBNum, uint16_t * FreedUpPages, uint8_t * pageBitMap)
  {
  uint16_t phyFromEBlock = EMPTY_WORD; /*2*/
  uint16_t phyToEBlock = EMPTY_WORD; /*2*/
  uint16_t pageOffset = 0; /*2*/
  uint16_t phyPageOffset = 0; /*2*/
  uint16_t freePageIndex = 0; /*2*/
  uint8_t isMoved = 0; /*1*/
  FLASH_PAGE_INFO pageInfo = {0, 0,
    {0, 0}}; /*11*/
  FLASH_STATUS flashStatus = FLASH_PASS;

#if(FTL_EBLOCK_CHAINING == true)
  uint16_t logChainEBNum = EMPTY_WORD; /*2*/
  uint16_t phyChainEBNum = EMPTY_WORD; /*2*/
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  phyFromEBlock = GetPhysicalEBlockAddr(devID, reserveEBNum);
  phyToEBlock = GetPhysicalEBlockAddr(devID, logicalEBNum);

#if(FTL_EBLOCK_CHAINING == true)
  logChainEBNum = GetChainLogicalEBNum(devID, logicalEBNum);
  if(logChainEBNum != EMPTY_WORD)
    {
    phyChainEBNum = GetChainPhyEBNum(devID, logicalEBNum);
    }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  TABLE_ClearFreeBitMap(devID, logicalEBNum);

  pageInfo.devID = devID;
  pageInfo.vPage.pageOffset = 0;
  pageInfo.byteCount = NUMBER_OF_BYTES_PER_PAGE;

  // Loop through bit map of "From" EBlock
  for(pageOffset = 0; pageOffset < NUM_PAGES_PER_EBLOCK; pageOffset++)
    {
    isMoved = false;
    if(GCMoveArrayNotEmpty == true)
      {
      IsPageMoved(pageOffset, &isMoved);
      }
    if(isMoved == true)
      {
      *FreedUpPages = *FreedUpPages + 1;
      SetPPASlot(devID, logicalEBNum, pageOffset, EMPTY_INVALID);
      }
    else
      {
      // convert logical page to physical page
      phyPageOffset = GetPPASlot(devID, logicalEBNum, pageOffset);
      if(phyPageOffset == EMPTY_INVALID)
        {
        *FreedUpPages = *FreedUpPages + 1;
        }
      else
        {
        // Page must be valid - Copy Page
        pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyFromEBlock, phyPageOffset);

#if(FTL_EBLOCK_CHAINING == true)
        if(phyPageOffset == CHAIN_INVALID)
          {
          phyPageOffset = GetPPASlot(devID, logChainEBNum, pageOffset);
          pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyChainEBNum, phyPageOffset);
          }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

        if(FLASH_RamPageReadDataBlock(&pageInfo, &pseudoRPB[devID][0]) != FLASH_PASS)
          {
          return FTL_ERR_GC_PAGE_LOAD1;
          }
        freePageIndex = GetFreePageIndex(devID, logicalEBNum);
        pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyToEBlock, freePageIndex);
        flashStatus = FLASH_RamPageWriteDataBlock(&pageInfo, &pseudoRPB[devID][0]);
        if(flashStatus != FLASH_PASS)
          {
          if(flashStatus == FLASH_PARAM)
            {
            return FTL_ERR_FAIL;
            }
          return FTL_ERR_GC_PAGE_WR1;
          }
        UpdatePageTableInfo(devID, logicalEBNum, pageOffset, freePageIndex, BLOCK_INFO_VALID_PAGE);
        // Update bit map in GC log entry
        SetBitMapField(&pageBitMap[0], pageOffset, 1, GC_MOVED_PAGE);
        }
      }
    }

  // Update Block_Info table
  // Erase Count was written by FTL_SwapDataReserveEBlock()
  SetGCOrFreePageNum(devID, logicalEBNum, GCNum[devID]++);
  SetDirtyCount(devID, logicalEBNum, 0);

#if(FTL_EBLOCK_CHAINING == true)
  if(logChainEBNum != EMPTY_WORD)
    {
    ClearChainLink(devID, logicalEBNum, logChainEBNum);
    SetDirtyCount(devID, logChainEBNum, 0);
    MarkEBlockMappingTableEntryDirty(devID, logChainEBNum);
    MarkAllPagesStatus(devID, logChainEBNum, BLOCK_INFO_STALE_PAGE);
    }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  // Update dirty bit in Block_Info table
  MarkEBlockMappingTableEntryDirty(devID, logicalEBNum);
  return FTL_ERR_PASS;
  }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

//---------------------------------------

FTL_STATUS DataGC(FTL_DEV devID, uint16_t logicalEBNum,
                  uint16_t * FreedUpPages, uint16_t * freePageIndex, uint8_t WLflag)
  {
  GC_LOG_ENTRY gcLog;
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

#if (ENABLE_EB_ERASED_BIT == true)
  uint8_t eraseStatus = false; /*1*/
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

  uint16_t phyFromEBlock = EMPTY_WORD; /*2*/
  uint16_t phyToEBlock = EMPTY_WORD; /*2*/
  uint16_t count = 0; /*2*/
  FLASH_PAGE_INFO pageInfo = {0, 0,
    {0, 0}}; /*11*/
  uint8_t useGcInfo = 0; /*1*/
  uint16_t logToEBlock = EMPTY_WORD; /*2*/
  uint8_t pageBitMap[GC_MOVE_BITMAP];
  uint8_t typeCount = 0; /*1*/
  uint8_t checkFlag = false;

#if (FTL_ENABLE_UNUSED_EB_SWAP == true)
  uint16_t logSwapEBlock = EMPTY_WORD; /*2*/
#endif  // #if (FTL_ENABLE_UNUSED_EB_SWAP == true)

#if(FTL_EBLOCK_CHAINING == true)
  uint16_t chainEBNum = EMPTY_WORD; /*2*/
  CHAIN_INFO chainInfo = {0, 0, 0, 0, 0}; /*10*/
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  *FreedUpPages = 0;
  *freePageIndex = 0;
  useGcInfo = false;
  if((EMPTY_BYTE == devID) || (EMPTY_WORD == logicalEBNum))
    {
    useGcInfo = true;
    devID = GC_Info.devID;
    logicalEBNum = GC_Info.logicalEBlock;
    }
  if((status = FTL_CheckForGCLogSpace(devID)) != FTL_ERR_PASS)
    {
    return status;
    }

#if ((DEBUG_FTL_API_ANNOUNCE == 1) || (DEBUG_GC_ANNOUNCE == 1))
  DBG_Printf("FTL_ForcedGC: useGcInfo = %d, ", useGcInfo, 0);
  DBG_Printf("devID = %d, ", devID, 0);
  DBG_Printf("logicalEBNum = 0x%X\n", logicalEBNum, 0);
#endif  // #if (DEBUG_FTL_API_ANNOUNCE == 1 || DEBUG_GC_ANNOUNCE == 1)

#if DEBUG_CHECK_TABLES
  status = DBG_CheckPPAandBitMap(devID, logicalEBNum);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }
#endif  // #if DEBUG_CHECK_TABLES

#if(FTL_EBLOCK_CHAINING == true)
  chainInfo.isChained = false;
  chainInfo.devID = EMPTY_BYTE;
  chainInfo.logChainToEB = EMPTY_WORD;
  chainInfo.phyChainToEB = EMPTY_WORD;
  chainInfo.phyPageAddr = EMPTY_WORD;
  chainEBNum = GetChainLogicalEBNum(devID, logicalEBNum);
  if(chainEBNum != EMPTY_WORD)
    {
    chainInfo.isChained = true;
    chainInfo.logChainToEB = chainEBNum;
    chainInfo.phyChainToEB = GetChainPhyEBNum(devID, logicalEBNum);

#if (DEBUG_FTL_API_ANNOUNCE == 1 || DEBUG_GC_ANNOUNCE == 1)
    DBG_Printf("  logChainToEB = 0x%X, ", chainEBNum, 0);
    DBG_Printf("phyChainToEB = 0x%X\n", chainInfo.phyChainToEB, 0);
#endif  // #if (DEBUG_FTL_API_ANNOUNCE == 1 || DEBUG_GC_ANNOUNCE == 1)

    }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  phyFromEBlock = GetPhysicalEBlockAddr(devID, logicalEBNum);
  // Identify EB to copy to, not chaining, regular GC, so swap will not erase, nor create a log, no need to worry about bad block
  if((status = FTL_SwapDataReserveEBlock(devID, logicalEBNum, &phyToEBlock, &logToEBlock, WLflag, false)) != FTL_ERR_PASS)
    {
    return status;
    }

#if DEBUG_GC_BLOCKS
  DBG_Printf("Forced_GC: phyFromEBlock = 0x%X, ", phyFromEBlock, 0);
  DBG_Printf("phyToEBlock = 0x%X", phyToEBlock, 0);

#if(FTL_EBLOCK_CHAINING == true)
  if(chainEBNum != EMPTY_WORD)
    {
    DBG_Printf(",  phyChainEB = 0x%X", chainInfo.phyChainToEB, 0);
    }
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  DBG_Printf("\n", 0, 0);
#endif  // #if DEBUG_GC_BLOCKS

  // initialize page bitmap
  for(count = 0; count < GC_MOVE_BITMAP; count++)
    {
    pageBitMap[count] = 0;
    }

  // Create and initialize GC log entry
  gcLog.partA.GCNum = GCNum[devID];
  gcLog.partA.type = GC_TYPE_A;
  for(count = 0; count < sizeof (gcLog.partA.reserved); count++)
    {
    gcLog.partA.reserved[count] = EMPTY_BYTE;
    }
  gcLog.partA.holdForMerge = GCMoveArrayNotEmpty;
  gcLog.partA.logicalEBAddr = logicalEBNum;
  gcLog.partA.reservedEBAddr = logToEBlock;
  for(typeCount = 0; typeCount < NUM_GC_TYPE_B; typeCount++)
    {
    gcLog.partB[typeCount].type = GC_TYPE_B;
    for(count = 0; count < sizeof (gcLog.partB[typeCount].pageMovedBitMap); count++)
      {
      gcLog.partB[typeCount].pageMovedBitMap[count] = 0;
      }
    for(count = 0; count < sizeof (gcLog.partB[typeCount].reserved); count++)
      {
      gcLog.partB[typeCount].reserved[count] = EMPTY_BYTE;
      }
    gcLog.partB[typeCount].checkWord = EMPTY_WORD;
    }

  // Write GC log entry to flash
  status = GetNextLogEntryLocation(devID, &pageInfo);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }
  status = FTL_WriteLogInfo(&pageInfo, (uint8_t *) & gcLog.partA);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }

#if (ENABLE_EB_ERASED_BIT == true)
  eraseStatus = GetEBErased(devID, logicalEBNum);

#if DEBUG_PRE_ERASED
  if(true == eraseStatus)
    {
    DBG_Printf("FTL_InternalForcedGC: EBlock 0x%X is already erased\n", logicalEBNum, 0);
    }
#endif  // #if DEBUG_PRE_ERASED

  if(false == eraseStatus)
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

    {
    status = FTL_EraseOp(devID, logicalEBNum);
    if(FTL_ERR_PASS != status)
      {
      if(FTL_ERR_FAIL == status)
        {
        return status;
        }
      return FTL_ERR_GC_ERASE2;
      }
    }

#if (ENABLE_EB_ERASED_BIT == true)
  SetEBErased(devID, logicalEBNum, false); /*since the EB will be written to now*/
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

  MarkEBlockMappingTableEntryDirty(devID, logicalEBNum);

  status = CopyPagesForDataGC(devID, logicalEBNum, logToEBlock, FreedUpPages, &pageBitMap[0]);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }

  // copy page bitmap to GC_TYPE_B
  for(typeCount = 0; typeCount < NUM_GC_TYPE_B; typeCount++)
    {
    memcpy(&gcLog.partB[typeCount].pageMovedBitMap[0], \
                  &pageBitMap[typeCount * NUM_ENTRIES_GC_TYPE_B], \
                  NUM_ENTRIES_GC_TYPE_B);
    }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  // Write GC log entry to flash
  for(typeCount = 0; typeCount < NUM_GC_TYPE_B; typeCount++)
    {
    status = GetNextLogEntryLocation(devID, &pageInfo);
    if(FTL_ERR_PASS != status)
      {
      return status;
      }
    status = FTL_WriteLogInfo(&pageInfo, (uint8_t *) & gcLog.partB[typeCount]);
    if(FTL_ERR_PASS != status)
      {
      return status;
      }
    }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  // Pack GC log type A entry to flash
  if((status = InitPackedLogs()) != FTL_ERR_PASS)
    {
    return status;
    }

  // Pack GC log type B entry to flash
  for(typeCount = 0; typeCount < NUM_GC_TYPE_B; typeCount++)
    {
    status = CopyPackedLogs(typeCount, (uint8_t *) & gcLog.partB[typeCount]);
    if(FTL_ERR_PASS != status)
      {
      return status;
      }
    }
  // Write GC log entry to flash
  status = GetNextLogEntryLocation(devID, &pageInfo);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }
  status = WritePackedLogs(&pageInfo);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

#if DEBUG_CHECK_TABLES
  status = DBG_CheckPPAandBitMap(devID, logicalEBNum);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }
#endif  // #if DEBUG_CHECK_TABLES

  if(FTL_DeleteFlag == false)
    {
    if((true == GCMoveArrayNotEmpty) || (EMPTY_WORD != gcSave[0].phyEbNum))
      {
      // Save the EB if it is to be merged or if a previous EB has been saved
      status = FTL_AddToGCSave(devID, phyFromEBlock);
      if(status != FTL_ERR_PASS)
        {
        return status;
        }

#if(FTL_EBLOCK_CHAINING == true)
      if(chainEBNum != EMPTY_WORD)
        {
        // this is a chained EB, lets check teh phyPageAddr
        // Save the EB if it is to be merged or if a previous EB has been saved
        status = FTL_AddToGCSave(devID, chainInfo.phyChainToEB);
        if(status != FTL_ERR_PASS)
          {
          return status;
          }
        }
#endif  // #if(FTL_EBLOCK_CHAINING == true)
      checkFlag = true;
      }
    }

#if (FTL_SUPER_SYS_EBLOCK == true)
  if(false == gProtectForSuperSysEBFlag)
    {
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if (FTL_ENABLE_UNUSED_EB_SWAP == true)
    if((status = FindAndSwapUnusedEB(devID, logToEBlock, &logSwapEBlock)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(EMPTY_WORD != logSwapEBlock)
      {
      status = CreateSwapEBLog(devID, logSwapEBlock, logToEBlock);
      if(FTL_ERR_PASS != status)
        {
        return status;
        }
      }

#if(FTL_EBLOCK_CHAINING == true)
    if(chainInfo.isChained == true)
      {
      if((status = FindAndSwapUnusedEB(devID, chainInfo.logChainToEB, &logSwapEBlock)) != FTL_ERR_PASS)
        {
        return status;
        }
      if(EMPTY_WORD != logSwapEBlock)
        {
        status = CreateSwapEBLog(devID, logSwapEBlock, chainInfo.logChainToEB);
        if(FTL_ERR_PASS != status)
          {
          return status;
          }
        }
      }
#endif  // #if(FTL_EBLOCK_CHAINING == true)
#endif  // #if (FTL_ENABLE_UNUSED_EB_SWAP == true)

#if (FTL_SUPER_SYS_EBLOCK == true)
    }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

  if(false == checkFlag)
    {
    if((status = TABLE_InsertReservedEB(devID, logToEBlock)) != FTL_ERR_PASS)
      {
      return status;
      }
#if(FTL_EBLOCK_CHAINING == true)
    if(chainEBNum != EMPTY_WORD)
      {
      if((status = TABLE_InsertReservedEB(devID, chainInfo.logChainToEB)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
#endif
    }
  return FTL_ERR_PASS;
  }

#if(FTL_UNLINK_GC == true)
//---------------------------------------

FTL_STATUS CopyPagesForUnlinkGC(FTL_DEV devID, uint16_t logFromEBNum, uint16_t logToEBNum, uint8_t * pageBitMap)
  {
  uint16_t phyFromEBNum = EMPTY_WORD; /*2*/
  uint16_t phyToEBNum = EMPTY_WORD; /*2*/
  uint16_t pageOffset = 0; /*2*/
  uint16_t phyPageOffset = 0; /*2*/
  uint16_t freePageIndex = 0; /*2*/
  FLASH_PAGE_INFO pageInfo = {0, 0,
    {0, 0}}; /*11*/
  FREE_BIT_MAP_TYPE bitMap = 0; /*1*/
  FLASH_STATUS flashStatus = FLASH_PASS;

  phyFromEBNum = GetPhysicalEBlockAddr(devID, logFromEBNum);
  phyToEBNum = GetPhysicalEBlockAddr(devID, logToEBNum);

  pageInfo.devID = devID;
  pageInfo.byteCount = NUMBER_OF_BYTES_PER_PAGE;
  pageInfo.vPage.pageOffset = 0;

  // Loop through bit map of "From" EBlock
  for(pageOffset = 0; pageOffset < NUM_PAGES_PER_EBLOCK; pageOffset++)
    {
    bitMap = GetBitMapField(&pageBitMap[0], pageOffset, 1);
    if(bitMap == GC_MOVED_PAGE)
      {
      // Page must be valid - Copy Page
      phyPageOffset = GetPPASlot(devID, logFromEBNum, pageOffset);
      pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyFromEBNum, phyPageOffset);
      if(FLASH_RamPageReadDataBlock(&pageInfo, &pseudoRPB[devID][0]) != FLASH_PASS)
        {
        return FTL_ERR_FLASH_LOAD_02;
        }
      /*get a index to write to*/
      freePageIndex = GetFreePageIndex(devID, logToEBNum);
      pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyToEBNum, freePageIndex);
      flashStatus = FLASH_RamPageWriteDataBlock(&pageInfo, &pseudoRPB[devID][0]);
      if(flashStatus != FLASH_PASS)
        {
        if(flashStatus == FLASH_PARAM)
          {
          return FTL_ERR_FAIL;
          }
        return FTL_ERR_FLASH_COMMIT_06;
        }
      /* update chained-to EBlock status */
      UpdatePageTableInfo(devID, logToEBNum, pageOffset, freePageIndex, BLOCK_INFO_VALID_PAGE);
      /* clear chained-from EBlock status */
      UpdatePageTableInfo(devID, logFromEBNum, pageOffset, EMPTY_INVALID, BLOCK_INFO_STALE_PAGE);
      }
    }
  return FTL_ERR_PASS;
  }

//---------------------------------------

FTL_STATUS UnlinkGC(FTL_DEV devID, uint16_t fromLogicalEBlock)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t fromPhysicalEBlock = EMPTY_WORD; /*2*/
  uint16_t toLogicalEBlock = EMPTY_WORD; /*2*/
  uint16_t toPhysicalEBlock = EMPTY_WORD; /*2*/
  uint16_t validPages = 0; /*2*/
  uint16_t freePages = 0; /*2*/
  uint16_t pageOffset = 0; /*2*/
  uint16_t phyPageOffset = 0; /*2*/
  uint16_t count = 0; /*2*/
  uint16_t typeCount = 0; /*2*/
  FLASH_PAGE_INFO pageInfo = {0, 0,
    {0, 0}}; /*11*/
  UNLINK_LOG_ENTRY unlinkLog;
  uint8_t pageBitMap[GC_MOVE_BITMAP];
  uint8_t checkFlag = false; /*1*/

#if (FTL_ENABLE_UNUSED_EB_SWAP == true)
  uint16_t logSwapEBlock = EMPTY_WORD; /*2*/
#endif  // #if (FTL_ENABLE_UNUSED_EB_SWAP == true)

  if((status = FTL_CheckForGCLogSpace(devID)) != FTL_ERR_PASS)
    {
    return status;
    }

  fromPhysicalEBlock = GetPhysicalEBlockAddr(devID, fromLogicalEBlock);
#if( FTL_EBLOCK_CHAINING == true)
  toLogicalEBlock = GetChainLogicalEBNum(devID, fromLogicalEBlock);
  toPhysicalEBlock = GetChainPhyEBNum(devID, fromLogicalEBlock);
#endif  // #if( FTL_EBLOCK_CHAINING == true)

#if(FTL_DEFECT_MANAGEMENT == true)
  StoreSourceBadBlockInfo(devID, toLogicalEBlock, FTL_ERR_DATA_RESERVE);
  StoreTargetBadBlockInfo(devID, fromLogicalEBlock, FTL_ERR_DATA_RESERVE);
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

  for(count = 0; count < GC_MOVE_BITMAP; count++)
    {
    pageBitMap[count] = 0;
    }
  unlinkLog.partA.checkWord = EMPTY_WORD;
  unlinkLog.partA.fromLogicalEBAddr = fromLogicalEBlock;
  unlinkLog.partA.toLogicalEBAddr = toLogicalEBlock;
  for(count = 0; count < sizeof (unlinkLog.partA.reserved); count++)
    {
    unlinkLog.partA.reserved[count] = EMPTY_BYTE;
    }
  for(typeCount = 0; typeCount < NUM_UNLINK_TYPE_B; typeCount++)
    {
    unlinkLog.partB[typeCount].checkWord = EMPTY_WORD;
    unlinkLog.partB[typeCount].type = UNLINK_LOG_TYPE_B;
    for(count = 0; count < sizeof (unlinkLog.partB[typeCount].pageMovedBitMap); count++)
      {
      unlinkLog.partB[typeCount].pageMovedBitMap[count] = 0;
      }
    for(count = 0; count < sizeof (unlinkLog.partB[typeCount].reserved); count++)
      {
      unlinkLog.partB[typeCount].reserved[count] = EMPTY_BYTE;
      }
    }

  validPages = GetNumValidPages(devID, fromLogicalEBlock);
  freePages = GetNumFreePages(devID, toLogicalEBlock);

  if(validPages == 0)
    {
    if((status = UnlinkChain(devID, fromLogicalEBlock, toLogicalEBlock)) != FTL_ERR_PASS)
      {
      return status;
      }

    unlinkLog.partA.type = UNLINK_LOG_TYPE_A1;
    if((status = GetNextLogEntryLocation(devID, &pageInfo)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = FTL_WriteLogInfo(&pageInfo, (uint8_t *) & unlinkLog.partA)) != FTL_ERR_PASS)
      {
      return status;
      }
    }
  else if(freePages >= validPages)
    {
    unlinkLog.partA.type = UNLINK_LOG_TYPE_A2;
    for(pageOffset = 0; pageOffset < NUM_PAGES_PER_EBLOCK; pageOffset++)
      {
      phyPageOffset = GetPPASlot(devID, fromLogicalEBlock, pageOffset);
      if((phyPageOffset != EMPTY_INVALID) && (phyPageOffset != CHAIN_INVALID))
        {
        SetBitMapField(&pageBitMap[0], pageOffset, 1, GC_MOVED_PAGE);
        }
      }
    for(count = 0; count < NUM_UNLINK_TYPE_B; count++)
      {
      memcpy(&unlinkLog.partB[count].pageMovedBitMap[0], &pageBitMap[count * NUM_ENTRIES_UNLINK_TYPE_B], NUM_ENTRIES_UNLINK_TYPE_B);
      }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
    if((status = GetNextLogEntryLocation(devID, &pageInfo)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = FTL_WriteLogInfo(&pageInfo, (uint8_t *) & unlinkLog.partA)) != FTL_ERR_PASS)
      {
      return status;
      }
    for(count = 0; count < NUM_UNLINK_TYPE_B; count++)
      {
      if((status = GetNextLogEntryLocation(devID, &pageInfo)) != FTL_ERR_PASS)
        {
        return status;
        }
      if((status = FTL_WriteLogInfo(&pageInfo, (uint8_t *) & unlinkLog.partB[count])) != FTL_ERR_PASS)
        {
        return status;
        }
      }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
    if((status = InitPackedLogs()) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = CopyPackedLogs(0, (uint8_t *) & unlinkLog.partA)) != FTL_ERR_PASS)
      {
      return status;
      }
    for(count = 0; count < NUM_UNLINK_TYPE_B; count++)
      {
      if((status = CopyPackedLogs((count + 1), (uint8_t *) & unlinkLog.partB[count])) != FTL_ERR_PASS)
        {
        return status;
        }
      }
    if((status = GetNextLogEntryLocation(devID, &pageInfo)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = WritePackedLogs(&pageInfo)) != FTL_ERR_PASS)
      {
      return status;
      }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

    if((status = CopyPagesForUnlinkGC(devID, fromLogicalEBlock, toLogicalEBlock, &pageBitMap[0])) != FTL_ERR_PASS)
      {
      return status;
      }

    if((status = UnlinkChain(devID, fromLogicalEBlock, toLogicalEBlock)) != FTL_ERR_PASS)
      {
      return status;
      }
    }

  if(FTL_DeleteFlag == false)
    {
    // Save the EB if a previous EB has been saved
    if((status = FTL_AddToGCSave(devID, fromPhysicalEBlock)) != FTL_ERR_PASS)
      {
      return status;
      }
    checkFlag = true;
    }

#if (FTL_ENABLE_UNUSED_EB_SWAP == true)
  if((status = FindAndSwapUnusedEB(devID, toLogicalEBlock, &logSwapEBlock)) != FTL_ERR_PASS)
    {
    return status;
    }
  if(logSwapEBlock != EMPTY_WORD)
    {
    if((status = CreateSwapEBLog(devID, logSwapEBlock, toLogicalEBlock)) != FTL_ERR_PASS)
      {
      return status;
      }
    }
#endif  // #if (FTL_ENABLE_UNUSED_EB_SWAP == true)

  if(false == checkFlag)
    {
    if((status = TABLE_InsertReservedEB(devID, toLogicalEBlock)) != FTL_ERR_PASS)
      {
      return status;
      }
    }

  return FTL_ERR_PASS;
  }
#endif  // #if(FTL_UNLINK_GC == true)

//---------------------------------------

FTL_STATUS FTL_InternalForcedGC(FTL_DEV devID, uint16_t logicalEBNum,
                                uint16_t * FreedUpPages, uint16_t * freePageIndex, uint8_t WLflag)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

#if(FTL_UNLINK_GC == true)
  FTL_DEV inDevID = 0; /*1*/
  uint16_t inLogicalEBlock = EMPTY_WORD; /*2*/
  uint16_t chainEBNum = EMPTY_WORD; /*2*/
  uint16_t validPages = 0; /*2*/
  uint16_t freePages = 0; /*2*/

  if(devID == EMPTY_BYTE)
    {
    inDevID = GC_Info.devID;
    }
  else
    {
    inDevID = devID;
    }

  if(logicalEBNum == EMPTY_WORD)
    {
    inLogicalEBlock = GC_Info.logicalEBlock;
    }
  else
    {
    inLogicalEBlock = logicalEBNum;
    }

#if (FTL_SUPER_SYS_EBLOCK == true)
  if(false == gProtectForSuperSysEBFlag)
    {
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if( FTL_EBLOCK_CHAINING == true)
    chainEBNum = GetChainLogicalEBNum(inDevID, inLogicalEBlock);
#endif  // #if( FTL_EBLOCK_CHAINING == true)
    if((WLflag == false) && (GCMoveArrayNotEmpty == false) && (chainEBNum != EMPTY_WORD))
      {
      validPages = GetNumValidPages(inDevID, inLogicalEBlock);
      freePages = GetNumFreePages(inDevID, chainEBNum);
      if((validPages == 0) || (freePages >= validPages))
        {
        if((status = UnlinkGC(inDevID, inLogicalEBlock)) != FTL_ERR_PASS)
          {
          return status;
          }
        return FTL_ERR_PASS;
        }
      }

#if (FTL_SUPER_SYS_EBLOCK == true)
    }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
#endif  // #if(FTL_UNLINK_GC == true)

  if((status = DataGC(devID, logicalEBNum, FreedUpPages, freePageIndex, WLflag)) != FTL_ERR_PASS)
    {
    return status;
    }
  return FTL_ERR_PASS;
  }

//----------------------------------

FTL_STATUS FTL_ForcedGC(FTL_DEV devID, uint16_t logicalEBNum,
                        uint16_t * FreedUpPages, uint16_t * freePageIndex)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

#if (FTL_SUPER_SYS_EBLOCK == true)
#if(FTL_DEFECT_MANAGEMENT == true)
  uint16_t sanityCounter = 0;
#endif
#endif

#if (DEBUG_ENABLE_LOGGING == true)
  if((status = DEBUG_InsertLog((uint32_t) logicalEBNum, EMPTY_DWORD, DEBUG_LOG_FORCED_GC)) != FTL_ERR_PASS)
    {
    return (status);
    }
#endif

  if((status = FTL_CheckPointer(FreedUpPages)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckPointer(freePageIndex)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckMount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }

#if (FTL_SUPER_SYS_EBLOCK == true)
#if(FTL_DEFECT_MANAGEMENT == true)
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
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if (CACHE_RAM_BD_MODULE == true)
  if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, logicalEBNum, CACHE_WRITE_TYPE)))
    {
    return status;
    }
#endif

#if(FTL_DEFECT_MANAGEMENT == true)
  status = InternalForcedGCWithBBManagement(devID, logicalEBNum, FreedUpPages, freePageIndex, false);

#else
  status = FTL_InternalForcedGC(devID, logicalEBNum, FreedUpPages, freePageIndex, false);
#endif

  if(status != FTL_ERR_PASS)
    {
    FTL_ClearMTLockBit();
    return status;
    }

#if(FTL_DEFECT_MANAGEMENT == true)
  if(GetTransLogEBFailedBadBlockInfo() == true)
    {
    if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS)
      {
      FTL_ClearMTLockBit();
      return status;
      }
    }
  ClearTransLogEBBadBlockInfo();
#endif

  FTL_ClearMTLockBit();
  FTL_UpdatedFlag = UPDATED_DONE;
  return FTL_ERR_PASS;
  }

//----------------------------------

FTL_STATUS Flush_GC(FTL_DEV devID)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t logicalEBNum = 0; /*2*/
  uint16_t physicalEBNum = EMPTY_WORD; /*2*/
  uint16_t logGCEBNum[MIN_FLUSH_GC_EBLOCKS];
  uint32_t latestIncNumber = 0; /*4*/
  FLASH_PAGE_INFO flashPageInfo = {0, 0,
    {0, 0}}; /*11*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  SYS_EBLOCK_INFO_PTR sysTempPtr = NULL; /*4*/
  uint16_t bitCount = 0; /*2*/
  int16_t eBlockCount = 0; /*2*/
  uint32_t key = 0; /*4*/
  KEY_TABLE_ENTRY flushEBNum[NUM_FLUSH_LOG_EBLOCKS]; /*16*/
  uint16_t flushEBCount = 0; /*2*/

  FLASH_STATUS flashStatus = FLASH_PASS;
#if (CACHE_RAM_BD_MODULE == true)
  uint8_t present = EMPTY_BYTE;
  uint16_t index = EMPTY_WORD;
  uint8_t dependency = EMPTY_BYTE;
  CACHE_INFO_EBLOCK_PPAMAP eBlockPPAMapInfo = {0, 0};
  FLASH_PAGE_INFO flushStructPageInfo = {0, 0,
    {0, 0}};
  FLASH_PAGE_INFO flushRAMTablePageInfo = {0, 0,
    {0, 0}};
  uint8_t tempCache[FLUSH_RAM_TABLE_SIZE];
  uint8_t count = 0;
  SYS_EBLOCK_FLUSH_INFO sysEBlockFlushInfo;
  uint16_t flushEB = 0;
  uint8_t flushTypeCnt = 0;
  uint16_t phyEBlockAddrTmp = 0;
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  uint16_t * flushInfoEntry = NULL; /*4*/
  uint8_t tempArray[SECTOR_SIZE];
  uint16_t count2 = 0;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

#endif /* #if (CACHE_RAM_BD_MODULE == true) */
#if (ENABLE_EB_ERASED_BIT == true)
  uint8_t eraseStatus = false; /*1*/
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

#if(FTL_DEFECT_MANAGEMENT == true)
  uint16_t sanityCounter = 0; /*2*/
  uint8_t checkBBMark = false; /*1*/
  uint16_t storeTemp[MIN_FLUSH_GC_EBLOCKS]; /*2*/
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

  latestIncNumber = GetFlushEBCounter(devID);
  for(eBlockCount = 0; eBlockCount < NUM_FLUSH_LOG_EBLOCKS; eBlockCount++)
    {
    flushEBNum[eBlockCount].logicalEBNum = EMPTY_WORD;
    flushEBNum[eBlockCount].phyAddr = EMPTY_WORD;
    flushEBNum[eBlockCount].key = EMPTY_DWORD;
#if (CACHE_RAM_BD_MODULE == true)
    flushEBNum[eBlockCount].cacheNum = EMPTY_BYTE;
#endif
    }
#if(FTL_DEFECT_MANAGEMENT == true)    
  for(eBlockCount = 0; eBlockCount < MIN_FLUSH_GC_EBLOCKS; eBlockCount++)
    {
    storeTemp[eBlockCount] = EMPTY_WORD;
    }
#endif
  for(eBlockCount = 0; eBlockCount < NUM_FLUSH_LOG_EBLOCKS; eBlockCount++)
    {
    if((status = TABLE_GetFlushLogEntry(devID, eBlockCount, &logicalEBNum, &physicalEBNum, &key)) != FTL_ERR_PASS)
      {
      return status;
      }
    if(key != EMPTY_DWORD)
      {
      flushEBNum[flushEBCount].logicalEBNum = logicalEBNum;
      flushEBNum[flushEBCount].phyAddr = physicalEBNum;
      flushEBNum[flushEBCount].key = key;
#if (CACHE_RAM_BD_MODULE == true)
      TABLE_GetFlushLogCacheEntry(devID, physicalEBNum, &flushEBNum[flushEBCount].cacheNum);
#endif
      /* Makes all flush entries of target EB occupied, in case that the tail of flush entries in the EB are skipped.
      This is the case that available entries are smaller than the required entries. */
      SetGCOrFreePageNum(devID, logicalEBNum, MAX_FLUSH_ENTRIES_PER_LOG_EBLOCK);
      flushEBCount++;
      }
    }
  if((status = TABLE_FlushEBClear(devID)) != FTL_ERR_PASS)
    {
    return status;
    }
  for(eBlockCount = 0; eBlockCount < MIN_FLUSH_GC_EBLOCKS; eBlockCount++)
    {
    if((status = TABLE_GetReservedEB(devID, &logicalEBNum, false)) != FTL_ERR_PASS)
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
#if(FTL_DEFECT_MANAGEMENT == true)
    storeTemp[eBlockCount] = logicalEBNum;
#endif
#if (ENABLE_EB_ERASED_BIT == true)
    eraseStatus = GetEBErased(devID, logicalEBNum);

#if DEBUG_PRE_ERASED
    if(true == eraseStatus)
      {
      DBG_Printf("Flush_GC: EBlock 0x%X is already erased\n", logicalEBNum, 0);
      }
#endif  // #if DEBUG_PRE_ERASED
    if(false == eraseStatus)
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

      {
      if((status = FTL_EraseOp(devID, logicalEBNum)) != FTL_ERR_PASS)
        {
#if(FTL_DEFECT_MANAGEMENT == true)
        if(status == FTL_ERR_FAIL)
          {
          return status;
          }
        SetBadEBlockStatus(devID, logicalEBNum, true);
        flashPageInfo.devID = devID;
        physicalEBNum = GetPhysicalEBlockAddr(devID, logicalEBNum);
        flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(physicalEBNum, 0);
        flashPageInfo.vPage.pageOffset = 0;
        flashPageInfo.byteCount = 0;
        if(FLASH_MarkDefectEBlock(&flashPageInfo) != FLASH_PASS)
          {
          // do nothing, just try to mark bad, even if it fails we move on.
          }
        for(eBlockCount = 0; eBlockCount < MIN_FLUSH_GC_EBLOCKS; eBlockCount++)
          {
          if(EMPTY_WORD != storeTemp[eBlockCount])
            {
            TABLE_InsertReservedEB(devID, storeTemp[eBlockCount]);
            }
          }
        if((status = TABLE_FlushEBClear(devID)) != FTL_ERR_PASS)
          {
          return status;
          }
        for(eBlockCount = 0; eBlockCount < flushEBCount; eBlockCount++)
          {
          status = TABLE_FlushEBInsert(devID, flushEBNum[eBlockCount].logicalEBNum,
                                       flushEBNum[eBlockCount].phyAddr, flushEBNum[eBlockCount].key);
          if(status != FTL_ERR_PASS)
            {
            return status;
            }
          SetFlushLogEBCounter(devID, flushEBNum[eBlockCount].key);
          }
        return FTL_ERR_FLUSH_FLUSH_GC_FAIL;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
        return status;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

        }
      }
    latestIncNumber++;
    SetFlushLogEBCounter(devID, latestIncNumber);
    physicalEBNum = GetPhysicalEBlockAddr(devID, logicalEBNum);
    status = TABLE_FlushEBInsert(devID, logicalEBNum, physicalEBNum, latestIncNumber);
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
    SetGCOrFreePageNum(devID, logicalEBNum, 1);
    MarkEBlockMappingTableEntryDirty(devID, logicalEBNum);
    logGCEBNum[eBlockCount] = logicalEBNum;
    }
  /* make dirty */
#if (CACHE_RAM_BD_MODULE == false)
  for(bitCount = 0; bitCount < BITS_EBLOCK_DIRTY_BITMAP_DEV_TABLE; bitCount++)
    {
    MarkEBlockMappingTableSectorDirty(devID, bitCount);
    }
  for(bitCount = 0; bitCount < BITS_PPA_DIRTY_BITMAP_DEV_TABLE; bitCount++)
    {
    MarkPPAMappingTableSectorDirty(devID, bitCount);
    }
#else

  for(flushTypeCnt = 0; flushTypeCnt < 2; flushTypeCnt++)
    {
    for(bitCount = 0; bitCount < MAX_EBLOCK_MAP_INDEX; bitCount++)
      {
      if((((bitCount * FLUSH_RAM_TABLE_SIZE) % ((uint16_t) (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD))) == 0))
        {
        logicalEBNum = (uint16_t) ((bitCount * FLUSH_RAM_TABLE_SIZE) / (uint16_t) (EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD));
        }
      else
        {
        logicalEBNum = (uint16_t) ((bitCount * FLUSH_RAM_TABLE_SIZE) / (uint16_t) ((EBLOCK_MAPPING_ENTRY_SIZE + CACHE_EBLOCK_MAPPING_ENTRY_PAD)) + 1);
        }

      // check dirty index.
      CACHE_IsPresentEB(devID, logicalEBNum, &present);
      if(CACHE_EBM_PPA_PRESENT == present)
        {
        if(FTL_ERR_PASS != (status = (CACHE_GetIndex(devID, logicalEBNum, &index, &dependency))))
          {
          return status;
          }
        if(true == CACHE_IsDirtyIndex(devID, index))
          {
          continue; // skip
          }
        }


#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
      for(count = 0; count < sizeof (sysEBlockFlushInfo.reserved); count++)
        {
        sysEBlockFlushInfo.reserved[count] = EMPTY_BYTE;
        }

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
      sysEBlockFlushInfo.tableCheckWord = EMPTY_WORD;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

      sysEBlockFlushInfo.eBlockNumLoc = EMPTY_WORD;
      sysEBlockFlushInfo.entryIndexLoc = EMPTY_WORD;
      sysEBlockFlushInfo.endPoint = EMPTY_BYTE;
      sysEBlockFlushInfo.logIncNum = EMPTY_DWORD;

      sysEBlockFlushInfo.tableOffset = bitCount;


      if(0 == flushTypeCnt)
        {
        // PPA
        sysEBlockFlushInfo.tableOffset = (uint16_t) (PPA_CACHE_TABLE_OFFSET * sysEBlockFlushInfo.tableOffset);
        sysEBlockFlushInfo.type = PPA_MAP_TABLE_FLUSH;
        for(count = 0; count < PPA_CACHE_TABLE_OFFSET; count++)
          {
          if(FTL_ERR_PASS != (status = CACHE_GetEBlockAndPPAMap(devID, (uint16_t) ((bitCount * PPA_CACHE_TABLE_OFFSET) + count), &eBlockPPAMapInfo, CACHE_PPAMAP)))
            {
            return status;
            }

          if((CACHE_EMPTY_ENTRY_INDEX == eBlockPPAMapInfo.entryIndex) && (CACHE_EMPTY_FLASH_LOG_ARRAY == eBlockPPAMapInfo.flashLogEBArrayCount))
            {
            continue;
            }

          for(eBlockCount = 0; eBlockCount < NUM_FLUSH_LOG_EBLOCKS; eBlockCount++)
            {
            if(flushEBNum[eBlockCount].cacheNum == eBlockPPAMapInfo.flashLogEBArrayCount)
              {
              phyEBlockAddrTmp = flushEBNum[eBlockCount].phyAddr;
              break;
              }

            }

          if((status = GetFlushLoc(devID, phyEBlockAddrTmp, eBlockPPAMapInfo.entryIndex, &flushStructPageInfo, &flushRAMTablePageInfo)) != FTL_ERR_PASS)
            {
            return status;
            }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
          if((FLASH_RamPageReadDataBlock(&flushRAMTablePageInfo, tempCache)) != FLASH_PASS)
            {
            return FTL_ERR_FLASH_READ_15;
            }
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
          flashStatus = FLASH_RamPageReadDataBlock(&flushRAMTablePageInfo, tempArray);
          memcpy((uint8_t *)&(tempCache[0]), &tempArray[FLUSH_INFO_SIZE], FLUSH_RAM_TABLE_SIZE);
          if(flashStatus != FLASH_PASS)
            {
            return FTL_ERR_FLASH_READ_06;
            }
#endif

          if((status = GetNextFlushEntryLocation(devID, &flushStructPageInfo,
                                                 &flushRAMTablePageInfo, &flushEB)) != FTL_ERR_PASS)
            {
            return status;
            }

          // set position
          eBlockPPAMapInfo.entryIndex = (uint16_t) GetGCNum(devID, flushEB);
          TABLE_GetFlushLogCacheEntry(devID, GetPhysicalEBlockAddr(devID, flushEB), &eBlockPPAMapInfo.flashLogEBArrayCount);

          if(FTL_ERR_PASS != (status = CACHE_SetEBlockAndPPAMap(devID, sysEBlockFlushInfo.tableOffset, &eBlockPPAMapInfo, CACHE_PPAMAP)))
            {
            return status;
            }

          // Inc free page
          IncGCOrFreePageNum(devID, flushEB);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
          if(FLASH_RamPageWriteDataBlock(&flushRAMTablePageInfo, tempCache) != FLASH_PASS)
            {
            return FTL_ERR_FLASH_WRITE_05;
            }

          // Write END SIGNATURE, @Time T2
          if((status = FTL_WriteFlushInfo(&flushStructPageInfo, &sysEBlockFlushInfo)) != FTL_ERR_PASS)
            {
            return status;
            }
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
          memcpy((uint8_t *) & tempArray[0], (uint8_t *) & sysEBlockFlushInfo, FLUSH_INFO_SIZE);
          memcpy((uint8_t *) & tempArray[FLUSH_INFO_SIZE], &tempCache[0], FLUSH_RAM_TABLE_SIZE);
          for(count2 = FLUSH_INFO_SIZE + FLUSH_RAM_TABLE_SIZE; count2 < SECTOR_SIZE; count2++)
            {
            tempArray[count2] = EMPTY_BYTE;
            }

          flushInfoEntry = (uint16_t *) & tempArray[0];
          flushInfoEntry[FLUSH_INFO_TABLE_CHECK_WORD] = CalcCheckWord(&flushInfoEntry[FLUSH_INFO_TABLE_START], (FLUSH_RAM_TABLE_SIZE / 2));
          flushInfoEntry[FLUSH_INFO_CHECK_WORD] = CalcCheckWord(&flushInfoEntry[FLUSH_INFO_DATA_START], FLUSH_INFO_DATA_WORDS);

          flashStatus = FLASH_RamPageWriteDataBlock(&flushRAMTablePageInfo, &tempArray[0]);
#endif

          sysEBlockFlushInfo.tableOffset++;
          }
        }


      if(1 == flushTypeCnt)
        {
        // EB
        sysEBlockFlushInfo.type = EBLOCK_MAP_TABLE_FLUSH;
        if(FTL_ERR_PASS != (status = CACHE_GetEBlockAndPPAMap(devID, bitCount, &eBlockPPAMapInfo, CACHE_EBLOCKMAP)))
          {
          return status;
          }

        if((CACHE_EMPTY_ENTRY_INDEX == eBlockPPAMapInfo.entryIndex) && (CACHE_EMPTY_FLASH_LOG_ARRAY == eBlockPPAMapInfo.flashLogEBArrayCount))
          {
          continue;
          }

        for(eBlockCount = 0; eBlockCount < NUM_FLUSH_LOG_EBLOCKS; eBlockCount++)
          {
          if(flushEBNum[eBlockCount].cacheNum == eBlockPPAMapInfo.flashLogEBArrayCount)
            {
            phyEBlockAddrTmp = flushEBNum[eBlockCount].phyAddr;
            break;
            }
          }

        if((status = GetFlushLoc(devID, phyEBlockAddrTmp, eBlockPPAMapInfo.entryIndex, &flushStructPageInfo, &flushRAMTablePageInfo)) != FTL_ERR_PASS)
          {
          return status;
          }

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
        if((FLASH_RamPageReadDataBlock(&flushRAMTablePageInfo, tempCache)) != FLASH_PASS)
          {
          return FTL_ERR_FLASH_READ_15;
          }
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

        flashStatus = FLASH_RamPageReadDataBlock(&flushRAMTablePageInfo, tempArray);
        memcpy((uint8_t *)&(tempCache[0]), &tempArray[FLUSH_INFO_SIZE], FLUSH_RAM_TABLE_SIZE);
        if(flashStatus != FLASH_PASS)
          {
          return FTL_ERR_FLASH_READ_06;
          }
#endif

        if((status = GetNextFlushEntryLocation(devID, &flushStructPageInfo,
                                               &flushRAMTablePageInfo, &flushEB)) != FTL_ERR_PASS)
          {
          return status; // go Flush GC.
          }

        // set position
        eBlockPPAMapInfo.entryIndex = (uint16_t) GetGCNum(devID, flushEB);
        TABLE_GetFlushLogCacheEntry(devID, GetPhysicalEBlockAddr(devID, flushEB), &eBlockPPAMapInfo.flashLogEBArrayCount);

        if(FTL_ERR_PASS != (status = CACHE_SetEBlockAndPPAMap(devID, sysEBlockFlushInfo.tableOffset, &eBlockPPAMapInfo, CACHE_EBLOCKMAP)))
          {
          return status;
          }

        // Inc free page
        IncGCOrFreePageNum(devID, flushEB);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
        if(FLASH_RamPageWriteDataBlock(&flushRAMTablePageInfo, tempCache) != FLASH_PASS)
          {
          return FTL_ERR_FLASH_WRITE_05;
          }

        // Write END SIGNATURE, @Time T2
        if((status = FTL_WriteFlushInfo(&flushStructPageInfo, &sysEBlockFlushInfo)) != FTL_ERR_PASS)
          {
          return status;
          }
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
        memcpy((uint8_t *) & tempArray[0], (uint8_t *) & sysEBlockFlushInfo, FLUSH_INFO_SIZE);
        memcpy((uint8_t *) & tempArray[FLUSH_INFO_SIZE], &tempCache[0], FLUSH_RAM_TABLE_SIZE);
        for(count2 = FLUSH_INFO_SIZE + FLUSH_RAM_TABLE_SIZE; count2 < SECTOR_SIZE; count2++)
          {
          tempArray[count2] = EMPTY_BYTE;
          }

        flushInfoEntry = (uint16_t *) & tempArray[0];
        flushInfoEntry[FLUSH_INFO_TABLE_CHECK_WORD] = CalcCheckWord(&flushInfoEntry[FLUSH_INFO_TABLE_START], (FLUSH_RAM_TABLE_SIZE / 2));
        flushInfoEntry[FLUSH_INFO_CHECK_WORD] = CalcCheckWord(&flushInfoEntry[FLUSH_INFO_DATA_START], FLUSH_INFO_DATA_WORDS);

        flashStatus = FLASH_RamPageWriteDataBlock(&flushRAMTablePageInfo, &tempArray[0]);
#endif

        }
      }
    }
#endif
  /* flush */
  if((status = TABLE_FlushDevice(devID, FLUSH_NORMAL_MODE)) != FTL_ERR_PASS)
    {
#if(FTL_DEFECT_MANAGEMENT == true)
    if(status == FTL_ERR_FLUSH_FLUSH_FAIL)
      {
      if((status = TABLE_FlushEBClear(devID)) != FTL_ERR_PASS)
        {
        return status;
        }
      for(eBlockCount = 0; eBlockCount < flushEBCount; eBlockCount++)
        {
        status = TABLE_FlushEBInsert(devID, flushEBNum[eBlockCount].logicalEBNum,
                                     flushEBNum[eBlockCount].phyAddr, flushEBNum[eBlockCount].key);
        if(status != FTL_ERR_PASS)
          {
          return status;
          }
        SetFlushLogEBCounter(devID, flushEBNum[eBlockCount].key);
        }
      for(eBlockCount = 0; eBlockCount < MIN_FLUSH_GC_EBLOCKS; eBlockCount++)
        {
        if(EMPTY_WORD != storeTemp[eBlockCount])
          {
          TABLE_InsertReservedEB(devID, storeTemp[eBlockCount]);
          }
        }
      return FTL_ERR_FLUSH_FLUSH_GC_FAIL;
      }
    else
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

      {
      return status;
      }
    }

  flashPageInfo.devID = devID;
  flashPageInfo.vPage.pageOffset = 0;
  flashPageInfo.byteCount = sizeof (SYS_EBLOCK_INFO);
  sysEBlockInfo.type = SYS_EBLOCK_INFO_FLUSH;
  sysEBlockInfo.checkVersion = EMPTY_WORD;
  sysEBlockInfo.oldSysBlock = EMPTY_WORD;
  sysEBlockInfo.fullFlushSig = EMPTY_WORD;
  for(bitCount = 0; bitCount < sizeof (sysEBlockInfo.reserved); bitCount++)
    {
    sysEBlockInfo.reserved[bitCount] = EMPTY_BYTE;
    }
  latestIncNumber = GetFlushEBCounter(devID);
  for(eBlockCount = (MIN_FLUSH_GC_EBLOCKS - 1); eBlockCount >= 0; eBlockCount--)
    {
    logicalEBNum = logGCEBNum[eBlockCount];
    physicalEBNum = GetPhysicalEBlockAddr(devID, logicalEBNum);
    /* Erase GC eblk */
    flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(physicalEBNum, 0);
    /* Write system eblk info to GC eblk */
    sysEBlockInfo.incNumber = latestIncNumber--;
    sysEBlockInfo.phyAddrThisEBlock = physicalEBNum;
#if (MIN_FLUSH_GC_EBLOCKS > 1)
    if(eBlockCount < (MIN_FLUSH_GC_EBLOCKS - 1))
      {
      sysEBlockInfo.fullFlushSig = FULL_FLUSH_SIGNATURE;
      }
#endif

#if(FTL_CHECK_VERSION == true)
    if(eBlockCount == 0)
      {
      sysEBlockInfo.checkVersion = CalcCheckWord((uint16_t *) FTL_FLASH_IMAGE_VERSION, NUM_WORDS_OF_VERSION);
      }
#endif  // #if(FTL_CHECK_VERSION == true)

    // moved the sys block writing function here, from above for PFT.
    if((status = FTL_WriteSysEBlockInfo(&flashPageInfo, &sysEBlockInfo)) != FTL_ERR_PASS)
      {
#if(FTL_DEFECT_MANAGEMENT == true)
      if(status == FTL_ERR_FAIL)
        {
        return status;
        }
      SetBadEBlockStatus(devID, logicalEBNum, true);
      if(FLASH_MarkDefectEBlock(&flashPageInfo) != FLASH_PASS)
        {
        // do nothing, just try to mark bad, even if it fails we move on.
        }
      if((status = TABLE_FlushEBClear(devID)) != FTL_ERR_PASS)
        {
        return status;
        }
      for(eBlockCount = 0; eBlockCount < flushEBCount; eBlockCount++)
        {
        status = TABLE_FlushEBInsert(devID, flushEBNum[eBlockCount].logicalEBNum,
                                     flushEBNum[eBlockCount].phyAddr, flushEBNum[eBlockCount].key);
        if(status != FTL_ERR_PASS)
          {
          return status;
          }
        SetFlushLogEBCounter(devID, flushEBNum[eBlockCount].key);
        }
      for(eBlockCount = 0; eBlockCount < MIN_FLUSH_GC_EBLOCKS; eBlockCount++)
        {
        if(EMPTY_WORD != storeTemp[eBlockCount])
          {
          TABLE_InsertReservedEB(devID, storeTemp[eBlockCount]);
          }
        }
      return FTL_ERR_FLUSH_FLUSH_GC_FAIL;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
      return status;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

      }
#if (ENABLE_EB_ERASED_BIT == true)
    SetEBErased(devID, logicalEBNum, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
    }
  /* Mark old flush log eblks */
  // It does not matter if the ECC is turned off for these records.
  // This operation is messing up the Check Word, so the Sys Info will be invalid anyway
  sysEBlockInfo.oldSysBlock = OLD_SYS_BLOCK_SIGNATURE;
  for(eBlockCount = 0; eBlockCount < flushEBCount; eBlockCount++)
    {
    logicalEBNum = flushEBNum[eBlockCount].logicalEBNum;
#if(FTL_DEFECT_MANAGEMENT == false)
    if((status = TABLE_InsertReservedEB(devID, logicalEBNum)) != FTL_ERR_PASS)
      {
      return status;
      }
#endif
    flashPageInfo.devID = devID;
    physicalEBNum = flushEBNum[eBlockCount].phyAddr;
    flashPageInfo.vPage.vPageAddr = CalcPhyPageAddrFromLogIndex(physicalEBNum, 0);
    flashPageInfo.vPage.pageOffset = (uint16_t) ((uint32_t) (&sysTempPtr->oldSysBlock));
    flashPageInfo.byteCount = OLD_SYS_BLOCK_SIGNATURE_SIZE;
    //     flashPageInfo.byteCount = sizeof(sysEBlockInfo.oldSysBlock);
    flashStatus = FLASH_RamPageWriteMetaData(&flashPageInfo, (uint8_t *) & sysEBlockInfo.oldSysBlock);
    if(flashStatus != FLASH_PASS)
      {
#if(FTL_DEFECT_MANAGEMENT == true)
      if(flashStatus == FLASH_PARAM)
        {
        return FTL_ERR_FLASH_WRITE_13;
        }
      SetBadEBlockStatus(devID, logicalEBNum, true);
      // just try to mark bad, even if it fails we move on.
      FLASH_MarkDefectEBlock(&flashPageInfo);

      checkBBMark = true;

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
      return FTL_ERR_FLASH_WRITE_13;
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

      }
#if(FTL_DEFECT_MANAGEMENT == true)
    if((status = TABLE_InsertReservedEB(devID, logicalEBNum)) != FTL_ERR_PASS)
      {
      return status;
      }
#endif
    }

#if(FTL_DEFECT_MANAGEMENT == true)
  // Erase the log entries, only if its not empty...
  while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
    {
    status = FTL_EraseAllTransLogBlocksOp(devID);
    if(FTL_ERR_MARKBB_COMMIT == status)
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
    return FTL_ERR_FLUSH_FLUSH_GC_FAIL;
    }
#else  // #if(FTL_DEFECT_MANAGEMENT == true)

  // Erase the log entries, only if its not empty...
  if((status = FTL_EraseAllTransLogBlocksOp(devID)) != FTL_ERR_PASS)
    {
    return status;
    }
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

  return FTL_ERR_PASS;
  }

//--------------------------------------------------------------

FTL_STATUS FTL_SetGCThreshold(uint8_t type, uint32_t threshold) /*  1,  4*/
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

  if((status = FTL_CheckMount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }
  switch(type)
    {
    case 0:
      GC_THRESHOLD = (uint16_t) threshold;
      break;
    case 1:
      Delete_GC_Threshold = (uint8_t) threshold;
      break;
    case 2:
      GC_THRESHOLD = (uint16_t) threshold;
      Delete_GC_Threshold = (uint8_t) threshold;
      break;
    default:
      status = FTL_ERR_GC_SET_THRESHOLD;
      break;
    }
  FTL_ClearMTLockBit();
  return status;
  }

// This is the entry point to the static wear leveling functionality.
// this needs to be called only when there is no pending GC.
// A new parameter, pickHottest, is added to FTL_InternalForcedGC.  FTL_InternalForcedGC()
//   passes this parameter to FTL_SwapDataReserveEB() which passes it to pickEBCandidate(). 
//   All callers of FTL_InternalForcedGC, except FTL_StaticWearLevelData(),
//   will set this parameter to false.

//---------------------------------------
#if (FTL_STATIC_WEAR_LEVELING == true)

FTL_STATUS FTL_StaticWearLevelData(void)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FTL_DEV devID = EMPTY_BYTE; /*1*/
  uint32_t eraseRange = 0; /*2*/
  uint16_t coldestEB = 0; /*2*/
  uint16_t freedUpPages = 0; /*2*/
  uint16_t freePageIndex = 0; /*2*/
#if (true == FTL_EBLOCK_CHAINING)
  uint16_t chainEB = 0;
#endif  // #if (true == FTL_EBLOCK_CHAINING)

  for(devID = 0; devID < NUMBER_OF_DEVICES; devID++)
    {
    eraseRange = FindEraseCountRange(devID, &coldestEB);
#if (DEBUG_CHECK_WL == true)
    if(EMTPY_WORD == eraseRange)
      {
      return FTL_ERR_FAIL;
      }
#endif
    if(eraseRange > FTL_DATA_WEAR_LEVEL_RANGE)
      {
#if (CACHE_RAM_BD_MODULE == true)
      if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, coldestEB, CACHE_WRITE_TYPE)))
        {
        return status;
        }
#endif
#if (true == FTL_EBLOCK_CHAINING)
      chainEB = GetChainLogicalEBNum(devID, coldestEB);
      if(EMPTY_WORD == chainEB)
        {
        if(coldestEB >= NUMBER_OF_DATA_EBLOCKS)
          {
          // Coldest EB is already in Reserved-Available pool
          continue;
          }
        }
      else
        {
        if(coldestEB >= NUMBER_OF_DATA_EBLOCKS)
          {
          // Coldest EB is already in Reserved-Available pool
          coldestEB = chainEB;
          // Coldest EB is changed to chainEB in DataEB, so CACHE_LoadEB needs to be invoked again.
#if (CACHE_RAM_BD_MODULE == true)
          if(FTL_ERR_PASS != (status = CACHE_LoadEB(devID, coldestEB, CACHE_WRITE_TYPE)))
            {
            return status;
            }
#endif
          }
        }
#else  // #if (true == FTL_EBLOCK_CHAINING)
      if(coldestEB >= NUMBER_OF_DATA_EBLOCKS)
        {
        // Coldest EB is already in Reserved-Available pool
        continue;
        }
#endif  // #else  // #if (true == FTL_EBLOCK_CHAINING)
      //        StaticWLInfo.staticWLCallCounter++;

#if(FTL_DEFECT_MANAGEMENT == true)
      status = InternalForcedGCWithBBManagement(devID, coldestEB, &freedUpPages, &freePageIndex, true);

#else
      status = FTL_InternalForcedGC(devID, coldestEB, &freedUpPages, &freePageIndex, true);
#endif

      if(status != FTL_ERR_PASS)
        {
        return status;
        }

#if(FTL_DEFECT_MANAGEMENT == true)
      if(GetTransLogEBFailedBadBlockInfo() == true)
        {
        if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS)
          {
          return status;
          }
        }
      ClearTransLogEBBadBlockInfo();
#endif

      }
    }
  return FTL_ERR_PASS;
  }
#endif  // #if (FTL_STATIC_WEAR_LEVELING != true)

//--------------------------------

uint32_t FindEraseCountRange(FTL_DEV devID, uint16_t * coldestEB)
  {
  uint16_t logicalBlockNum = 0; /*2*/
#if (CACHE_RAM_BD_MODULE == false)
  uint32_t eraseCount = 0; /*2*/
#endif
  uint32_t lowestCount = EMPTY_DWORD; /*2*/
  uint32_t highestCount = 0; /*2*/

  // If the group of coldest EBs includes one of the Reserved-Available EBs, 
  //   we want to select that one.  Therefore, we search that group first
#if (CACHE_RAM_BD_MODULE == true)
  GetSaveStaticWL(devID, &logicalBlockNum, &highestCount, CACHE_WL_HIGH);
  GetSaveStaticWL(devID, coldestEB, &lowestCount, CACHE_WL_LOW);
#else
  for(logicalBlockNum = NUMBER_OF_DATA_EBLOCKS; logicalBlockNum < NUM_EBLOCKS_PER_DEVICE; logicalBlockNum++)
    {
#if (DEBUG_CHECK_WL == false)
    if(FTL_ERR_PASS == TABLE_CheckUsedSysEB(devID, logicalBlockNum))
      {
      continue;
      }
#endif
    eraseCount = GetTrueEraseCount(devID, logicalBlockNum);
    if(lowestCount > eraseCount)
      {
      *coldestEB = logicalBlockNum;
      lowestCount = eraseCount;
      }
    if(highestCount < eraseCount)
      {
      highestCount = eraseCount;
      }
    }
  for(logicalBlockNum = 0; logicalBlockNum < NUMBER_OF_DATA_EBLOCKS; logicalBlockNum++)
    {
    eraseCount = GetTrueEraseCount(devID, logicalBlockNum);
    if(lowestCount > eraseCount)
      {
      *coldestEB = logicalBlockNum;
      lowestCount = eraseCount;
      }
    if(highestCount < eraseCount)
      {
      highestCount = eraseCount;
      }
    }
#endif

#if (DEBUG_CHECK_WL == true)
  if(FTL_ERR_PASS == TABLE_CheckUsedSysEB(devID, *coldestEB))
    {
    DBG_Printf("Warning FindEraseCountRange: Get System EB 0x%X\n", *coldestEB, 0);
    return EMTPY_WORD;
    }
#endif

  return highestCount - lowestCount;
  }

//-----------------------------
// This function replaces the function of the same name
//   If pickHottest is true, this function will return the Hottest candidate instead of the Coldest

uint16_t pickEBCandidate(EMPTY_LIST_PTR emptyList, uint16_t totalEmpty, uint8_t pickHottest)
  {
  uint16_t count = 0; /*2*/
  uint32_t selectedScore = EMPTY_DWORD; /*4*/
  uint16_t selectedIndex = 0; /*2*/

  // If pickHottest = false, the entry with the lowest weight is picked
  // If pickHottest = true, the entry with the highest weight is picked
  if(true == pickHottest)
    {
    selectedScore = 0;
    }
  for(count = 0; count < totalEmpty; count++)
    {
    if((false == pickHottest) && (emptyList[count].isErased == true))
      {
      return emptyList[count].logEBNum;
      }
    if(((true == pickHottest) && (selectedScore < emptyList[count].eraseScore)) ||
       ((false == pickHottest) && (selectedScore > emptyList[count].eraseScore)))
      {
      selectedScore = emptyList[count].eraseScore;
      selectedIndex = count;
      }
    }
  return emptyList[selectedIndex].logEBNum;
  }

FTL_STATUS FTL_CheckForGCLogSpace(FTL_DEV devID)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t latestIncNumber = EMPTY_DWORD; /*4*/
  uint16_t logLogicalEBNum = EMPTY_WORD; /*2*/
  uint16_t logPhyEBAddr = EMPTY_WORD; /*2*/
  uint16_t entryIndex = 0; /*2*/
  uint16_t numLogEntries = 0; /*2*/

  latestIncNumber = GetTransLogEBCounter(devID);
  status = TABLE_TransLogEBGetLatest(devID, &logLogicalEBNum, &logPhyEBAddr, latestIncNumber);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }

  // Get free index
  entryIndex = GetFreePageIndex(devID, logLogicalEBNum);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  // + 1 to account for the type A
  numLogEntries = NUM_GC_TYPE_B + 1;

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  numLogEntries = 2;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

#if(FTL_ENABLE_UNUSED_EB_SWAP  == true)
  /* for EB swap entry */
  numLogEntries++;
#if(FTL_EBLOCK_CHAINING == true)
  numLogEntries++;
#endif  // #if(FTL_EBLOCK_CHAINING == true)
#endif  // #if(FTL_ENABLE_UNUSED_EB_SWAP  == true)

  if((entryIndex + numLogEntries) > NUM_LOG_ENTRIES_PER_EBLOCK)
    {
#if(FTL_DEFECT_MANAGEMENT == true)
    if(GetTransLogEBArrayCount(devID) < (NUM_TRANSACTLOG_EBLOCKS - LOG_EBLOCK_RETRIES))

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    if(GetTransLogEBArrayCount(devID) < NUM_TRANSACTLOG_EBLOCKS)
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

      {
      if((status = CreateNextTransLogEBlock(devID, logLogicalEBNum)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
    else
      {
      status = TABLE_Flush(FLUSH_NORMAL_MODE);
      if(FTL_ERR_PASS != status)
        {
        return status;
        }
      }
    }
  return status;
  }


#if(FTL_DEFECT_MANAGEMENT == true)
//---------------------------

uint32_t GetBBPageMoved(uint16_t pageOffset)
  {
  return badBlockPhyPageAddr[pageOffset];
  }

FTL_STATUS InternalForcedGCWithBBManagement(FTL_DEV devID, uint16_t logicalEBNum,
                                            uint16_t * FreedUpPages, uint16_t * freePageIndex, uint8_t WLflag)
  {
  FTL_STATUS GC_result = FTL_ERR_PASS;
  FTL_STATUS badBlockStatus = FTL_ERR_PASS;
  uint16_t GCCount = 0;
  do
    {
    GC_result = FTL_InternalForcedGC(devID, logicalEBNum, FreedUpPages, freePageIndex, WLflag);
    if(GC_result != FTL_ERR_PASS)
      {
      if(GCCount >= MAX_BAD_BLOCK_SANITY_TRIES)
        {
        return (GC_result); //exit the transfer
        }
      else
        {
        GCCount++;
        badBlockStatus = TranslateBadBlockError(GC_result);
        if(badBlockStatus == FTL_ERR_LOG_WR)
          {
          // log write failure
          badBlockStatus = BB_ManageBadBlockErrorForGCLog();
          }
        else if(badBlockStatus == FTL_ERR_BAD_BLOCK_SOURCE)
          {
          // UnlinkGC failure
          badBlockStatus = BB_ManageBadBlockErrorForSource();
          }
        else if(badBlockStatus == FTL_ERR_BAD_BLOCK_TARGET)
          {
          // data write failure
          badBlockStatus = BB_ManageBadBlockErrorForTarget(); // all flash changes will be in the reserve pool
          }
        else if(badBlockStatus != FTL_ERR_PASS)
          {
          return badBlockStatus;
          }
        }
      }
    ClearBadBlockInfo();

    }
  while(GC_result != FTL_ERR_PASS);

  return GC_result;
  }

/*---------------------------------------
This function will pick an new block, upate the chaining and EB mapping info, erase it if needed. copy hte pages and return
before calling this function, the eblock mapping table has to be restored before calling this function.
//---------------------------------------*/
FTL_STATUS BadBlockCopyPages(FTL_DEV devID, uint16_t logicalEBNum)
  {

  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FLASH_STATUS flashStatus = FLASH_PASS;
#if (ENABLE_EB_ERASED_BIT == true)
  uint8_t eraseStatus = false; /*1*/
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

  uint16_t phyFromEBlock = EMPTY_WORD; /*2*/
  uint16_t phyToEBlock = EMPTY_WORD; /*2*/
  uint16_t phyPageOffset = EMPTY_WORD;
  FLASH_PAGE_INFO pageInfo = {0, 0,
    {0, 0}}; /*11*/
  uint16_t logToEBlock = EMPTY_WORD; /*2*/
  uint16_t logicalPageOffset = EMPTY_WORD;
  uint16_t pageCount = 0; /*2*/
  uint8_t isMoved = false; /*1*/
  uint16_t sanityCounter = 0;

  pageInfo.devID = devID;
  pageInfo.vPage.pageOffset = 0;
  pageInfo.byteCount = NUMBER_OF_BYTES_PER_PAGE;

  phyFromEBlock = GetPhysicalEBlockAddr(devID, logicalEBNum);

  do
    {
    sanityCounter++;
    if(sanityCounter > MAX_BAD_BLOCK_SANITY_TRIES)
      {
      return status;
      }

    pageCount = 0;
    status = FTL_ERR_PASS;

    // Identify EB to copy to, not chaining, regular GC, so swap will not erase, nor create a log, no need to worry about bad block
    if((status = FTL_SwapDataReserveEBlock(devID, logicalEBNum, &phyToEBlock, &logToEBlock, false, true)) != FTL_ERR_PASS)
      {
      return status;
      }

#if (ENABLE_EB_ERASED_BIT == true)
    eraseStatus = GetEBErased(devID, logicalEBNum);

    if(false == eraseStatus)
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
      {
      status = FTL_EraseOp(devID, logicalEBNum);
      if(FTL_ERR_PASS != status)
        {
        if(FTL_ERR_FAIL == status)
          {
          return FTL_ERR_GC_ERASE3;
          }
        SetBadEBlockStatus(devID, logicalEBNum, true);
        if(FLASH_MarkDefectEBlock(&pageInfo) != FLASH_PASS)
          {
          // do nothing, just try to mark bad, even if it fails we move on.
          }
        continue;
        }
      }

#if (ENABLE_EB_ERASED_BIT == true)
    SetEBErased(devID, logicalEBNum, false); /*since the EB will be written to now*/
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

    MarkEBlockMappingTableEntryDirty(devID, logicalEBNum);

    // -------------------------------------------------------------------------------------------------
    // COPY PAGES
    for(logicalPageOffset = 0; logicalPageOffset < NUM_PAGES_PER_EBLOCK; logicalPageOffset++)
      {

      phyPageOffset = GetPPASlot(devID, logicalEBNum, logicalPageOffset);
      if((EMPTY_INVALID != phyPageOffset) && (CHAIN_INVALID != phyPageOffset))
        {
        // Page must be valid - Copy Page
        pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyFromEBlock, phyPageOffset);
        flashStatus = FLASH_RamPageReadDataBlock(&pageInfo, &pseudoRPB[devID][0]);
        if(FLASH_PASS != flashStatus)
          {
          return FTL_ERR_GC_BB_LOAD1;
          }
        pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyToEBlock, phyPageOffset);
        flashStatus = FLASH_RamPageWriteDataBlock(&pageInfo, &pseudoRPB[devID][0]);
        if(FLASH_PASS != flashStatus)
          {
          status = FTL_ERR_GC_BB_PAGE_WR1;
          if(FLASH_PARAM == flashStatus)
            {
            return status;
            }
          SetBadEBlockStatus(devID, logicalEBNum, true);
          if(FLASH_MarkDefectEBlock(&pageInfo) != FLASH_PASS)
            {
            // do nothing, just try to mark bad, even if it fails we move on.
            }
          break;
          }
        pageCount++;
        }

      }
    if(FTL_ERR_PASS != status)
      {
      continue;
      }
    if((GC_Info.devID != EMPTY_BYTE) && (GCMoveArrayNotEmpty == true))
      {
      for(logicalPageOffset = 0; logicalPageOffset < NUM_PAGES_PER_EBLOCK; logicalPageOffset++)
        {
        IsPageMoved(logicalPageOffset, &isMoved);
        if(isMoved == true)
          {
          pageInfo.vPage.vPageAddr = GetBBPageMoved(logicalPageOffset);
          if(pageInfo.vPage.vPageAddr != EMPTY_DWORD)
            {
            flashStatus = FLASH_RamPageReadDataBlock(&pageInfo, &pseudoRPB[devID][0]);
            if(FLASH_PASS != flashStatus)
              {
              return FTL_ERR_GC_BB_LOAD2;
              }
            phyPageOffset = GetFreePageIndex(devID, logicalEBNum);
            pageInfo.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(phyToEBlock, phyPageOffset);
            flashStatus = FLASH_RamPageWriteDataBlock(&pageInfo, &pseudoRPB[devID][0]);
            if(FLASH_PASS != flashStatus)
              {
              status = FTL_ERR_GC_BB_PAGE_WR2;
              if(FLASH_PARAM == flashStatus)
                {
                return status;
                }
              SetBadEBlockStatus(devID, logicalEBNum, true);
              if(FLASH_MarkDefectEBlock(&pageInfo) != FLASH_PASS)
                {
                // do nothing, just try to mark bad, even if it fails we move on.
                }
              break;
              }
            pageCount++;
            }
          }
        }
      }
    }
  while(status != FTL_ERR_PASS);

  if((GC_Info.devID != EMPTY_BYTE) && (GCMoveArrayNotEmpty == true))
    {
    for(logicalPageOffset = 0; logicalPageOffset < NUM_PAGES_PER_EBLOCK; logicalPageOffset++)
      {
      IsPageMoved(logicalPageOffset, &isMoved);
      if(isMoved == true)
        {
        pageInfo.vPage.vPageAddr = GetBBPageMoved(logicalPageOffset);
        if(pageInfo.vPage.vPageAddr != EMPTY_DWORD)
          {
          phyPageOffset = GetFreePageIndex(devID, logicalEBNum);
          UpdatePageTableInfo(devID, logicalEBNum, logicalPageOffset, phyPageOffset, BLOCK_INFO_VALID_PAGE);
          }
        }
      }
    }
#if DEBUG_CHECK_TABLES
  if(pageCount > NUM_PAGES_PER_EBLOCK)
    {
    return FTL_ERR_GC_BB_PAGE_SANITY;
    }
  status = DBG_CheckPPAandBitMap(devID, logicalEBNum);
  if(FTL_ERR_PASS != status)
    {
    return status;
    }
#endif  // #if DEBUG_CHECK_TABLES    
  return FTL_ERR_PASS;
  }



//----------------------------------------

FTL_STATUS BadBlockEraseFailure(FTL_DEV devID, uint16_t eBlockNum)
  {
  FTL_STATUS status = FTL_ERR_FAIL;
  FLASH_PAGE_INFO flashPage = {0, 0,
    {0, 0}}; /*11*/
  uint16_t sanityCounter = 0;
  uint16_t phyToEBlock = EMPTY_WORD;
  uint16_t logToEBlock = EMPTY_WORD;

#if (ENABLE_EB_ERASED_BIT == true)
  uint8_t eraseStatus = false; /*1*/
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

  while((status != FTL_ERR_PASS) && (sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES))
    {
    sanityCounter++;
    SetBadEBlockStatus(devID, eBlockNum, true);
    flashPage.devID = devID;
    flashPage.byteCount = 0;
    flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(devID, eBlockNum), 0);
    flashPage.vPage.pageOffset = 0;
    if(FLASH_MarkDefectEBlock(&flashPage) != FLASH_PASS)
      {
      // do nothing, just try to mark bad, even if it fails we move on;             
      }
    if((status = FTL_SwapDataReserveEBlock(devID, eBlockNum, &phyToEBlock, &logToEBlock, false, true)) != FTL_ERR_PASS)
      {
      return status;
      }
#if (ENABLE_EB_ERASED_BIT == true)
    eraseStatus = GetEBErased(devID, eBlockNum);

#if DEBUG_PRE_ERASED
    if(true == eraseStatus)
      {
      DBG_Printf("BadBlockEraseFailure: EBlock 0x%X is already erased\n", eBlockNum, 0);
      }
#endif  // #if DEBUG_PRE_ERASED
    if(false == eraseStatus)
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
      {
      status = FTL_EraseOp(devID, eBlockNum);
      if(FTL_ERR_PASS != status)
        {
        if(FTL_ERR_FAIL == status)
          {
          return status;
          }
        continue;
        }
      }
    }
  return status;
  }

FTL_STATUS BB_ManageBadBlockErrorForSource(void)
  {
  /*        // the strategy for dealing with data area writes is to 
          1) mark the block bad, 
          2) copy the orginal data using the saved EB mapping info to a new block in the reseve pool, 
          3) swap the bad the good block
          4) overwrite the old EB mapping table with the saved one
          5) update the phy eb info to point to the new block
          5) flush
          6) restart the transfer by building the transfer map again
   */
  FTL_STATUS status = FTL_ERR_PASS;
  FLASH_PAGE_INFO flashPage = {0, 0,
    {0, 0}}; /*11*/
  uint16_t badPhyEB = EMPTY_WORD;
  uint16_t BBLogicalEB = EMPTY_WORD;
  uint8_t chainFlag = false;

  if(badBlockInfo.operation == FTL_ERR_CHAIN_NOT_FULL_EB)
    {
    BB_FindBBInChainedEBs(&BBLogicalEB, &chainFlag, &badPhyEB);
    }
  RestoreSourceBadBlockInfo();
  RestoreTargetBadBlockInfo();
#if (ENABLE_EB_ERASED_BIT == true)
  SetEBErased(badBlockInfo.devID, badBlockInfo.sourceLogicalEBNum, false); /*since the EB will be written to now*/
  if(badBlockInfo.targetLogicalEBNum != EMPTY_WORD)
    {
    SetEBErased(badBlockInfo.devID, badBlockInfo.targetLogicalEBNum, false); /*since the EB will be written to now*/
    }
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
  // set the erase bit for both the blocks
  if(badBlockInfo.operation == FTL_ERR_CHAIN_NOT_FULL_EB)
    {
    SetBadEBlockStatus(badBlockInfo.devID, BBLogicalEB, true);
    flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(badBlockInfo.devID, BBLogicalEB), 0);
    }
  else
    {
    SetBadEBlockStatus(badBlockInfo.devID, badBlockInfo.sourceLogicalEBNum, true);
    flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(badBlockInfo.devID, badBlockInfo.sourceLogicalEBNum), 0);
    }
  status = BadBlockCopyPages(badBlockInfo.devID, badBlockInfo.sourceLogicalEBNum);
  if(status != FTL_ERR_PASS)
    {
    return status;
    }
  if((badBlockInfo.devID != EMPTY_BYTE) && (badBlockInfo.sourceLogicalEBNum != EMPTY_WORD))
    {
    flashPage.devID = badBlockInfo.devID;
    flashPage.byteCount = 0;
    flashPage.vPage.pageOffset = 0;
    if(FLASH_MarkDefectEBlock(&flashPage) != FLASH_PASS)
      {
      // do nothing, just try to mark bad, even if it fails we move on;             
      }
    }
  if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS) // to save the bad block mark
    {
    FTL_ClearMTLockBit();
    return status;
    }
  ClearBadBlockInfo();
  return status;
  }

FTL_STATUS BB_ManageBadBlockErrorForChainErase(void)
  {
  FTL_STATUS status = FTL_ERR_PASS;
  FLASH_PAGE_INFO flashPage = {0, 0,
    {0, 0}}; /*11*/

  RestoreTargetBadBlockInfo();
  if((badBlockInfo.devID != EMPTY_BYTE) && (badBlockInfo.sourceLogicalEBNum != EMPTY_WORD))
    {
    RestoreSourceBadBlockInfo();
    SetBadEBlockStatus(badBlockInfo.devID, badBlockInfo.sourceLogicalEBNum, true);
    flashPage.devID = badBlockInfo.devID;
    flashPage.byteCount = 0;
    flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(badBlockInfo.devID, badBlockInfo.sourceLogicalEBNum), 0);
    flashPage.vPage.pageOffset = 0;
    if(FLASH_MarkDefectEBlock(&flashPage) != FLASH_PASS)
      {
      // do nothing, just try to mark bad, even if it fails we move on;             
      }
    }

  if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS) // to save the bad block mark
    {
    FTL_ClearMTLockBit();
    return status;
    }
  ClearBadBlockInfo();

  // 1 Mark Block Bad
  // 5 simple flush 
  return status;
  }

FTL_STATUS BB_FindBBInChainedEBs(uint16_t * BBlogicalEB, uint8_t * chainFlag, uint16_t * badPhyEB)
  {
  FTL_STATUS status = FTL_ERR_PASS;
  uint32_t physicalPage = EMPTY_DWORD;
  uint16_t phyEBNum = EMPTY_WORD;
  uint16_t physicalEBNum = EMPTY_WORD;
  uint32_t currentLBA = EMPTY_DWORD;
  uint32_t logicalPageAddr = EMPTY_DWORD;
  uint16_t logicalEBNum = EMPTY_WORD;
  uint16_t chainEBNum = EMPTY_WORD;
  FTL_DEV devID = EMPTY_BYTE;
  uint16_t index = EMPTY_WORD;

  index = FTL_GetCurrentIndex();
  if(index == 0)
    {
    return FTL_ERR_GC_BB_INDEX;
    }
  else
    {
    index--;
    }

  *BBlogicalEB = EMPTY_WORD;
  currentLBA = GetTMStartLBA(index);
  devID = GetTMDevID(index);
  physicalPage = GetTMPhyPage(index);


  if((currentLBA != EMPTY_DWORD) && (devID != EMPTY_BYTE))
    {
    status = GetLogicalEBNum(physicalPage, &physicalEBNum);
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
    status = GetPageNum(currentLBA, &logicalPageAddr);
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
    status = GetLogicalEBNum(logicalPageAddr, &logicalEBNum);
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
    phyEBNum = GetPhysicalEBlockAddr(devID, logicalEBNum);

    if(physicalEBNum == phyEBNum)
      {
      *badPhyEB = phyEBNum;
      *BBlogicalEB = logicalEBNum;
      *chainFlag = false;
      }
    else
      {
#if(FTL_EBLOCK_CHAINING == true)
      chainEBNum = GetChainLogicalEBNum(devID, logicalEBNum);
      if(chainEBNum != EMPTY_WORD)
        {
        phyEBNum = GetPhysicalEBlockAddr(devID, chainEBNum);
        if(physicalEBNum == phyEBNum)
          {
          *badPhyEB = phyEBNum;
          *BBlogicalEB = chainEBNum;
          *chainFlag = true;
          }
        else
          {
          *badPhyEB = EMPTY_WORD;
          *BBlogicalEB = EMPTY_WORD;
          *chainFlag = false;
          }
        }
      else
#endif
        {
        *badPhyEB = EMPTY_WORD;
        *BBlogicalEB = EMPTY_WORD;
        *chainFlag = false;
        }
      }
    }
  else
    {
    return FTL_ERR_GC_BB_CHAIN;
    }
  return status;
  }

FTL_STATUS BB_ManageBadBlockErrorForTarget(void)
  {
  FTL_STATUS status = FTL_ERR_PASS;
  FLASH_PAGE_INFO flashPage = {0, 0,
    {0, 0}}; /*11*/

  RestoreSourceBadBlockInfo();
  if((badBlockInfo.devID != EMPTY_BYTE) && (badBlockInfo.targetLogicalEBNum != EMPTY_WORD))
    {
    RestoreTargetBadBlockInfo();
    SetBadEBlockStatus(badBlockInfo.devID, badBlockInfo.targetLogicalEBNum, true);
    flashPage.devID = badBlockInfo.devID;
    flashPage.byteCount = 0;
    flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(badBlockInfo.devID, badBlockInfo.targetLogicalEBNum), 0);
    flashPage.vPage.pageOffset = 0;
    if(FLASH_MarkDefectEBlock(&flashPage) != FLASH_PASS)
      {
      // do nothing, just try to mark bad, even if it fails we move on;             
      }
    }

  if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS) // to save the bad block mark
    {
    FTL_ClearMTLockBit();
    return status;
    }
  ClearBadBlockInfo();

  // 1 Mark Block Bad
  // 5 simple flush 
  return status;
  }

FTL_STATUS BB_ManageBadBlockErrorForGCLog(void)
  {
  FTL_STATUS status = FTL_ERR_PASS;
  //    nothing to do here, managed by the System area bad block system 
  // 1 Mark Block Bad
  // 5 simple flush
  // TABLE_Flush() should not be called.
  /*
  if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS) // to save the bad block mark
  {
     FTL_ClearMTLockBit();
     return status;
  }
   */
  return status;
  }

#endif
