#include "if_ex.h"
#include "def.h"
#include "common.h"

uint32_t CALC_MixPageAddress(uint32_t pageAddr)
  {
  uint32_t tempAddr = EMPTY_DWORD;
  uint32_t tempAddr1 = EMPTY_DWORD;

  tempAddr = (NUMBER_OF_PAGES_PER_EBLOCK * (pageAddr / NUMBER_OF_PAGES_PER_EBLOCK));
  tempAddr1 = NUMBER_OF_PAGES_PER_DEVICE - tempAddr;
  tempAddr1 += (pageAddr - tempAddr);
  return tempAddr1;
  }
//------------------------------

uint8_t GetBitMapField(uint8_t* table, uint16_t fieldNum, uint8_t fieldSize)
  {
  uint16_t shift = 0; /*2*/
  uint16_t index = 0; /*2*/
  uint8_t mask = 0; /*1*/

  switch(fieldSize)
    {
    case 1:
      index = fieldNum >> 3;
      shift = 7 - (fieldNum & 0x7);
      mask = 0x1;
      break;
    case 2:
      index = fieldNum >> 2;
      shift = 6 - ((fieldNum & 0x3) << 1);
      mask = 0x3;
      break;
    case 3:
      index = fieldNum / 3;
      shift = 5 - ((fieldNum % 3) * 3);
      mask = 0x7;
      break;
    case 4:
      index = fieldNum >> 1;
      shift = 4 - ((fieldNum & 0x1) << 2);
      mask = 0xF;
      break;
    default: // 5, 6, 7, or 8
      index = fieldNum;
      shift = 8 - fieldSize;
      mask = ((1 << fieldSize) - 1) << shift;
      break;
    }
  return mask & (table[index] >> shift);
  }

//---------------------

void SetBitMapField(uint8_t* table, uint16_t fieldNum, uint8_t fieldSize, uint8_t value)
  {
  uint16_t shift = 0; /*2*/
  uint16_t index = 0; /*2*/
  uint8_t mask = 0; /*1*/

  switch(fieldSize)
    {
    case 1:
      index = fieldNum >> 3;
      shift = 7 - (fieldNum & 0x7);
      mask = 0x1 << shift;
      break;
    case 2:
      index = fieldNum >> 2;
      shift = 6 - ((fieldNum & 0x3) << 1);
      mask = 0x3 << shift;
      break;
    case 3:
      index = fieldNum / 3;
      shift = 5 - ((fieldNum % 3) * 3);
      mask = 0x7 << shift;
      break;
    case 4:
      index = fieldNum >> 1;
      shift = 4 - ((fieldNum & 0x1) << 2);
      mask = 0xF << shift;
      break;
    default: // 5, 6, 7, or 8
      index = fieldNum;
      shift = 8 - fieldSize;
      mask = ((1 << fieldSize) - 1) << shift;
      break;
    }
  table[index] = (table[index] & ~mask) | ((value << shift) & mask);
  }

//-------------------

FTL_STATUS IncPageAddr(ADDRESS_STRUCT_PTR pageAddr)
  {
  if(pageAddr->devID == (NUM_DEVICES - 1))
    {
    pageAddr->devID = 0;
    (pageAddr->logicalPageNum)++;
    }
  else
    {
    (pageAddr->devID)++;
    }
  pageAddr->pageOffset = 0;
  return FTL_ERR_PASS;
  }

//-----------------------

FTL_STATUS GetPageNum(uint32_t LBA, uint32_t * pageAddress)
  {
  *pageAddress = (LBA & PAGE_ADDRESS_BIT_MAP) >> PAGE_ADDRESS_SHIFT;
  return FTL_ERR_PASS;
  }

//------------------------------------------------------------------------

FTL_STATUS GetDevNum(uint32_t LBA, uint8_t * devNum)
  {
  *devNum = (uint8_t) (LBA & DEVICE_BIT_MAP) >> DEVICE_BIT_SHIFT;
  return FTL_ERR_PASS;
  }

//-------------------------------------------------------------------------

FTL_STATUS GetSectorNum(uint32_t LBA, uint8_t * secNum)
  {
  *secNum = (uint8_t) (LBA & SECTOR_BIT_MAP) >> SECTOR_BIT_SHIFT;
  return FTL_ERR_PASS;
  }

//-----------------------------------------------------------------------

FTL_STATUS GetLogicalEBNum(uint32_t pageAddress, uint16_t * logicalEBNum)
  {
  *logicalEBNum = (uint16_t) (pageAddress / NUM_PAGES_PER_EBLOCK);
  return FTL_ERR_PASS;
  }

//---------------------------------------------------------------------------

FTL_STATUS GetLogicalPageOffset(uint32_t pageAddress, uint16_t * pageOffset)
  {
  (*pageOffset) = (uint16_t) (pageAddress % NUM_PAGES_PER_EBLOCK);
  return FTL_ERR_PASS;
  }

// Note: in some technologies the Logical offset is different than the physical
//---------------------------------------------------------------------------

FTL_STATUS GetPhysicalPageOffset(uint32_t pageAddress, uint16_t * pageOffset)
  {
  (*pageOffset) = (uint16_t) (pageAddress % NUM_PAGES_PER_EBLOCK);
  return FTL_ERR_PASS;
  }

//-----------------------------------------------------------------------------

FTL_STATUS SetPhyPageIndex(FTL_DEV deviceNum, uint16_t logicalPageAddr, uint16_t physicalPage)
  {
  FTL_STATUS status = FTL_ERR_PASS; /*4*/
  uint16_t pageOffset = 0; /*2*/
  uint16_t logicalPageOffset = 0; /*2*/
  uint16_t logicalEBNum = 0; /*2*/

  if((status = GetPhysicalPageOffset(physicalPage, &pageOffset)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = GetLogicalPageOffset(logicalPageAddr, &logicalPageOffset)) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = GetLogicalEBNum(logicalPageAddr, &logicalEBNum)) != FTL_ERR_PASS)
    {
    return status;
    }
  SetPPASlot(deviceNum, logicalEBNum, logicalPageOffset, pageOffset);
  MarkPPAMappingTableEntryDirty(deviceNum, logicalEBNum, logicalPageOffset);
  return FTL_ERR_PASS;
  }

//-----------------------------------------------------------------------------

FTL_STATUS GetPageSpread(uint32_t LBA, uint32_t NB, ADDRESS_STRUCT_PTR startPage,
                         uint32_t * totalPages, ADDRESS_STRUCT_PTR endPage)
  {
  uint32_t aTemp = 0; /*4*/
  uint32_t bTemp = 0; /*4*/
  uint32_t cTemp = 0; /*4*/
  FTL_STATUS status = FTL_ERR_PASS; /*4*/

  if((status = GetDevNum(LBA, (&(startPage->devID)))) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = GetPageNum(LBA, (&(startPage->logicalPageNum)))) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = GetSectorNum(LBA, (&(startPage->pageOffset)))) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = GetDevNum(LBA + NB, (&(endPage->devID)))) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = GetPageNum(LBA + NB, (&(endPage->logicalPageNum)))) != FTL_ERR_PASS)
    {
    return status;
    }
  if((status = GetSectorNum(LBA + NB, (&(endPage->pageOffset)))) != FTL_ERR_PASS)
    {
    return status;
    }
  *totalPages = 0;
  if(startPage->pageOffset > 0)
    {
    (*totalPages)++;
    aTemp = (NUM_SECTORS_PER_PAGE - startPage->pageOffset);
    }
  if(endPage->pageOffset > 0)
    {
    (*totalPages)++;
    bTemp = endPage->pageOffset;
    }
  if((endPage->devID == startPage->devID) &&
     (endPage->logicalPageNum == startPage->logicalPageNum) &&
     (bTemp != 0) && (aTemp != 0))
    {
    (*totalPages)--;
    return FTL_ERR_PASS;
    }
  cTemp = NB - aTemp - bTemp;
  (*totalPages) += cTemp / NUM_SECTORS_PER_PAGE;

#ifdef FTL_LEVEL_ONE_DEFINED
  if(cTemp % NUM_SECTORS_PER_PAGE) /*cTemp must be whole pages in size, if not we have error*/
    {
    return FTL_ERR_CALC_2;
    }
#endif  // #ifdef FTL_LEVEL_ONE_DEFINED

  return FTL_ERR_PASS;
  }

//----------------------

uint32_t CalcPhyPageAddrFromLogIndex(uint16_t phyEBNum, uint16_t logIndex)
  {
  if(logIndex == EMPTY_WORD)
    {
    return EMPTY_DWORD;
    }
  return ((phyEBNum * NUM_PAGES_PER_EBLOCK) + (logIndex / (VIRTUAL_PAGE_SIZE / LOG_ENTRY_DELTA)));
  }

//-------------------------

uint32_t CalcPhyPageAddrFromPageOffset(uint16_t phyEBNum, uint16_t pageOffset)
  {
  if(pageOffset == EMPTY_INVALID)
    {
    return EMPTY_DWORD;
    }
  return ((phyEBNum * NUM_PAGES_PER_EBLOCK) + pageOffset);
  }

//-----------------------------

uint32_t CalcFlushRamTablePages(uint16_t phyEBNum, uint16_t index)
  {
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  if(index == 0)
    {
    return EMPTY_WORD;
    }
#endif

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
#if (CACHE_RAM_BD_MODULE == true)
  return (uint32_t) ((phyEBNum * NUM_PAGES_PER_EBLOCK)+((EBLOCK_SIZE - (index * FLUSH_RAM_TABLE_SIZE)) / VIRTUAL_PAGE_SIZE));
#else
  return (uint32_t) ((phyEBNum * NUM_PAGES_PER_EBLOCK)+((EBLOCK_SIZE - (index * SECTOR_SIZE)) / VIRTUAL_PAGE_SIZE));
#endif
#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  return (uint32_t) ((phyEBNum * NUM_PAGES_PER_EBLOCK)+((index * SECTOR_SIZE) / VIRTUAL_PAGE_SIZE));
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  }

//------------------------------

uint16_t CalcFlushRamTableOffset(uint16_t index)
  {
#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  if(index == 0)
    {
    return EMPTY_WORD;
    }
#endif

#if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
#if (CACHE_RAM_BD_MODULE == true)
  return ((EBLOCK_SIZE - (index * FLUSH_RAM_TABLE_SIZE)) % VIRTUAL_PAGE_SIZE);
#else
  return ((EBLOCK_SIZE - (index * SECTOR_SIZE)) % VIRTUAL_PAGE_SIZE);
#endif

#elif(FTL_DEVICE_TYPE == FTL_DEVICE_NAND)
  return ((index * SECTOR_SIZE) % VIRTUAL_PAGE_SIZE);
#endif  // #if(FTL_DEVICE_TYPE == FTL_DEVICE_NOR)
  }

//---------------------------

uint16_t GetIndexFromPhyPage(uint32_t freePageAddr)
  {
  return (uint16_t) (freePageAddr % NUM_PAGES_PER_EBLOCK);
  }

//------------------------------

uint16_t CalcNumLogEntries(uint32_t numPages)
  {
  uint32_t temp0 = 0; /*4*/
  uint32_t temp = 0; /*4*/

  if(numPages > (NUM_ENTRIES_TYPE_A + (NUM_ENTRIES_TYPE_B * TEMP_B_ENTRIES)))
    {
    return EMPTY_WORD;
    }
  if(numPages <= NUM_ENTRIES_TYPE_A)
    {
    return MIN_LOG_ENTRIES_NEEDED;
    }
  temp0 = numPages - NUM_ENTRIES_TYPE_A;
  temp = temp0 / NUM_ENTRIES_TYPE_B;
  if(temp0 % NUM_ENTRIES_TYPE_B)
    {
    temp++;
    }
  return (uint16_t) (temp + MIN_LOG_ENTRIES_NEEDED); /*to account for TYPE_A and account for TYPE_C*/
  }

//------------------------

uint16_t CalcCheckWord(uint16_t * dataPtr, uint16_t nDataWords)
  {
  uint16_t count = 0; /*2*/
  uint16_t sum = 0; /*2*/

  sum = 0;
  for(count = 0; count < nDataWords; count++)
    {
    sum += *dataPtr;
    dataPtr++;
    }
  return (sum ^ EMPTY_WORD) + 1;
  }

//------------------------

uint8_t VerifyCheckWord(uint16_t * dataPtr, uint16_t nDataWords, uint16_t checkWord)
  {
  uint16_t localWord = 0; /*2*/

  localWord = CalcCheckWord(dataPtr, nDataWords);
  if(localWord != checkWord)
    {
    return true; // Fail
    }
  return false; // Pass
  }

//-------------------------

uint32_t CalculateEBEraseWeight(FTL_DEV devID, uint16_t logEBNum)
  {
  uint32_t eraseCount = 0; /*4*/
  uint32_t useCount = 0; /*4*/

  eraseCount = GetTrueEraseCount(devID, logEBNum);
  useCount = GetNumUsedPages(devID, logEBNum);
  return (eraseCount * NUM_PAGES_PER_EBLOCK) +useCount;
  }

#if (SPANSCRC32 == true)

void CalcInitCRC()
  {
  int i, j;
  unsigned int c;

  for(i = 0; i < 256; ++i)
    {
    for(c = i << 24, j = 8; j > 0; --j)
      c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
    crc32_table[i] = c;
    }
  }

void CalcCalculateCRC(uint8_t * data, uint32_t * crc32)
  {
  uint8_t *p;
  unsigned int crc;
  int len = PAYLOAD_SIZE_BYTES_CRC;

  if(!crc32_table[1]) /* if not already done, */
    CalcInitCRC(); /* build table */

  crc = 0xffffffff; /* preload shift register, per CRC-32 spec */
  for(p = data; len > 0; ++p, --len)
    crc = (crc << 8) ^ crc32_table[(crc >> 24) ^ *p];

  crc = ~crc; /* transmit complement, per CRC-32 spec */
  *crc32 = crc;

  }

FTL_STATUS CalcCompareCRC(uint32_t origCRC, uint32_t newCRC)
  {
  if(origCRC == newCRC)
    {
    return CRC_ERROR_NONE;
    }
  else
    {
    return CRC_ERROR;
    }
  }
#endif