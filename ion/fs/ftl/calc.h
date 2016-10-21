#ifndef FTL_CALC_H
#define FTL_CALC_H

#include "def.h"

uint8_t GetBitMapField(uint8_t * table, uint16_t fieldNum, uint8_t fieldSize);
void SetBitMapField(uint8_t * table, uint16_t fieldNum, uint8_t fieldSize, uint8_t value);
FTL_STATUS IncPageAddr(ADDRESS_STRUCT_PTR pageAddr );
FTL_STATUS GetPageNum(uint32_t LBA, uint32_t * pageAddress);
FTL_STATUS GetDevNum(uint32_t LBA, uint8_t * devNum);
FTL_STATUS GetSectorNum(uint32_t LBA, uint8_t * secNum);
FTL_STATUS GetLogicalEBNum(uint32_t pageAddress, uint16_t * logicalEBNum);
FTL_STATUS GetLogicalPageOffset(uint32_t pageAddress, uint16_t * pageOffset);
FTL_STATUS GetPhysicalPageOffset(uint32_t pageAddress, uint16_t * pageOffset);
FTL_STATUS GetPageSpread(uint32_t LBA, uint32_t NB, ADDRESS_STRUCT_PTR startPage, uint32_t * totalPages, ADDRESS_STRUCT_PTR endPage );
uint32_t CalcPhyPageAddrFromLogIndex(uint16_t phyEBNum, uint16_t logIndex);
uint32_t CalcPhyPageAddrFromPageOffset(uint16_t phyEBNum, uint16_t pageOffset);
uint16_t GetIndexFromPhyPage(uint32_t freePageAddr);
FTL_STATUS SetPhyPageIndex(FTL_DEV deviceNum, uint16_t logicalPageAddr, uint16_t physicalPage);
uint16_t CalcNumLogEntries(uint32_t numPages);
uint16_t CalcCheckWord(uint16_t * dataPtr, uint16_t nDataWords);
uint8_t VerifyCheckWord(uint16_t * dataPtr, uint16_t nDataWords, uint16_t checkWord);
//uint16_t pickEBCandidate(EMPTY_LIST_PTR emptyList, uint16_t totalEmpty, uint16_t * index);
uint16_t pickEBCandidate(EMPTY_LIST_PTR emptyList, uint16_t totalEmpty, uint8_t pickHottest);
uint32_t CalculateEBEraseWeight(FTL_DEV devID, uint16_t logEBNum);
uint32_t CALC_MixPageAddress(uint32_t pageAddr);
/* 
 * Spansion CRC algorithm generates standard CRC32(4byte) per 2048byte data array.
 */
 

#if (SPANSCRC32 == true)
typedef enum
{
    CRC_ERROR_NONE = 0,                 /* no error             */
    CRC_ERROR                           /* data error           */
} crc_error;

/*
 * Build auxiliary table for parallel byte-at-a-time CRC-32.
 */
#define CRC32_POLY 0x04c11db7     /* AUTODIN II, Ethernet, & FDDI */
#define PAYLOAD_SIZE_BYTES_CRC VIRTUAL_PAGE_SIZE

extern void CalcInitCRC(void);

void CalcCalculateCRC(uint8_t * data, uint32_t * crc32);
FTL_STATUS CalcCompareCRC(uint32_t origCRC, uint32_t newCRC);

#endif /*SPANSCRC32*/

#endif  // #ifndef FTL_CALC_H
