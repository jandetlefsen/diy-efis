#ifndef __INC_H_lldh
#define __INC_H_lldh

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

 
#define LLD_VERSION   "15.2.1"   //  Year.Quarter.Minor
 
#ifdef S26KSxxxS_S26KLxxxS
#define SA_OFFSET_MASK	0xFFFE0000   /* mask off the offset */
#else
#define SA_OFFSET_MASK  0xFFFFF000   /* mask off the offset */
#endif //#ifdef S26KSxxxS_S26KLxxxS

/* LLD Command Definition */
#define NOR_CFI_QUERY_CMD                ((0x98)*LLD_DEV_MULTIPLIER)
#define NOR_CHIP_ERASE_CMD               ((0x10)*LLD_DEV_MULTIPLIER)
#define NOR_ERASE_SETUP_CMD              ((0x80)*LLD_DEV_MULTIPLIER)
#define NOR_RESET_CMD                    ((0xF0)*LLD_DEV_MULTIPLIER)
#define NOR_SECSI_SECTOR_ENTRY_CMD       ((0x88)*LLD_DEV_MULTIPLIER)
#define NOR_SECTOR_ERASE_CMD             ((0x30)*LLD_DEV_MULTIPLIER)
#define NOR_WRITE_BUFFER_LOAD_CMD        ((0x25)*LLD_DEV_MULTIPLIER)
#define NOR_WRITE_BUFFER_PGM_CONFIRM_CMD ((0x29)*LLD_DEV_MULTIPLIER) 
#define NOR_SET_CONFIG_CMD           ((0xD0)*LLD_DEV_MULTIPLIER)
#define NOR_BIT_FIELD_CMD        ((0xBF)*LLD_DEV_MULTIPLIER)

#define NOR_ERASE_SUSPEND_CMD      ((0xB0)*LLD_DEV_MULTIPLIER)
#define NOR_ERASE_RESUME_CMD       ((0x30)*LLD_DEV_MULTIPLIER)
#define NOR_PROGRAM_SUSPEND_CMD      ((0x51)*LLD_DEV_MULTIPLIER)
#define NOR_PROGRAM_RESUME_CMD       ((0x50)*LLD_DEV_MULTIPLIER)
#define NOR_STATUS_REG_READ_CMD      ((0x70)*LLD_DEV_MULTIPLIER)
#define NOR_STATUS_REG_CLEAR_CMD     ((0x71)*LLD_DEV_MULTIPLIER)
#define NOR_BLANK_CHECK_CMD        ((0x33)*LLD_DEV_MULTIPLIER)

/* Command code definition */
#define NOR_AUTOSELECT_CMD               ((0x90)*LLD_DEV_MULTIPLIER)
#define NOR_PROGRAM_CMD                  ((0xA0)*LLD_DEV_MULTIPLIER)
#define NOR_SECSI_SECTOR_EXIT_SETUP_CMD  ((0x90)*LLD_DEV_MULTIPLIER)
#define NOR_SECSI_SECTOR_EXIT_CMD        ((0x00)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_BYPASS_ENTRY_CMD      ((0x20)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_BYPASS_PROGRAM_CMD    ((0xA0)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_BYPASS_RESET_CMD1     ((0x90)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_BYPASS_RESET_CMD2     ((0x00)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_DATA1                 ((0xAA)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_DATA2                 ((0x55)*LLD_DEV_MULTIPLIER)
#define NOR_SUSPEND_CMD                  ((0xB0)*LLD_DEV_MULTIPLIER)
#define NOR_RESUME_CMD                   ((0x30)*LLD_DEV_MULTIPLIER)
#define NOR_READ_CONFIG_CMD          ((0xC6)*LLD_DEV_MULTIPLIER)
#define NOR_WRITE_BUFFER_ABORT_RESET_CMD ((0xF0)*LLD_DEV_MULTIPLIER)

/* Sector protection command definition */
#define PPB_PROTECTED                           (0*LLD_DEV_MULTIPLIER)
#define PPB_UNPROTECTED                         (1*LLD_DEV_MULTIPLIER)

#define WSXXX_LOCK_REG_ENTRY      (0x40*LLD_DEV_MULTIPLIER)
#define WSXXX_LOCK_REG2_ENTRY     (0x41*LLD_DEV_MULTIPLIER)  // for GL-R
#define WSXXX_PSWD_PROT_CMD_ENTRY (0x60*LLD_DEV_MULTIPLIER)
#define WSXXX_PSWD_UNLOCK_1       (0x25*LLD_DEV_MULTIPLIER)
#define WSXXX_PSWD_UNLOCK_2       (0x03*LLD_DEV_MULTIPLIER)
#define WSXXX_PSWD_UNLOCK_3       (0x29*LLD_DEV_MULTIPLIER)
#define WSXXX_PPB_ENTRY           (0xC0*LLD_DEV_MULTIPLIER)
#define WSXXX_PPB_ERASE_CONFIRM   (0x30*LLD_DEV_MULTIPLIER)
#define WSXXX_PPB_LOCK_ENTRY      (0x50*LLD_DEV_MULTIPLIER)
#define WSXXX_DYB_ENTRY           (0xE0*LLD_DEV_MULTIPLIER)
#define WSXXX_DYB_CLEAR           (0x01*LLD_DEV_MULTIPLIER)

#define NOR_LOCK_REG_ENTRY        (0x40*LLD_DEV_MULTIPLIER)
#define NOR_SECTOR_LOCK_CMD     (0x60*LLD_DEV_MULTIPLIER)
#define NOR_LOAD_SECTOR_ADR     (0x61*LLD_DEV_MULTIPLIER)
#define NOR_SECTOR_UNLOCK_ADR6    (0x40*LLD_DEV_MULTIPLIER)
#define NOR_SECTOR_LOCK_ADR6    ((~NOR_SECTOR_UNLOCK_ADR6)*LLD_DEV_MULTIPLIER)

#ifdef S26KSxxxS_S26KLxxxS
#define NOR_ENTER_DEEP_POWER_DOWN_CMD                 ((0xB9)*LLD_DEV_MULTIPLIER)
#define NOR_MEASURE_TEMPERATURE_CMD                   ((0xA9)*LLD_DEV_MULTIPLIER)
#define NOR_READ_TEMPERATURE_REG_CMD                    ((0xA8)*LLD_DEV_MULTIPLIER)
#define NOR_PROGRAM_POR_TIMER_CMD                           ((0x34)*LLD_DEV_MULTIPLIER)
#define NOR_READ_POR_TIMER_CMD                                  ((0x3C)*LLD_DEV_MULTIPLIER)
#define NOR_LOAD_INTERRUPT_CFG_REG_CMD                 ((0x36)*LLD_DEV_MULTIPLIER)
#define NOR_READ_INTERRUPT_CFG_REG_CMD                 ((0xC4)*LLD_DEV_MULTIPLIER)
#define NOR_LOAD_INTERRUPT_STATUS_REG_CMD           ((0x37)*LLD_DEV_MULTIPLIER)
#define NOR_READ_INTERRUPT_STATUS_REG_CMD           ((0xC5)*LLD_DEV_MULTIPLIER)
#define NOR_LOAD_VOLATILE_CFG_REG_CMD                   ((0x38)*LLD_DEV_MULTIPLIER)
#define NOR_READ_VOLATILE_CFG_REG_CMD                   ((0xC7)*LLD_DEV_MULTIPLIER)
#define NOR_PROGRAM_NON_VOLATILE_CFG_REG_CMD    ((0x39)*LLD_DEV_MULTIPLIER)
#define NOR_ERASE_NON_VOLATILE_CFG_REG_CMD          ((0xC8)*LLD_DEV_MULTIPLIER)
#define NOR_READ_NON_VOLATILE_CFG_REG_CMD           ((0xC6)*LLD_DEV_MULTIPLIER)
#define NOR_EVALUATE_ERASE_STATUS_CMD                   ((0xD0)*LLD_DEV_MULTIPLIER)
#define NOR_CRC_ENTRY_CMD                                             ((0x78)*LLD_DEV_MULTIPLIER) 
#define NOR_LOAD_CRC_START_ADDR_CMD                       ((0xC3)*LLD_DEV_MULTIPLIER) 
#define NOR_LOAD_CRC_END_ADDR_CMD                           ((0x3C)*LLD_DEV_MULTIPLIER)
#define NOR_CRC_SUSPEND_CMD                                        ((0xC0)*LLD_DEV_MULTIPLIER)
#define NOR_CRC_RESUME_CMD                                          ((0xC1)*LLD_DEV_MULTIPLIER)
#define NOR_CRC_READ_CHECKVALUE_RESLUT_REG_CMD  ((0x60)*LLD_DEV_MULTIPLIER)
#define NOR_CRC_EXIT_CMD                                               ((0xF0)*LLD_DEV_MULTIPLIER)  
#define NOR_SA_PROTECT_STATUS_CMD           ((0x60)*LLD_DEV_MULTIPLIER)
#endif /* #ifdef S26KSxxxS_S26KLxxxS  */

#if defined(S29GL064S)
#define NOR_EVALUATE_ERASE_STATUS_CMD                   ((0x35)*LLD_DEV_MULTIPLIER)
#endif /* #if defined(S29GL064S) */

/* polling routine options */
typedef enum
{
LLD_P_POLL_NONE = 0,      /* pull program status */
LLD_P_POLL_PGM,           /* pull program status */
LLD_P_POLL_WRT_BUF_PGM,     /* Poll write buffer   */
LLD_P_POLL_SEC_ERS,         /* Poll sector erase   */
LLD_P_POLL_CHIP_ERS,      /* Poll chip erase     */
LLD_P_POLL_RESUME,
LLD_P_POLL_BLANK          /* Poll device sector blank check */
}POLLING_TYPE;

/* polling return status */
typedef enum {
 DEV_STATUS_UNKNOWN = 0,
 DEV_NOT_BUSY,
 DEV_BUSY,
 DEV_EXCEEDED_TIME_LIMITS,
 DEV_SUSPEND,
 DEV_WRITE_BUFFER_ABORT,
 DEV_STATUS_GET_PROBLEM,
 DEV_VERIFY_ERROR,
 DEV_BYTES_PER_OP_WRONG,
 DEV_ERASE_ERROR,       
 DEV_PROGRAM_ERROR,       
 DEV_SECTOR_LOCK,
 DEV_PROGRAM_SUSPEND,     /* Device is in program suspend mode */
 DEV_PROGRAM_SUSPEND_ERROR,   /* Device program suspend error */
 DEV_ERASE_SUSPEND,       /* Device is in erase suspend mode */
 DEV_ERASE_SUSPEND_ERROR,   /* Device erase suspend error */
 DEV_BUSY_IN_OTHER_BANK,     /* Busy operation in other bank */
 DEV_CONTINUITY_CHECK_PATTERN_ERROR, /*Continuity Check error, detected continuity pattern as unexpected */
 DEV_CONTINUITY_CHECK_NO_PATTERN_ERROR, /* Continuity Check error, no detected continuity pattern as unexpected */
 DEV_CONTINUITY_CHECK_PATTERN_DETECTED /* Continuity Check successfully and pattern detected */
} DEVSTATUS;

#include "lld_target_specific.h"

typedef enum 
{
  FLSTATE_NOT_BUSY = 0,
  FLSTATE_ERASE = 1,
  FLSTATE_WRITEBUFFER = 2
} FLSTATE;

#define FLRESUME 0
#define FLSUSPEND 1

#define DEV_RDY_MASK      (0x80*LLD_DEV_MULTIPLIER) /* Device Ready Bit */
#define DEV_ERASE_SUSP_MASK   (0x40*LLD_DEV_MULTIPLIER) /* Erase Suspend Bit */
#define DEV_ERASE_MASK      (0x20*LLD_DEV_MULTIPLIER) /* Erase Status Bit */
#define DEV_PROGRAM_MASK    (0x10*LLD_DEV_MULTIPLIER) /* Program Status Bit */
#define DEV_RFU_MASK      (0x08*LLD_DEV_MULTIPLIER) /* Reserved */
#define DEV_PROGRAM_SUSP_MASK (0x04*LLD_DEV_MULTIPLIER) /* Program Suspend Bit */
#define DEV_SEC_LOCK_MASK   (0x02*LLD_DEV_MULTIPLIER) /* Sector lock Bit */
#define DEV_BANK_MASK     (0x01*LLD_DEV_MULTIPLIER) /* Operation in current bank */

#ifdef S26KSxxxS_S26KLxxxS
#define DEV_CRCSSB_MASK			(0x0100*LLD_DEV_MULTIPLIER)	/* CRC Suspend Bit, 1: suspend, 0: no suspend*/
#define DEV_ESTAT_MASK        (0x01*LLD_DEV_MULTIPLIER)	/* Sector Erase Status Bit (for Evaluate Erase Status)*/
                                                                                              /*0=previous erase did not complete successfully*/
                                                                                              /*1=previous erase completed successfully*/
#endif //#ifdef S26KSxxxS_S26KLxxxS                                                                                             

#if defined(S29GL064S)
#define DEV_CONTINUITY_CHECK_MASK     (0x01*LLD_DEV_MULTIPLIER) /* Continuity Check */
#endif  /* #if defined(S29GL064S) */

/*****************************************************
* Define Flash read/write macro to be used by LLD    *
*****************************************************/
#define FLASH_OFFSET(b,o)       (*(( (volatile FLASHDATA*)(b) ) + (o)))

#ifdef LLD_DEV_FLASH
  #ifdef TRACE
    #define FLASH_WR(b,o,d)         FlashWrite( b,o,d )
    #define FLASH_RD(b,o)           FlashRead(b,o)
  #else
    #ifdef EXTEND_ADDR
      #define FLASH_WR(b,o,d)     FlashWrite_Extend(b,o,d)
      #define FLASH_RD(b,o)       FlashRead_Extend(b,o) 
    #else
      #ifdef USER_SPECIFIC_CMD
        #define FLASH_WR(b,o,d) FlashWriteUserCmd((uint32_t) ((volatile FLASHDATA *)(b) + (uint32_t)o),d)
        #define FLASH_RD(b,o) FlashReadUserCmd((uint32_t) ((volatile FLASHDATA *)(b) + (uint32_t)o))
      #else
        #ifdef USER_SPECIFIC_CMD_2 // S26KSxxxS_S26KLxxxS LLD verification IO
          #define FLASH_WR(b,o,d)         ApiDrvLLDWriteWord(o,d )
          #define FLASH_RD(b,o)           ApiDrvLLDReadWord(o)
          #define FLASH_PAGE_RD(b,o,buf,cnt) ApiDrvReadArray((volatile FLASHDATA *)buf, (uint32_t) ((volatile FLASHDATA *)(b) + (uint32_t)o), cnt)
        #else
          #ifdef USER_SPECIFIC_CMD_3 //added NOR Page Read
            #define FLASH_WR(b,o,d) FlashWriteUserCmd((uint32_t) ((volatile FLASHDATA *)(b) + (uint32_t)o),d)
            #define FLASH_RD(b,o) FlashReadUserCmd((uint32_t) ((volatile FLASHDATA *)(b) + (uint32_t)o))
            #define FLASH_PAGE_RD(b,o,buf,cnt) FlashPageReadUserCmd((FLASHDATA *)(b), (uint32_t)o, (FLASHDATA *)(buf),  (FLASHDATA) cnt)
          #else
            #define FLASH_WR(b,o,d) FLASH_OFFSET((b),(o)) = (d)
            #define FLASH_RD(b,o)   FLASH_OFFSET((b),(o))
          #endif //#ifdef USER_SPECIFIC_CMD_3
        #endif //#ifdef USER_SPECIFIC_CMD_2
      #endif //#ifdef USER_SPECIFIC_CMD
    #endif // #ifdef EXTEND_ADDR
  #endif // #ifdef TRACE
#else
    #include "lld_dev_sim.h"
#endif // #ifdef LLD_DEV_FLASH


#ifdef  LLD_CONFIGURATX16_AS_X16           // LLD_KEEP
#define LLD_DEV_MULTIPLIER 0x00000001
#define LLD_DB_READ_MASK   0x0000FFFF
#define LLD_DEV_READ_MASK  0x0000FFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000002
#ifdef S29WSxxxN
#define LLD_CFI_UNLOCK_ADDR1 0x00000555
#else
#define LLD_CFI_UNLOCK_ADDR1 0x00000055
#endif
typedef uint16_t FLASHDATA;
#define LLD_BUF_SIZE_MULTIPLIER 1

#elif defined  LLD_CONFIGURATX32_AS_X32           
#define LLD_DEV_MULTIPLIER 0x00000001
#define LLD_DB_READ_MASK   0xFFFFFFFF 
#define LLD_DEV_READ_MASK  0xFFFFFFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000004
#define LLD_CFI_UNLOCK_ADDR1 0x00000055
typedef uint32_t FLASHDATA;
#define LLD_BUF_SIZE_MULTIPLIER 1

#elif defined LLD_CONFIGURATX8X16_AS_X8    // LLD_KEEP
#define LLD_DEV_MULTIPLIER 0x00000001
#define LLD_DB_READ_MASK   0x000000FF
#define LLD_DEV_READ_MASK  0x000000FF
#define LLD_UNLOCK_ADDR1   0x00000AAA
#define LLD_UNLOCK_ADDR2   0x00000555
#define LLD_BYTES_PER_OP   0x00000001
#ifdef S29WSxxxN
#define LLD_CFI_UNLOCK_ADDR1 0x00000AAA
#else
#define LLD_CFI_UNLOCK_ADDR1 0x000000AA
#endif
#if defined(S29GL064S)
#define LLD_CONTINUITY_CHECK_ADDR_1 0xD5554AB
#define LLD_CONTINUITY_CHECK_ADDR_2 0x2AAAB54
#define LLD_CONTINUITY_CHECK_DATA_1 0xFF
#define LLD_CONTINUITY_CHECK_DATA_1 0x00
#endif
typedef uint8_t FLASHDATA;
#define LLD_BUF_SIZE_MULTIPLIER 2

#elif defined LLD_CONFIGURATX8X16_AS_X16   // LLD_KEEP
#define LLD_DEV_MULTIPLIER 0x00000001
#define LLD_DB_READ_MASK   0x0000FFFF
#define LLD_DEV_READ_MASK  0x0000FFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000002
#ifdef S29WSxxxN
#define LLD_CFI_UNLOCK_ADDR1 0x00000555
#else
#define LLD_CFI_UNLOCK_ADDR1 0x00000055
#endif
#if defined(S29GL064S)
#define LLD_CONTINUITY_CHECK_ADDR_1 0x6AAAA55
#define LLD_CONTINUITY_CHECK_ADDR_2 0x15555AA
#define LLD_CONTINUITY_CHECK_DATA_1 0xFF00
#define LLD_CONTINUITY_CHECK_DATA_2 0x00FF
#endif
typedef uint16_t FLASHDATA;
#define LLD_BUF_SIZE_MULTIPLIER 1

#elif defined LLD_CONFIGURATX16_AS_X32     // LLD_KEEP
#define LLD_DEV_MULTIPLIER 0x00010001
#define LLD_DB_READ_MASK   0xFFFFFFFF
#define LLD_DEV_READ_MASK  0x0000FFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000004
#ifdef S29WSxxxN
#define LLD_CFI_UNLOCK_ADDR1 0x00000555
#else
#define LLD_CFI_UNLOCK_ADDR1 0x00000055
#endif
typedef uint32_t FLASHDATA;
#define LLD_BUF_SIZE_MULTIPLIER 1

#elif defined LLD_CONFIGURATX8X16_AS_X32   // LLD_KEEP
#define LLD_DEV_MULTIPLIER 0x00010001
#define LLD_DB_READ_MASK   0xFFFFFFFF
#define LLD_DEV_READ_MASK  0x0000FFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000004
#ifdef S29WSxxxN
#define LLD_CFI_UNLOCK_ADDR1 0x00000555
#else
#define LLD_CFI_UNLOCK_ADDR1 0x00000055
#endif
typedef uint32_t FLASHDATA;
#define LLD_BUF_SIZE_MULTIPLIER 1

#elif defined LLD_CONFIGURATX8_AS_X8       // LLD_KEEP
#define LLD_DEV_MULTIPLIER 0x00000001
#define LLD_DB_READ_MASK   0x000000FF
#define LLD_DEV_READ_MASK  0x000000FF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000001
#ifdef S29WSxxxN
#define LLD_CFI_UNLOCK_ADDR1 0x00000555
#else
#define LLD_CFI_UNLOCK_ADDR1 0x00000055
#endif
typedef uint8_t FLASHDATA;
#define LLD_BUF_SIZE_MULTIPLIER 1

#elif defined LLD_CONFIGURATX8_AS_X16     // LLD_KEEP
#define LLD_DEV_MULTIPLIER 0x00000101
#define LLD_DB_READ_MASK   0x0000FFFF
#define LLD_DEV_READ_MASK  0x0000FFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000002
#ifdef S29WSxxxN
#define LLD_CFI_UNLOCK_ADDR1 0x00000555
#else
#define LLD_CFI_UNLOCK_ADDR1 0x00000055
#endif
typedef uint16_t FLASHDATA;
#define LLD_BUF_SIZE_MULTIPLIER 2

#elif defined LLD_CONFIGURATX8_AS_X32     // LLD_KEEP
#define LLD_DEV_MULTIPLIER 0x01010101
#define LLD_DB_READ_MASK   0xFFFFFFFF
#define LLD_DEV_READ_MASK  0xFFFFFFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000004
#ifdef S29WSxxxN
#define LLD_CFI_UNLOCK_ADDR1 0x00000555
#else
#define LLD_CFI_UNLOCK_ADDR1 0x00000055
#endif
typedef uint32_t FLASHDATA;
#define LLD_BUF_SIZE_MULTIPLIER 2
#endif     // LLD_KEEP              

/* public function prototypes */

/* Operation Functions */
#ifdef LLD_READ_OP
extern FLASHDATA lld_ReadOp
(
FLASHDATA * base_addr,      /* device base address is system */
uint32_t offset          /* address offset from base address */
);

extern void lld_PageReadOp
(
FLASHDATA * base_addr,    /* device base address is system */
uint32_t offset,        /* address offset from base address */
FLASHDATA * read_buf,  /* read data */
FLASHDATA cnt        /* read count */
);
#endif 

#ifdef LLD_WRITE_BUFFER_OP
extern DEVSTATUS lld_WriteBufferProgramOp
(
FLASHDATA * base_addr,      /* device base address is system */
uint32_t offset,         /* address offset from base address */
uint32_t word_cnt,       /* number of words to program */
FLASHDATA *data_buf       /* buffer containing data to program */
);
#endif

#if defined(LLD_WRITE_BUFFER_OP) && defined(S29GL064S)
extern DEVSTATUS lld_UnlockBypassBufferWriteProgramOp
(
FLASHDATA * base_addr,      /* device base address is system */
uint32_t offset,         /* address offset from base address */
uint32_t word_cnt,       /* number of words to program */
FLASHDATA *data_buf       /* buffer containing data to program */
);
#endif

#ifdef LLD_POLL_TOGGLE_AS_STATUS_API
extern FLASHDATA lld_StatusEmulateReg
(
FLASHDATA * base_addr   /* device base address in system */
);
#endif
#ifdef LLD_PROGRAM_OP
extern DEVSTATUS lld_ProgramOp
(
FLASHDATA * base_addr,      /* device base address is system */
uint32_t offset,         /* address offset from base address */
FLASHDATA write_data      /* variable containing data to program */
);
#endif

#if defined(LLD_PROGRAM_OP) && defined(S29GL064S)
extern DEVSTATUS lld_UnlockBypassProgramOp
(
FLASHDATA * base_addr,      /* device base address is system */
uint32_t offset,         /* address offset from base address */
FLASHDATA write_data    /* variable containing data to program */
);
#endif

#ifdef LLD_SECTOR_ERASE_OP
extern DEVSTATUS lld_SectorEraseOp
(
FLASHDATA * base_addr,      /* device base address is system */
uint32_t offset          /* address offset from base address */
);
#endif

#if defined(LLD_SECTOR_ERASE_OP) && defined(S29GL064S)
extern DEVSTATUS lld_UnlockBypassSectorEraseOp
(
FLASHDATA * base_addr,    /* device base address is system */
uint32_t offset        /* address offset from base address */
);
#endif

#ifdef LLD_CHIP_ERASE_OP
extern DEVSTATUS lld_ChipEraseOp
(
FLASHDATA * base_addr     /* device base address is system */
);
#endif

#if defined(LLD_CHIP_ERASE_OP) && defined(S29GL064S)
extern DEVSTATUS lld_UnlockBypassChipEraseOp
(
FLASHDATA * base_addr   /* device base address in system */
);
#endif 

extern void lld_GetVersion( uint8_t versionStr[]);

extern void lld_InitCmd
(
FLASHDATA * base_addr     /* device base address in system */
);

extern void lld_ResetCmd
(
FLASHDATA * base_addr     /* device base address in system */
);

#ifdef LLD_SECTOR_ERASE_CMD_1
extern void lld_SectorEraseCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);
#endif
#ifdef LLD_SECTOR_ERASE_CMD_2
extern void lld_SectorEraseCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);
#endif

#ifdef LLD_CHIP_ERASE_CMD_1 
extern void lld_ChipEraseCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
#endif
#ifdef LLD_CHIP_ERASE_CMD_2
extern void lld_ChipEraseCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
#endif

#ifdef LLD_PROGRAM_CMD_1
extern void lld_ProgramCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset,         /* address offset from base address */
FLASHDATA *pgm_data_ptr     /* variable containing data to program */
);
#endif
#ifdef LLD_PROGRAM_CMD_2
extern void lld_ProgramCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset,         /* address offset from base address */
FLASHDATA *pgm_data_ptr     /* variable containing data to program */
);
#endif

#ifdef LLD_SECSI_SECTOR_CMD_1
extern void lld_SecSiSectorExitCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
extern void lld_SecSiSectorEntryCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
#endif
#if defined(LLD_SECSI_SECTOR_CMD_2) || defined(LLD_SECSI_SECTOR_CMD_3)
extern void lld_SecSiSectorExitCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
extern void lld_SecSiSectorEntryCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);
#endif 

#ifdef LLD_WRITE_BUFFER_CMD_1
extern void lld_WriteToBufferCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);

extern void lld_ProgramBufferToFlashCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);
extern void lld_WriteBufferAbortResetCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
#endif /* LLD_WRITE_BUFFER_CMD */

#ifdef LLD_WRITE_BUFFER_CMD_2
extern void lld_WriteToBufferCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);

extern void lld_ProgramBufferToFlashCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);
#endif /* LLD_WRITE_BUFFER_CMD */


#ifdef LLD_CFI_CMD_1
FLASHDATA lld_ReadCfiWord
(
FLASHDATA * base_addr,      /* device base address is system */
uint32_t offset          /* address offset from base address */
);
extern void lld_CfiExitCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
extern void lld_CfiEntryCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
#endif
#ifdef LLD_CFI_CMD_2
FLASHDATA lld_ReadCfiWord
(
FLASHDATA * base_addr,      /* device base address is system */
uint32_t offset          /* address offset from base address */
);
extern void lld_CfiExitCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
extern void lld_CfiEntryCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t     offset        /* address offset from base address */
);
#endif

#ifdef LLD_STATUS_REG
#ifdef LLD_STATUS_GET
DEVSTATUS lld_StatusGet
(
FLASHDATA *  base_addr,      /* device base address in system */
uint32_t      offset          /* address offset from base address */
);
#endif
#ifdef LLD_SIMULTANEOUS_OP_NONE
void lld_StatusClear
(
FLASHDATA *  base_addr      /* device base address in system */
);
#else
void lld_StatusClear
(
FLASHDATA *  base_addr,      /* device base address in system */
uint32_t      offset          /* address offset from base address */
);
#endif

FLASHDATA lld_StatusGetReg
(
FLASHDATA *  base_addr,      /* device base address in system */
uint32_t      offset          /* address offset from base address */
);

#else // No LLD_STATUS_REG
#ifdef LLD_STATUS_GET

DEVSTATUS lld_StatusGet
(
FLASHDATA *  base_addr,      /* device base address in system */
uint32_t      offset          /* address offset from base address */
);

void lld_StatusClear
(
FLASHDATA *  base_addr,      /* device base address in system */
uint32_t      offset          /* address offset from base address */
);

FLASHDATA lld_StatusGetReg
(
FLASHDATA *  base_addr,      /* device base address in system */
uint32_t      offset          /* address offset from base address */
);
#endif /// LLD_STATUS_GET 
#endif // LLD_STATUS_REG

#ifdef LLD_MEMCPY_OP
DEVSTATUS lld_memcpy
(
FLASHDATA * base_addr,      /* device base address is system */
uint32_t offset,         /* address offset from base address */
uint32_t words_cnt,      /* number of words to program */
FLASHDATA *data_buf       /* buffer containing data to program */
);
#endif

#ifdef LLD_STATUS_REG
#ifdef LLD_SIMULTANEOUS_OP_NONE
extern void lld_StatusRegClearCmd
(
FLASHDATA * base_addr     /* device base address in system */
);

extern void lld_StatusRegReadCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
#else
extern void lld_StatusRegClearCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* sector address offset from base address */
);

extern void lld_StatusRegReadCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);
#endif
#endif // LLD_STATUS_REG

#ifdef LLD_UNLOCKBYPASS_CMD
extern void lld_UnlockBypassEntryCmd
(
FLASHDATA * base_addr     /* device base address in system */
);

extern void lld_UnlockBypassProgramCmd
(
FLASHDATA * base_addr,           /* device base address in system */
uint32_t offset,                  /* address offset from base address */
FLASHDATA *pgm_data_ptr          /* variable containing data to program */
);

extern void lld_UnlockBypassResetCmd
(
FLASHDATA * base_addr     /* device base address in system */
);

#if defined(S29GL064S)
extern void lld_UnlockBypassWriteToBufferCmd
(
FLASHDATA * base_addr,               /* device base address in system */
uint32_t offset,                       /* address offset from base address */
uint32_t word_count, /* number of words to program        */
FLASHDATA *data_buf   /* buffer containing data to program */
);

extern void lld_UnlockBypassProgramBufferToFlashCmd
(
FLASHDATA * base_addr,               /* device base address in system */
uint32_t offset                       /* address offset from base address */
);

extern void lld_UnlockBypassWriteToBufferAbortResetCmd
(
FLASHDATA * base_addr        /* device base address in system */
);

extern void lld_UnlockBypassSectorEraseCmd
(
FLASHDATA * base_addr,        /* device base address in system */
uint32_t offset                       /* address offset from base address */
);

extern void lld_UnlockChipEraseCmd
(
FLASHDATA * base_addr        /* device base address in system */
);
#endif /* defined(S29GL064S) */
#endif

#ifdef LLD_AUTOSELECT_CMD
extern void lld_AutoselectEntryCmd
(
FLASHDATA * base_addr     /* device base address in system */
);

extern void lld_AutoselectExitCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
#endif

#ifdef LLD_AUTOSELECT_CMD_2
extern void lld_AutoselectEntryCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);

extern void lld_AutoselectExitCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
#endif /* LLD_AUTOSELECT_CMD_2 */

#ifdef LLD_SUSP_RESUME_CMD_1
extern void lld_ProgramSuspendCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);

extern void lld_EraseSuspendCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);

extern void lld_EraseResumeCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);

extern void lld_ProgramResumeCmd
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);
#endif
#ifdef LLD_SUSP_RESUME_CMD_2
extern void lld_ProgramSuspendCmd
(
FLASHDATA * base_addr     /* device base address in system */
);

extern void lld_EraseSuspendCmd
(
FLASHDATA * base_addr     /* device base address in system */
);

extern void lld_EraseResumeCmd
(
FLASHDATA * base_addr     /* device base address in system */
);

extern void lld_ProgramResumeCmd
(
FLASHDATA * base_addr     /* device base address in system */
);
#endif 
#ifdef LLD_SUSP_RESUME_OP_2
extern DEVSTATUS lld_EraseSuspendOp
(
FLASHDATA * base_addr     /* device base address is system */
);
extern DEVSTATUS lld_ProgramSuspendOp
(
FLASHDATA * base_addr     /* Device base address is system */
);
#endif

#ifdef LLD_POLL_TOGGLE
extern DEVSTATUS lld_Poll
(
FLASHDATA * base_addr,          /* device base address in system */
uint32_t offset,         /* address offset from base address */
FLASHDATA *exp_data_ptr,    /* expect data */
FLASHDATA *act_data_ptr,    /* actual data */
POLLING_TYPE polling_type   /* type of polling to perform */
);
#endif
#ifdef LLD_POLL_STATUS
extern FLASHDATA lld_Poll
(
FLASHDATA * base_addr,      /* device base address in system */
uint32_t offset          /* address offset from base address */
);
#endif

#ifdef LLD_GET_ID_CMD_1
extern unsigned int lld_GetDeviceId
(
FLASHDATA * base_addr     /* device base address is system */
);
#endif
#ifdef LLD_GET_ID_CMD_2
extern unsigned int lld_GetDeviceId
(
FLASHDATA * base_addr,      /* device base address is system */
uint32_t offset          /* address offset from base address */
);
#endif
#ifdef LLD_GET_ID_CMD_3
extern unsigned int lld_GetDeviceId
(
FLASHDATA * base_addr,   /* device base address in system */
uint32_t offset
);
#endif
#ifdef LLD_CONFIG_REG_CMD_1
extern void lld_SetConfigRegCmd
(
  FLASHDATA *   base_addr,    /* device base address in system */
  FLASHDATA value       /* Configuration Register 0 value*/
);

extern FLASHDATA lld_ReadConfigRegCmd
(
  FLASHDATA *   base_addr   /* device base address in system */
);
#endif
#ifdef LLD_CONFIG_REG_CMD_2
extern void lld_SetConfigRegCmd
(
  FLASHDATA *   base_addr,    /* device base address in system */
  uint32_t offset,       /* address offset from base address */
  FLASHDATA value       
  
);
extern FLASHDATA lld_ReadConfigRegCmd
(
  FLASHDATA *   base_addr,    /* device base address in system */
  uint32_t offset        /* memory overlay offset */
);
#endif
#ifdef LLD_CONFIG_REG_CMD_3 /* WS_P device */
extern void lld_SetConfigRegCmd
(
  FLASHDATA *   base_addr,    /* device base address in system */
  FLASHDATA value,        /* Configuration Register 0 value*/
  FLASHDATA value1        /* Configuration Register 1 value*/
);

extern FLASHDATA lld_ReadConfigRegCmd
(
  FLASHDATA *   base_addr,    /* device base address in system */
  FLASHDATA offset        /* configuration reg. offset 0/1 */
);
#endif 

#if (defined LLD_BLANK_CHK_CMD) 
extern void lld_BlankCheckCmd
(
FLASHDATA * base_addr,    /* device base address in system */
uint32_t offset        /* address offset from base address */
);
#endif

#if (defined LLD_BLANK_CHK_CMD) 
extern DEVSTATUS lld_BlankCheckOp
(
FLASHDATA * base_addr,    /* device base address in system */
uint32_t offset        /* address offset from base address */
);
#endif 

  

#ifdef LLD_DELAY_MSEC
/* WARNING - Make sure the macro DELAY_1us (lld.c)           */
/* is defined appropriately for your system !!                     */

extern void DelayMilliseconds(int milliseconds);

extern void DelayMicroseconds(int microseconds);
#endif

#ifdef TRACE
extern void FlashWrite(FLASHDATA * addr, uint32_t offset, FLASHDATA data);
extern FLASHDATA FlashRead(FLASHDATA * addr, uint32_t offset);
#endif

#ifdef EXTEND_ADDR
extern void FlashWrite_Extend(FLASHDATA *base_addr, uint32_t offset, FLASHDATA data);
extern FLASHDATA FlashRead_Extend(FLASHDATA *base_addr, uint32_t offset);
#endif

#ifdef USER_SPECIFIC_CMD
extern void FlashWriteUserCmd(uint32_t address, FLASHDATA data);
extern FLASHDATA FlashReadUserCmd(uint32_t address);
#endif

#ifdef USER_SPECIFIC_CMD_3 //for NOR Page Read
extern void FlashWriteUserCmd(uint32_t address, FLASHDATA data);
extern FLASHDATA FlashReadUserCmd(uint32_t address);
extern void FlashPageReadUserCmd(FLASHDATA * base_address, uint32_t offset, FLASHDATA * buf, FLASHDATA cnt);
#endif

/**********************************************************
* Sector protection functions prototype.
**********************************************************/

#ifdef LLD_LOCKREG_CMD_1
extern void lld_LockRegEntryCmd     
( 
 FLASHDATA *   base_addr    /* device base address in system */ 
);
extern void lld_LockRegBitsProgramCmd 
( 
 FLASHDATA *   base_addr,   /* device base address in system */
 FLASHDATA value
);
extern FLASHDATA lld_LockRegBitsReadCmd 
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
extern void lld_LockRegExitCmd      
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
#endif

#ifdef LLD_LOCKREG_CMD_2
extern void lld_SSRLockRegEntryCmd 
( 
 FLASHDATA *   base_addr,   /* device base address in system */
 uint32_t offset 
);
extern void lld_SSRLockRegBitsProgramCmd 
( 
 FLASHDATA *   base_addr,   /* device base address in system */
 uint32_t offset, 
 FLASHDATA value 
);
extern FLASHDATA lld_SSRLockRegBitsReadCmd 
( 
 FLASHDATA *   base_addr,   /* device base address in system */
 uint32_t offset 
);
extern void lld_SSRLockRegExitCmd 
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
#endif

#ifdef LLD_PASSWORD_CMD
extern void lld_PasswordProtectionEntryCmd
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
extern void lld_PasswordProtectionProgramCmd
( 
 FLASHDATA * base_addr,     /* device base address in system */
 uint32_t offset,        /* address offset from base address */
 FLASHDATA pwd 
);
extern void lld_PasswordProtectionReadCmd
( 
 FLASHDATA *    base_addr,    /* device base address in system */
 FLASHDATA *pwd0,       /* Password 0 */
 FLASHDATA *pwd1,       /* Password 1 */
 FLASHDATA *pwd2,       /* Password 2 */
 FLASHDATA *pwd3        /* Password 3 */
);
extern void lld_PasswordProtectionUnlockCmd
( 
 FLASHDATA *  base_addr,    /* device base address in system */
 FLASHDATA pwd0,        /* Password 0 */
 FLASHDATA pwd1,        /* Password 1 */
 FLASHDATA pwd2,        /* Password 2 */
 FLASHDATA pwd3         /* Password 3 */
);
extern void lld_PasswordProtectionExitCmd
( 
 FLASHDATA *  base_addr     /* device base address in system */
);
#endif

#ifdef LLD_PPB_CMD
extern void lld_PpbEntryCmd       
( 
 FLASHDATA *   base_addr   /* device base address in system */
);

extern void lld_PpbProgramCmd     
( 
 FLASHDATA *   base_addr,   /* device base address in system */
 uint32_t   offset       /* address offset from base address */
);
extern void lld_PpbAllEraseCmd      
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
extern FLASHDATA lld_PpbStatusReadCmd 
( 
 FLASHDATA *   base_addr,   /* device base address in system */
 uint32_t   offset       /* address offset from base address */
);
extern void lld_PpbExitCmd        
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
#endif
#ifdef LLD_LOCKBIT_CMD
extern void lld_PpbLockBitEntryCmd    
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
extern void lld_PpbLockBitSetCmd    
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
extern FLASHDATA lld_PpbLockBitReadCmd  
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
extern void lld_PpbLockBitExitCmd   
(
 FLASHDATA *   base_addr    /* device base address in system */
);
#endif
#ifdef LLD_DYB_CMD
extern void lld_DybEntryCmd       
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
extern void lld_DybSetCmd       
( 
 FLASHDATA *   base_addr,   /* device base address in system */
 uint32_t   offset       /* address offset from base address */
);
extern void lld_DybClrCmd       
( 
 FLASHDATA *   base_addr,   /* device base address in system */
 uint32_t   offset       /* address offset from base address */
);
extern FLASHDATA lld_DybReadCmd     
( FLASHDATA *   base_addr,    /* device base address in system */
 uint32_t   offset       /* address offset from base address */
);
extern void lld_DybExitCmd        
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
#endif
#ifdef LLD_LOCK_BIT_RD_OP
extern FLASHDATA  lld_PpbLockBitReadOp  
( 
 FLASHDATA *  base_addr   /* device base address in system */
);
#endif
#ifdef LLD_PPB_ERASE_OP
extern int lld_PpbAllEraseOp      
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
#endif
#ifdef LLD_PPB_ERASE_OP
extern FLASHDATA lld_PpbStatusReadOp  
( 
 FLASHDATA *  base_addr,    /* device base address in system */
 uint32_t offset
);
#endif
#ifdef LLD_PPB_PGM_OP
extern int lld_PpbProgramOp       
( 
 FLASHDATA *  base_addr,    /* device base address in system */
 uint32_t offset         /* address offset from base address */
);
#endif
#ifdef LLD_LOCK_BIT_SET_OP
extern int lld_PpbLockBitSetOp      
( 
 FLASHDATA *   base_addr    /* device base address in system */
);
#endif

#ifdef LLD_LOCKREG_RD_OP_1
extern FLASHDATA lld_LockRegBitsReadOp  
( 
 FLASHDATA *  base_addr   /* device base address in system */
);
#endif
#ifdef LLD_LOCKREG_RD_OP_2
extern FLASHDATA lld_SSRLockRegBitsReadOp 
( 
 FLASHDATA *base_addr,      /* device base address in system */
 uint32_t offset         /* address offset from base address */
);
#endif 

#ifdef LLD_LOCKREG_WR_OP_1
extern int lld_LockRegBitsProgramOp   
( 
 FLASHDATA *  base_addr,    /* device base address in system */
 FLASHDATA value
);
#endif
#ifdef LLD_LOCKREG_WR_OP_2
extern int lld_SSRLockRegBitsProgramOp 
( 
 FLASHDATA *base_addr,      /* device base address in system */
 uint32_t offset,        /* address offset from base address */
 FLASHDATA value        /* New value */
);
#endif

#ifdef LLD_SECTOR_LOCK_CMD
extern void lld_SectorLockCmd     
( 
 FLASHDATA *  base_addr    /* device base address in system */
);
#endif
#ifdef LLD_SECTOR_UNLOCK_CMD
extern void lld_SectorUnlockCmd     
( FLASHDATA * base_addr,    /* device base address in system */
 uint32_t offset         /* address offset from base address */
);
#endif
#ifdef LLD_LOCK_RANGE_CMD
extern int  lld_SectorLockRangeCmd    
( 
 FLASHDATA *  base_addr,    /* device base address in system */
 uint32_t Startoffset,     /* Start sector offset */
 uint32_t Endoffset        /* End sector offset */
);
#endif

#ifdef LLD_ENTER_DEEP_POWER_DOWN
extern void lld_EnterDeepPowerDownCmd
(
 FLASHDATA *base_addr
);

extern void lld_ReleaseDeepPowerDownCmd
(
  FLASHDATA *base_addr,
  FLASHDATA mode
);
#endif

#ifdef LLD_MEASURE_TEMPERATURE
extern void lld_MeasureTemperatureCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_READ_TEMPERATURE_REG
extern FLASHDATA lld_ReadTemperatureRegCmd
(
 FLASHDATA *base_addr
);

extern DEVSTATUS lld_MeasureTemperatureRegOp 
(
 FLASHDATA *base_addr,
 FLASHDATA *temperature_reg
);
#endif

#ifdef LLD_PROGRAM_POR_TIMER_REG
extern void lld_ProgramPORTimerRegCmd
(
 FLASHDATA *base_addr,
 FLASHDATA portime      /* Power On Reset Time */
);
#endif

#ifdef LLD_READ_POR_TIMER_REG
extern FLASHDATA lld_ReadPORTimerRegCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_LOAD_INTERRUPT_CONFIG_REG
extern void lld_LoadInterruptConfigRegCmd
(
 FLASHDATA *base_addr,
 FLASHDATA icr        /* Interrupt Configuration Register */
);
#endif

#ifdef LLD_READ_INTERRUPT_CONFIG_REG
extern FLASHDATA lld_ReadInterruptConfigRegCmd
(
 FLASHDATA *base_addr
);
#endif 

#ifdef LLD_LOAD_INTERRUPT_STATUS_REG
extern void lld_LoadInterruptStatusRegCmd
(
 FLASHDATA *base_addr,
 FLASHDATA isr
);
#endif

#ifdef LLD_READ_INTERRUPT_STATUS_REG
extern FLASHDATA lld_ReadInterruptStatusRegCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_LOAD_VOLATILE_CONFIG_REG
extern void lld_LoadVolatileConfigRegCmd
(
 FLASHDATA *base_addr,
 FLASHDATA vcr
);
#endif

#ifdef LLD_READ_VOLATILE_CONFIG_REG
extern FLASHDATA lld_ReadVolatileConfigRegCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_PROGRAM_VOLATILE_CONFIG_REG_OP
DEVSTATUS lld_ProgramVolatileConfigRegOp
(
 FLASHDATA *base_addr,
 FLASHDATA vcr
);
#endif

#ifdef LLD_PROGRAM_NON_VOLATILE_CONFIG_REG
extern void lld_ProgramNonVolatileConfigRegCmd
(
 FLASHDATA *base_addr,
 FLASHDATA nvcr
);
#endif

#ifdef LLD_ERASE_NON_VOLATILE_CONFIG_REG
extern void lld_EraseNonVolatileConfigRegCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_READ_NON_VOLATILE_CONFIG_REG
extern FLASHDATA lld_ReadNonVolatileConfigRegCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_PROGRAM_NON_VOLATILE_CONFIG_REG_OP
DEVSTATUS lld_ProgramNonVolatileConfigRegOp
(
 FLASHDATA *base_addr,
 FLASHDATA nvcr
);
#endif

#ifdef LLD_EVALUATE_ERASE_STATUS
extern void lld_EvaluateEraseStatusCmd
(
 FLASHDATA *base_addr, 
 uint32_t   offset
);
#endif

#ifdef LLD_CRC_ENTER_CMD
extern void lld_CRCEnterCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_LOAD_CRC_START_ADDR_CMD
extern void lld_LoadCRCStartAddrCmd
(
 FLASHDATA *base_addr, 
 uint32_t   bl
);
#endif

#ifdef LLD_LOAD_CRC_END_ADDR_CMD
extern void lld_LoadCRCEndAddrCmd
(
 FLASHDATA *base_addr, 
 uint32_t   el
);
#endif

#ifdef LLD_CRC_SUSPENDCMD
extern void lld_CRCSuspendCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_CRC_ARRAY_READ_CMD
extern FLASHDATA lld_CRCArrayReadCmd
(
 FLASHDATA *base_addr, 
 uint32_t   offset
);
#endif

#ifdef LLD_CRC_RESUME_CMD
extern void lld_CRCResumeCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_READ_CHECK_VALUE_LOW_RESULT_REG_CMD
extern FLASHDATA lld_ReadCheckvalueLowResultRegCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_READ_CHECK_VALUE_HIGH_RESULT_REG_CMD
extern FLASHDATA lld_ReadCheckvalueHighResultRegCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_CRC_EXIT_CMD
extern void lld_CRCExitCmd
(
 FLASHDATA *base_addr
);
#endif

#ifdef LLD_PPB_SA_PROTECT_STATUS
extern FLASHDATA lld_PpbSAProtectStatusCmd
(
 FLASHDATA *base_addr,
 uint32_t   offset
);
#endif

#ifdef LLD_DYB_SA_PROTECT_STATUS
extern FLASHDATA lld_DybSAProtectStatusCmd
(
 FLASHDATA *base_addr,
 uint32_t   offset
);
#endif

#if defined(S29GL064S)
#ifdef LLD_CONTINUITY_CHECK_CMD
extern DEVSTATUS lld_ContinuityCheckOp
(
 FLASHDATA *base_addr
);
#endif
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_H_lldh  */


