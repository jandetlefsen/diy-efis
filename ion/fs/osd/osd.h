#ifndef __ion_osd_h__
#define __ion_osd_h__

#include "../../ion.h"


/*-----------------------------------------------------------------------------
 DECLARE FUNCTION PROTO-TYPE
-----------------------------------------------------------------------------*/

#if defined (__cplusplus )
extern "C" {
#endif

void os_zinit_osd( void );

/* Funtions related to Output. */
bool set_safe_mode( bool is_safe );

extern void os_break( void );
extern void os_assert( bool condition );
extern void os_exit( uint32_t exit_code );
extern result_t os_init_sm();
extern result_t os_terminate_sm();

#if defined (__cplusplus )
}
#endif
/* Define functions related to OS. */
#define lock()          0
#define unlock()        0

extern handle_t sm_fat;
extern handle_t sm_ofile;
extern handle_t sm_dpath;
extern handle_t sm_dcache;

#define fat_lock()      mutex_lock(sm_fat, wait_indefinite)
#define fat_unlock()    mutex_unlock(sm_fat)

#define ofile_lock()    mutex_lock(sm_ofile, wait_indefinite)
#define ofile_unlock()  mutex_unlock(sm_ofile)

#define path_lock()     mutex_lock(sm_dpath,wait_indefinite)
#define path_unlock()   mutex_unlock(sm_dpath)

#define cache_lock()     mutex_lock(sm_dcache, wait_indefinite)
#define cache_unlock()   mutex_unlock(sm_dcache)

#endif

/*-----------------------------------------------------------------------------
 END OF FILE
-----------------------------------------------------------------------------*/

