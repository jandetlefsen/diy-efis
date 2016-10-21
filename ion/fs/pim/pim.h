/* FILE: ion_pim.h */
/**************************************************************************
* Copyright (C)2009 Spansion LLC and its licensors. All Rights Reserved. 
*
* This software is owned by Spansion or its licensors and published by: 
* Spansion LLC, 915 DeGuigne Dr. Sunnyvale, CA  94088-3453 ("Spansion").
*
* BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND 
* BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
*
* This software constitutes source code for use in programming Spansion's Flash 
* memory components. This software is licensed by Spansion to be adapted only 
* for use in systems utilizing Spansion's Flash memories. Spansion is not be 
* responsible for misuse or illegal use of this software for devices not 
* supported herein.  Spansion is providing this source code "AS IS" and will 
* not be responsible for issues arising from incorrect user implementation 
* of the source code herein.  
*
* SPANSION MAKES NO WARRANTY, EXPRESS OR IMPLIED, ARISING BY LAW OR OTHERWISE, 
* REGARDING THE SOFTWARE, ITS PERFORMANCE OR SUITABILITY FOR YOUR INTENDED 
* USE, INCLUDING, WITHOUT LIMITATION, NO IMPLIED WARRANTY OF MERCHANTABILITY, 
* FITNESS FOR A  PARTICULAR PURPOSE OR USE, OR NONINFRINGEMENT.  SPANSION WILL 
* HAVE NO LIABILITY (WHETHER IN CONTRACT, WARRANTY, TORT, NEGLIGENCE OR 
* OTHERWISE) FOR ANY DAMAGES ARISING FROM USE OR INABILITY TO USE THE SOFTWARE, 
* INCLUDING, WITHOUT LIMITATION, ANY DIRECT, INDIRECT, INCIDENTAL, 
* SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS OF DATA, SAVINGS OR PROFITS, 
* EVEN IF SPANSION HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  
*
* This software may be replicated in part or whole for the licensed use, 
* with the restriction that this Copyright notice must be included with 
* this software, whether used in part or whole, at all times.  
*/


#if !defined( PIM_H_27112005 )
#define PIM_H_27112005

/*-----------------------------------------------------------------------------
 INCLUDE HEADER FILES
-----------------------------------------------------------------------------*/

#include "../global/global.h"




/*-----------------------------------------------------------------------------
 DEFINE DEFINITIONS
-----------------------------------------------------------------------------*/

/* The maximum size of setor. */
#define PIM_ALLOW_MAX_SECT_SIZE 512




/*-----------------------------------------------------------------------------
 DEFINE ENUMERATIONS
-----------------------------------------------------------------------------*/

/* Describe the command of I/O contorl. */
typedef enum pim_ioctl_cmd_e {
   eIOCTL_Unknown,

   eIOCTL_format,
   eIOCTL_init,
   eIOCTL_terminate,
   eIOCTL_open,
   eIOCTL_close,
   eIOCTL_attach,
   eIOCTL_eject,

   eIOCTL_end

} pim_ioctl_cmd_t;


/* Describe the feature of device. */
typedef enum pim_flag_e {
   ePIM_NeedErase    = (1<<0),  /* Devices which need Erase operation like NAND
                                   flash memory etc. */
   ePIM_Removable    = (1<<1),  /* Removable device like MMC/SD memory etc.. */
   ePIM_PartialRead  = (1<<2),  /* Devices that can be Partial-Read like NOR
                                   flash memory etc.*/
   ePIM_End          = (1<<31)

} pim_flag_t;




/*-----------------------------------------------------------------------------
 DEFINE DATA TYPE & STRUCTURES
-----------------------------------------------------------------------------*/

/* Define function type about each operation */
typedef int32_t (*pim_ioctl_t) ( int32_t, pim_ioctl_cmd_t, void *);
typedef int32_t (*pim_read_sector_t) ( int32_t, uint32_t, uint8_t *, uint32_t );
typedef int32_t (*pim_read_at_sector_t) ( int32_t, uint32_t, uint32_t, uint32_t, uint8_t * );
typedef int32_t (*pim_write_sector_t) ( int32_t, uint32_t, uint8_t *, uint32_t );
typedef int32_t (*pim_erase_sector_t) ( int32_t, uint32_t, uint32_t );


/* Function pointer to each operation of device. */
typedef struct pim_devop_s {
   pim_ioctl_t ioctl;                    /* function point to ioctl */
   pim_read_sector_t read_sector;        /* function point to read sector */
   pim_read_at_sector_t read_at_sector;  /* function point to read at sector */
   pim_write_sector_t write_sector;      /* function point to write sector */
   pim_erase_sector_t erase_sector;      /* function point to erase sector */

} pim_devop_t;


/* Describe information of device. */
typedef struct pim_devinfo_s {
   bool opened;                /* the open status of device */

   uint32_t totsect_cnt;         /* count of total sector */
   uint32_t start_sect;          /* start sector number */
   uint32_t end_sect;            /* end sector number */

   uint32_t bytes_per_sect;      /* number of bytes per sector */
   uint32_t bits_per_sectsize;   /* bit per sectorsize */
   uint32_t sects_per_block;     /*The number of sectors per block*/

   uint32_t dev_flag;            /* refer to pim_flag_t */

   pim_devop_t op;               /* function pointer to operation */

   char * dev_name;            /* name of flash type */

} pim_devinfo_t;




extern pim_devinfo_t pim_dev[DEVICE_NUM];

/* Access to the information of a specific device. */
#define GET_PIM_DEV(dev)   &pim_dev[dev]




/*-----------------------------------------------------------------------------
 DECLARE FUNTION PROTO-TYPE
-----------------------------------------------------------------------------*/

void pim_zinit_pim( void );
int32_t pim_init( void );
int32_t pim_open( int32_t dev_id );
int32_t pim_close( int32_t dev_id );
int32_t pim_terminate( void );
int32_t pim_read_sector( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t count );
int32_t pim_read_at_sector( int32_t dev_id, uint32_t sect_no, uint32_t offs, uint32_t len, uint8_t *buf );
int32_t pim_write_sector( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t count );
int32_t pim_erase_sector( int32_t dev_id, uint32_t sect_no, uint32_t count );
int32_t pim_get_sectors( int32_t dev_id );
int32_t pim_get_devicetype( int32_t dev_id, char * name );

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

