#include "lowlevel.h"
#include "common.h"
#include "if_in.h"

//-----------------------------------------------------------------

FTL_STATUS FTL_InitAll(FTL_INIT_STRUCT_PTR initStructPtr)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FTL_DEV devCount = 0; /*1*/
  uint8_t restartCount = 0; /*1*/
  uint16_t formatCount = 0; /*2*/
  uint8_t initLoop = 0; /*1*/
#if (DEBUG_INIT_CHECK == true)
  FTL_DEV devID = 0; /*1*/
  uint16_t logicalEBNum = EMPTY_WORD; /*2*/
#endif  // #if DEBUG_INIT_CHECK
#if (FTL_SUPER_SYS_EBLOCK == true)
  uint16_t storeSysEB[MAX_NUM_SYS_EBLOCKS];
  uint8_t checkSuperPF = false; /*1*/
  uint16_t eBlockNum = 0x0; /*2*/
  uint32_t latestIncNumber = 0x0; /*4*/
  uint16_t phyEBAddr = 0x0; /*2*/
  uint16_t logicalAddr = 0x0; /*2*/
#if(FTL_DEFECT_MANAGEMENT == true)
  uint16_t sanityCounter = 0; /*2*/
#endif  // #if (FTL_DEFECT_MANAGEMENT == true)
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
#if (DEBUG_ENABLE_LOGGING == true)
  if((status = DEBUG_InsertLog(EMPTY_DWORD, EMPTY_DWORD, DEBUG_LOG_INIT)) != FTL_ERR_PASS)
    {
    return (status);
    }
#endif
#if (DEBUG_FTL_API_ANNOUNCE == 1)
  DBG_Printf("FTL_InitAll start: .format = %d, \n", initStructPtr->format, 0);
#endif  // #if (DEBUG_FTL_API_ANNOUNCE == 1)

  if((status = FTL_CheckPointer(initStructPtr)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = FTL_CheckUnmount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    if(status == FTL_ERR_MOUNTED)
      {
      /* already mounted */
      return FTL_ERR_PASS;
      }
    else
      {
      return status;
      }
    }
//  PRINT_VERSION; /*If printf is not supported, this can be changed to print to a port, or removed if compile fails*/

#if(FTL_DEBUG_STRUCT_SIZE == true)
  status = FTL_DebugStructSize();
  if(status != FTL_ERR_PASS)
    {
    return status;
    }
#endif  // #if(FTL_DEBUG_STRUCT_SIZE == true)


  if((FLASH_Init()) != FLASH_PASS)
    {
    FTL_ClearMTLockBit();
    return FTL_ERR_FLASH_INIT_02;
    }
#if (CACHE_RAM_BD_MODULE == true && CACHE_DYNAMIC_ALLOCATION == true)
  if((status = CACHE_DynamicAllocation(initStructPtr->total_ram_allowed)))
    {
    FTL_ClearMTLockBit();
    return status;
    }
#endif

  if((status = InitFTLRAMSTables()) != FTL_ERR_PASS)
    {
    FTL_ClearMTLockBit();
    return status;
    }
  if(initStructPtr->format == FTL_FORCE_FORMAT)
    {
    if((status = FTL_InternalFormat()) != FTL_ERR_PASS)
      {
      FTL_ClearMTLockBit();
      return status;
      }
    }
  for(restartCount = 0; restartCount < 2; restartCount++)
    {
    /*Find the flush eblock per device*/
    for(devCount = 0; devCount < NUM_DEVICES; devCount++)
      {
#if (FTL_SUPER_SYS_EBLOCK == true)
      SuperEBInfo[devCount].checkLost = false;
      SuperEBInfo[devCount].checkSuperPF = false;
      /* Get Super System EB */
      if((status = FTL_FindSuperSysEB(devCount)) == FTL_ERR_PASS)
        {
        /* Get Super System Log */
        if((status = GetSuperSysInfoLogs(devCount, storeSysEB, (uint8_t *) & checkSuperPF)) == FTL_ERR_PASS)
          {
          formatCount = 0;
          /* Set System EB to Ram Table*/
          if((status = SetSysEBRamTable(devCount, storeSysEB, (uint16_t *) & formatCount)) == FTL_ERR_PASS)
            {
            if(2 <= formatCount)
              {
              /* Succeed */
              SuperEBInfo[devCount].checkLost = false;
              SuperEBInfo[devCount].checkSuperPF = checkSuperPF;
              formatCount = NUM_SYSTEM_EBLOCKS - formatCount;
              }
            else
              {
              /* Fail created Ram Table of Super System EB */
              /* Lack System EB*/
              SuperEBInfo[devCount].checkLost = true;
              SuperEBInfo[devCount].checkSuperPF = true;
              }
            }
          else
            {
            /* Fail created Ram Table of Super System EB */
            /* Overflow Ram Table of Super System EB */
            SuperEBInfo[devCount].checkLost = true;
            SuperEBInfo[devCount].checkSuperPF = true;
            }
          }
        else
          {
          /* Fail Super System Log */
          SuperEBInfo[devCount].checkLost = true;
          SuperEBInfo[devCount].checkSuperPF = checkSuperPF;
          }
        }
      else
        {
        /* Fail Super System EB */
        TABLE_SuperSysEBClear(devCount);
        TABLE_SuperSysEBInsert(devCount, EMPTY_WORD, EMPTY_WORD, EMPTY_DWORD); // for next Super System EB
        SuperEBInfo[devCount].checkLost = true;
        SuperEBInfo[devCount].checkSuperPF = true;
        }
      if(true == SuperEBInfo[devCount].checkLost)
        {
        if((status = TABLE_InitEBOrderingTable(devCount)) != FTL_ERR_PASS)
          {
          return status;
          }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
        formatCount = 0;
        if((status = UpdateEBOrderingTable(devCount, FTL_START_EBLOCK, (uint16_t *) & formatCount)) != FTL_ERR_PASS)
          {
          FTL_ClearMTLockBit();
          return status;
          }
        if(formatCount > (NUM_SYSTEM_EBLOCKS - 2)) // Must find at least 2 System EBlocks
          {
          // cound not find system eblocks Format needed
          if(initStructPtr->format == FTL_FORMAT_AS_NEEDED)
            {
            initLoop++;
            if((status = FTL_InternalFormat()) != FTL_ERR_PASS)
              {
              FTL_ClearMTLockBit();
              return status;
              }
            // ok, done formatting, lets start again
            if(initLoop == 1) // first pass, so lets go again
              {
              if((status = TABLE_InitEBOrderingTable(devCount)) != FTL_ERR_PASS)
                {
                return status;
                }
              break;
              }
            FTL_ClearMTLockBit();
            // should not come here, if we do, format failed,
            // something is seriously wrong
            return FTL_ERR_INIT_FORMAT_FAILED;
            }
          else
            {
            FTL_ClearMTLockBit();
            return FTL_ERR_NOT_FORMATTED;
            }
          }
#if (FTL_SUPER_SYS_EBLOCK == true)
        }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
      }
    if(formatCount <= (NUM_SYSTEM_EBLOCKS - 2)) // Must find at least 2 System EBlocks
      {
      break;
      }
    }

#if(FTL_CHECK_VERSION == true)
  if((status = FTL_CheckVersion()) != FTL_ERR_PASS)
    {
    if((status == FTL_ERR_VERSUNKNOWN) || (status == FTL_ERR_VERSMISMATCH))
      {
      FTL_ClearMTLockBit();
      return FTL_ERR_NOT_FORMATTED;
      }
    else
      {
      FTL_ClearMTLockBit();
      return status;
      }
    }
#endif  // #if(FTL_CHECK_VERSION == true)

  /*done sequencing the EBlocks, let load up the RAM tables from teh flush EB*/
  if((status = TABLE_LoadFlushTable()) != FTL_ERR_PASS)
    {
    FTL_ClearMTLockBit();
    return status;
    }
#if (FTL_SUPER_SYS_EBLOCK == true)
  for(devCount = 0; devCount < NUM_DEVICES; devCount++)
    {
    if(true == SuperEBInfo[devCount].checkLost || true == SuperEBInfo[devCount].checkSuperPF || true == SuperEBInfo[devCount].checkSysPF)
      {
#if(FTL_DEFECT_MANAGEMENT == true)
      while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
        {
        if((status = CreateNextSuperSystemEBlockOp(devCount)) != FTL_ERR_PASS)
          {
          return status;
          }
        if((status = FTL_CreateSuperSysEBLog(devCount, SYS_EBLOCK_INFO_CHANGED)) == FTL_ERR_PASS)
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
#else
      if((status = CreateNextSuperSystemEBlockOp(devCount)) != FTL_ERR_PASS)
        {
        return status;
        }
      if((status = FTL_CreateSuperSysEBLog(devCount, SYS_EBLOCK_INFO_CHANGED)) != FTL_ERR_PASS)
        {
        return status;
        }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

      // Clear
      SuperEBInfo[devCount].checkLost = false;
      SuperEBInfo[devCount].checkSuperPF = false;
      SuperEBInfo[devCount].checkSysPF = false;
      }
    else
      {
      for(eBlockNum = 0; eBlockNum < NUM_SUPER_SYS_EBLOCKS; eBlockNum++)
        {
        if((status = TABLE_GetSuperSysEBEntry(devCount, eBlockNum, &logicalAddr, &phyEBAddr, &latestIncNumber)) != FTL_ERR_PASS)
          {
          return status; // trying to excess outside table.
          }
        if((logicalAddr == EMPTY_WORD) && (phyEBAddr == EMPTY_WORD) && (latestIncNumber == EMPTY_DWORD))
          {
          break; // no more entries in table
          }
        SetGCOrFreePageNum(devCount, logicalAddr, SuperEBInfo[devCount].storeFreePage[eBlockNum] + 1);
        }
      }
    }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
  /* Initialize the GcNum[] table */

#if (FTL_SUPER_SYS_EBLOCK == false)
  status = TABLE_InitGcNum();
  if(FTL_ERR_PASS != status)
    {
    FTL_ClearMTLockBit();
    return status;
    }
#endif
#if(FTL_RPB_CACHE == true)
  if((status = InitRPBCache()) != FTL_ERR_PASS)
    {
    FTL_ClearMTLockBit();
    return status;
    }
#endif  // #if(FTL_RPB_CACHE == true)

#if (DEBUG_INIT_CHECK == true)
  for(devID = 0; devID < NUM_DEVICES; devID++)
    {
    for(logicalEBNum = 0; logicalEBNum < NUM_EBLOCKS_PER_DEVICE; logicalEBNum++)
      {
      status = DBG_CheckPPAandBitMap(devID, logicalEBNum);
      if(FTL_ERR_PASS != status)
        {
        FTL_ClearMTLockBit();
        return status;
        }
      }
    }
  status = DBG_CheckMappingTables();
  if(FTL_ERR_PASS != status)
    {
    FTL_ClearMTLockBit();
    return status;
    }
#endif  // DEBUG_INIT_CHECK

#if (DEBUG_COMPARE_TABLES == true)
  if(debugFlushDone == true)
    {
    debugFlushDone = false;
    status = DEBUG_CompareTablesAll();
    if(status != FTL_ERR_PASS)
      {
      FTL_ClearMTLockBit();
      return status;
      }
    }
#endif  // #if (DEBUG_COMPARE_TABLES == true)

#if(FTL_CHECK_BAD_BLOCK_LIMIT == true)
  for(devCount = 0; devCount < NUM_DEVICES; devCount++)
    {
    gBBCount[devCount] = 0; /* for FTL_InternalFormat in FTL_InitAll */
    for(eBlockNum = SYSTEM_START_EBLOCK; eBlockNum < NUM_EBLOCKS_PER_DEVICE; eBlockNum++)
      {
      if(GetBadEBlockStatus(devCount, eBlockNum) == true)
        {
        gBBCount[devCount]++;
        if(gBBCount[devCount] > gBBDevLimit[devCount])
          {
          DBG_Printf("[Warning] bad block is more than %d", gBBDevLimit[devCount], 0);
          DBG_Printf(" in dev%d. \n", devCount, 0);
          break;
          }
        }
      }
    }
#endif

  FTL_SetMountBit();
  FTL_ClearMTLockBit();
  FTL_initFlag = INIT_DONE;
  FTL_UpdatedFlag = UPDATED_NOT_DONE;

#if (DEBUG_FTL_API_ANNOUNCE == 1)
  DBG_Printf("FTL_InitAll done: \n", 0, 0);
#endif  // #if (DEBUG_FTL_API_ANNOUNCE == 1)

  return FTL_ERR_PASS;
  }

//-----------------------------------

FTL_STATUS INIT_InitBasic(void)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FTL_DEV devID = 0; /*1*/

  if((status = TRANS_InitTransMap()) != FTL_ERR_PASS)
    {
    return status;
    }
  if(INIT_FORMATTED == FTL_initFlag)
    {
    FTL_initFlag = INIT_NOT_DONE;
    }
  else
    {
#if (CACHE_RAM_BD_MODULE == true)
    if((status = CACHE_ClearAll()) != FTL_ERR_PASS)
      {
      return status;
      }
#endif

    if((status = TABLE_InitMappingTable()) != FTL_ERR_PASS)
      {
      return status;
      }
    }
  if((status = TABLE_InitPPAMappingTable()) != FTL_ERR_PASS)
    {
    return status;
    }
  for(devID = 0; devID < NUM_DEVICES; devID++)
    {
    if((status = TABLE_InitEBOrderingTable(devID)) != FTL_ERR_PASS)
      {
      return status;
      }
#if (FTL_SUPER_SYS_EBLOCK == true)
    if((status = TABLE_SuperSysEBClear(devID)) != FTL_ERR_PASS)
      {
      return status;
      }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if(FTL_CHECK_BAD_BLOCK_LIMIT == true)
    gBBCount[devID] = 0;
#endif
    }
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  if((status = TABLE_InitTransLogEntry()) != FTL_ERR_PASS)
    {
    return status;
    }
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)

  return status;
  }

//---------------------------------------------------------------------------

FTL_STATUS FTL_Shutdown(void)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

#if(FTL_RPB_CACHE == true || FTL_SUPER_SYS_EBLOCK == true)
  uint8_t devID = 0; /*1*/
#if(FTL_DEFECT_MANAGEMENT == true)
  uint16_t sanityCounter = 0;
#endif
#endif  // #if(FTL_RPB_CACHE == true)

#if (DEBUG_FTL_API_ANNOUNCE == 1)
  DBG_Printf("FTL_Shutdown Start: status=%d\n", status, 0);
#endif  // #if (DEBUG_FTL_API_ANNOUNCE == 1)

  if((status = FTL_CheckMount_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }

  if(UPDATED_DONE == FTL_UpdatedFlag)
    {

#if(FTL_RPB_CACHE == true)
    for(devID = 0; devID < NUM_DEVICES; devID++)
      {
      if((status = FlushRPBCache(devID)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
#endif  // #if(FTL_RPB_CACHE == true)


#if (FTL_SUPER_SYS_EBLOCK == true)
#if(FTL_DEFECT_MANAGEMENT == true)
    while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
      {
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)
      for(devID = 0; devID < NUM_DEVICES; devID++)
        {
        if((status = FTL_CheckForSuperSysEBLogSpace(devID, SYS_EBLOCK_INFO_SYSEB)) != FTL_ERR_PASS)
          {
          return status;
          }
        }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

      if((status = TABLE_Flush(FLUSH_SHUTDOWN_MODE)) != FTL_ERR_PASS)
        {
        FTL_ClearMTLockBit();
        return status;
        }

#if (FTL_SUPER_SYS_EBLOCK == true)
#if(FTL_DEFECT_MANAGEMENT == true)
      for(devID = 0; devID < NUM_DEVICES; devID++)
        {
        if((status = FTL_CreateSuperSysEBLog(devID, SYS_EBLOCK_INFO_SYSEB)) != FTL_ERR_PASS)
          {
          break; // Fail creating Super System Log. Go to BB routine
          }
        }
      if(status != FTL_ERR_PASS)
        {
        if(status != FTL_ERR_SUPER_WRITE_02)
          {
          return status;
          }
        }
      else
        {
        break; //Succeed
        }
      if(sanityCounter >= MAX_BAD_BLOCK_SANITY_TRIES)
        {
        return status;
        }
      sanityCounter++;
      }
#else
      for(devID = 0; devID < NUM_DEVICES; devID++)
        {
        if((status = FTL_CreateSuperSysEBLog(devID, SYS_EBLOCK_INFO_SYSEB)) != FTL_ERR_PASS)
          {
          return status;
          }
        }
#endif  //#if(FTL_DEFECT_MANAGEMENT == true)
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if (DEBUG_COMPARE_TABLES == true)
    status = DEBUG_FlushTablesAll();
    if(status != FTL_ERR_PASS)
      {
      FTL_ClearMTLockBit();
      return status;
      }
    debugFlushDone = true;
#endif  // #if (DEBUG_COMPARE_TABLES == true)

    } // if (UPDATED_DONE == FTL_UpdatedFlag)

  if((FLASH_Shutdown()) != FLASH_PASS)
    {
    FTL_ClearMTLockBit();
    return FTL_ERR_FLASH_SHUTDOWN_01;
    }
  FTL_ClearMountBit();
  FTL_ClearMTLockBit();
  FTL_initFlag = INIT_NOT_DONE;

#if (DEBUG_FTL_API_ANNOUNCE == 1)
  DBG_Printf("FTL_Shutdown done: status=%d\n", status, 0);
#endif  // #if (DEBUG_FTL_API_ANNOUNCE == 1)

  return FTL_ERR_PASS;
  }

//---------------------------------------------------------------------------

FTL_STATUS FTL_InternalFormat(void)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  FTL_DEV devCount = 0; /*1*/
  uint16_t eBlockCount = 0; /*2*/
  uint8_t tempCount = 0; /*1*/
#if (CACHE_RAM_BD_MODULE == false)
  uint32_t tempErase = 0; /*2*/
#endif
  FLASH_PAGE_INFO flashPage = {0, 0,
    {0, 0}}; /*11*/
  SYS_EBLOCK_INFO sysEBlockInfo; /*16*/
  uint16_t logEBlock = TRANSACTLOG_START_EBLOCK;
  uint16_t flushEBlock = FLUSH_LOG_START_EBLOCK;
#if (FTL_SUPER_SYS_EBLOCK == true)
  uint16_t superEBlock = SUPER_LOG_START_EBLOCK;
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

#if(FTL_DEFECT_MANAGEMENT == true)
#if (CACHE_RAM_BD_MODULE == false)
  uint16_t phyToEBlock = EMPTY_WORD;
  uint16_t logToEBlock = EMPTY_WORD;
#endif
  uint16_t sanityCounter = 0; /*2*/
  uint16_t badSysEBlockCount = 0; /*2*/
  uint8_t foundBadBlock = false;
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

  FTL_ClearMountBit();
  if((FLASH_Init()) != FLASH_PASS)
    {
    return FTL_ERR_FLASH_INIT_01;
    }

#if (CACHE_RAM_BD_MODULE == true)
#if (CACHE_PARAM_DISPLAY == true)
  DBG_Printf("Total BD Size = %d\n", TOTAL_BD_RAM_SIZE, 0);
  DBG_Printf("EBLOCKMAPINDEX_SIZE Size = %d\n", EBLOCKMAPINDEX_SIZE, 0);
  DBG_Printf("PPAMAPINDEX_SIZE Size = %d\n", PPAMAPINDEX_SIZE, 0);
  DBG_Printf("RAMMAPINDEX_SIZE Size = %d\n", RAMMAPINDEX_SIZE, 0);
  DBG_Printf("EBMCACHEINDEX_SIZE Size = %d\n", EBMCACHEINDEX_SIZE, 0);
  DBG_Printf("EBLOCKMAPPINGCAACHE_SIZE Size = %d\n", EBLOCKMAPPINGCAACHE_SIZE, 0);
  DBG_Printf("EBLOCKMAPPINGCAACHE_SIZE Size = %d\n", EBLOCKMAPPINGCAACHE_SIZE, 0);
  DBG_Printf("EBLOCKMAPINDEX_SIZE Size = %d\n", EBLOCKMAPINDEX_SIZE, 0);
#endif
#endif

#if (CACHE_RAM_BD_MODULE == true)
#if (FTL_SUPER_SYS_EBLOCK == false)
  DBG_Printf("Recommend FTL_SUPER_SYS_EBLOCK == true\n", 0, 0);
#endif
#endif

#if (FTL_DEFECT_MANAGEMENT == true)
  for(devCount = 0; devCount < NUM_DEVICES; devCount++)
    {
    TABLE_ClearReservedEB(devCount);
    for(eBlockCount = SYSTEM_START_EBLOCK; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
      {
#if (FTL_SUPER_SYS_EBLOCK == true)
      if(logEBlock == eBlockCount || flushEBlock == eBlockCount || superEBlock == eBlockCount)
        {
        continue;
        }
#else
      if(logEBlock == eBlockCount || flushEBlock == eBlockCount)
        {
        continue;
        }
#endif

      flashPage.devID = devCount;
      flashPage.byteCount = 0;
      flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(eBlockCount, 0);
      flashPage.vPage.pageOffset = 0;
      if((FLASH_CheckDefectEBlock(&flashPage)) != FLASH_PASS)
        {
        continue;
        }

#if (CACHE_RAM_BD_MODULE == true)
      if(FTL_ERR_PASS != (status = CACHE_LoadEB(devCount, eBlockCount, CACHE_INIT_TYPE)))
        {
        return status;
        }
#endif

      if((status = TABLE_InsertReservedEB(devCount, eBlockCount)) != FTL_ERR_PASS)
        {
        return status;
        }
      }
    }
#endif
  if(INIT_NOT_DONE != FTL_initFlag)
    {
#if (DEBUG_ENABLE_LOGGING == true)
    DEBUG_ClearLog();
#endif
    // Clear Mapping Table, but save erase counts

#if (CACHE_RAM_BD_MODULE == false)
    for(devCount = 0; devCount < NUM_DEVICES; devCount++)
      {
      for(eBlockCount = 0; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
        {
        tempErase = GetTrueEraseCount(devCount, eBlockCount) + 1;
        if(0 == tempErase)
          {
          tempErase = 1;
          }

#if (true == ENABLE_EB_ERASED_BIT)
        tempErase |= ERASE_STATUS_GET_DWORD_MASK;
#endif  // #if (true == ENABLE_EB_ERASED_BIT)
        EBlockMappingTable[devCount][GetPhysicalEBlockAddr(devCount, eBlockCount)].chainToFrom = tempErase;
        }
      for(eBlockCount = 0; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
        {
        TABLE_ClearMappingTable(devCount, eBlockCount, eBlockCount, EBlockMappingTable[devCount][eBlockCount].chainToFrom);
        EBlockMappingTable[devCount][eBlockCount].chainToFrom = EMPTY_DWORD;
        }
      }
#endif
    FTL_initFlag = INIT_FORMATTED;
    }
#if (FTL_DEFECT_MANAGEMENT == true)
  /*Check for Factory Defect Mark, and set EBlock Mapping Table*/
  for(eBlockCount = 0; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
    {
    for(devCount = 0; devCount < NUM_DEVICES; devCount++)
      {

#if (CACHE_RAM_BD_MODULE == true)
      if(FTL_ERR_PASS != (status = CACHE_LoadEB(devCount, eBlockCount, CACHE_INIT_TYPE)))
        {
        return status;
        }
#endif

      flashPage.devID = devCount;
      flashPage.byteCount = 0;
      flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(devCount, eBlockCount), 0);
      flashPage.vPage.pageOffset = 0;
      if((FLASH_CheckDefectEBlock(&flashPage)) != FLASH_PASS)
        {
        foundBadBlock = true;

#if (FTL_CHECK_BAD_BLOCK_LIMIT == true)
        gBBCount[devCount]++;
        if(gBBCount[devCount] > gBBDevLimit[devCount])
          {
          DBG_Printf("[Warning] Initial bad block is more than %d", gBBDevLimit[devCount], 0);
          DBG_Printf(" in dev%d. \n", devCount, 0);
          }
#endif

#if (CACHE_RAM_BD_MODULE == false)
        SetBadEBlockStatus(devCount, eBlockCount, true);
#endif
        if(eBlockCount < NUM_DATA_EBLOCKS)
          {
#if (CACHE_RAM_BD_MODULE == false)
          if((status = FTL_SwapDataReserveEBlock(devCount, eBlockCount, &phyToEBlock, &logToEBlock, false, false)) != FTL_ERR_PASS)
            {
            if(status == FTL_ERR_ECHAIN_SETUP_SANITY1)
              {
              return FTL_ERR_FORMAT_DATA_INIT;
              }
            else
              {
              return status;
              }
            }
#endif
          }
        else if(eBlockCount >= SYSTEM_START_EBLOCK)
          {
#if (CACHE_RAM_BD_MODULE == true)
          SetBadEBlockStatus(devCount, eBlockCount, true);
#endif
          badSysEBlockCount++;
          if(eBlockCount == logEBlock)
            {
            logEBlock++;
            if(logEBlock == flushEBlock)
              {
              flushEBlock++;
#if (FTL_SUPER_SYS_EBLOCK == true)
              if(flushEBlock == superEBlock)
                {
                superEBlock++;
                }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
              }
            }
          if(eBlockCount == flushEBlock)
            {
            flushEBlock++;
#if (FTL_SUPER_SYS_EBLOCK == true)
            if(flushEBlock == superEBlock)
              {
              superEBlock++;
              }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
            }
#if (FTL_SUPER_SYS_EBLOCK == true)
          if(eBlockCount == superEBlock)
            {
            superEBlock++;
            }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
          }
        }
      }
    }
  if((NUMBER_OF_SYSTEM_EBLOCKS - badSysEBlockCount) < MIN_RESERVE_EBLOCKS)
    {
    return FTL_ERR_FORMAT_SYS_INIT;
    }


#endif
  for(eBlockCount = 0; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
    {
    for(devCount = 0; devCount < NUM_DEVICES; devCount++)
      {

#if (CACHE_RAM_BD_MODULE == true)
      if(FTL_ERR_PASS != (status = CACHE_LoadEB(devCount, eBlockCount, CACHE_INIT_TYPE)))
        {
        return status;
        }
#endif

#if (FTL_DEFECT_MANAGEMENT == true)
      if(GetBadEBlockStatus(devCount, eBlockCount) == true)
        {
        continue;
        }
#endif

      if(FTL_ERR_PASS != (status = FTL_EraseOpNoDirty(devCount, eBlockCount)))
        {
#if (FTL_DEFECT_MANAGEMENT == true)
        if(status == FTL_ERR_FAIL)
          {
          return status;
          }
        foundBadBlock = true;
        if(eBlockCount < NUM_DATA_EBLOCKS)
          {
#if (CACHE_RAM_BD_MODULE == false)
          status = BadBlockEraseFailure(devCount, eBlockCount);
          if(status != FTL_ERR_PASS)
            {
            return status; // failed dealing with the bad block, return failure
            }
          else
            {
            continue; // managed the erase failure, move on
            }
#else
          flashPage.devID = devCount;
          flashPage.byteCount = 0;
          flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(devCount, eBlockCount), 0);
          flashPage.vPage.pageOffset = 0;
          if(FLASH_MarkDefectEBlock(&flashPage) != FLASH_PASS)
            {
            // do nothing, just try to mark bad, even if it fails we move on;             
            }
          continue;
#endif
          }
        else
          {
          SetBadEBlockStatus(devCount, eBlockCount, true);
          flashPage.devID = devCount;
          flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(devCount, eBlockCount), 0);
          flashPage.vPage.pageOffset = 0;
          flashPage.byteCount = 0;
          if(FLASH_MarkDefectEBlock(&flashPage) != FLASH_PASS)
            {
            // do nothing, just try to mark bad, even if it fails we move on.
            }
          if(eBlockCount >= SYSTEM_START_EBLOCK)
            {
            if(eBlockCount == logEBlock)
              {
              logEBlock++;
              if(logEBlock == flushEBlock)
                {
                flushEBlock++;
#if (FTL_SUPER_SYS_EBLOCK == true)
                if(flushEBlock == superEBlock)
                  {
                  superEBlock++;
                  }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
                }
              }
            if(eBlockCount == flushEBlock)
              {
              flushEBlock++;
#if (FTL_SUPER_SYS_EBLOCK == true)
              if(flushEBlock == superEBlock)
                {
                superEBlock++;
                }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
              }
#if (FTL_SUPER_SYS_EBLOCK == true)
            if(eBlockCount == superEBlock)
              {
              superEBlock++;
              }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
            }
          continue;
          }
#else
        return status;
#endif
        }
      }
    }
  for(tempCount = 0; tempCount < NUM_SYS_RESERVED_BYTES; tempCount++) /*there should be a better way to initalize the array*/
    {
    sysEBlockInfo.reserved[tempCount] = 0xFF;
    }
  for(devCount = 0; devCount < NUM_DEVICES; devCount++)
    {
    /*construct the system page for the log and flush eblocks*/
    flashPage.devID = devCount;
    flashPage.byteCount = sizeof (SYS_EBLOCK_INFO);
    flashPage.vPage.pageOffset = 0;
    sysEBlockInfo.incNumber = 0; /*start with 0*/
    sysEBlockInfo.type = SYS_EBLOCK_INFO_LOG;
    sysEBlockInfo.checkVersion = EMPTY_WORD;
    sysEBlockInfo.oldSysBlock = EMPTY_WORD;
    sysEBlockInfo.fullFlushSig = EMPTY_WORD;

#if(FTL_DEFECT_MANAGEMENT == true)
    while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
      {
      if(logEBlock >= NUMBER_OF_ERASE_BLOCKS)
        {
        return FTL_ERR_FORMAT_LOG_EBLOCK;
        }
#if (ENABLE_EB_ERASED_BIT == true)
      SetEBErased(devCount, logEBlock, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
      flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(devCount, logEBlock), 0); //(TRANSACTLOG_START_EBLOCK * NUM_PAGES_PER_EBLOCK);
      sysEBlockInfo.phyAddrThisEBlock = GetPhysicalEBlockAddr(devCount, logEBlock);
      if((status = FTL_WriteSysEBlockInfo(&flashPage, &sysEBlockInfo)) == FTL_ERR_PASS)
        {
        break;
        }
      if(status == FTL_ERR_FAIL)
        {
        return status;
        }
      foundBadBlock = true;
      SetBadEBlockStatus(devCount, logEBlock, true);
      // just try to mark bad, even if it fails we move on.
      FLASH_MarkDefectEBlock(&flashPage);
      sanityCounter++;
      logEBlock++;
      if(logEBlock == flushEBlock)
        {
        flushEBlock++;
#if (FTL_SUPER_SYS_EBLOCK == true)
        if(flushEBlock == superEBlock)
          {
          superEBlock++;
          }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
        }
      }
    if(status != FTL_ERR_PASS)
      {
      return status;
      }

#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(devCount, logEBlock), 0); //(TRANSACTLOG_START_EBLOCK * NUM_PAGES_PER_EBLOCK);
    sysEBlockInfo.phyAddrThisEBlock = GetPhysicalEBlockAddr(devCount, logEBlock);
    if((status = FTL_WriteSysEBlockInfo(&flashPage, &sysEBlockInfo)) != FTL_ERR_PASS)
      {
      return status;
      }
#if (ENABLE_EB_ERASED_BIT == true)
    SetEBErased(devCount, logEBlock, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

    /*construct the log eblock page*/
    flashPage.byteCount = sizeof (SYS_EBLOCK_INFO);
    flashPage.vPage.pageOffset = 0;
    sysEBlockInfo.incNumber = 0; /*start with 0*/
    sysEBlockInfo.type = SYS_EBLOCK_INFO_FLUSH;
    sysEBlockInfo.oldSysBlock = EMPTY_WORD;
    sysEBlockInfo.fullFlushSig = EMPTY_WORD;

#if(FTL_CHECK_VERSION == true)
    sysEBlockInfo.checkVersion = CalcCheckWord((uint16_t *) FTL_FLASH_IMAGE_VERSION, NUM_WORDS_OF_VERSION);

#else  // #if(FTL_CHECK_VERSION == true)
    sysEBlockInfo.checkVersion = EMPTY_WORD;
#endif  // #else  // #if(FTL_CHECK_VERSION == true)

#if(FTL_DEFECT_MANAGEMENT == true)
    sanityCounter = 0;
    while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
      {
      if(flushEBlock >= NUMBER_OF_ERASE_BLOCKS)
        {
        return FTL_ERR_FORMAT_FLUSH_EBLOCK;
        }
#if (ENABLE_EB_ERASED_BIT == true)
      SetEBErased(devCount, flushEBlock, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
      flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(devCount, (flushEBlock)), 0); //((FLUSH_LOG_START_EBLOCK ) * NUM_PAGES_PER_EBLOCK);
      sysEBlockInfo.phyAddrThisEBlock = GetPhysicalEBlockAddr(devCount, (flushEBlock));
      if((status = FTL_WriteSysEBlockInfo(&flashPage, &sysEBlockInfo)) == FTL_ERR_PASS)
        {
        break;
        }
      if(status == FTL_ERR_FAIL)
        {
        return status;
        }
      foundBadBlock = true;
      SetBadEBlockStatus(devCount, flushEBlock, true);
      // just try to mark bad, even if it fails we move on.
      FLASH_MarkDefectEBlock(&flashPage);
      sanityCounter++;
      flushEBlock++;
#if (FTL_SUPER_SYS_EBLOCK == true)
      if(flushEBlock == superEBlock)
        {
        superEBlock++;
        }
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
      }
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(devCount, (flushEBlock)), 0); //((FLUSH_LOG_START_EBLOCK ) * NUM_PAGES_PER_EBLOCK);
    sysEBlockInfo.phyAddrThisEBlock = GetPhysicalEBlockAddr(devCount, (flushEBlock));
    if((status = FTL_WriteSysEBlockInfo(&flashPage, &sysEBlockInfo)) != FTL_ERR_PASS)
      {
      return status;
      }
#if (ENABLE_EB_ERASED_BIT == true)
    SetEBErased(devCount, flushEBlock, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)

#if (FTL_SUPER_SYS_EBLOCK == true)
    /*construct the super eblock page*/
    flashPage.byteCount = sizeof (SYS_EBLOCK_INFO);
    flashPage.vPage.pageOffset = 0;
    sysEBlockInfo.incNumber = 0; /*start with 0*/
    sysEBlockInfo.type = SYS_EBLOCK_INFO_SUPER;
    sysEBlockInfo.oldSysBlock = EMPTY_WORD;
    sysEBlockInfo.fullFlushSig = EMPTY_WORD;

#if(FTL_CHECK_VERSION == true)
    sysEBlockInfo.checkVersion = CalcCheckWord((uint16_t *) FTL_FLASH_IMAGE_VERSION, NUM_WORDS_OF_VERSION);

#else  // #if(FTL_CHECK_VERSION == true)
    sysEBlockInfo.checkVersion = EMPTY_WORD;
#endif  // #else  // #if(FTL_CHECK_VERSION == true)

    // Create RAM Table
    if((status = TABLE_FlushEBInsert(devCount, flushEBlock, flushEBlock, 0)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = TABLE_TransLogEBInsert(devCount, logEBlock, logEBlock, 0)) != FTL_ERR_PASS)
      {
      return status;
      }
    SetSuperSysEBCounter(devCount, sysEBlockInfo.incNumber);
    if((status = TABLE_SuperSysEBInsert(devCount, superEBlock, superEBlock, 0)) != FTL_ERR_PASS)
      {
      return status;
      }

#if(FTL_DEFECT_MANAGEMENT == true)
    sanityCounter = 0;
    while(sanityCounter < MAX_BAD_BLOCK_SANITY_TRIES)
      {
      if(superEBlock >= NUMBER_OF_ERASE_BLOCKS)
        {
        return FTL_ERR_FORMAT_SUPER_EBLOCK;
        }
#if (ENABLE_EB_ERASED_BIT == true)
      SetEBErased(devCount, superEBlock, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
      flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(devCount, superEBlock), 0);
      sysEBlockInfo.phyAddrThisEBlock = GetPhysicalEBlockAddr(devCount, (superEBlock));
      if((status = FTL_WriteSysEBlockInfo(&flashPage, &sysEBlockInfo)) == FTL_ERR_PASS)
        {
        if((status = FTL_CreateSuperSysEBLog(devCount, SYS_EBLOCK_INFO_CHANGED)) == FTL_ERR_PASS)
          {
          if((status = FTL_CreateSuperSysEBLog(devCount, SYS_EBLOCK_INFO_SYSEB)) == FTL_ERR_PASS)
            {
            break;
            }
          }
        if(status != FTL_ERR_SUPER_WRITE_02)
          {
          return status;
          }
        }
      if(status == FTL_ERR_FAIL)
        {
        return status;
        }

      SetBadEBlockStatus(devCount, superEBlock, true);
      // just try to mark bad, even if it fails we move on.
      FLASH_MarkDefectEBlock(&flashPage);
      foundBadBlock = true;

      // Clear RAM Table of System EB
      if((status = TABLE_InitEBOrderingTable(devCount)) != FTL_ERR_PASS)
        {
        return status;
        }
      if((status = TABLE_SuperSysEBClear(devCount)) != FTL_ERR_PASS)
        {
        return status;
        }

      // Create RAM Table of System EB
      if((status = TABLE_FlushEBInsert(devCount, flushEBlock, flushEBlock, 0)) != FTL_ERR_PASS)
        {
        return status;
        }
      if((status = TABLE_TransLogEBInsert(devCount, logEBlock, logEBlock, 0)) != FTL_ERR_PASS)
        {
        return status;
        }
      SetSuperSysEBCounter(devCount, sysEBlockInfo.incNumber);
      if((status = TABLE_SuperSysEBInsert(devCount, superEBlock, superEBlock, 0)) != FTL_ERR_PASS)
        {
        return status;
        }
      sanityCounter++;
      superEBlock++;
      }
    if(status != FTL_ERR_PASS)
      {
      return status;
      }
#else  // #if(FTL_DEFECT_MANAGEMENT == true)
    flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(GetPhysicalEBlockAddr(devCount, superEBlock), 0);
    sysEBlockInfo.phyAddrThisEBlock = GetPhysicalEBlockAddr(devCount, (superEBlock));
    if((status = FTL_WriteSysEBlockInfo(&flashPage, &sysEBlockInfo)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = FTL_CreateSuperSysEBLog(devCount, SYS_EBLOCK_INFO_CHANGED)) != FTL_ERR_PASS)
      {
      return status;
      }
    if((status = FTL_CreateSuperSysEBLog(devCount, SYS_EBLOCK_INFO_SYSEB)) != FTL_ERR_PASS)
      {
      return status;
      }
#if (ENABLE_EB_ERASED_BIT == true)
    SetEBErased(devCount, superEBlock, false);
#endif  // #if (ENABLE_EB_ERASED_BIT == true)
#endif  // #else  // #if(FTL_DEFECT_MANAGEMENT == true)       
    if((status = TABLE_InitEBOrderingTable(devCount)) != FTL_ERR_PASS)
      {
      return status;
      }
#if(FTL_DEFECT_MANAGEMENT == true)
    if(foundBadBlock == false)
      {
#endif
      if((status = TABLE_SuperSysEBClear(devCount)) != FTL_ERR_PASS)
        {
        return status;
        }
#if(FTL_DEFECT_MANAGEMENT == true)
      }
#endif
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)

    }

#if(FTL_DEFECT_MANAGEMENT == true)
  for(devCount = 0; devCount < NUM_DEVICES; devCount++)
    {
#if (CACHE_RAM_BD_MODULE == false)
    sanityCounter = 0;
    for(eBlockCount = SYSTEM_START_EBLOCK; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
      {
      if(GetBadEBlockStatus(devCount, eBlockCount) == false)
        {
        sanityCounter++;
        }
      }
    if(sanityCounter < (MIN_SYSTEM_EBLOCKS + PFT_RECOVERY_RESERVE))
      {
      return FTL_ERR_FORMAT_SYS_EBLOCK;
      }
#endif
    if(foundBadBlock == true)
      {
      /*
          initialze the structs used by 
       */
      sanityCounter = 0;
      if((status = UpdateEBOrderingTable(devCount, SYSTEM_START_EBLOCK, (uint16_t *) & sanityCounter)) != FTL_ERR_PASS) /*reuse sanityCounter, dont need it anymore*/
        {
        return status;
        }

#if (CACHE_RAM_BD_MODULE == true)
      for(eBlockCount = 0; eBlockCount < NUM_DATA_EBLOCKS; eBlockCount++)
        {
        if(FTL_ERR_PASS != (status = CACHE_LoadEB(devCount, eBlockCount, CACHE_WRITE_TYPE)))
          {
          return status;
          }

        flashPage.devID = devCount;
        flashPage.byteCount = 0;
        flashPage.vPage.vPageAddr = CalcPhyPageAddrFromPageOffset(eBlockCount, 0);
        flashPage.vPage.pageOffset = 0;
        if((FLASH_CheckDefectEBlock(&flashPage)) == FLASH_PASS)
          {
          continue;
          }

        SetBadEBlockStatus(devCount, eBlockCount, true);

        status = BadBlockEraseFailure(devCount, eBlockCount);
        if(status != FTL_ERR_PASS)
          {
          return status; // failed dealing with the bad block, return failure
          }
        }
#endif

      if((status = TABLE_Flush(FLUSH_NORMAL_MODE)) != FTL_ERR_PASS)
        {
        FTL_ClearMTLockBit();
        return status;
        }
      }
#if (CACHE_RAM_BD_MODULE == true)
    sanityCounter = 0;
    for(eBlockCount = SYSTEM_START_EBLOCK; eBlockCount < NUM_EBLOCKS_PER_DEVICE; eBlockCount++)
      {
      if(GetBadEBlockStatus(devCount, eBlockCount) == false)
        {
        sanityCounter++;
        }
      }
    if(sanityCounter < (MIN_SYSTEM_EBLOCKS + PFT_RECOVERY_RESERVE))
      {
      return FTL_ERR_FORMAT_SYS_EBLOCK;
      }
#endif
    }
#endif  // #if(FTL_DEFECT_MANAGEMENT == true)

  return status;
  }

//---------------------------------------------------------------------------
#if (CACHE_RAM_BD_MODULE == true && CACHE_DYNAMIC_ALLOCATION == true)
FTL_STATUS FTL_Format(FTL_INIT_STRUCT *initStructPtr)
#else

FTL_STATUS FTL_Format(void)
#endif
{
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

#if (DEBUG_FTL_API_ANNOUNCE == 1)
  DBG_Printf("FTL_Format start: \n", 0, 0);
#endif  // #if (DEBUG_FTL_API_ANNOUNCE == 1)

  if((status = FTL_SetMTLockBit()) != FTL_ERR_PASS)
    {
    return status;
    }
#if (CACHE_RAM_BD_MODULE == true && CACHE_DYNAMIC_ALLOCATION == true)
  if((status = CACHE_DynamicAllocation(initStructPtr->total_ram_allowed)))
    {
    FTL_ClearMTLockBit();
    return status;
    }
#endif
  if((status = InitFTLRAMSTables()) != FTL_ERR_PASS)
    {
    FTL_ClearMTLockBit();
    return status;
    }

#if (CACHE_RAM_BD_MODULE == true) // debug
#if (CACHE_PARAM_DISPLAY == true)
  DBG_Printf("Total BD Size = %d\n", TOTAL_BD_RAM_SIZE, 0);
  DBG_Printf("EBLOCKMAPINDEX_SIZE Size = %d\n", EBLOCKMAPINDEX_SIZE, 0);
  DBG_Printf("PPAMAPINDEX_SIZE Size = %d\n", PPAMAPINDEX_SIZE, 0);
  DBG_Printf("RAMMAPINDEX_SIZE Size = %d\n", RAMMAPINDEX_SIZE, 0);
  DBG_Printf("EBMCACHEINDEX_SIZE Size = %d\n", EBMCACHEINDEX_SIZE, 0);
  DBG_Printf("EBLOCKMAPPINGCAACHE_SIZE Size = %d\n", EBLOCKMAPPINGCAACHE_SIZE, 0);
  DBG_Printf("PPAMAPPINGCACHE_SIZE Size = %d\n", PPAMAPPINGCACHE_SIZE, 0);
  DBG_Printf("EBLOCKMAPINDEX_SIZE Size = %d\n", EBLOCKMAPINDEX_SIZE, 0);
  DBG_Printf("NUM_EBLOCK_MAP_INDEX = %d\n", NUM_EBLOCK_MAP_INDEX, 0);
  DBG_Printf("CACHE_INDEX_CHANGE_AREA = %d\n", CACHE_INDEX_CHANGE_AREA, 0);
  DBG_Printf("NUM_PPA_MAP_INDEX = %d\n", NUM_PPA_MAP_INDEX, 0);
  DBG_Printf("FLUSH_RAM_TABLE_SIZE = %d\n", FLUSH_RAM_TABLE_SIZE, 0);
  DBG_Printf("EBLOCK_MAPPING_ENTRY_SIZE = %d\n", EBLOCK_MAPPING_ENTRY_SIZE, 0);
  DBG_Printf("PPA_CACHE_TABLE_OFFSET = %d\n", PPA_CACHE_TABLE_OFFSET, 0);
#endif
#endif

  if((status = FTL_InternalFormat()) != FTL_ERR_PASS)
    {
    FTL_ClearMTLockBit();
    return status;
    }
  FTL_ClearMTLockBit();

#if (DEBUG_FTL_API_ANNOUNCE == 1)
  DBG_Printf("FTL_Format done: \n", 0, 0);
#endif  // #if (DEBUG_FTL_API_ANNOUNCE == 1)

  return FTL_ERR_PASS;
  }

FTL_STATUS InitFTLRAMSTables(void)
  {
  FTL_STATUS status = FTL_ERR_PASS;

  previousDevice = EMPTY_BYTE;
  GC_THRESHOLD = FTL_GC_THRESHOLD;
  Delete_GC_Threshold = DELETE_GC_THRESHOLD;
  LastTransLogLba = EMPTY_BYTE;
  LastTransLogNpages = 0;

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  writeLogFlag = false;
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)

#if (FTL_STATIC_WEAR_LEVELING == true)
  status = InitStaticWLInfo();
  if(status != FTL_ERR_PASS)
    {
    return status;
    }
#endif
#if(FTL_DEFECT_MANAGEMENT == true)
  ClearBadBlockInfo();
  ClearTransLogEBBadBlockInfo();
#endif

  FTL_ClearGCSave(CLEAR_GC_SAVE_INIT_MODE);

  ClearTransferEB();
  // Init the GC_Info
  if((status = ClearGC_Info()) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = ClearDeleteInfo()) != FTL_ERR_PASS)
    {
    return status;
    }
#if (FTL_SUPER_SYS_EBLOCK == true)
  ClearSuperEBInfo();
  gProtectForSuperSysEBFlag = false;
#endif  // #if (FTL_SUPER_SYS_EBLOCK == true)
  if((status = INIT_InitBasic()) != FTL_ERR_PASS) /*Init the RAM tables*/
    {
    return status;
    }
  Init_PseudoRPB();
  return status;
  }
