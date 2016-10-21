#ifndef __bsp_h__
#define	__bsp_h__

#include "ion.h"
#include "gdi/gdi.h"

#ifdef __cplusplus
extern "C" {
#endif
  
/* These are the functions that must be provided by a port of ion to
 * a hardware platform.
 */

/**
 * Create a canvas given the extents given
 * @param size  Size of canvas to create
 * @param hndl  handle to new canvas
 * @return s_ok if created ok
 */  
extern result_t bsp_create_canvas_rect(const extent_t *size, handle_t *hndl);
/**
 * Close a canvas and return all resources
 * @param hndl  handle to canvas
 * @return s_ok if released ok
 */
extern result_t bsp_canvas_close(handle_t hndl);
  
#ifdef __cplusplus
  }
#endif


#endif	/* BSP_H */

