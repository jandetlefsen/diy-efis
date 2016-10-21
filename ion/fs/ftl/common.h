#ifndef FTL_COMMON_H
#define FTL_COMMON_H

#include "debug.h"
#include "calc.h"


#ifdef __cplusplus
extern "C"
  {
#endif

#if (FTL_DEFECT_MANAGEMENT == true)
  uint8_t GetBadEBlockStatus(FTL_DEV devID, uint16_t logicalEBNum);
  void SetBadEBlockStatus(FTL_DEV devID, uint16_t logicalEBNum, uint8_t badBlockStatus);
#endif
  FTL_STATUS TRANS_InitTransMap(void);
  void TABLE_ClearFreeBitMap(FTL_DEV devID, uint16_t eBlockNum);
  void TABLE_ClearMappingTable(FTL_DEV devID, uint16_t logicalEBNum, uint16_t phyEBAddr, uint32_t eraseCount);
  FTL_STATUS TABLE_InitMappingTable(void);
  void TABLE_ClearPPATable(FTL_DEV devID, uint16_t eBlockNum);
  FTL_STATUS TABLE_InitPPAMappingTable(void);
  FTL_STATUS FTL_WriteSysEBlockInfo(FLASH_PAGE_INFO_PTR flashPagePtr, SYS_EBLOCK_INFO_PTR sysInfoPtr);
  uint8_t GetTMDevID(uint16_t index);
  void SetTMDevID(uint16_t index, FTL_DEV devId);
  uint8_t GetTMNumSectors(uint16_t index);
  void SetTMNumSectors(uint16_t index, uint8_t sec);
  uint32_t GetTMStartLBA(uint16_t index);
  void SetTMStartLBA(uint16_t index, uint32_t lba);
  TRANS_MAP_ENTRY_PTR GetTMPointer(uint16_t index);
  uint32_t GetTMPhyPage(uint16_t index);
  void SetTMPhyPage(uint16_t index, uint32_t phyPage);
  uint32_t GetTMMergePage(uint16_t index);
  void SetTMMergePage(uint16_t index, uint32_t mergePage);
  uint8_t GetTMStartSector(uint16_t index);
  void SetTMStartSector(uint16_t index, uint8_t sector);
  void SetTMLogInfo(uint16_t index, uint16_t value);
  uint16_t GetTMLogInfo(uint16_t index);
  void DecGCOrFreePageNum(FTL_DEV devID, uint16_t logicalEBNum);
  uint16_t GetTotalFreePages(FTL_DEV devID, uint16_t logicalEBNum);
  FTL_STATUS INIT_InitBasic(void);
  uint16_t GetPhysicalEBlockAddr(FTL_DEV devID, uint16_t logicalBlockNum);
  uint16_t GetLogicalEBlockAddr(FTL_DEV devID, uint16_t physicalBlockNum);
  void SetPhysicalEBlockAddr(FTL_DEV devID, uint16_t logicalBlockNum, uint16_t phyBlockNum);
  uint32_t GetEraseCount(FTL_DEV devID, uint16_t logicalBlockNum);
  void SetEraseCount(FTL_DEV devID, uint16_t logicalBlockNum, uint32_t eraseCount);
  void IncEraseCount(FTL_DEV devID, uint16_t logicalBlockNum);
  uint16_t GetDirtyCount(FTL_DEV devID, uint16_t logicalBlockNum);
  void SetDirtyCount(FTL_DEV devID, uint16_t logicalBlockNum, uint16_t dirtyCount);
  uint16_t GetFreePageIndex(FTL_DEV devID, uint16_t logicalBlockNum);
  void SetFreePageIndex(FTL_DEV devID, uint16_t logicalBlockNum, uint16_t freePageIdx);
  uint32_t GetGCNum(FTL_DEV devID, uint16_t logicalBlockNum);

#if(FTL_EBLOCK_CHAINING == true)
  uint16_t GetChainLogicalEBNum(FTL_DEV devID, uint16_t logicalBlockNum);
  uint16_t GetChainPhyEBNum(FTL_DEV devID, uint16_t logicalBlockNum);
  void SetChainLogicalEBNum(FTL_DEV devID, uint16_t logicalBlockNum, uint16_t logEBNum);
  void SetChainPhyEBNum(FTL_DEV devID, uint16_t logicalBlockNum, uint16_t PhyEBNum);
  void SetChainLink(FTL_DEV devID, uint16_t logEBNumFrom, uint16_t logEBNumTo,
                    uint16_t phyEBNumFrom, uint16_t phyEBNumTo);
  void ClearChainLink(FTL_DEV devID, uint16_t logicalBlockNumFrom, uint16_t logicalBlockNumTo);
  uint16_t GetLongestChain(FTL_DEV devID);
  uint16_t GetChainWithLowestVaildUsedRatio(FTL_DEV devID);
  uint16_t GetChainWithLowestVaildPages(FTL_DEV devID);
#endif  // #if(FTL_EBLOCK_CHAINING == true)

  FREE_BIT_MAP_TYPE GetEBlockMapFreeBitIndex(FTL_DEV devID, uint16_t logicalBlockNum, uint16_t phyPageOffset);
  void SetEBlockMapFreeBitIndex(FTL_DEV devID, uint16_t logicalBlockNum, uint16_t phyPageOffset, FREE_BIT_MAP_TYPE value);
  void SetGCOrFreePageNum(FTL_DEV devID, uint16_t logicalBlockNum, uint32_t GCNum);
  void ClearEBlockMapFreeBitIndex(FTL_DEV devID, uint16_t logicalEBNum);
  void MarkEBlockMappingTableSectorDirty(FTL_DEV devID, uint16_t sector);
  void MarkEBlockMappingTableEntryDirty(FTL_DEV devID, uint16_t logicalEBNum);
  void MarkEBlockMappingTableSectorClean(FTL_DEV devID, uint16_t sector);
  uint8_t IsEBlockMappingTableSectorDirty(FTL_DEV devID, uint16_t sector);
  uint16_t GetPPASlot(FTL_DEV devID, uint16_t logicalEBNum, uint16_t logicalPageOffset);
  void SetPPASlot(FTL_DEV devID, uint16_t logicalEBNum, uint16_t logicalPageOffset, uint16_t value);
  void MarkPPAMappingTableSectorDirty(FTL_DEV devID, uint16_t sector);
  void MarkPPAMappingTableEntryDirty(FTL_DEV devID, uint16_t logicalEBNum, uint16_t logicalPageOffset);
  void MarkPPAMappingTableSectorClean(FTL_DEV devID, uint16_t sector);
  uint8_t IsPPAMappingTableSectorDirty(FTL_DEV devID, uint16_t sector);
  FTL_STATUS TABLE_InitEBOrderingTable(FTL_DEV devID);
  FTL_STATUS TABLE_ClearReservedEB(FTL_DEV devID);
  FTL_STATUS TABLE_InsertReservedEB(FTL_DEV devID, uint16_t logicalAddr);
  FTL_STATUS TABLE_GetReservedEB(FTL_DEV devID, uint16_t * logicalAddrPtr, uint8_t WLflag);
  uint16_t TABLE_GetReservedEBlockNum(FTL_DEV devID); /* changed by Nobu Feb 18, 2015 : uint8_t -> uint16_t for ML16G2 */
  uint16_t TABLE_GetUsedSysEBCount(FTL_DEV devID);
  FTL_STATUS TABLE_CheckUsedSysEB(FTL_DEV devID, uint16_t logicalAddr);
  FTL_STATUS TABLE_FlushEBInsert(FTL_DEV devID, uint16_t logicalAddr, uint16_t phyEBAddr, uint32_t key);
  FTL_STATUS TABLE_FlushEBRemove(FTL_DEV devID, uint16_t blockNum);
  FTL_STATUS TABLE_FlushEBGetNext(FTL_DEV devID, uint16_t * logicalAddrPtr, uint16_t * phyEBAddrPtr, uint32_t * keyPtr);
  FTL_STATUS TABLE_FlushEBGetLatest(FTL_DEV devID, uint16_t * flushEBlockPtr,
                                    uint16_t * phyEBAddrPtr, uint32_t keyPtr);
  FTL_STATUS TABLE_FlushEBClear(FTL_DEV devID);
#if (CACHE_RAM_BD_MODULE == false)
  FTL_STATUS AdjustFlushEBlockFreePage(FTL_DEV devID, uint16_t logicalFlushEBlockNum, uint16_t dirtyBitMapCount);
#endif
  FTL_STATUS RestoreFlushEBlockFreePage(FTL_DEV devID, uint16_t logicalFlushEBlockNum);
  FTL_STATUS TABLE_TransLogEBInsert(FTL_DEV devID, uint16_t logicalAddr, uint16_t phyEBAddr, uint32_t key);
  FTL_STATUS TABLE_TransLogEBGetNext(FTL_DEV devID, uint16_t * logicalAddrPtr, uint16_t * phyEBAddrPtr, uint32_t * keyPtr);
  FTL_STATUS TABLE_TransEBClear(FTL_DEV devID);
  FTL_STATUS TABLE_TransLogEBRemove(FTL_DEV devID, uint16_t blockNum);
  void TABLE_SortTransTable(FTL_DEV devID);
  FTL_STATUS TABLE_TransLogEBGetLatest(FTL_DEV devID, uint16_t * LogEBlockPtr,
                                       uint16_t * phyEBAddrPtr, uint32_t keyPtr);
  uint16_t GetTransLogEBArrayCount(FTL_DEV devID);
  uint32_t GetTransLogEBCounter(FTL_DEV devID);
  void SetTransLogEBCounter(FTL_DEV devID, uint32_t counter);
  uint32_t GetFlushEBCounter(FTL_DEV devID);
  void SetFlushLogEBCounter(FTL_DEV devID, uint32_t counter);
  uint16_t GetFlushLogEBArrayCount(FTL_DEV devID);
  FTL_STATUS TABLE_GetTransLogEntry(FTL_DEV devID, uint16_t blockNum, uint16_t * logicalEBNumPtr, uint16_t * phyAddrPtr, uint32_t * keyPtr);
  FTL_STATUS TABLE_SetTransLogEntry(FTL_DEV devID, uint16_t blockNum, uint16_t logicalEBNum, uint16_t phyAddr, uint32_t key);
  FTL_STATUS TABLE_GetFlushLogEntry(FTL_DEV devID, uint16_t blockNum, uint16_t * logicalEBNumPtr, uint16_t * phyAddrPtr, uint32_t * keyPtr);
  FTL_STATUS TABLE_SetFlushLogEntry(FTL_DEV devID, uint16_t blockNum, uint16_t logicalEBNum, uint16_t phyAddr, uint32_t key);
#if (CACHE_RAM_BD_MODULE == true)
#if (CACHE_DYNAMIC_ALLOCATION == true)
  FTL_STATUS CACHE_DynamicAllocation(uint32_t total_ram_allowed);
#endif
  FTL_STATUS TABLE_GetFlushLogCacheEntry(FTL_DEV devID, uint16_t phyAddr, uint8_t * cacheNum_ptr);
  FTL_STATUS TABLE_GetFlushLogPhyEntry(FTL_DEV devID, uint8_t cacheNum, uint16_t * phyAddr_ptr);
  FTL_STATUS TABLE_GetFlushLogCountEntry(FTL_DEV devID, uint8_t cacheNum, uint16_t * blockNum_ptr);
  FTL_STATUS ClearSaveStaticWL(FTL_DEV devID, uint16_t logicalEBNum, uint32_t eraseCount, uint8_t hlFlag);
  FTL_STATUS SetSaveStaticWL(FTL_DEV devID, uint16_t logicalEBNum, uint32_t eraseCount);
  FTL_STATUS GetSaveStaticWL(FTL_DEV devID, uint16_t * logicalEBNum, uint32_t * eraseCount, uint8_t hlFlag);
#endif
  FTL_STATUS FTL_SwapDataReserveEBlock(FTL_DEV devID, uint16_t logicalPageNum, uint16_t * ptrPhyReservedBlock, uint16_t * ptrLogicalReservedBlock, uint8_t WLflag, uint8_t badBlockFlagIn);
  FTL_STATUS TABLE_Flush(uint8_t flushMode);
  FTL_STATUS TABLE_LoadFlushTable(void);
  FTL_STATUS TRANS_ClearTransMap(void);
  FTL_STATUS FTL_CheckForFreePages(ADDRESS_STRUCT_PTR startPage, ADDRESS_STRUCT_PTR endPage, uint32_t totalPages);
  FTL_STATUS GetPhyPageAddr(ADDRESS_STRUCT_PTR currentPage, uint16_t phyEBNum,
                            uint16_t logEBNum, uint32_t * phyPage);
  FTL_STATUS ClearGC_Info(void);
  FTL_STATUS ClearMergeGC_Info(FTL_DEV DevID, uint16_t logicalEBNum, uint32_t logicalPageNum);
  FTL_STATUS UpdateTransferMap(uint32_t currentLBA, ADDRESS_STRUCT_PTR currentPage,
                               ADDRESS_STRUCT_PTR endPage, ADDRESS_STRUCT_PTR startPage, uint32_t totalPages,
                               uint32_t phyPage, uint32_t mergePage, uint8_t isWrite, uint8_t isChained);
  FTL_STATUS ClearGCPageBitMap(void);
  FTL_STATUS SetPageMoved(uint16_t pageAddress, uint32_t phyPageAddr);
  FTL_STATUS IsPageMoved(uint32_t pageAddr, uint8_t * isMoved);

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  FTL_STATUS TABLE_InitTransLogEntry(void);
  FTL_STATUS InsertEntryIntoLogEntry(uint16_t index, uint32_t phyPageAddr,
                                     uint32_t currentLBA, ADDRESS_STRUCT_PTR currentPage, uint8_t isChained);
  FTL_STATUS ProcessPageLoc(FTL_DEV devID, LOG_PHY_PAGE_LOCATPTR pageLocPtr, uint32_t pageAddress);
  FTL_STATUS UpdateRAMTablesUsingTransLogs(FTL_DEV devID);
  FTL_STATUS FTL_ClearA(void);
  FTL_STATUS FTL_ClearB(uint16_t count);
  FTL_STATUS FTL_ClearC(uint16_t seqNum);
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

  void SetOldPageIndex(FTL_DEV devID, uint16_t logicalBlockNum, uint16_t oldPageIdx);
  FTL_STATUS FTL_WriteLogInfo(FLASH_PAGE_INFO_PTR flashPagePtr, uint8_t * logPtr);
  FTL_STATUS FTL_WriteFlushInfo(FLASH_PAGE_INFO_PTR flashPagePtr, SYS_EBLOCK_FLUSH_INFO_PTR flushInfoPtr);
  void IncGCOrFreePageNum(FTL_DEV devID, uint16_t logicalEBNum);
  FTL_STATUS GetNextFlushEntryLocation(FTL_DEV devID, FLASH_PAGE_INFO_PTR flushInfoPtr, FLASH_PAGE_INFO_PTR flushRamTablePtr, uint16_t * logicalBlockNumPtr);
  FTL_STATUS CreateNextFlushEntryLocation(FTL_DEV devID, uint16_t logicalBlockNum);
  FTL_STATUS GetFlushLoc(FTL_DEV devID, uint16_t phyEBlockAddr, uint16_t freePageIndex, FLASH_PAGE_INFO_PTR flushInfoPtr, FLASH_PAGE_INFO_PTR flushRamTablePtr);
  FTL_STATUS FTL_GetNextTransferMapEntry(uint16_t * nextEntryIndex, uint16_t * startIndex, uint16_t * endIndex);
  uint16_t FTL_GetCurrentIndex(void);
  void SetUsedPageIndex(FTL_DEV devID, uint16_t logicalBlockNum, uint16_t usedPageIdx);
  FTL_STATUS TABLE_InitGcNum(void);
  void TABLE_SortFlushTable(FTL_DEV devID);
  FTL_STATUS GetNextLogEntryLocation(FTL_DEV devID, FLASH_PAGE_INFO_PTR pageInfoPtr);
  FTL_STATUS CreateNextLogEntryLocation(FTL_DEV devID, uint16_t logicalBlockNum);
  FTL_STATUS CreateNextTransLogEBlock(FTL_DEV devID, uint16_t logicalBlockNum);
  FTL_STATUS UpdateEBOrderingTable(FTL_DEV devID, uint16_t startEB, uint16_t * formatCount);
  FTL_STATUS CheckFlushSpace(FTL_DEV devID);
  FTL_STATUS FTL_EraseOp(FTL_DEV devID, uint16_t logicalEBNum);
  FTL_STATUS FTL_EraseOpNoDirty(FTL_DEV devID, uint16_t logicalEBNum);
  FTL_STATUS FTL_EraseAllTransLogBlocksOp(FTL_DEV devID);
  uint32_t CalcFlushRamTablePages(uint16_t phyEBNum, uint16_t index);
  uint16_t CalcFlushRamTableOffset(uint16_t index);
  FTL_STATUS LoadRamTable(FLASH_PAGE_INFO_PTR flashPage, uint8_t * ramTablePtr, uint16_t tableOffset, uint32_t devTableSize);
  FTL_STATUS GetTransLogsSetRAMTables(FTL_DEV devID, LOG_ENTRY_LOC_PTR startLoc, uint8_t * ramTablesUpdated, uint8_t * logEblockFount);
  FTL_STATUS FTL_FindEmptyTransLogEBlock(FTL_DEV devID, uint16_t * logicalEBNumPtr, uint16_t * physicalEBNumPtr);
  FTL_STATUS UpdateRAMTablesUsingGCLogs(FTL_DEV devID, GC_LOG_ENTRY_PTR ptrGCLog);

#if(FTL_EBLOCK_CHAINING == true)
  FTL_STATUS UpdateRAMTablesUsingChainLogs(FTL_DEV devID, CHAIN_LOG_ENTRY_PTR chainLogPtr);
#endif  // #if(FTL_EBLOCK_CHAINING == true)

#if (FTL_ENABLE_UNUSED_EB_SWAP == true)
  FTL_STATUS UpdateRAMTablesUsingEBSwapLogs(FTL_DEV devID, EBSWAP_LOG_ENTRY_PTR EBSwapLogPtr);
  FTL_STATUS CreateSwapEBLog(FTL_DEV devID, uint16_t logicalDataEB, uint16_t logicalReservedEB);
#endif  // #if (FTL_ENABLE_UNUSED_EB_SWAP == true)

#if(FTL_UNLINK_GC == true)
  FTL_STATUS UnlinkChain(FTL_DEV devID, uint16_t logicalBlockNumFrom, uint16_t logicalBlockNumTo);
  FTL_STATUS UpdateRAMTablesUsingUnlinkLogs(FTL_DEV devID, UNLINK_LOG_ENTRY_PTR ptrUnlinkLog);
#endif  // #if(FTL_UNLINK_GC == true)

  uint16_t FTL_GetNumLogEntries(ADDRESS_STRUCT_PTR startPage, uint32_t totalPages);

  FTL_STATUS TABLE_FlushDevice(FTL_DEV devID, uint8_t flushMode);
  FTL_STATUS TRANS_ClearEntry(uint16_t index);
  uint32_t GetTotalDirtyBitCnt(FTL_DEV devID);
  uint16_t GetNumFreePages(FTL_DEV devID, uint16_t logicalEBNum);
  uint16_t GetNumValidPages(FTL_DEV devID, uint16_t logicalEBNum);
  uint16_t GetNumUsedPages(FTL_DEV devID, uint16_t logicalEBNum);
  uint16_t GetNumInvalidPages(FTL_DEV devID, uint16_t logicalEBNum);
  void GetNumValidUsedPages(FTL_DEV devID, uint16_t logicalEBNum, uint16_t * used, uint16_t * valid);
  FTL_STATUS Flush_GC(FTL_DEV devID);
  void UpdatePageTableInfo(FTL_DEV devID, uint16_t logicalBlockNum, uint16_t logicalPageIndex,
                           uint16_t phyPageIdx, uint8_t bitStatus);
  FTL_STATUS DBG_CheckPPAandBitMap(FTL_DEV devID, uint16_t logicalEBNum);
  FTL_STATUS DBG_CheckMappingTables(void);

  extern GC_SAVE gcSave[MAX_BLOCKS_TO_SAVE];
  extern uint16_t gcSaveCount;
  void FTL_ClearGCSave(uint8_t clearMode);
  FTL_STATUS FTL_AddToGCSave(FTL_DEV devId, uint16_t phyEbNum);
  FTL_STATUS FTL_CheckForGCLogSpace(FTL_DEV devID);
  /***ERASE COUNT**********/
#if (ENABLE_EB_ERASED_BIT == true)
  uint8_t GetEBErased(FTL_DEV devID, uint16_t logEBNum);
  void SetEBErased(FTL_DEV devID, uint16_t logEBNum, uint8_t eraseStatus);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)

#if (FTL_SUPER_SYS_EBLOCK == true)
  FTL_STATUS FTL_WriteSuperInfo(FLASH_PAGE_INFO_PTR flashPagePtr, uint8_t * logPtr);
  FTL_STATUS FTL_FindSuperSysEB(FTL_DEV devID);
  FTL_STATUS GetSuperSysInfoLogs(FTL_DEV devID, uint16_t * storePhySysEB, uint8_t * checkSuperPF);
  FTL_STATUS SetSysEBRamTable(FTL_DEV devID, uint16_t * storeSysEB, uint16_t * formatCount);
  FTL_STATUS FTL_CheckForSuperSysEBLogSpace(FTL_DEV devID, uint8_t mode);
  FTL_STATUS FTL_CreateSuperSysEBLog(FTL_DEV devID, uint8_t mode);

  FTL_STATUS GetNextSuperSysEBEntryLocation(FTL_DEV devID, FLASH_PAGE_INFO_PTR pageInfoPtr, uint16_t * entryIndexPtr);
  FTL_STATUS CreateNextSuperSystemEBlockOp(FTL_DEV devID);

  FTL_STATUS FTL_FindEmptySuperSysEBlock(FTL_DEV devID, uint16_t * logicalEBNumPtr, uint16_t * physicalEBNumPtr);
  FTL_STATUS FTL_FindAllAreaSuperSysEBlock(FTL_DEV devID, uint16_t * findDataEBNumPtr, uint16_t * findSystemEBNumPtr);
  FTL_STATUS DataGCForSuperSysEB(void);
  FTL_STATUS ClearSuperEBInfo(void);
  void SetSuperSysEBCounter(FTL_DEV devID, uint32_t counter);
  uint32_t GetSuperSysEBCounter(FTL_DEV devID);
  FTL_STATUS TABLE_GetSuperSysEBEntry(FTL_DEV devID, uint16_t blockNum, uint16_t * logEBlockPtr, uint16_t * phyAddrPtr, uint32_t * keyPtr);
  FTL_STATUS TABLE_SuperSysEBClear(FTL_DEV devID);
  FTL_STATUS TABLE_SuperSysEBInsert(FTL_DEV devID, uint16_t logicalAddr, uint16_t phyEBAddr, uint32_t key);
  FTL_STATUS TABLE_SuperSysEBGetLatest(FTL_DEV devID, uint16_t * logEBlockPtr, uint16_t * phyEBAddrPtr, uint32_t key);
  FTL_STATUS TABLE_SuperSysEBGetNext(FTL_DEV devID, uint16_t * logicalAddrPtr, uint16_t * phyEBAddrPtr, uint32_t * keyPtr);
  void TABLE_SortSuperTable(FTL_DEV devID);
  FTL_STATUS TABLE_SuperSysEBRemove(FTL_DEV devID, uint16_t blockNum);
  FTL_STATUS TABLE_GetPhySysEB(FTL_DEV devID, uint16_t * countPtr, uint16_t * phyEBAddrPtr);
  FTL_STATUS TABLE_CheckUsedSuperEB(FTL_DEV devID, uint16_t logicalAddr);
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

  void SwapUnusedEBlock(FTL_DEV devID, uint16_t logicalDataEB, uint16_t logicalReservedEB);
  void MarkAllPagesStatus(FTL_DEV devID, uint16_t logEBNum, uint8_t bitStatus);
  FTL_STATUS FindAndSwapUnusedEB(FTL_DEV devID, uint16_t logFrom, uint16_t * logTo);
  uint32_t GetTrueEraseCount(FTL_DEV devID, uint16_t logicalBlockNum);

#ifndef FTL_RAM_TABLES_C
#if (CACHE_RAM_BD_MODULE == false)
  extern EBLOCK_MAPPING_ENTRY EBlockMappingTable[NUM_DEVICES][NUM_EBLOCKS_PER_DEVICE];
  extern PPA_MAPPING_ENTRY PPAMappingTable[NUM_DEVICES][NUM_EBLOCKS_PER_DEVICE][NUM_PAGES_PER_EBLOCK];
  extern uint8_t PPAMappingTableDirtyBitMap[NUM_DEVICES][PPA_DIRTY_BITMAP_DEV_TABLE_SIZE];
  extern uint8_t EBlockMappingTableDirtyBitMap[NUM_DEVICES][EBLOCK_DIRTY_BITMAP_DEV_TABLE_SIZE];
#else
#if (CACHE_DYNAMIC_ALLOCATION == true)
  extern uint32_t gSave_Total_ram_allowed;
  extern uint8_t gCheckFirst;
  extern uint8_t ** EBlockMappingCache;
  extern uint8_t ** PPAMappingCache;
  extern uint16_t * EBMCacheIndex;

  extern uint16_t numBlockMapIndex;
  extern uint16_t numPpaMapIndex;
  extern uint16_t cacheIndexChangeArea;
  extern uint16_t thesholdDirtyCount;
  extern uint16_t ebmCacheIndexSize;
  extern uint16_t eblockMappingCacheSize;
  extern uint16_t ppaMappingCacheSize;
#endif
#endif
  extern KEY_TABLE_ENTRY FlushLogEBArray[NUM_DEVICES][NUM_FLUSH_LOG_EBLOCKS];
  extern KEY_TABLE_ENTRY TransLogEBArray[NUM_DEVICES][NUM_TRANSACTLOG_EBLOCKS];
  extern uint16_t FlushLogEBArrayCount[NUM_DEVICES];
  extern uint16_t TransLogEBArrayCount[NUM_DEVICES];
  extern uint32_t TransLogEBCounter[NUM_DEVICES];
  extern uint32_t FlushLogEBCounter[NUM_DEVICES];
  extern uint32_t GCNum[NUM_DEVICES];
  extern GC_INFO GC_Info;
  extern uint16_t GC_THRESHOLD;
  extern TRANSFER_MAP_STRUCT transferMap[NUM_TRANSFER_MAP_ENTRIES];
  extern uint16_t TransferMapIndexEnd;
  extern uint16_t TransferMapIndexStart;
  extern FTL_DEV previousDevice;
  extern uint8_t GCMoveArray[NUM_PAGES_PER_EBLOCK];
  extern uint8_t GCMoveArrayNotEmpty;

#if (FTL_SUPER_SYS_EBLOCK == true)
  extern SUPER_EB_INFO SuperEBInfo[NUM_DEVICES];
  extern uint8_t gProtectForSuperSysEBFlag;
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  extern uint8_t packedSuperInfo[SECTOR_SIZE];
#endif
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  extern TRANS_LOG_ENTRY TransLogEntry;
  extern uint16_t TranslogBEntries;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

  extern uint8_t pseudoRPB[NUM_DEVICES][NUMBER_OF_BYTES_PER_PAGE];
  extern uint32_t LastTransLogLba;
  extern uint8_t LastTransLogNpages;
  extern uint8_t Delete_GC_Threshold;
  extern uint8_t FTL_initFlag;
  extern uint8_t FTL_UpdatedFlag;
  extern uint8_t FTL_DeleteFlag;

#if(FTL_DEFECT_MANAGEMENT == true)
  extern BAD_BLOCK_INFO badBlockInfo;
  extern uint32_t badBlockPhyPageAddr[NUM_PAGES_PER_EBLOCK];
#endif
#if (SPANSCRC32 == true)
  extern uint32_t crc32_table[256];
#endif

#endif  // #ifndef FTL_RAM_TABLES_C

  void ClearTransferEB(void);
  void SetTransferEB(ADDRESS_STRUCT_PTR startPage, ADDRESS_STRUCT_PTR endPage);
  void GetTransferEB(uint8_t eblockCount, TRANSFER_EB_PTR transferEBPtr);
  FTL_STATUS ClearDeleteInfo(void);
  FTL_STATUS InitFTLRAMSTables(void);
  FTL_STATUS InitDeleteInfo(uint8_t devID, uint32_t logicalPageAddr, uint8_t startSector, uint8_t numSectors);
  FTL_STATUS HitDeleteInfo(uint8_t devID, uint32_t logicalPageAddr);
  FTL_STATUS UpdateDeleteInfo(uint8_t startSector, uint8_t numSectors);
  uint8_t GetDeleteInfoNumSectors(void);

#if(FTL_RPB_CACHE == true)
  uint8_t GetRPBCacheStatus(uint8_t devID);
  void SetRPBCacheStatus(uint8_t devID, uint8_t status);
  uint32_t GetRPBCacheLogicalPageAddr(uint8_t devID);
  void SetRPBCacheLogicalPageAddr(uint8_t devID, uint32_t logicalPageAddr);
  void ClearRPBCache(uint8_t devID);
  uint8_t * GetRPBCache(uint8_t devID);
  FTL_STATUS InitRPBCache(void);
  FTL_STATUS UpdateRPBCache(uint8_t devID, uint32_t logicalPageAddr, uint32_t startSector, uint32_t numSectors, uint8_t * byteBuffer);
  FTL_STATUS ReadRPBCache(uint8_t devID, uint32_t logicalPageAddr, uint32_t startSector, uint32_t numSectors, uint8_t * byteBuffer);
  FTL_STATUS FlushRPBCache(uint8_t devID);
  FTL_STATUS FillRPBCache(uint8_t devID, uint32_t logicalPageAddr);
  FTL_STATUS ReadFlash(uint8_t devID, uint32_t logicalPageAddr, uint32_t startSector, uint32_t NB, uint8_t * byteBuffer);
  FTL_STATUS RPBCacheForWrite(uint8_t * *byteBuffer, uint32_t * LBA, uint32_t * NB, uint32_t * bytesDone);
  FTL_STATUS RPBCacheForRead(uint8_t * *byteBuffer, uint32_t * LBA, uint32_t * NB, uint32_t * bytesDone);
  extern RPB_CACHE_READ_GROUP RPBCacheReadGroup;
#endif  // #if(FTL_RPB_CACHE == true)

#if(FTL_CHECK_ERRORS == true)
  extern uint8_t mountStatus;
  extern uint8_t lockStatus;
#endif  // #if (FTL_CHECK_ERRORS == true)

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  FTL_STATUS InitPackedLogs(void);
  FTL_STATUS CopyPackedLogs(uint16_t offset, uint8_t * logPtr);
  FTL_STATUS WritePackedLogs(FLASH_PAGE_INFO_PTR flashPagePtr);
  FTL_STATUS ReadPackedGCLogs(uint8_t * logPtr, GC_LOG_ENTRY_PTR ptrGCLog);

#if(FTL_UNLINK_GC == true)
  FTL_STATUS ReadPackedUnlinkLogs(uint8_t * logPtr, UNLINK_LOG_ENTRY_PTR ptrUnlinkLog);
#endif  // #if(FTL_UNLINK_GC == true)

  FTL_STATUS VerifyRamTable(uint16_t * tablePtr);
  FTL_STATUS FTL_WriteSpareInfo(FLASH_PAGE_INFO_PTR flashPagePtr, SPARE_INFO_PTR spareInfoPtr);
  FTL_STATUS GetSpareInfoSetPPATable(void);

  extern uint8_t packedLog[SECTOR_SIZE];
  extern uint8_t writeLogFlag;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

  FTL_STATUS FTL_InternalFormat(void);
  FTL_STATUS FTL_InternalForcedGC(FTL_DEV DeviceID, uint16_t logicalEBNum, uint16_t * FreedUpPages,
                                  uint16_t * freePageIndex, uint8_t flag);

  FTL_STATUS FTL_CheckMount_SetMTLockBit(void);
  FTL_STATUS FTL_CheckUnmount_SetMTLockBit(void);
  FTL_STATUS FTL_SetMTLockBit(void);
  FTL_STATUS FTL_ClearMTLockBit(void);
  void FTL_SetMountBit(void);
  void FTL_ClearMountBit(void);
  FTL_STATUS FTL_CheckDevID(uint8_t DevID);
  FTL_STATUS FTL_CheckRange(uint32_t LBA, uint32_t NB);
  FTL_STATUS FTL_CheckPointer(void *Ptr);
  FTL_STATUS ResetIndexValue(FTL_DEV devID, LOG_ENTRY_LOC_PTR startLoc);

  // WEAR LEVELLING
  uint32_t FindEraseCountRange(FTL_DEV devID, uint16_t * coldestEB);
  FTL_STATUS FTL_StaticWearLevelData(void);
  uint32_t GetStaticWLThreshold(void);
  void IncStaticWLCount(void);
  void CheckStaticWLCount(void);
  extern STATIC_WL_INFO StaticWLInfo;
  FTL_STATUS InitStaticWLInfo(void);

#if(FTL_CHECK_VERSION == true)
  FTL_STATUS FTL_CheckVersion(void);
#endif // #if(FTL_CHECK_VERSION == true)

  void Init_PseudoRPB(void);

#if (FTL_DEFECT_MANAGEMENT == true)
  FTL_STATUS BadBlockEraseFailure(FTL_DEV devID, uint16_t eBlockNum);
  void StoreSourceBadBlockInfo(FTL_DEV devID, uint16_t logicalEB, uint16_t currentOperation);
  void StoreTargetBadBlockInfo(FTL_DEV devID, uint16_t logicalEB, uint16_t currentOperation);
  FTL_STATUS BadBlockCopyPages(FTL_DEV devID, uint16_t logicalEBNum);
  void ClearSourceBadBlockInfo(void);
  void ClearTargetBadBlockInfo(void);
  void RestoreSourceBadBlockInfo(void);
  void RestoreTargetBadBlockInfo(void);
  void ClearBadBlockInfo(void);
  void ClearTransLogEBBadBlockInfo(void);
  void SetTransLogEBFailedBadBlockInfo(void);
  uint8_t GetTransLogEBFailedBadBlockInfo(void);
  void SetTransLogEBNumBadBlockInfo(uint16_t logicalEBNum);
  uint16_t GetTransLogEBNumBadBlockInfo(void);
  FTL_STATUS TransLogEBFailure(FLASH_PAGE_INFO_PTR flashPagePtr, uint8_t * logPtr);

  FTL_STATUS isBadBlockError(FTL_STATUS status);
  FTL_STATUS TranslateBadBlockError(FTL_STATUS status);
  FTL_STATUS BB_FindBBInChainedEBs(uint16_t * BBlogicalEB, uint8_t * chainFlag, uint16_t * badPhyEB);
  FTL_STATUS BB_ManageBadBlockErrorForTarget(void);
  FTL_STATUS BB_ManageBadBlockErrorForGCLog(void);
  FTL_STATUS BB_ManageBadBlockErrorForSource(void);
  FTL_STATUS BB_ManageBadBlockErrorForChainErase(void);
  FTL_STATUS InternalForcedGCWithBBManagement(FTL_DEV devID, uint16_t logicalEBNum,
                                              uint16_t * FreedUpPages, uint16_t * freePageIndex, uint8_t WLflag);
  uint32_t GetBBPageMoved(uint16_t pageOffset);
#endif



#if (CACHE_RAM_BD_MODULE == true)
#if (CACHE_DYNAMIC_ALLOCATION == false)
  extern uint8_t EBlockMappingCache[NUM_EBLOCK_MAP_INDEX * NUMBER_OF_DEVICES][FLUSH_RAM_TABLE_SIZE];
  extern uint8_t PPAMappingCache[NUM_PPA_MAP_INDEX * NUMBER_OF_DEVICES][FLUSH_RAM_TABLE_SIZE];

  extern uint16_t EBMCacheIndex[NUM_EBLOCK_MAP_INDEX * NUMBER_OF_DEVICES];
#else
  extern uint8_t ** EBlockMappingCache;
  extern uint8_t ** PPAMappingCache;

  extern uint16_t *EBMCacheIndex;
#endif // #if (CACHE_DYNAMIC_ALLOCATION == false)
  extern uint16_t EBlockMapIndex[MAX_EBLOCK_MAP_INDEX * NUMBER_OF_DEVICES];
  extern uint16_t PPAMapIndex[MAX_PPA_MAP_INDEX * NUMBER_OF_DEVICES];
  extern uint32_t RamMapIndex[NUMBER_OF_ERASE_BLOCKS * NUMBER_OF_DEVICES];

  extern uint8_t gCounterLRU;
  extern uint16_t gCounterDirty;
  extern uint16_t gDataAreaCounterDirty;
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  extern uint8_t gCheckPfForNand;
  extern uint16_t gTargetPftEBForNand;
#endif

  extern SAVE_STATIC_WL SaveStaticWL[NUMBER_OF_DEVICES];
  extern SAVE_CAHIN_VAILD_USED_PAGE SaveValidUsedPage[(NUM_CHAIN_EBLOCKS * NUMBER_OF_DEVICES)];

  extern uint16_t gCrossedLEB[NUM_CROSS_LEB];

  FTL_STATUS GetLogEntryLocation(FTL_DEV devID, LOG_ENTRY_LOC_PTR nextLoc);

  // Main Interface
  FTL_STATUS CACHE_LoadEB(FTL_DEV devID, uint16_t logicalEBNum, uint8_t typeAPI);
  FTL_STATUS CACHE_GetRAMOffsetEB(FTL_DEV devID, uint16_t logicalEBNum, uint32_t * EBMramStructPtr, uint32_t * PPAramStructPtr);
  FTL_STATUS CACHE_MarkEBDirty(FTL_DEV devID, uint16_t logicalEBNum);
  FTL_STATUS CACHE_ClearAll(void);
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  FTL_STATUS CACHE_SetPfEBForNAND(FTL_DEV devID, uint16_t logicalEBNum, uint8_t flag);
  uint8_t CACHE_IsPfEBForNAND(FTL_DEV devID, uint16_t logicalEBNum);
  FTL_STATUS CACHE_ClearAllPfEBForNAND(FTL_DEV devID);
#endif // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

  // Index Basis Interface
  FTL_STATUS CACHE_AllocateIndex(FTL_DEV devID, uint16_t logicalEBNum, uint16_t * index_ptr, uint8_t * dependency_ptr, uint8_t typeAPI);
  FTL_STATUS CACHE_FindIndexForAlign(FTL_DEV devID, uint16_t * index_ptr, uint16_t skipIndex, uint16_t skipIndex2, uint16_t removed);
  FTL_STATUS CACHE_FindIndexForCross(FTL_DEV devID, uint16_t logicalEBNum, uint16_t * index_ptr, uint8_t * dependency_ptr, uint8_t typeAPI);
  FTL_STATUS CACHE_RemoveIndex(FTL_DEV devID, uint16_t index);
  FTL_STATUS CACHE_MoveIndex(FTL_DEV devID, uint16_t fromIndex, uint16_t toIndex);
  FTL_STATUS CACHE_NoDpendIndex(FTL_DEV devID, uint16_t index);
  FTL_STATUS CACHE_IsThesholdDirtyIndex(void);
#ifdef DEBUG_PROTOTYPE
  FTL_STATUS TABLE_Flush(uint8_t flushMode);
#endif
  FTL_STATUS CACHE_UpdateLRUIndex(FTL_DEV devID, uint16_t index, uint8_t typeAPI);
  FTL_STATUS CACHE_CleanAllDirtyIndex(FTL_DEV devID);

  // EB Basis Interface
  FTL_STATUS CACHE_InsertEB(FTL_DEV devID, uint16_t logicalEBNum, uint16_t index, uint8_t dependency, uint8_t present, uint8_t typeAPI);
  FTL_STATUS CACHE_RemoveEB(FTL_DEV devID, uint16_t logicalEBNum);
  FTL_STATUS CACHE_GetSector(uint16_t logicalEBNum, uint32_t * sector_ptr);
  uint8_t * CACHE_GetEBMCachePtr(FTL_DEV devID, uint16_t index);
  uint8_t * CACHE_GetPPACachePtr(FTL_DEV devID, uint16_t index);

  // Index EB Translation Interface
  FTL_STATUS CACHE_GetIndex(FTL_DEV devID, uint16_t logicalEBNum, uint16_t * index_ptr, uint8_t * dependency_ptr);
  FTL_STATUS CACHE_GetEB(FTL_DEV devID, uint16_t index, uint16_t * logicalEBNum_ptr);

  // Query Interface of Index State
  FTL_STATUS CACHE_IsFreeIndex(FTL_DEV devID, uint16_t index);
  FTL_STATUS CACHE_IsCleanIndex(FTL_DEV devID, uint16_t index);
  FTL_STATUS CACHE_IsDirtyIndex(FTL_DEV devID, uint16_t index);

  // Query Interface of LEB State
  FTL_STATUS CACHE_IsPresentEB(FTL_DEV devID, uint16_t logicalEBNum, uint8_t * present_ptr);
  FTL_STATUS CACHE_IsCrossedEB(FTL_DEV devID, uint16_t logicalEBNum);

  // Structure Access Interface of EBlockMapIndex and PPAMapIndex
  FTL_STATUS CACHE_SetEBlockAndPPAMap(FTL_DEV devID, uint16_t index, CACHE_INFO_EBLOCK_PPAMAP_PTR eBlockPPAMapInfo_ptr, uint8_t type);
  FTL_STATUS CACHE_GetEBlockAndPPAMap(FTL_DEV devID, uint16_t index, CACHE_INFO_EBLOCK_PPAMAP_PTR eBlockPPAMapInfo_ptr, uint8_t type);
  FTL_STATUS CACHE_ClearEBlockandPPAMap(FTL_DEV devID, uint16_t index, uint8_t type);

  // Structure Access Interface of RamMapIndex
  FTL_STATUS CACHE_SetRamMap(FTL_DEV devID, uint16_t logicalEBNum, CACHE_INFO_RAMMAP_PTR ramMapInfo_ptr);
  FTL_STATUS CACHE_GetRamMap(FTL_DEV devID, uint16_t logicalEBNum, CACHE_INFO_RAMMAP_PTR ramMapInfo_ptr);
  FTL_STATUS CACHE_ClearRamMap(FTL_DEV devID, uint16_t logicalEBNum);
  FTL_STATUS CACHE_MoveRamMap(FTL_DEV devID, uint16_t fromIndex, uint16_t toIndex);

  // Structure Access Interface of EBMCacheIndex
  FTL_STATUS CACHE_SetEBMCache(FTL_DEV devID, uint16_t index, CACHE_INFO_EBMCACHE_PTR ebmCacheInfo_ptr);
  FTL_STATUS CACHE_GetEBMCache(FTL_DEV devID, uint16_t index, CACHE_INFO_EBMCACHE_PTR ebmCacheInfo_ptr);
  FTL_STATUS CACHE_ClearEBMCache(FTL_DEV devID, uint16_t index);
  FTL_STATUS CACHE_MoveEBMCache(FTL_DEV devID, uint16_t fromIndex, uint16_t toIndex);

  // Structure Access Interface of gLRUCounter
  FTL_STATUS CACHE_SetgLRUCounter(uint8_t counterLRU);
  FTL_STATUS CACHE_GetgLRUCounter(uint8_t * counterLRU_ptr);
  FTL_STATUS CACHE_CleargLRUCounter(void);

  // Subroutine of CACHE_FindCandidateIndexForCrossBoundary
  FTL_STATUS CACHE_FindIndexNeitherPresented(FTL_DEV devID, uint16_t * index_ptr, uint8_t * dependency_ptr);
  FTL_STATUS CACHE_FindIndexUpsidePresented(FTL_DEV devID, uint16_t * index_ptr, uint8_t * dependency_ptr, uint16_t indexUp);
  FTL_STATUS CACHE_FindIndexDownsidePresented(FTL_DEV devID, uint16_t * index_ptr, uint8_t * dependency_ptr, uint16_t indexDown);
  FTL_STATUS CACHE_FindIndexBothPresented(FTL_DEV devID, uint16_t * index_ptr, uint8_t * dependency_ptr, uint16_t indexUp, uint16_t indexDown, uint16_t logicalEBNum, uint8_t typeAPI);

  // Low level Interface
  FTL_STATUS CACHE_FlashToCache(FTL_DEV devID, CACHE_INFO_EBLOCK_PPAMAP eBlockPPAMapInfo, uint16_t toIndex, uint8_t type);
  FTL_STATUS CACHE_CacheToFlash(FTL_DEV devID, uint16_t fromIndex, CACHE_INFO_EBLOCK_PPAMAP eBlockPPAMapInfo, uint8_t type, uint8_t flushMode);
  FTL_STATUS CACHE_CacheToCache(FTL_DEV devID, uint16_t fromIndex, uint16_t toIndex);

  // Debug function
  FTL_STATUS DEBUG_CACHE_EBMCacheIndexToRamMapIndex(void);
  FTL_STATUS DEBUG_CACHE_RamMapIndexToEBMCacheIndex(void);
#ifdef DEBUG_TEST_ARRAY
  FTL_STATUS DEBUG_CACHE_InsertTestMapping(FTL_DEV devID, uint16_t logicalEBNum, uint32_t * testData_ptr, uint32_t * testData2_ptr);
  FTL_STATUS DEBUG_CACHE_ClearTestMapping(FTL_DEV devID, uint16_t logicalEBNum);
  FTL_STATUS DEBUG_CACHE_CompTestMapping(FTL_DEV devID, uint16_t logicalEBNum, uint32_t * EBMramStructPtr, uint32_t * PPAramStructPtr);
#endif
#ifdef DEBUG_DATA_CLEAR
  FTL_STATUS DEBUG_CACHE_SetEBMCacheFree(FTL_DEV devID, uint16_t index);
  FTL_STATUS DEBUG_CACHE_CheckEBMCacheFree(void);
#endif
  FTL_STATUS DEBUG_CACHE_TABLE_DISPLY(FTL_DEV devID, uint16_t tlogicalEBNum);

#endif // #if (CACHE_RAM_BD_MODULE == true)

#if(FTL_CHECK_BAD_BLOCK_LIMIT == true)
  extern uint16_t gBBCount[NUMBER_OF_DEVICES];
  extern uint16_t gBBDevLimit[NUMBER_OF_DEVICES];
#endif

#ifdef __cplusplus
  }
#endif

#endif  // #ifndef FTL_COMMON_H
