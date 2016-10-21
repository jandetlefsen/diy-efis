#ifndef __ionfs_h__
#define __ionfs_h__

#include <stdint.h>
#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <stdlib.h>
#include "../can-aerospace/can_msg.h"

#include "types.h"

#if defined (__cplusplus )
extern "C"
  {
#endif

#include "fs/custionfs.h"

typedef uint16_t status_t;
typedef uint32_t mod_t;
typedef int32_t offs_t;

#define UNUSED(x) (void)(x)
#define ionfs_local

#ifdef FILENAME_MAX
#undef FILENAME_MAX
#endif

#define FILENAME_MAX 255

typedef enum
  {
  FAT16 = 16,
  FAT32 = 32
  } fstype_t;

typedef struct dirent_s
  {
  /* Name of the directory entry */
  char d_name[FILENAME_MAX + 1/*null char*/];
  } dirent_t;

typedef struct DIR_s
  {
  int32_t fd; /* The file descriptor */
  uint32_t attr, /* The attribute of directory */
  ctime, /* Time of creation of the directory */
  atime, /* Time of last access of the directory */
  wtime, /* Time of last writing of the directory */
  filesize; /* Size of the directory in bytes */
#if defined( SFILEMODE )
  uint32_t filemode;
#endif
  dirent_t dirent;

    } dir_t;

#ifndef STAT_DEFINED

typedef struct stat_s
  {
  uint16_t dev, /* Drive number of the disk containing the file (same as rdev) */
  i_no, /* i-node number (not used on DOS) */
  mode, /* Bit mask for file-mode information */
  nlink; /* Always 1 on non-NTFS file systems */
  int32_t uid, /* Numeric identifier of user who owns file (UNIX-specific) */
  gid; /* Numeric identifier of group that owns file (UNIX-specific) */
  uint16_t rdev; /* Drive number of the disk containing the file (same as dev) */
  uint32_t size, /* Size of the file in bytes */
  atime, /* Time of last access of file */
  mtime, /* Time of last modification of file */
  ctime; /* Time of creation of file */
#if defined( SFILEMODE )
  uint32_t filemode;
#endif

    } stat_t;
#endif

typedef struct statdir_s
  {
  uint32_t files, /* count of total files in directory */
  size, /* size of total files in directory */
  alloc_size, /* allocated size of total files in directory */
  atime, /* Time of last access of file */
  mtime, /* Time of last modification of file */
  ctime; /* Time of creation of file */
  } statdir_t;

typedef struct statfs_s
  {
  int32_t blk_size, /* fundamental file system block size */
  io_size, /* optimal transfer block size */
  blocks, /* total data blocks in file system */
  free_blks, /* free blocks in fs */
  fs_id, /* id of file system */
  type; /* type of filesystem */

    } statfs_t;

typedef struct partinfo_s
  {
  int32_t start_sect, /* start sector */
  end_sect, /* end sector */
  sectors, /* count of total sectors */
  bytes_per_sect; /* byte per sector */

    } partinfo_t; /* partition info */

typedef struct ioarg_s
  {
  uint32_t sect_no,
  sect_cnt;
  uint8_t *buf;

    } iorw_t;

typedef struct mcb_s
  {
  uint32_t words[32];

    } mcb_t;

typedef struct fsver_s
  {
  uint8_t name[32];
  uint32_t major,
  minor,
  stable;
    } fsver_t;

/* ionfs_format()'s flag */
#define FORMAT_FAST (1<<0)
#define FORMAT_SCAN (1<<1)


/* Non-ANSI names for POSIX */
#define O_RDONLY  0x0000   /* open for reading only */
#define O_WRONLY  0x0001   /* open for writing only */
#define O_RDWR    0x0002   /* open for reading and writing */
#define O_APPEND  0x0008   /* writes done at eof */
#define O_CREAT   0x0100   /* create and open file */
#define O_TRUNC   0x0200   /* open and truncate */
#define O_EXCL    0x0400   /* open only if file doesn't already exist */

/* permissions */
#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100
#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010
#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

/* File position */
#undef SEEK_SET
#undef SEEK_CUR
#undef SEEK_END
#define SEEK_SET  0        /* Indicates the beginning of the file. */
#define SEEK_CUR  1        /* Indicates the current of the file. */
#define SEEK_END  2        /* Indicates the end position. */

/* File permittion */
#define R_OK  4            /* for read */
#define W_OK  2            /* for write */
#define X_OK  1            /* for execute */
#define F_OK  0            /* for existence */

/* File attribute */
#define FA_RDONLY    0x01  /* a file for reading only */
#define FA_HIDDEN    0x02  /* a hidden file */
#define FA_SYSTEM    0x04  /* a system file */
#define FA_VOLUME    0x08  /* a volume */
#define FA_DIRECTORY 0x10  /* a directory */
#define FA_ARCHIVE   0x20  /* a archive file */

/* Ioctl function */
#define IO_MS_ATTACH    (1<<0)
#define IO_MS_DETACH    (1<<1)
#define IO_DEV_ATTACH   (1<<2)
#define IO_DEV_DETACH   (1<<3)
#define IO_PART_INFO    (1<<4)  /* partition info */
#define IO_OP           (1<<5)
#define IO_READ         (IO_OP | 1<<6)
#define IO_WRITE        (IO_OP | 1<<7)
#define IO_ERASE        (IO_OP | 1<<8)
    
/** used to initialize the ion operating system
 */
typedef struct _nv_pair_t {
  const char *name;
  uint32_t value;
  } nv_pair_t;

 #ifndef _TM_DEFINED
  #define _TM_DEFINED
  
struct tm {
  int tm_sec;   // seconds [0,61]
  int tm_min;   // minutes [0,59]
  int tm_hour;  // hour [0,23]
  int tm_mday;  // day of month [1,31]
  int tm_mon;   // month of year [0,11]
  int tm_year;  // years since 1900
  int tm_wday;  // day of week [0,6] (Sunday = 0)
  int tm_yday;  //  day of year [0,365]
  int tm_isdst; // always 0 in iondaylight savings flag
  };
#endif

 typedef struct tm tm_t;
 
#define __time_t_defined
typedef uint32_t time_t;

/*--------------------------------MISC FUNCTIONS------------------------------*/
  /**
   * Initialize the ION file system.
   * @param init_values   Values to set kernel running
   * @param num_values    Number of initialize values
   * @return s_ok if successfull, otherwise error
   */
extern result_t ion_init(const nv_pair_t *init_values, size_t num_values);
/**
 * Set the global error number (not threadsafe)
 * @param err_no  Error number
 * @return        previous error
 */
extern result_t set_errno(result_t err_no);
/**
 * Return the previous error
 * @return Error number
 */
extern result_t get_errno(void);
/**
 * Allocate an area of memory
 * @param size  bytes to allocate
 * @return pointer to memory, otherwise 0
 */
extern void *malloc(size_t size);
/**
 * Free a block of memory
 * @param blk Memory to free
 */
extern void free(void *blk);

typedef uint8_t thread_priority_t;
typedef void (*thread_func)(void *);
typedef uint32_t ticks_t;
#define wait_indefinite 0xFFFFFFFF

typedef enum {
  thread_ready,
  thread_running,
  thread_blocked,
  thread_suspended,
  thread_deleted
  } thread_state_t;

/**
 * Get the current calendar time as a value of type time_t
 * @param timer Pointer to an object of type time_t, where the time value is
 *  stored. Alternatively, this parameter can be a null pointer, in which case
 *  the parameter is not used (the function still returns a value of type
 *  time_t with the result).
 * @return The current calendar time as a time_t object. If the argument is not
 *  a null pointer, the return value is the same as the one stored in the
 *  location pointed by argument timer.If the function could not retrieve the
 *  calendar time, it returns a value of -1.
 * @remarks the time is set by the can_aerospace attributes for gmt date
 * and time.  If the GPS has not received a lock, then -1 is returned.
 * This can be used to determine initial GPS lock.  Once an initial GPS lock is
 * made then the time is updated with the system tick.
 */  
extern time_t time(time_t *timer);
/**
 * Uses the value pointed by timer to fill a tm structure with the values that
 * represent the corresponding time, expressed for the local timezone.
 * @param timer Pointer to an object of type time_t that contains a time value.
 * @return A pointer to a tm structure with its members filled with the values
 * that correspond to the local time representation of timer. The returned
 * value points to an internal object whose validity or value may be altered by
 *  any subsequent call to gmtime or localtime.
 * @remarks  ION knows nothing of time-zones.  All time is reported in GMT
 * which is provided by the GPS can aerospace values.  As such localtime and
 * gmtime are the same function
 */
extern struct tm *localtime (const time_t *timer);
/**
 * Uses the value pointed by timer to fill a tm structure with the values that
 * represent the corresponding time, expressed as a UTC time (i.e., the time
 * at the GMT timezone).
 * @param timer Pointer to an object of type time_t that contains a time value.
 * @return A pointer to a tm structure with its members filled with the values
 * that correspond to the local time representation of timer. The returned
 * value points to an internal object whose validity or value may be altered by
 *  any subsequent call to gmtime or localtime.
 */
extern struct tm *gmtime(const time_t *timer);

/*---------------------------------Thread functions---------------------------*/
/**
 * Create a new thread
 * @param func          function to execute
 * @param stack_depth   number of words for stack
 * @param arg           argument to pass to thread function
 * @param priority      priority of the thread
 * @param new_task      pointer to new thread
 * @return s_ok if created
 */
extern result_t thread_create(thread_func func, size_t stack_depth, void *arg,
                              thread_priority_t priority, handle_t *new_task);
/**
 * Terminate a thread and release all resources
 * @param thread  Thread to terminate
 * @return s_ok if deleted ok
 */
extern result_t thread_delete(handle_t thread);
/**
 * Delay the current thread
 * @param ms  Milliseconds to delay (assume 1ms tick)
 */
extern void thread_delay(ticks_t ms);
/**
 * Return the priority of the thread
 * @param thread  Thread to get priority of
 * @return Priority
 */
extern thread_priority_t get_thread_priority(handle_t thread);
/**
 * Set the priority of a thread
 * @param thread  Thread to change priority of
 * @param pri     New priority
 * @return s_ok if success
 */
extern result_t set_thread_priority(handle_t thread, thread_priority_t pri);
/**
 * Stop a threas
 * @param thread  Thread to stop
 * @return s_ok if thread was suspended
 */
extern result_t thread_suspend(handle_t thread);
/**
 * Resume a suspended thread
 * @param thread  Thread to resume
 * @return s_ok if resumed ok
 */
extern result_t thread_resume(handle_t thread);
/**
 * Get the calling thread handle
 * @return handle of the calling thread
 */
extern handle_t get_thread_handle();
/**
 * Return the state of a thread
 * @param thread  Thread to get state of
 * @return state of the thread
 */
extern thread_state_t get_thread_state(handle_t thread);

/*-----------------------------Queues-----------------------------------------*/
/**
 * Create a queue
 * @param length        Maximum elements in queue
 * @param element_size  Size of each element
 * @param handle        created queue handle
 * @return s_ok if queue created ok
 */
extern result_t queue_create(size_t length, size_t element_size, handle_t *handle);
/**
 * Delete a queue and release all resources
 * @param handle  queue to delete
 * @return s_ok if deleted
 */
extern result_t queue_delete(handle_t handle);
/**
 * Reset a queue to an empty state
 * @param handle  queue to empty
 * @return s_ok if queue is empty
 */
extern result_t queue_reset(handle_t handle);
/**
 * Push an item to the end of the queue
 * @param queue         queue to push
 * @param item          item to put onto queue
 * @param time_to_wait  ms to wait 
 * @return s_ok if done otherwise error
 */
extern result_t queue_push_back(handle_t queue, const void *item, ticks_t time_to_wait);
/**
 * Push an item onto the front of the queue
 * @param queue         queue to push
 * @param item          item to queue
 * @param time_to_wait  ms to wait for
 * @return s_ok if done othewise error
 */
extern result_t queue_push_front(handle_t queue, const void *item, ticks_t time_to_wait);
/**
 * Remove an item from the head of a queue
 * @param queue         queue to get item from
 * @param item          item removed
 * @param time_to_wait  ms to wait for item
 * @return s_ok if item was available
 */
extern result_t queue_pop_front(handle_t queue, void *item, ticks_t time_to_wait);
/**
 * Return the number of items in the queue
 * @param queue   queue to query
 * @return items in the queue
 */
extern size_t get_queue_waiting(handle_t queue);
/**
 * Return the maximum number of items that a queue can handle
 * @param queue queue to query
 * @return maximum number of items in queue
 */
extern size_t get_queue_length(handle_t queue);
/**
 * Receive an item from a queue without removing it from the queue
 * @param queue         queue to query
 * @param item          item queried
 * @param time_to_wait  time to wait for an item
 * @return s_ok if item queiried ok
 */
extern result_t queue_peek_front(handle_t queue, void *item, ticks_t time_to_wait);

/*----------------------------Semaphores--------------------------------------*/
/**
 * Create a semaphore
 * @param max_count       maximum number of times set can be called
 * @param initial_count   Initial count of semaphore
 * @param hndl            created handle
 * @return s_ok if created
 */
extern result_t semaphore_create(size_t max_count, size_t initial_count, handle_t *hndl);
/**
 * Delete a semaphore and release all resources
 * @param hndl  semaphore to delete
 * @return s_ok if deleted ok
 */
extern result_t semaphore_delete(handle_t hndl);
/**
 * Return the current count of the semaphore
 * @param hndl  semaphore to query
 * @return number of time semaphore_set is called
 */
extern size_t get_semaphore_count(handle_t hndl);
/**
 * Increment a semaphore count, and release any thread waiting
 * @param hndl  semaphore to set
 * @return s_ok if processed
 */
extern result_t semaphore_set(handle_t hndl);
/**
 * Wait on a semaphore being signaled
 * @param hndl          semaphore to wait on
 * @param time_to_wait  amount of time to wait
 * @return s_ok if signaled, or e_timeout if wait expired
 */
extern result_t semaphore_get(handle_t hndl, ticks_t time_to_wait);

/*--------------------------Mutex---------------------------------------------*/
/**
 * Create a mutex to protect shared resources
 * @param hndl  handle of craeted mutex
 * @return s_ok if created ok
 */
extern result_t mutex_create(handle_t *hndl);
/**
 * Delete a mutex
 * @param hndl  Mutex to delete
 * @return s_ok if deleted ok
 */
extern result_t mutex_delete(handle_t hndl);
/**
 * Obtain a mutual exclusion area
 * @param hndl          handle to mutex
 * @param time_to_wait  amount of time to wait
 * @return s_ok if mutex obtained
 */
extern result_t mutex_lock(handle_t hndl, ticks_t time_to_wait);
/**
 * Release a mutual exclusion area
 * @param hndl  mutex to release
 * @return s_ok if processed ok
 */
extern result_t mutex_unlock(handle_t hndl);

/*-----------------------------Events-----------------------------------------*/
/**
 * Create an event
 * @param auto_clear    if true the clear event after a wait
 * @param hndl          created event
 * @return s_ok if created ok
 */
extern result_t event_create(bool auto_clear, handle_t *hndl);
/**
 * Delete an event
 * @param hndl  event to delete
 * @return s_ok if deleted ok
 */
extern result_t event_delete(handle_t hndl);
/**
 * Wait for an event to be set
 * @param hndl  event to wait on
 * @param ms    how long to wait
 * @return s_ok if event was set
 */
extern result_t event_wait(handle_t hndl, ticks_t ms);
/**
 * Set an event
 * @param hndl  event to set
 * @return s_ok if set ok
 */
extern result_t event_set(handle_t hndl);

/**
 * Initialize the display.  Must be called before any other operations.
 * @param orientation orientation to use
 * @return s_ok if display can be used.
 */
extern result_t display_init(int orientation);
/**
 * Get the display orientation of the screen
 * @return orientation
 */
extern int get_display_orientation();
/**
 * Set the screen orientation
 * @param orientation orientation to use
 * @return s_ok if can be switched, e_invalidarg if screens open
 */
extern result_t set_display_orientation(int orientation);
/**
 * Get the number of defined screens
 * @return number of defined screens
 */
extern size_t display_count();
/**
 * Return the metrics of a canvas
 * @param num     canvas number to enumerate
 * @param metrics metrics of canvas
 * @param name    name of the canvas
 * @return 
 */
extern result_t display_metrics(size_t num, metrics_t *metrics, const char **name);
/**
 * Open a defined canvas
 * @param name  number of the screen to open
 * @param hndl  canvas handle
 * @return s_ok if the screen can be opened.
 */
extern result_t display_open(size_t num, handle_t *hndl);
/**
 * Close a canvas.
 * @param   canvas to release
 * @return s_ok if all resources freed.
 */
extern result_t canvas_close(handle_t);
/**
 * Create an off screen canvas
 * @param size    dimensions of the display context
 * @param hndl    created handle
 * @return s_ok if enough memory for canvas
 */
extern result_t canvas_create_rect(const extent_t *size, handle_t *hndl);
/**
 * Create a canvas from the dimensions of the bitmap and select the pixels
 * into the canvas
 * @param bitmap    bitmap to create from
 * @param hndl      created canvas
 * @return s_ok if canvas created ok
 */
extern result_t canvas_create_bitmap(const bitmap_t *bitmap, handle_t *hndl);
/**
 * Draw a polyline
 * @param canvas      canvas to draw on
 * @param clip_rect   rectangle to clip to
 * @param pen         pen to draw with
 * @param points      points to draw
 * @param count       number of points
 * @return s_ok if succeeded
 */
extern result_t canvas_polyline(handle_t canvas, const rect_t *clip_rect, const pen_t *pen, const point_t *points, size_t count);
/**
 * Fill a rectangle
 * @param canvas      canvas to draw on
 * @param clip_rect   rectangle to clip to
 * @param area        area to fill
 * @param color       color to fill with
 * @return  s_ok if succeeded
 */
extern result_t canvas_fill_rect(handle_t canvas, const rect_t *clip_rect, const rect_t *area, color_t color);
/**
 * Draw an ellipse
 * @param canvas      canvas to draw on
 * @param clip_rect   rectangle to clip to
 * @param pen         pen to draw ellipse with
 * @param color       color to fill ellipse with
 * @param area        area of the ellise
 * @return  s_ok if succeeded
 */
extern result_t canvas_ellipse(handle_t canvas, const rect_t *clip_rect, const pen_t *pen, color_t color, const rect_t *area);
/**
 * Draw a polygon and optionally fill it
 * @param canvas      canvas to draw on
 * @param clip_rect   rectangle to clip to
 * @param pen         pen to draw lines with
 * @param color       color to fill with
 * @param points      points of the polygon
 * @param count       number of points
 * @param interior_fill true to fill the interior
 * @return  s_ok if succeeded
 */
extern result_t canvas_polygon(handle_t canvas, const rect_t *clip_rect, const pen_t *pen, color_t color, const point_t *points, size_t count, bool interior_fill);
/**
 * Draw a filled rectangle
 * @param canvas        canvas to draw on
 * @param clip_rect     rectangle to clip to
 * @param pen           pen to outline with
 * @param color         color to fill with
 * @param area          area of rectangle
 * @return  s_ok if succeeded
 */
extern result_t canvas_rectangle(handle_t canvas, const rect_t *clip_rect, const pen_t *pen, color_t color, const rect_t *area);
/**
 * Draw a rectangle with rounded corners
 * @param canvas        canvas to draw on
 * @param clip_rect     rectangle to clip to
 * @param pen           pen to outline with
 * @param color         color to fill with
 * @param area          area of rectangle
 * @param corners       radius of corners
 * @return  s_ok if succeeded
 */
extern result_t canvas_round_rect(handle_t canvas, const rect_t *clip_rect, const pen_t *pen, color_t color, const rect_t *area, const extent_t *corners);
/**
 * Copy pixels from one canvas to another
 * @param canvas        canvas to draw on
 * @param clip_rect     rectangle to clip to
 * @param dest_rect     rectangle to draw into
 * @param src_canvas    canvas to copy from
 * @param src_clip_rect area of canvas to copy from
 * @param src_pt        top-left point of the source rectangle
 * @param operation     raster operation
 * @return  s_ok if succeeded
 */
extern result_t canvas_bit_blt(handle_t canvas, const rect_t *clip_rect, const rect_t *dest_rect, handle_t src_canvas, const rect_t *src_clip_rect, const point_t *src_pt, raster_operation operation);
/**
 * Copy a rectangle and mask with a bitmap
 * @param canvas        canvas to draw on
 * @param clip_rect     rectangle to clip to
 * @param dest_rect     recttangle to draw into
 * @param src_canvas    canvas to copy from
 * @param src_clip_rect area in source to clip to
 * @param src_point     top left point of source rectangle
 * @param mask_bitmap   bitmap to mask to.  If width lt source width then will be tiled
 * @param mask_point    point in bitmap to copy from
 * @param operation     raster operation
 * @return  s_ok if succeeded
 */
extern result_t canvas_mask_blt(handle_t canvas,
                                const rect_t *clip_rect,
                                const rect_t *dest_rect,
                                handle_t src_canvas,
                                const rect_t *src_clip_rect,
                                const point_t *src_point,
                                const bitmap_t *mask_bitmap,
                                const point_t *mask_point,
                                raster_operation operation);
/**
 * Copy a bitmap and apply a rotation on the copy
 * @param canvas        canvas to copy to
 * @param clip_rect     destination to clip to
 * @param dest_center   center point copy destination
 * @param src           canvas to copy from
 * @param src_clip_rect area in source that is available
 * @param src_point     center point on source
 * @param radius        area around center to copy
 * @param angle         rotation to be applied, 0-359 degrees
 * @param operation     raster operation to apply
 * @return  s_ok if succeeded
 */
extern result_t canvas_rotate_blt(handle_t canvas,
                                  const rect_t *clip_rect,
                                  const point_t *dest_center,
                                  handle_t src, 
                                  const rect_t *src_clip_rect,
                                  const point_t *src_point,
                                  gdi_dim_t radius,
                                  int angle,
                                  raster_operation operation);
/**
 * Return pixel 
 * @param canvas      canvas to query
 * @param clip_rect   rectangle to clip to
 * @param pt          point to get
 * @return pixel at point
 */
extern color_t canvas_get_pixel(handle_t canvas,
                                const rect_t *clip_rect,
                                const point_t *pt);
/**
 * Set a pixel
 * @param canvas      canvas to write to
 * @param clip_rect   rectangle to clip to
 * @param pt          point to wite
 * @param c           color to write
 * @return old pixel
 */
extern color_t canvas_set_pixel(handle_t canvas,
                                const rect_t *clip_rect,
                                const point_t *pt, color_t c);
/**
 * Draw an arc
 * @param canvas      canvas to draw on
 * @param clip_rect   rectangle to clip to
 * @param pen         pen to use for line
 * @param pt          center point
 * @param radius      radius of arc
 * @param start       start angle in degrees 0-359
 * @param end         end angle in degress 0-359
 * @return s_ok if completed
 */
extern result_t canvas_arc(handle_t canvas,
                           const rect_t *clip_rect,
                           const pen_t *pen,
                           const point_t *pt,
                           gdi_dim_t radius,
                           int start,
                           int end);
/**
 * Draw a pie
 * @param canvas      canvas to write to
 * @param clip_rect   rectangle to clip to
 * @param pen         pen to use for outline
 * @param color       color to fill with
 * @param pt          center point
 * @param start       start angle in degrees 0-359
 * @param end         end angle in degrees 0-359
 * @param radii       pie radius
 * @param inner       innert radius
 * @return  s_ok if completed
 */
extern result_t canvas_pie(handle_t canvas,
                           const rect_t *clip_rect,
                           const pen_t *pen,
                           color_t color,
                           const point_t *pt,
                           int start,
                           int end,
                           gdi_dim_t radii,
                           gdi_dim_t inner);
/**
 * Draw text
 * @param canvas      canvas to write to
 * @param clip_rect   rectangle to clip to
 * @param font        font to write
 * @param fg          foreground color
 * @param bg          background color
 * @param str         text to write
 * @param count       number of characters
 * @param src_pt      top left point of text
 * @param txt_clip_rect area to clip text to
 * @param format      text operation
 * @param char_widths optional array of widths of characters written
 * @return  s_ok if completed
 */
extern result_t canvas_draw_text(handle_t canvas,
                                 const rect_t *clip_rect,
                                 const font_t *font,
                                 color_t fg,
                                 color_t bg,
                                 const char *str,
                                 size_t count, 
                                 const point_t *src_pt,
                                 const rect_t *txt_clip_rect,
                                 text_flags format,
                                 size_t *char_widths);
/**
 * Return the area text draws within
 * @param canvas      canvas to write to
 * @param font        font to use
 * @param str         text to write
 * @param count       number of characters
 * @return  s_ok if completed
 */
extern extent_t canvas_text_extent(handle_t canvas,
                                   const font_t *font,
                                   const char *str, size_t count);
/**
 * Invalidate the area of the canvas.
 * @param canvas  canvas to invalidate
 * @param rect    rectangle invalidated
 * @return 
 */
extern result_t canvas_invalidate_rect(handle_t canvas, const rect_t *rect);


/*---------------------------FILE SYSTEM FUNCTIONS----------------------------*/
extern result_t format(const char *vol, uint32_t dev_id, uint32_t part_no, uint32_t start_sec,
                   uint32_t cnt, const char *fs_type, uint32_t opt);
extern result_t mount(const char *vol, uint32_t dev_id, uint32_t part_no, uint32_t opt);
extern result_t umount(const char *vol, uint32_t opt);
extern result_t sync(const char *vol);
extern result_t statfs(const char *vol, statfs_t *statbuf);
extern result_t ioctl(const char *vol, uint32_t func, void *param);
extern result_t mkdir(const char *path, mod_t mode/*permission*/);
extern result_t rmdir(const char *path);
extern dir_t* opendir(const char *path);
extern dirent_t *readdir(dir_t *de);
extern result_t rewinddir(dir_t *debuf);
extern result_t closedir(dir_t *debuf);
extern result_t cleandir(const char *path);
extern result_t statdir(const char *path, statdir_t *statbuf);
extern result_t access(const char *path, int32_t amode);
extern int32_t creat(const char *path, mod_t mode);
extern int32_t open(const char *path, uint32_t flag, ... /* mod_t mode */);
extern size_t read(int32_t fd, void *buf, size_t bytes);
extern size_t write(int32_t fd, const void *buf, size_t bytes);
extern offs_t lseek(int32_t fd, offs_t offset, int32_t whence);
extern result_t fsync(int32_t fd);
extern result_t close(int32_t fd);
extern result_t closeall(const char *vol);
extern result_t unlink(const char *path);
extern result_t truncate(int32_t fd, size_t new_size);
extern result_t tell(int32_t fd);
extern result_t rename(const char *oldpath, const char *newpath);
extern result_t stat(const char *path, stat_t *statbuf);
extern result_t fstat(int32_t fd, stat_t *statbuf);
extern result_t getattr(const char *path, uint32_t *attrbuf);
extern result_t fgetattr(int32_t fd, uint32_t *attrbuf);
extern result_t setattr(const char *path, uint32_t attr);
extern result_t fsetattr(int32_t fd, uint32_t attr);
extern bool fsm_set_safe_mode(bool issafe);
extern bool set_safe_mode(bool is_safe);
extern result_t get_sectors(int32_t dev_id);
extern result_t get_devicetype(int32_t dev_id, char * name);
extern const fsver_t *get_version(void);



#define RESULT_DEFINED
#define HANDLE_DEFINED

#if defined (__cplusplus )
  }
#endif

#endif


/*----------------------------------------------------------------------------
 END OF FILE
----------------------------------------------------------------------------*/
