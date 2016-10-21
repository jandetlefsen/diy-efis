#include "../ion.h"
#include "../bsp.h"
#include "FreeRTOS/include/FreeRTOS.h"
//#include "FreeRTOS/include/portable.h"

static HeapRegion_t heap_regions[] =
  {
  { ( uint8_t * ) 0x80000000UL, 0x10000 },    // filled in from init
 	{ NULL, 0 }
  };

static const char *vol_a = "/a/";

result_t ion_init(const nv_pair_t *init_values, size_t num_values)
  {
  // assign the memory region we are passed
  for(size_t param = 0; param < num_values; param++)
    {
    if(strcmp(init_values[param].name, "memory-base")== 0)
      heap_regions[0].pucStartAddress = (uint8_t *)init_values[param].value;
    else if(strcmp(init_values[param].name, "memory-length")== 0)
      heap_regions[0].xSizeInBytes = (size_t)init_values[param].value;
    }
  
  // let FreeRTOS heap routines know what areas are available
  vPortDefineHeapRegions(heap_regions);
  
  // try to mount the volume
  if(mount(vol_a, 0, 0, 0) < 0)
    {
    if(get_errno() == e_mbr)
      {
      // this is a new volume so we need to format it
      int32_t sec_cnt = get_sectors(0);
      if(format(vol_a, 0, 0, 0, sec_cnt, "FAT16", 0)== 0)
        {
        // mount the new valume.
        mount(vol_a, 0, 0, 0);
        
        // the only file that needs to exist is the file efis.ini
        // which is the ini file that is used to load the application
        int32_t fd = creat("/a/efis.ini", 0);
        static const char *initial_ini = \
            "[freertos-hal]\n" \
            "screen-x=480\n" \
            "screen-y=640\n" \
            "rotation=90\n" \
            "[diy-efis]\n" \
            "node-id=100\n";
        write(fd, initial_ini, sizeof(initial_ini));
        close(fd);
        
        sync(vol_a);
        }
      }
    }
  
  }