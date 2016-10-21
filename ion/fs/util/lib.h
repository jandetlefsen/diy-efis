#ifndef __lib_h__
#define __lib_h__

#include "../osd/osd.h"
#include "../../ion.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined (__cplusplus )
extern "C" {
#endif
#if defined( __arm ) && (__ARMCC_VERSION > 210000)  && !(defined(__TARGET_CPU_ARM920T) || defined(__TARGET_CPU_ARM7TDMI))
#define break()                     __breakpoint(0xFE)
#elif defined( WIN32 )
#define break()                     __debugbreak()
#else
#define break()                     os_break()
#endif
#define exit(code)                  os_exit(code)


/* Type-conversion Macro */
#define ARR8_2_UINT16(pU8) ((uint16_t)((pU8)[0] | ((pU8)[1]<<8)))
#define ARR8_2_UINT32(pU8) ((uint32_t)((pU8)[0] | ((pU8)[1]<<8) | \
                            ((pU8)[2]<<16) | ((pU8)[3]<<24)))
#define UINT16_2_ARR8(pU8, U16) do {\
                            ((uint8_t*)(pU8))[0]=(uint8_t)(U16);\
                            ((uint8_t*)(pU8))[1]=(uint8_t)((uint32_t)(U16)>>8);\
                            } while ( 0 )
#define UINT32_2_ARR8(pU8, U32) do {\
                            ((uint8_t*)(pU8))[0]=(uint8_t)(U32);\
                            ((uint8_t*)(pU8))[1]=(uint8_t)((uint32_t)(U32)>>8);\
                            ((uint8_t*)(pU8))[2]=(uint8_t)((uint32_t)(U32)>>16);\
                            ((uint8_t*)(pU8))[3]=(uint8_t)((uint32_t)(U32)>>24);\
                            } while ( 0 )

#if defined (__cplusplus )
}
#endif

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

