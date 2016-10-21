#ifndef __gdi_h__
#define	__gdi_h__


extern result_t gdi_init();

#define NUM_DISPLAYS 3

typedef struct _canvas_t canvas_t;

/**
 * Get a pixel from the canvas
 * @param canvas    Canvas to reference
 * @param src       point on canvas
 * @return color at point
 */
typedef color_t (*get_pixel_fn)(canvas_t *canvas, const point_t *src);
/**
 * Set a pixel on the canvas
 * @param canvas  Canvas to reference
 * @param dest    point on canvas
 * @param color   color to set
 */
typedef void (*set_pixel_fn)(canvas_t *canvas, const point_t *dest, color_t color);
/**
 * Fast fill a region
 * @param canvas      Canvas to write to
 * @param dest        destination rectange to fill
 * @param words       Number of words to fill
 * @param fill_color  Fill color
 * @param rop         Operation
 *
 * @remarks This routine assumes a landscape fill.  (_display_mode == 0 | 180)
 * So if the display mode is 90 or 3 then things get performed vertically
 */
typedef void (*fast_fill_fn)(canvas_t *canvas, const rect_t *dest, color_t fill_color, raster_operation rop);

/**
 * Draw a line between 2 points
 * @param canvas      canvas to draw on
 * @param p1          first point
 * @param p2          second point
 * @param fill_color  color to fill with
 * @param rop         draw operation
 */
typedef void (*fast_line_fn)(canvas_t *canvas, const point_t *p1, const point_t *p2, color_t fill_color, raster_operation rop);
/**
 * Perform a fast copy from source to destination
 * @param dest        Destination in pixel buffer (0,0)
 * @param src_canvas  Canvas to copy from
 * @param src         area to copy
 * @param rop         copy mode
 * @remarks The implementation can assume that the src rectangle will be clipped to the
 * destination canvas metrics
 */
typedef void (*fast_copy_fn)(canvas_t *canvas, const point_t *dest, const canvas_t *src_canvas, const rect_t *src, raster_operation rop);
/**
 * Execute a raster operation on the two colors
 * @param src   color to merge
 * @param dst   color to merge into
 * @param rop   operation type
 * @return      merged color
 */
typedef color_t (*execute_rop_fn)(canvas_t *canvas, color_t src, color_t dst, raster_operation rop);

// shared structure for all canvas's
typedef struct _canvas_t
  {
  // sizeof the canvas.  This should include any variable data.
  // minumum is sizeof(canvas_t)
  size_t version;
  // dimensions of the canvas
  metrics_t metrics;
  
  // get pixel function
  get_pixel_fn get_pixel;
  set_pixel_fn set_pixel;
  fast_fill_fn fast_fill;
  fast_line_fn fast_line;
  fast_copy_fn fast_copy;
  execute_rop_fn execute_rop;
  } canvas_t;

#endif

