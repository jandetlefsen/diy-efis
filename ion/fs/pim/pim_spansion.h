#if !defined( PIM_SPANSH )
#define PIM_SPANSH
#include "pim.h"

#define SPAN_SECTOR_SIZE 512
#define SPAN_SECTOR_SIZE_BITS 9
#define SPAN_START_SECTOR 0

#define SPAN_DEVA   0    // device A is drive number 0
#if ( DEVICE_NUM > 1 ) 
#define SPAN_DEVB   1    // device B is drive number 1
#endif
//========= functions =========

int32_t pim_setup_span( pim_devinfo_t *de, char * name );
int32_t pim_ioctl_span( int32_t dev_id, pim_ioctl_cmd_t cmd, void *arg );
int32_t pim_readsector_span( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t cnt );
int32_t pim_writesector_span( int32_t dev_id, uint32_t sect_no, uint8_t *buf, uint32_t cnt );
int32_t pim_erasesector_span( int32_t dev_id, uint32_t sect_no, uint32_t cnt );

#endif

