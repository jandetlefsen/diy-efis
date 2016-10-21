/**************************************************************************
* Special API for RPC2 device (KS-S/KL-S)
**************************************************************************/

#ifdef LLD_ENTER_DEEP_POWER_DOWN
/******************************************************************************
* 
* lld_EnterDeepPowerDownCmd - Enter Deep Power Down Command.
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_EnterDeepPowerDownCmd
(
  FLASHDATA *base_addr  /* device base address in system */
)
{
  /* Issue Enter Deep Power Down command */
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, 0, NOR_ENTER_DEEP_POWER_DOWN_CMD);
}

/******************************************************************************
* 
* lld_ReleaseDeepPowerDownCmd - Release Deep Power Down.
* Note: 
*     mode = 0: issue write (e.g. a "dummy" write) to release deep power down (default)
*     mode = 1: issue read (e.g. read array) to release deep power down 
* RETURNS: n/a
*
*/
extern void lld_ReleaseDeepPowerDownCmd
(
  FLASHDATA *base_addr,  /* device base address in system */
  FLASHDATA mode            /* mode for release deep power down */
)
{
  switch (mode)  
  {
    case 0: /* issue write (e.g. a "dummy" write) to release deep power down (default)*/
      FLASH_WR(base_addr, 0x00000000, 0x0000);
      DelayMicroseconds(LLD_DPD_DELAY);
      break;
    case 1: /* issue read (e.g. read array) to release deep power down */
      lld_ReadOp(base_addr, 0);
      DelayMicroseconds(LLD_DPD_DELAY);
      break;
    default:
      FLASH_WR(base_addr, 0x00000000, 0x0000);
      DelayMicroseconds(LLD_DPD_DELAY);
      break;
  }
}
#endif /* LLD_ENTER_DEEP_POWER_DOWN */

#ifdef LLD_MEASURE_TEMPERATURE
/******************************************************************************
* 
* lld_MeasureTemperatureCmd - Measure Temperature Command
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_MeasureTemperatureCmd
(
 FLASHDATA *base_addr
)
{
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_MEASURE_TEMPERATURE_CMD);
}
#endif /* LLD_MEASURE_TEMPERATURE */

#ifdef LLD_READ_TEMPERATURE_REG
/******************************************************************************
* 
* lld_ReadTemperatureRegCmd - Read Temperature Register Command
* Note: n/a
* RETURNS: FLASHDATA
*
*/
extern FLASHDATA lld_ReadTemperatureRegCmd
(
 FLASHDATA *base_addr
)
{
  FLASHDATA value;

  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_READ_TEMPERATURE_REG_CMD);  
  value = (FLASH_RD(base_addr, 0));

  return(value);
}  

/******************************************************************************
* 
* lld_MeasureTemperatureRegOp - Read Temperature Register Operation
* Note: n/a
* RETURNS of API: 
*    DEVSTATUS   ---- Measure Temperature Status (failed or completed)
* RETURNS Parameter:
*    *temperature_reg   ----Pointer of Temperature Register Value
*
*/
extern DEVSTATUS lld_MeasureTemperatureRegOp 
(
 FLASHDATA *base_addr,
 FLASHDATA *temperature_reg
)
{
  DEVSTATUS status;
  FLASHDATA status_reg;
 
  //Issue Measure Temperature Command
  lld_MeasureTemperatureCmd(base_addr);
  //Polling Status
  status_reg = lld_Poll(base_addr, 0);
  if( (status_reg & DEV_RDY_MASK) != DEV_RDY_MASK )
    status = DEV_BUSY;		/* measure failed */
  else
  {
    status = DEV_NOT_BUSY;			    /* measure complete */
    //Read Temperature Reg
    *temperature_reg = lld_ReadTemperatureRegCmd(base_addr);
  }
  
  return(status);
}  
#endif /* LLD_READ_TEMPERATURE_REG */

#ifdef LLD_PROGRAM_POR_TIMER_REG
/******************************************************************************
* 
* lld_ProgramPORTimerRegCmd - Program Power On Reset Timer Register Command
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_ProgramPORTimerRegCmd
(
 FLASHDATA *base_addr,
 FLASHDATA portime 			/* Power On Reset Time */
)
{
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_PROGRAM_POR_TIMER_CMD);
  FLASH_WR(base_addr, 0, portime);
}
#endif /* LLD_PROGRAM_POR_TIMER_REG */

#ifdef LLD_READ_POR_TIMER_REG
/******************************************************************************
* 
* lld_ReadPORTimerRegCmd - Read Power On Reset Timer Register Command
* Note: n/a
* RETURNS: FLASHDATA
*
*/
extern FLASHDATA lld_ReadPORTimerRegCmd
(
 FLASHDATA *base_addr
)
{
  FLASHDATA value;
  
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_READ_POR_TIMER_CMD);
  value = (FLASH_RD(base_addr, 0));

  return(value);
}
#endif /* LLD_READ_POR_TIMER_REG */

#ifdef LLD_LOAD_INTERRUPT_CONFIG_REG
/******************************************************************************
* 
* lld_LoadInterruptConfigRegCmd - Load Interrupt Configuration Register Command
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_LoadInterruptConfigRegCmd
(
 FLASHDATA *base_addr,
 FLASHDATA icr				/* Interrupt Configuration Register */
)
{
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_LOAD_INTERRUPT_CFG_REG_CMD);
  FLASH_WR(base_addr, 0, icr);
}
#endif /* LLD_LOAD_INTERRUPT_CONFIG_REG */

#ifdef LLD_READ_INTERRUPT_CONFIG_REG
/******************************************************************************
* 
* lld_ReadInterruptConfigRegCmd - Read Interrupt Configuration Register Command
* Note: n/a
* RETURNS: FLASHDATA
*
*/
extern FLASHDATA lld_ReadInterruptConfigRegCmd
(
 FLASHDATA *base_addr
)
{
  FLASHDATA value;
  
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_READ_INTERRUPT_CFG_REG_CMD);
  value = (FLASH_RD(base_addr, 0));

  return(value);
}
#endif /* LLD_READ_INTERRUPT_CONFIG_REG */

#ifdef LLD_LOAD_INTERRUPT_STATUS_REG
/******************************************************************************
* 
* lld_LoadInterruptStatusRegCmd - Load Interrupt Status Register Command
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_LoadInterruptStatusRegCmd
(
 FLASHDATA *base_addr,
 FLASHDATA isr      /* Interrupt Status Register */
)
{
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_LOAD_INTERRUPT_STATUS_REG_CMD);
  FLASH_WR(base_addr, 0, isr);
}
#endif /* LLD_LOAD_INTERRUPT_STATUS_REG */

#ifdef LLD_READ_INTERRUPT_STATUS_REG
/******************************************************************************
* 
* lld_ReadInterruptStatusRegCmd - Read Interrupt Status Register Command
* Note: n/a
* RETURNS: FLASHDATA
*
*/
extern FLASHDATA lld_ReadInterruptStatusRegCmd
(
 FLASHDATA *base_addr
)
{
  FLASHDATA value;
  
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_READ_INTERRUPT_STATUS_REG_CMD);
  value = (FLASH_RD(base_addr, 0));

  return(value);
}
#endif /* LLD_READ_INTERRUPT_STATUS_REG */

#ifdef LLD_LOAD_VOLATILE_CONFIG_REG
/******************************************************************************
* 
* lld_LoadVolatileConfigRegCmd - Load Volatile Configuration Register Command
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_LoadVolatileConfigRegCmd
(
 FLASHDATA *base_addr,
 FLASHDATA vcr
)
{
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_LOAD_VOLATILE_CFG_REG_CMD);
  FLASH_WR(base_addr, 0, vcr);
}
#endif /* LLD_LOAD_VOLATILE_CONFIG_REG */

#ifdef LLD_READ_VOLATILE_CONFIG_REG
/******************************************************************************
* 
* lld_ReadInterruptStatusRegCmd - Read Volatile Configuration Register Command
* Note: n/a
* RETURNS: FLASHDATA
*         
*/
extern FLASHDATA lld_ReadVolatileConfigRegCmd
(
 FLASHDATA *base_addr
)
{
  FLASHDATA value;
  
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_READ_VOLATILE_CFG_REG_CMD);
  value = (FLASH_RD(base_addr, 0));

  return(value);
}
#endif /* LLD_READ_VOLATILE_CONFIG_REG */

#ifdef LLD_PROGRAM_VOLATILE_CONFIG_REG_OP
/******************************************************************************
* 
* lld_ProgramVolatileConfigRegOp - Program Volatile Configuration Register Operation (with poll)
* Note: n/a
* RETURNS: DEVSTATUS
*         
*/
DEVSTATUS lld_ProgramVolatileConfigRegOp
(
 FLASHDATA *base_addr, 
 FLASHDATA vcr
)
{
  FLASHDATA status_reg;
  
  /* Load VCR */
  lld_LoadVolatileConfigRegCmd(base_addr, vcr);

  /* Poll for Program completion */
  status_reg = lld_Poll(base_addr, 0);

  if( (status_reg & DEV_PROGRAM_MASK) == DEV_PROGRAM_MASK )
	  return( DEV_PROGRAM_ERROR );		/* program error */

  return( DEV_NOT_BUSY );			    /* program complete */
}
#endif /* LLD_PROGRAM_VOLATILE_CONFIG_REG_OP */

#ifdef LLD_PROGRAM_NON_VOLATILE_CONFIG_REG
/******************************************************************************
* 
* lld_ProgramNonVolatileConfigRegCmd - Program Non-Volatile Configuration Register Command
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_ProgramNonVolatileConfigRegCmd
(
 FLASHDATA *base_addr, 
 FLASHDATA nvcr
)
{
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_PROGRAM_NON_VOLATILE_CFG_REG_CMD);
  FLASH_WR(base_addr, 0, nvcr);
}
#endif /* LLD_PROGRAM_NON_VOLATILE_CONFIG_REG */

#ifdef LLD_ERASE_NON_VOLATILE_CONFIG_REG
/******************************************************************************
* 
* lld_EraseNonVolatileConfigRegCmd - Erase Non-Volatile Configuration Register Command
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_EraseNonVolatileConfigRegCmd
(
 FLASHDATA *base_addr
)
{
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_ERASE_NON_VOLATILE_CFG_REG_CMD);
}
#endif /* LLD_ERASE_NON_VOLATILE_CONFIG_REG */

#ifdef LLD_READ_NON_VOLATILE_CONFIG_REG
/******************************************************************************
* 
* lld_ReadNonVolatileConfigRegCmd - Read Non-Volatile Configuration Register Command
* Note: n/a
* RETURNS: FLASHDATA
*
*/
extern FLASHDATA lld_ReadNonVolatileConfigRegCmd
(
 FLASHDATA *base_addr
)
{
  FLASHDATA value;
  
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_READ_NON_VOLATILE_CFG_REG_CMD);
  value = (FLASH_RD(base_addr, 0));

  return(value);
}
#endif /* LLD_READ_NON_VOLATILE_CONFIG_REG */

#ifdef LLD_PROGRAM_NON_VOLATILE_CONFIG_REG_OP
/******************************************************************************
* 
* lld_ProgramNonVolatileConfigRegOp - Program Non-Volatile Configuration Register Operation (with poll)
* Note: n/a
* RETURNS: DEVSTATUS
*         
*/
DEVSTATUS lld_ProgramNonVolatileConfigRegOp
(
 FLASHDATA *base_addr,
 FLASHDATA nvcr
)
{
  FLASHDATA status_reg;

  /* Erase NVCR */
  lld_EraseNonVolatileConfigRegCmd(base_addr);

  /* Poll for erase completion */
  status_reg = lld_Poll(base_addr, 0);
  if( (status_reg & DEV_ERASE_MASK) == DEV_ERASE_MASK )
    return( DEV_ERASE_ERROR );		/* erase  error */

  /* Program NVCR */
  lld_ProgramNonVolatileConfigRegCmd(base_addr, nvcr);

  /* Poll for program completion */
  status_reg = lld_Poll(base_addr, 0);
  if( (status_reg & DEV_PROGRAM_MASK) == DEV_PROGRAM_MASK )
    return( DEV_PROGRAM_ERROR );		/* program error */

  return( DEV_NOT_BUSY );			    /* program complete */
}
#endif /* LLD_PROGRAM_NON_VOLATILE_CONFIG_REG_OP */

#ifdef LLD_EVALUATE_ERASE_STATUS
/******************************************************************************
* 
* lld_EvaluateEraseStatusCmd - Evaluate Erase Status Command
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_EvaluateEraseStatusCmd
(
 FLASHDATA *base_addr, 
 ADDRESS   offset
)
{
  /* Write Evaluate Erase Status Command to Offset */
  FLASH_WR(base_addr, (offset & SA_OFFSET_MASK) + LLD_UNLOCK_ADDR1, NOR_EVALUATE_ERASE_STATUS_CMD);
}
#endif /* LLD_EVALUATE_ERASE_STATUS */

#ifdef LLD_CRC_ENTER_CMD
/******************************************************************************
* 
* lld_CRCEnterCmd - CRC Enter Command
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_CRCEnterCmd
(
 FLASHDATA *base_addr
)
{
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_UNLOCK_DATA1);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR2, NOR_UNLOCK_DATA2);
  FLASH_WR(base_addr, LLD_UNLOCK_ADDR1, NOR_CRC_ENTRY_CMD);
}
#endif /* LLD_CRC_ENTER_CMD */

#ifdef LLD_LOAD_CRC_START_ADDR_CMD
/******************************************************************************
* 
* lld_LoadCRCStartAddrCmd - Load CRC Start Address
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_LoadCRCStartAddrCmd
(
 FLASHDATA *base_addr, 
 ADDRESS   bl             /* Beginning location of checkvalue calculation */
)
{
  FLASH_WR(base_addr, bl, NOR_LOAD_CRC_START_ADDR_CMD);
}
#endif /* LLD_LOAD_CRC_START_ADDR_CMD */

#ifdef LLD_LOAD_CRC_END_ADDR_CMD
/******************************************************************************
* 
* lld_LoadCRCEndAddrCmd - Load CRC End Address (start calculation)
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_LoadCRCEndAddrCmd
(
 FLASHDATA *base_addr, 
 ADDRESS   el             /* Ending location of checkvalue calculation */
)
{
  FLASH_WR(base_addr, el, NOR_LOAD_CRC_END_ADDR_CMD);
}
#endif /* LLD_LOAD_CRC_END_ADDR_CMD */

#ifdef LLD_CRC_SUSPENDCMD
/******************************************************************************
* 
* lld_CRCSuspendCmd - CRC Suspend
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_CRCSuspendCmd
(
 FLASHDATA *base_addr
)
{
  FLASH_WR(base_addr, 0, NOR_CRC_SUSPEND_CMD);
}
#endif /* LLD_CRC_SUSPENDCMD */

#ifdef LLD_CRC_ARRAY_READ_CMD
/******************************************************************************
* 
* lld_CRCArrayReadCmd - Array Read (during suspend)
* Note: n/a
* RETURNS: FLASHDATA
*
*/
extern FLASHDATA lld_CRCArrayReadCmd
(
 FLASHDATA *base_addr, 
 ADDRESS   offset           /* Array Read Address */
)
{
  FLASHDATA value;
  value = (FLASH_RD(base_addr, offset));
  return(value);  
}
#endif /* LLD_CRC_ARRAY_READ_CMD */

#ifdef LLD_CRC_RESUME_CMD
/******************************************************************************
* 
* lld_CRCResumeCmd - CRC Resume
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_CRCResumeCmd
(
 FLASHDATA *base_addr
)
{
  FLASH_WR(base_addr, 0, NOR_CRC_RESUME_CMD);
}
#endif /* LLD_CRC_RESUME_CMD */

#ifdef LLD_READ_CHECK_VALUE_LOW_RESULT_REG_CMD
/******************************************************************************
* 
* lld_ReadCheckvalueLowResultRegCmd - Read Checkvalue Low Result Register
* Note: n/a
* RETURNS: FLASHDATA
*
*/
extern FLASHDATA lld_ReadCheckvalueLowResultRegCmd
(
 FLASHDATA *base_addr
)
{
  FLASHDATA value;
  FLASH_WR(base_addr, 0, NOR_CRC_READ_CHECKVALUE_RESLUT_REG_CMD);
  value = (FLASH_RD(base_addr, 0x00));
  return(value);  
}
#endif /* LLD_READ_CHECK_VALUE_LOW_RESULT_REG_CMD */

#ifdef LLD_READ_CHECK_VALUE_HIGH_RESULT_REG_CMD
/******************************************************************************
* 
* lld_ReadCheckvalueHighResultRegCmd - Read Checkvalue High Result Register
* Note: n/a
* RETURNS: FLASHDATA
*
*/
extern FLASHDATA lld_ReadCheckvalueHighResultRegCmd
(
 FLASHDATA *base_addr
)
{
  FLASHDATA value;
  FLASH_WR(base_addr, 0, NOR_CRC_READ_CHECKVALUE_RESLUT_REG_CMD);
  value = (FLASH_RD(base_addr, 0x01));
  return(value);  
}
#endif /* LLD_READ_CHECK_VALUE_HIGH_RESULT_REG_CMD */

#ifdef LLD_CRC_EXIT_CMD
/******************************************************************************
* 
* lld_CRCExitCmd - CRC Exit Command
* Note: n/a
* RETURNS: n/a
*
*/
extern void lld_CRCExitCmd
(
 FLASHDATA *base_addr
)
{
  FLASH_WR(base_addr, 0, NOR_CRC_EXIT_CMD);
}
#endif /* LLD_CRC_EXIT_CMD */

#ifdef LLD_PPB_SA_PROTECT_STATUS
/******************************************************************************
* 
* lld_PpbSAProtectStatusCmd
* Note: 
* RETURNS: FLASHDATA
*
*/
extern FLASHDATA lld_PpbSAProtectStatusCmd
(
 FLASHDATA *base_addr,   /* device base address in system */
 ADDRESS   offset        /* Sector Address for status read */
)
{
  FLASHDATA value;
  
  FLASH_WR(base_addr, 0, NOR_SA_PROTECT_STATUS_CMD);
  value = (FLASH_RD(base_addr, offset));
  return(value);
}
#endif

#ifdef LLD_DYB_SA_PROTECT_STATUS
/******************************************************************************
* 
* lld_DybSAProtectStatusCmd
* Note: 
* RETURNS: FLASHDATA
*
*/
extern FLASHDATA lld_DybSAProtectStatusCmd
(
 FLASHDATA *base_addr,   /* device base address in system */
 ADDRESS   offset        /* Sector Address for status read */
)
{
  FLASHDATA value;
  
  FLASH_WR(base_addr, 0, NOR_SA_PROTECT_STATUS_CMD);
  value = (FLASH_RD(base_addr, offset));
  return(value);
}
#endif
