/* FILE: ion_ramif.c */
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


#include "ramif.h"
#include "pim_ram.h"
#include "../global/global.h"

#if defined ( IONFS_PL_WIN32 )
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <assert.h>
#endif

typedef struct ram_info_s {
   uint32_t ram_area_start,
            ram_area_size,
            bytes_per_sect,
            start_sect,
            sect_cnt,
            end_sect;

} ram_info_t;




static ram_info_t ram_info[IONFS_DEVICE_NUM];
static bool_t ram_inited;

#if defined( __arm ) && !defined ( WIN32 )
#pragma arm section zidata="NoInitData",rwdata="NoInitData"
#endif

uint8_t ram_disk[IONFS_DEVICE_NUM][IONFS_RAM_AREA_SIZE];

#if defined( __arm ) && !defined ( WIN32 )
#pragma arm section zidata,rwdata
#endif

#if defined( IONFS_DBG )
uint32_t *p_write_addr,
         write_sect,
         write_sect_num,
         *p_read_addr,
         read_sect,
         read_sect_num,
         *p_delete_addr,
         delete_sect,
         delete_sect_num;

uint32_t break_sector = 0xFFFFFFFF;
bool_t is_on_delete;
#endif

#if defined( IONFS_PL_WIN32 )
uint8_t *ramdisk_pl;
uint32_t *plr_stop4w;
HANDLE hMapFile;
#endif



/*
 Name: ram_on_delete
 Desc:
 Params:
   - On:
 Returns: None.
 Caveats: None.
*/

void ram_on_delete( bool_t On )
{
   #if defined( IONFS_DBG )
   is_on_delete = On;
   #endif
}




#if defined( IONFS_DBG )
#if defined( IONFS_PL_T32 )
#define ram_delay(us)                  {int32_t i; for ( i = 0; i < ((us)*1024); i++ );}
#else
#define ram_delay(us)
#endif

#if (SYS_HW != HW_NONE)
uint32_t SYSDisableInterrupt( void );
void SYSRestoreCPUState( uint32_t psr );
#else
#define SYSDisableInterrupt()             0
#define SYSRestoreCPUState(x)
#endif

bool_t plr_stop = false;
uint32_t pl_disable_irq( void )
{
#if defined( IONFS_PL_WIN32 )
   (*plr_stop4w) =
#endif
   plr_stop = true;
   return SYSDisableInterrupt();
}
void pl_restore_irq( uint32_t cpu )
{
   ram_delay(1);
#if defined( IONFS_PL_WIN32 )
   (*plr_stop4w) =
#endif
   plr_stop = false;
   SYSRestoreCPUState( cpu );
}
#else
#define pl_disable_irq()                  0
#define pl_restore_irq(cpu)
#endif




/*
 Name: ram_init
 Desc: RAM initialize.
 Params:
   - dev_id: Device's id.
 Returns:
   int32_t  0(=RAM_OK) always.
 Caveats: None.
*/

int32_t ram_init( int32_t dev_id )
{
   ram_info_t *p_ram = &ram_info[0];
   #if defined( IONFS_PL_WIN32 )
   _TCHAR szName[] = TEXT("ionFST_PL_RAMDISK");
   _TCHAR plint_name[] = TEXT("ionFST_PL_Interrupt");


   hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE,    // use paging file
                 NULL,                    // default security 
                 PAGE_READWRITE,          // read/write access
                 0,                       // max. object size 
                 IONFS_RAM_AREA_SIZE,                // buffer size  
                 szName);                 // name of mapping object
   //hMapFile = OpenFileMapping(
   //                FILE_MAP_ALL_ACCESS,   // read/write access
   //                FALSE,                 // do not inherit the name
   //                szName);               // name of mapping object

   if( hMapFile != NULL )
   {
      ramdisk_pl = (uint8_t *) MapViewOfFile(hMapFile, // handle to map object
                  FILE_MAP_ALL_ACCESS,  // read/write permission
                  0,
                  0,
                  IONFS_RAM_AREA_SIZE);
   }

   p_ram[0].ram_area_start = (uint32_t) ramdisk_pl;
   p_ram[0].ram_area_size = sizeof(uint8_t) * IONFS_RAM_AREA_SIZE;
   p_ram[0].bytes_per_sect = IONFS_RAM_SECTOR_SIZE;
   p_ram[0].start_sect = 0;
   p_ram[0].sect_cnt = (sizeof(uint8_t) * IONFS_RAM_AREA_SIZE) / IONFS_RAM_SECTOR_SIZE;
   p_ram[0].end_sect = p_ram[0].sect_cnt - 1;


   hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE,    // use paging file
                 NULL,                    // default security 
                 PAGE_READWRITE,          // read/write access
                 0,                       // max. object size 
                 IONFS_RAM_AREA_SIZE,                // buffer size  
                 szName);                 // name of mapping object
   //hMapFile = OpenFileMapping(
   //                FILE_MAP_ALL_ACCESS,   // read/write access
   //                FALSE,                 // do not inherit the name
   //                plint_name);           // name of mapping object

   if( hMapFile != NULL )
   {
      plr_stop4w = (int *) MapViewOfFile(hMapFile, // handle to map object
                  FILE_MAP_ALL_ACCESS,  // read/write permission
                  0,
                  0,
                  sizeof(int) );
   }
   #else
   uint32_t i;


   for( i = 0; i < IONFS_DEVICE_NUM ; i++ ) {
      p_ram[i].ram_area_start = (uint32_t) &ram_disk[i];
      p_ram[i].ram_area_size = sizeof(ram_disk[i]);
      p_ram[i].bytes_per_sect = IONFS_RAM_SECTOR_SIZE;
      p_ram[i].start_sect = 0;
      p_ram[i].sect_cnt = sizeof(ram_disk[i]) / IONFS_RAM_SECTOR_SIZE;
      p_ram[i].end_sect = p_ram[i].sect_cnt - 1;
   }
   #endif

   ram_inited = ram_inited;  // prevent for warning  (shin.s.c)

   ram_inited = true;

   return RAM_OK;
}




/*
 Name: ram_open
 Desc: RAM open.
 Params:
   - dev_id: Device's id.
   - p_sect_cnt:
 Returns:
   int32_t  0(=RAM_OK) always.
 Caveats: None.
*/

int32_t ram_open( int32_t dev_id, uint32_t *p_sect_cnt )
{
   ram_info_t *p_ram = &ram_info[dev_id];


   *p_sect_cnt = p_ram->sect_cnt;

   return RAM_OK;
}




/*
 Name: ram_format
 Desc:
 Params:
 Returns:
 Caveats:
*/

int32_t ram_format( int32_t dev_id )
{
   ram_info_t *p_ram = &ram_info[dev_id];


   ram_init(dev_id);

   ionFS_memset( (void *) p_ram->ram_area_start, 0, IONFS_RAM_SECTOR_SIZE * 1024 );

   return 0;
}




/*
 Name: ram_write
 Desc:
 Params:
   - dev_id: Device's id.
   - sect_no:
   - buf:
   - count:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t ram_write( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t count )
{
   ram_info_t *p_ram = &ram_info[dev_id];
   uint32_t write_addr,
          write_size;
   uint32_t i, cpu_state;


   write_addr = p_ram->ram_area_start + (p_ram->bytes_per_sect * sect_no);
   write_size = p_ram->bytes_per_sect * count;

   if ( (write_addr + write_size) > (p_ram->ram_area_start + p_ram->ram_area_size) )
      return RAM_ERROR;

   #if defined( IONFS_DBG )
   write_sect = sect_no;
   p_write_addr = (uint32_t *) write_addr;
   write_sect_num = count;
   #endif

   for ( i = 0; i < count; i++ ) {
      cpu_state = pl_disable_irq();
      ionFS_memcpy((void *) (write_addr + (p_ram->bytes_per_sect * i)),
                                    buf + (p_ram->bytes_per_sect * i), p_ram->bytes_per_sect );
      pl_restore_irq( cpu_state );
   }

   return RAM_OK;
}




/*
Name: ram_read
Desc:
Params:
   - dev_id: Device's id.
   - sect_no:
   - buf:
   - count:
Returns:
   int32_t  =0 on success.
            <0 on fail.
Caveats: None.
*/

int32_t ram_read( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t count )
{
   ram_info_t *p_ram = &ram_info[dev_id];
   uint32_t read_addr,
          read_size;


   read_addr = p_ram->ram_area_start + (p_ram->bytes_per_sect * sect_no);
   read_size = p_ram->bytes_per_sect * count;

   if ( (read_addr + read_size) > (p_ram->ram_area_start + p_ram->ram_area_size) )
      return RAM_ERROR;

   #if defined( IONFS_DBG )
   read_sect = sect_no;
   p_read_addr = (uint32_t *) read_addr;
   read_sect_num = count;
   #endif

   ionFS_memcpy( buf, (const void *) read_addr, read_size );

   return RAM_OK;
}




/*
 Name: ram_delete
 Desc:
 Params:
   - dev_id: Device's id.
   - sect_no: Sector number.
   - count:
 Returns:
   int32_t  =0 on success.
            <0 on fail.
 Caveats: None.
*/

int32_t ram_delete( int32_t dev_id, uint32_t sect_no, uint32_t count )
{
   ram_info_t *p_ram = &ram_info[dev_id];
   uint32_t delete_addr,
          delete_size;
   uint32_t i, cpu_state;


   delete_addr = p_ram->ram_area_start + (p_ram->bytes_per_sect * sect_no);
   delete_size = p_ram->bytes_per_sect * count;

   if ( (delete_addr + delete_size) > (p_ram->ram_area_start + p_ram->ram_area_size) )
      return RAM_ERROR;

   #if defined( IONFS_DBG )
   delete_sect = sect_no;
   p_delete_addr = (uint32_t *) delete_addr;
   delete_sect_num = count;

   if ( !is_on_delete )
      delete_size /= 10;
   #endif

   for ( i = 0; i < count; i++ ) {
      cpu_state = pl_disable_irq();
      ionFS_memset( (void *) (delete_addr + (p_ram->bytes_per_sect * i)), 0xFF,
                                 p_ram->bytes_per_sect );
      pl_restore_irq( cpu_state );
   }

   return RAM_OK;
}




/*
 Name: ram_close
 Desc: RAM close.
 Params: None.
 Returns:
   int32_t  0(=RAM_OK) always.
 Caveats: None.
*/

int32_t ram_close( int32_t dev_id )
{
   #if defined ( IONFS_PL_WIN32 )
   UnmapViewOfFile(ramdisk_pl);
   CloseHandle(hMapFile);
   #endif

   return RAM_OK;
}



/*
 Name: ram_get_sects_per_blk
 Desc: RAM close.
 Params: None.
 Returns:
   int32_t  0(=RAM_OK) always.
 Caveats: None.
*/

int32_t ram_get_sects_per_blk(void)
{
   return RAM_SECTS_PER_BLK;
}

/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/

