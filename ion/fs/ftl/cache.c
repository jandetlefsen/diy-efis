#include "def.h"
#include "calc.h"
#include "common.h"

//------------------------

FTL_STATUS FTL_FlushTableCache(void)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

#if((FTL_RPB_CACHE == true) || ((FTL_SUPER_SYS_EBLOCK == true) && (FTL_DEFECT_MANAGEMENT == true)))
  uint8_t devID = 0; /*1*/
#if(FTL_DEFECT_MANAGEMENT == true)
  uint16_t sanityCounter = 0;
#endif
#endif  // #if(FTL_RPB_CACHE == true)

  if((status = FTL_CheckMount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }

#if (FTL_SUPER_SYS_EBLOCK == true)
#if(FTL_DEFECT_MANAGEMENT == true)
  for(devID = 0; devID < NUMBER_OF_DEVICES; devID++)
    {
    sanityCounter = 0;
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

#if(FTL_RPB_CACHE == true)
  for(devID = 0; devID < NUM_DEVICES; devID++)
    {
    if((status = FlushRPBCache(devID)) != FTL_ERR_PASS)
      {
      return status;
      }
    }
#endif  // #if(FTL_RPB_CACHE == true)

  if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS)
    {
    FTL_ClearMTLockBit();
    return status;
    }
  FTL_ClearMTLockBit();
  FTL_UpdatedFlag = UPDATED_DONE;
  return FTL_ERR_PASS;
  }

//------------------------------------------------

FTL_STATUS FTL_FlushDataCache(void)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

#if((FTL_RPB_CACHE == true) || ((FTL_SUPER_SYS_EBLOCK == true) && (FTL_DEFECT_MANAGEMENT == true)))
  uint8_t devID = 0; /*1*/
#if(FTL_DEFECT_MANAGEMENT == true)
  uint16_t sanityCounter = 0;
#endif
#endif  // #if((FTL_RPB_CACHE == true) || (FTL_SUPER_SYS_EBLOCK == true))

  if((status = FTL_CheckMount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }

#if (FTL_SUPER_SYS_EBLOCK == true)
#if(FTL_DEFECT_MANAGEMENT == true)
  for(devID = 0; devID < NUMBER_OF_DEVICES; devID++)
    {
    sanityCounter = 0;
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

#if(FTL_RPB_CACHE == true)
  for(devID = 0; devID < NUM_DEVICES; devID++)
    {
    if((status = FlushRPBCache(devID)) != FTL_ERR_PASS)
      {
      return status;
      }
    }
#endif  // #if(FTL_RPB_CACHE == true)

  FTL_ClearMTLockBit();
  FTL_UpdatedFlag = UPDATED_DONE;
  return FTL_ERR_PASS;
  }

#if(FTL_RPB_CACHE == true)
//------------------------------

FTL_STATUS InitRPBCache(void)
  {
  FTL_DEV devCount = 0; /*1*/

  for(devCount = 0; devCount < NUM_DEVICES; devCount++)
    {
    ClearRPBCache(devCount);
    }
  RPBCacheReadGroup.LBA = 0;
  RPBCacheReadGroup.NB = 0;
  RPBCacheReadGroup.byteBuffer = 0;
  return FTL_ERR_PASS;
  }

//------------------------------

FTL_STATUS UpdateRPBCache(uint8_t devID, uint32_t logicalPageAddr, uint32_t startSector, uint32_t numSectors, uint8_t * byteBuffer)
  {
  uint8_t *destBuf = NULL; /*4*/

  if(GetRPBCacheStatus(devID) == CACHE_EMPTY)
    {
    return FTL_ERR_RPB_CACHE_EMPTY_01;
    }
  if(GetRPBCacheLogicalPageAddr(devID) != logicalPageAddr)
    {
    return FTL_ERR_RPB_CACHE_MISS_01;
    }
  destBuf = GetRPBCache(devID) + (startSector * SECTOR_SIZE);
  memcpy(destBuf, byteBuffer, (numSectors * SECTOR_SIZE));
  SetRPBCacheLogicalPageAddr(devID, logicalPageAddr);
  SetRPBCacheStatus(devID, CACHE_DIRTY);
  return FTL_ERR_PASS;
  }

//------------------------

FTL_STATUS ReadRPBCache(uint8_t devID, uint32_t logicalPageAddr, uint32_t startSector, uint32_t numSectors, uint8_t * byteBuffer)
  {
  uint8_t *srcBuf = NULL; /*4*/

  if(GetRPBCacheStatus(devID) == CACHE_EMPTY)
    {
    return FTL_ERR_RPB_CACHE_EMPTY_02;
    }
  if(GetRPBCacheLogicalPageAddr(devID) != logicalPageAddr)
    {
    return FTL_ERR_RPB_CACHE_MISS_02;
    }
  srcBuf = GetRPBCache(devID) + (startSector * SECTOR_SIZE);
  memcpy(byteBuffer, srcBuf, (numSectors * SECTOR_SIZE));
  return FTL_ERR_PASS;
  }

//----------------------------

FTL_STATUS FlushRPBCache(uint8_t devID)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t LBA = 0; /*4*/
  uint32_t entries = 0; /*4*/
  uint8_t *byteBuffer = NULL; /*4*/

#if(FTL_DEFECT_MANAGEMENT == true)
  FTL_STATUS badBlockStatus = FTL_ERR_PASS; /*4*/
  uint16_t writeCount = 0; /*2*/
#endif

  if(GetRPBCacheStatus(devID) != CACHE_DIRTY)
    {
    return FTL_ERR_PASS;
    }
  LBA = ((GetRPBCacheLogicalPageAddr(devID) * NUM_DEVICES) + devID) * NUMBER_OF_SECTORS_PER_PAGE;

#if(FTL_DEFECT_MANAGEMENT == true)
  do
    {
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

    do
      {
      status = FTL_BuildTransferMapForWriteBlocking(LBA, NUMBER_OF_SECTORS_PER_PAGE, &entries);
      if(status != FTL_ERR_PASS)
        {

#if (FTL_DEFECT_MANAGEMENT == true)
        badBlockStatus = TranslateBadBlockError(status);
#endif

        if(status == FTL_ERR_DATA_GC_NEEDED)
          {
          uint32_t GC_status = FTL_ERR_PASS;
          uint16_t temp = EMPTY_WORD;
          uint16_t temp2 = EMPTY_WORD;

#if(FTL_DEFECT_MANAGEMENT == true)
          GC_status = InternalForcedGCWithBBManagement(EMPTY_BYTE, EMPTY_WORD, &temp, &temp2, false);

#else
          GC_status = FTL_InternalForcedGC(EMPTY_BYTE, EMPTY_WORD, &temp, &temp2, false);
#endif

          if(GC_status != FTL_ERR_PASS)
            {
            return GC_status;
            }
          }
#if (FTL_DEFECT_MANAGEMENT == true)
        else if(badBlockStatus == FTL_ERR_BAD_BLOCK_SOURCE)
          {
          if(writeCount >= MAX_BAD_BLOCK_SANITY_TRIES)
            {
            return status;
            }
          writeCount++;
          badBlockStatus = BB_ManageBadBlockErrorForChainErase(); // space check erase failure. so all flash changes will be in teh reserve pool
          if(badBlockStatus != FTL_ERR_PASS)
            {
            return status;
            }
          }
        else if(badBlockStatus == FTL_ERR_LOG_WR)
          {
          // should not come into here.
          return FTL_ERR_FAIL;
          /*
          if(writeCount >= MAX_BAD_BLOCK_SANITY_TRIES)
          {
              return status; 
          }
          writeCount++;
          badBlockStatus = BB_ManageBadBlockErrorForGCLog(); // space check erase failure. so all flash changes will be in teh reserve pool
          if(badBlockStatus !=FTL_ERR_PASS)
          {
              return status;
          }
          // change error code
          status = FTL_ERR_FAIL;
           */
          }
#endif
        else
          {
          return status;
          }
        }
      }
    while(status != FTL_ERR_PASS);

    /* check sanity */
    if(entries != 1)
      {
      return FTL_ERR_RPB_CACHE_NUM_01;
      }
    byteBuffer = GetRPBCache(devID);
    if((status = FTL_TransferPageForWrite(&byteBuffer, &entries)) != FTL_ERR_PASS)
      {

#if (FTL_DEFECT_MANAGEMENT == true)
      badBlockStatus = TranslateBadBlockError(status);
      if(badBlockStatus == FTL_ERR_LOG_WR)
        {
        if(writeCount >= MAX_BAD_BLOCK_SANITY_TRIES)
          {
          return status;
          }
        writeCount++;
        badBlockStatus = BB_ManageBadBlockErrorForGCLog(); // space check erase failure. so all flash changes will be in teh reserve pool
        if(badBlockStatus != FTL_ERR_PASS)
          {
          return status;
          }
        }
      else if(badBlockStatus == FTL_ERR_BAD_BLOCK_SOURCE)
        {
        if(writeCount >= MAX_BAD_BLOCK_SANITY_TRIES)
          {
          return status;
          }
        writeCount++;
        badBlockStatus = BB_ManageBadBlockErrorForSource(); // space check erase failure. so all flash changes will be in teh reserve pool
        if(badBlockStatus != FTL_ERR_PASS)
          {
          return status;
          }
        }
      else
#endif
        {
        return status;
        }
      }

#if(FTL_DEFECT_MANAGEMENT == true)
    ClearBadBlockInfo();
    }
  while(status != FTL_ERR_PASS);
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

#if(FTL_DEFECT_MANAGEMENT == true)
  if(GetTransLogEBFailedBadBlockInfo() == true)
    {
    if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS)
      {
      return status;
      }
    }
  ClearTransLogEBBadBlockInfo();
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

  SetRPBCacheStatus(devID, CACHE_CLEAN);
  return FTL_ERR_PASS;
  }

//-------------------------

FTL_STATUS ReadFlash(uint8_t devID, uint32_t logicalPageAddr, uint32_t startSector, uint32_t NB, uint8_t * byteBuffer)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t LBA = 0; /*4*/
  uint32_t entries = 0; /*4*/

  LBA = (((logicalPageAddr * NUM_DEVICES) + devID) * NUMBER_OF_SECTORS_PER_PAGE) + startSector;
  status = FTL_BuildTransferMapForReadBlocking(LBA, NB, &entries);
  if(status != FTL_ERR_PASS)
    {
    return status;
    }
  /* check sanity */
  if(entries != 1)
    {
    return FTL_ERR_RPB_CACHE_NUM_02;
    }
  if((status = FTL_TransferPageForRead(&byteBuffer, &entries)) != FTL_ERR_PASS)
    {
    return status;
    }
  return FTL_ERR_PASS;
  }

//-------------------------

FTL_STATUS FillRPBCache(uint8_t devID, uint32_t logicalPageAddr)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint8_t * byteBuffer = NULL; /*4*/

  byteBuffer = GetRPBCache(devID);
  if((status = ReadFlash(devID, logicalPageAddr, 0, NUMBER_OF_SECTORS_PER_PAGE, byteBuffer)) != FTL_ERR_PASS)
    {
    return status;
    }
  SetRPBCacheLogicalPageAddr(devID, logicalPageAddr);
  SetRPBCacheStatus(devID, CACHE_CLEAN);
  return FTL_ERR_PASS;
  }

//-----------------------------

FTL_STATUS RPBCacheForWrite(uint8_t * *byteBuffer, uint32_t * LBA, uint32_t * NB, uint32_t * bytesDone)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t totalPages = 0; /*4*/
  ADDRESS_STRUCT endPage = {0, 0, 0}; /*6*/
  ADDRESS_STRUCT startPage = {0, 0, 0}; /*6*/
  uint8_t devID = 0; /*1*/
  uint32_t logicalPageAddr = EMPTY_WORD; /*2*/
  uint8_t startSector = 0; /*1*/
  uint32_t numSectors = 0; /*4*/
  CACHE_INFO cache = {0, 0}; /*5*/
#if(FTL_RPB_CACHE_API_PFT_SAFE == false)
  uint8_t * dataBuf = NULL; /*4*/
#endif  // #if(FTL_RPB_CACHE_API_PFT_SAFE == false)

  numSectors = *NB;
  if(numSectors > 0)
    {
    if((status = GetPageSpread(*LBA, numSectors, &startPage, &totalPages, &endPage)) != FTL_ERR_PASS)
      {
      return status;
      }

    if(totalPages == 1)
      { /* single-page transaction */
      devID = startPage.devID;
      logicalPageAddr = startPage.logicalPageNum;
      startSector = startPage.pageOffset;
      cache.status = GetRPBCacheStatus(devID);
      cache.logicalPageAddr = GetRPBCacheLogicalPageAddr(devID);
      if(cache.status == CACHE_EMPTY)
        {
        if((status = FillRPBCache(devID, logicalPageAddr)) != FTL_ERR_PASS)
          {
          return status;
          }
        if((status = UpdateRPBCache(devID, logicalPageAddr, startSector, numSectors, *byteBuffer)) != FTL_ERR_PASS)
          {
          return status;
          }
        }
      else if(cache.status == CACHE_CLEAN)
        {
        if(logicalPageAddr != cache.logicalPageAddr)
          {
          if((status = FillRPBCache(devID, logicalPageAddr)) != FTL_ERR_PASS)
            {
            return status;
            }
          }
        if((status = UpdateRPBCache(devID, logicalPageAddr, startSector, numSectors, *byteBuffer)) != FTL_ERR_PASS)
          {
          return status;
          }
        }
      else if(cache.status == CACHE_DIRTY)
        {
        if(logicalPageAddr == cache.logicalPageAddr)
          {
          if((status = UpdateRPBCache(devID, logicalPageAddr, startSector, numSectors, *byteBuffer)) != FTL_ERR_PASS)
            {
            return status;
            }
          }
        else
          {
          if((status = FlushRPBCache(devID)) != FTL_ERR_PASS)
            {
            return status;
            }
          if((status = FillRPBCache(devID, logicalPageAddr)) != FTL_ERR_PASS)
            {
            return status;
            }
          if((status = UpdateRPBCache(devID, logicalPageAddr, startSector, numSectors, *byteBuffer)) != FTL_ERR_PASS)
            {
            return status;
            }
          }
        }
      *LBA += numSectors;
      *NB -= numSectors;
      if(*NB != 0)
        {
        return FTL_ERR_RPB_CACHE_NUM_03;
        }
      *byteBuffer += (numSectors * SECTOR_SIZE);
      *bytesDone += (numSectors * SECTOR_SIZE);
      }
    else
      { /* multiple-pages transaction */
#if(FTL_RPB_CACHE_API_PFT_SAFE == true)
      for(devID = 0; devID < NUM_DEVICES; devID++)
        {
        if((status = FlushRPBCache(devID)) != FTL_ERR_PASS)
          {
          return status;
          }
        ClearRPBCache(devID);
        }

#else  // #if(FTL_RPB_CACHE_API_PFT_SAFE == true)
      /*start page*/
      devID = startPage.devID;
      logicalPageAddr = startPage.logicalPageNum;
      startSector = startPage.pageOffset;
      numSectors = NUMBER_OF_SECTORS_PER_PAGE - startSector;
      cache.status = GetRPBCacheStatus(devID);
      cache.logicalPageAddr = GetRPBCacheLogicalPageAddr(devID);
      if(cache.status == CACHE_EMPTY)
        {

        }
      else if(cache.status == CACHE_CLEAN)
        {
        if(logicalPageAddr == cache.logicalPageAddr)
          {
          if((status = UpdateRPBCache(devID, logicalPageAddr, startSector, numSectors, *byteBuffer)) != FTL_ERR_PASS)
            {
            return status;
            }
          if((status = FlushRPBCache(devID)) != FTL_ERR_PASS)
            {
            return status;
            }
          *LBA += numSectors;
          *NB -= numSectors;
          *byteBuffer += (numSectors * SECTOR_SIZE);
          *bytesDone += (numSectors * SECTOR_SIZE);
          }
        }
      else if(cache.status == CACHE_DIRTY)
        {
        if(logicalPageAddr == cache.logicalPageAddr)
          {
          if((status = UpdateRPBCache(devID, logicalPageAddr, startSector, numSectors, *byteBuffer)) != FTL_ERR_PASS)
            {
            return status;
            }
          if((status = FlushRPBCache(devID)) != FTL_ERR_PASS)
            {
            return status;
            }
          *LBA += numSectors;
          *NB -= numSectors;
          *byteBuffer += (numSectors * SECTOR_SIZE);
          *bytesDone += (numSectors * SECTOR_SIZE);
          }
        else
          {
          if((status = FlushRPBCache(devID)) != FTL_ERR_PASS)
            {
            return status;
            }
          }
        }

      /*end page*/
      if(endPage.pageOffset == 0)
        {
        if(endPage.devID == 0)
          {
          devID = (NUM_DEVICES - 1);
          logicalPageAddr = endPage.logicalPageNum - 1;
          }
        else
          {
          devID = endPage.devID - 1;
          logicalPageAddr = endPage.logicalPageNum;
          }
        startSector = 0;
        numSectors = NUM_SECTORS_PER_PAGE;
        }
      else
        {
        devID = endPage.devID;
        logicalPageAddr = endPage.logicalPageNum;
        startSector = 0;
        numSectors = endPage.pageOffset;
        }
      cache.status = GetRPBCacheStatus(devID);
      cache.logicalPageAddr = GetRPBCacheLogicalPageAddr(devID);
      dataBuf = (*byteBuffer) + ((*NB - numSectors) * SECTOR_SIZE);
      if(cache.status == CACHE_EMPTY)
        {
        if((status = FillRPBCache(devID, logicalPageAddr)) != FTL_ERR_PASS)
          {
          return status;
          }
        if((status = UpdateRPBCache(devID, logicalPageAddr, startSector, numSectors, dataBuf)) != FTL_ERR_PASS)
          {
          return status;
          }
        *NB -= numSectors;
        *bytesDone += (numSectors * SECTOR_SIZE);
        }
      else if(cache.status == CACHE_CLEAN)
        {
        if(logicalPageAddr != cache.logicalPageAddr)
          {
          if((status = FillRPBCache(devID, logicalPageAddr)) != FTL_ERR_PASS)
            {
            return status;
            }
          }
        if((status = UpdateRPBCache(devID, logicalPageAddr, startSector, numSectors, dataBuf)) != FTL_ERR_PASS)
          {
          return status;
          }
        *NB -= numSectors;
        *bytesDone += (numSectors * SECTOR_SIZE);
        }
      else if(cache.status == CACHE_DIRTY)
        {
        return FTL_ERR_RPB_CACHE_DIRTY;
        }
#endif  // #else  // #if(FTL_RPB_CACHE_API_PFT_SAFE == true)
      }
    }
  return FTL_ERR_PASS;
  }

//------------------------

FTL_STATUS RPBCacheForRead(uint8_t * *byteBuffer, uint32_t * LBA, uint32_t * NB, uint32_t * bytesDone)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint32_t totalPages = 0; /*4*/
  ADDRESS_STRUCT endPage = {0, 0, 0}; /*6*/
  ADDRESS_STRUCT startPage = {0, 0, 0}; /*6*/
  ADDRESS_STRUCT currentPage = {0, 0, 0}; /*6*/
  uint8_t devID = 0; /*1*/
  uint32_t logicalPageAddr = EMPTY_WORD; /*2*/
  uint8_t startSector = 0; /*1*/
  uint32_t numSectors = 0; /*4*/
  uint32_t currentPageCount = 0; /*4*/
  uint32_t totalSectors = 0; /*4*/
  uint8_t * dataBuf = NULL; /*4*/
  CACHE_INFO cache = {0, 0}; /*5*/

  RPBCacheReadGroup.LBA = 0;
  RPBCacheReadGroup.NB = 0;
  RPBCacheReadGroup.byteBuffer = 0;
  numSectors = *NB;
  if(numSectors > 0)
    {
    if((status = GetPageSpread(*LBA, numSectors, &startPage, &totalPages, &endPage)) != FTL_ERR_PASS)
      {
      return status;
      }

    if(totalPages == 1)
      { /* single-page transaction */
      devID = startPage.devID;
      logicalPageAddr = startPage.logicalPageNum;
      startSector = startPage.pageOffset;
      cache.status = GetRPBCacheStatus(devID);
      cache.logicalPageAddr = GetRPBCacheLogicalPageAddr(devID);
      if(cache.status != CACHE_EMPTY)
        {
        if(logicalPageAddr == cache.logicalPageAddr)
          {
          if((status = ReadRPBCache(devID, logicalPageAddr, startSector, numSectors, *byteBuffer)) != FTL_ERR_PASS)
            {
            return status;
            }
          *LBA += numSectors;
          *NB -= numSectors;
          if(*NB != 0)
            {
            return FTL_ERR_RPB_CACHE_NUM_04;
            }
          *byteBuffer += (numSectors * SECTOR_SIZE);
          *bytesDone += (numSectors * SECTOR_SIZE);
          }
        }
      }
    else
      { /* multiple-pages transaction */
      cache.status = GetRPBCacheStatus(devID);
      cache.logicalPageAddr = GetRPBCacheLogicalPageAddr(devID);
      if(cache.status != CACHE_EMPTY)
        {
        currentPage = startPage;
        for(currentPageCount = 0; currentPageCount < totalPages; currentPageCount++)
          {
          devID = currentPage.devID;
          logicalPageAddr = currentPage.logicalPageNum;
          startSector = currentPage.pageOffset;
          if(currentPageCount == 0)
            { /*start page*/
            numSectors = NUM_SECTORS_PER_PAGE - startSector;
            if(logicalPageAddr == cache.logicalPageAddr)
              {
              if((status = ReadRPBCache(devID, logicalPageAddr, startSector, numSectors, *byteBuffer)) != FTL_ERR_PASS)
                {
                return status;
                }
              *LBA += numSectors;
              *NB -= numSectors;
              *byteBuffer += (numSectors * SECTOR_SIZE);
              *bytesDone += (numSectors * SECTOR_SIZE);
              break;
              }
            }
          else if(currentPageCount < (totalPages - 1))
            { /*middle pages*/
            numSectors = NUM_SECTORS_PER_PAGE;
            if(logicalPageAddr == cache.logicalPageAddr)
              {
              dataBuf = (*byteBuffer) + (totalSectors * SECTOR_SIZE);
              if((status = ReadRPBCache(devID, logicalPageAddr, startSector, numSectors, dataBuf)) != FTL_ERR_PASS)
                {
                return status;
                }
              RPBCacheReadGroup.LBA = (*LBA) + (totalSectors + numSectors);
              RPBCacheReadGroup.NB = (*NB) - (totalSectors + numSectors);
              RPBCacheReadGroup.byteBuffer = (*byteBuffer) + ((totalSectors + numSectors) * SECTOR_SIZE);
              *NB = totalSectors;
              *bytesDone += (numSectors * SECTOR_SIZE);
              break;
              }
            }
          else
            { /*end page*/
            if(endPage.pageOffset == 0)
              {
              numSectors = NUM_SECTORS_PER_PAGE;
              }
            else
              {
              numSectors = endPage.pageOffset;
              }
            if(logicalPageAddr == cache.logicalPageAddr)
              {
              dataBuf = (*byteBuffer) + ((*NB - numSectors) * SECTOR_SIZE);
              if((status = ReadRPBCache(devID, logicalPageAddr, startSector, numSectors, dataBuf)) != FTL_ERR_PASS)
                {
                return status;
                }
              *NB -= numSectors;
              *bytesDone += (numSectors * SECTOR_SIZE);
              break;
              }
            }
          totalSectors += numSectors;
          if((status = IncPageAddr(&currentPage)) != FTL_ERR_PASS)
            {
            return status;
            }
          }
        }
      }
    }
  return FTL_ERR_PASS;
  }
#endif  // #if(FTL_RPB_CACHE == true)
