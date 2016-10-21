#include "ion_hal.h"

#include "../../../gdi-lib/stddef.h"
#include "../../../gdi-lib/application.h"
#include "../../../gdi-lib/window.h"
#include "../../../gdi-lib/screen.h"
#include "../../../gdi-lib/spatial.h"
#include "../../../gdi-lib/can_driver.h"
#include "../../../gdi-lib/assert.h"
#include "can_driver.h"
#include "../../../widgets/pfd_application.h"
#include "../../../widgets/layout_window.h"
#include "../../../widgets/menu_window.h"

class ion_screen_t: public kotuku::screen_t
  {
public:
  ion_screen_t( kotuku::hal_t *hal, const  kotuku::screen_metrics_t &metrics, handle_t handle);
  virtual ~ion_screen_t();
  
  virtual kotuku::canvas_t &background_canvas();
  
  bool check_redraw();
  void draw();
private:
  handle_t handle;
  kotuku::canvas_t _background_canvas;
  };
  
kotuku::canvas_t &ion_screen_t::background_canvas()
  {
  return _background_canvas;
  }

inline ion_screen_t *as_screen_handle(kotuku::screen_t *h)
  {
  return reinterpret_cast<ion_screen_t *>(h);
  }

int ion_hal_t::_trace_level = 4;
  
static const char *vol_a = "/a/";

static const nv_pair_t init_values[] = {
  0, 0
  };

ion_hal_t::ion_hal_t()
  {
  _provider = 0;
  _trace_level = 4;
  
  ion_init(init_values, 0);
  }

int ion_hal_t::trace_level() const
  {
  return _trace_level;
  }

static const char *ion_hal_section = "ion-hal";

result_t ion_hal_t::initialize(const char *ini_file)
  {
  result_t result;
  if(failed(result = hal_t::initialize(ini_file)))
    return result;
  
  int orientation;
  get_config_value(ion_hal_section, "orientation", orientation);
  
  // set up the display orientation
  display_init(orientation);

  //_can_interface = new ion_can_driver_t();

  int receive_timeout = -1;
  get_config_value(ion_hal_section, "receive-timeout", receive_timeout);

  int send_timeout = -1;
  get_config_value(ion_hal_section, "send-timeout", send_timeout);

  // _can_interface->initialize(can_device.c_str(), receive_timeout, send_timeout);

  std::string main_window_name;
  if(failed(get_config_value(ion_hal_section, "main-screen", main_window_name)))
    main_window_name = "main";
  
  std::string menu_window_name;
  if(failed(get_config_value(ion_hal_section, "menu-screen", menu_window_name)))
    menu_window_name = "menu";
  
  std::string alert_window_name;
  if(failed(get_config_value(ion_hal_section, "alert-screen", alert_window_name)))
    alert_window_name = "alert";

  size_t num_screens = display_count();
  
  for(size_t s = 0; s < num_screens; s++)
    {
    const char *name;
    metrics_t metrics;
    
    if(succeeded(display_metrics(s, &metrics, &name)))
      {
      handle_t hndl;
      if(succeeded(display_open(s, &hndl)))
        {
        if(main_window_name == name)
          _root_window = new kotuku::layout_window_t(hndl);
        else if(menu_window_name == name)
          _menu_window = new kotuku::menu_window_t(hndl);
        else if(alert_window_name == name)
          {
          
          }
        }
    }

  return s_ok;
  }

result_t ion_hal_t::publish(const can_msg_t &msg)
  {
  return _can_interface->publish(msg);
  }

result_t ion_hal_t::set_can_provider(canaerospace_provider_t *pdata)
  {
  _provider = pdata;
  return _can_interface->set_can_provider(pdata);
  }

result_t ion_hal_t::get_can_provider(
    canaerospace_provider_t **prov)
  {
  *prov = _provider;
  return s_ok;
  }

ion_hal_t::time_base_t ion_hal_t::now()
  {
  time_t now = time(0);
  
  if(now == (time_t)-1)
    return 0;
  
  interval_t nowl = now;
  nowl *= interval_per_second;
  
  return hal_t::time_base_t(nowl);
  }

ion_hal_t::time_base_t ion_hal_t::mktime(uint16_t year,
    uint8_t month, uint8_t day, uint8_t hour,
    uint8_t minute, uint8_t second, uint16_t milliseconds,
    uint32_t nanoseconds)
  {
  ::tm now;

  now.tm_year = year - 1900;
  now.tm_mon = month - 1;
  now.tm_mday = day;
  now.tm_hour = hour;
  now.tm_min = minute;
  now.tm_sec = second;

  interval_t nowl = ::mktime(&now);
  nowl *= interval_per_second;

  return hal_t::time_base_t(nowl);
  }

// split a time value
void ion_hal_t::gmtime(time_base_t when, uint16_t *year,
    uint8_t *month, uint8_t *day, uint8_t *hour,
    uint8_t *minute, uint8_t *second, uint16_t *milliseconds,
    uint32_t *nanoseconds)
  {
  ::time_t gm_now = (::time_t) (when / interval_per_second);
  ::tm *now = ::gmtime(&gm_now);

  if(year != 0)
    *year = (uint16_t) now->tm_year + 1900;

  if(month != 0)
    *month = (uint8_t) now->tm_mon + 1;

  if(day != 0)
    *day = (uint8_t) now->tm_mday;

  if(hour != 0)
    *hour = (uint8_t) now->tm_hour;

  if(minute != 0)
    *minute = (uint8_t) now->tm_min;

  if(second != 0)
    *second = (uint8_t) now->tm_sec;

  if(milliseconds != 0)
    *milliseconds = (uint16_t) ((when - (when / interval_per_second))
        / 10000);

  if(nanoseconds != 0)
    *nanoseconds = 0;
  }

void ion_hal_t::assert_failed()
  {
  }

static const char *msgfmt = "%s";

void ion_hal_t::debug_output(int level, const char *msg)
  {
  if(level <= _trace_level)
    {
    }
  }

result_t ion_hal_t::is_bad_read_pointer(const void *p, size_t n)
  {
  return s_ok;
  }

result_t ion_hal_t::is_bad_write_pointer(void *p, size_t n)
  {
  return s_ok;
  }

long ion_hal_t::interlocked_increment(volatile long &n)
  {
  return __sync_fetch_and_add(&n, 1);
  }

long ion_hal_t::interlocked_decrement(volatile long &n)
  {
  return __sync_fetch_and_sub(&n, 1);
  }

// event functions
handle_t ion_hal_t::event_create(bool manual_reset, bool initial_state)
  {
  SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
  
  if(initial_state)
    xSemaphoreGive(semaphore);
  
  return semaphore;
  }

void ion_hal_t::event_close(handle_t h)
  {
  event_delete(h);
  }

bool ion_hal_t::event_wait(handle_t h, uint32_t timeout_ms)
  {
  return event_wait(h, timeout_ms);
  }

void ion_hal_t::event_set(handle_t h)
  {
  xSemaphoreGive((SemaphoreHandle_t)h);
  }

// critical section functions
handle_t ion_hal_t::critical_section_create()
  {
  return xSemaphoreCreateMutex();
  }

void ion_hal_t::critical_section_close(handle_t h)
  {
  xSemaphoreDelete((SemaphoreHandle_t)h);
  }

void ion_hal_t::critical_section_lock(handle_t h)
  {
  xSemaphoreTake((SemaphoreHandle_t)h, portMAX_DELAY);
  }

void ion_hal_t::critical_section_unlock(handle_t h)
  {
  xSemaphoreGive((SemaphoreHandle_t) h);
  }


handle_t ion_hal_t::thread_create(thread_func pfn,
    size_t stack_size, void *param, unsigned int *id)
  {
  ThreadHandle_t handle;
  xTaskCreate((TaskFunction_t)pfn, "", stack_size, param, thread_t::normal, &handle);
  
  if(id != 0)
    *id = 0;
  
  return handle;
  }

void ion_hal_t::thread_set_priority(handle_t h, uint8_t p)
  {
  vTaskPioritySet((ThreadHandle_t)h, p);
  }

uint8_t ion_hal_t::thread_get_priority(handle_t h)
  {
  return vTaskPriorityGet((ThreadHandle_t)h);
  }

void ion_hal_t::thread_suspend(handle_t h)
  {
  vTaskSuspend((ThreadHandle_t)h);
  }

void ion_hal_t::thread_resume(handle_t h)
  {
  vTaskResume((ThreadHandle_t)h);
  }

void ion_hal_t::thread_sleep(uint32_t n)
  {
  vTaskDelay(n);
  }

bool ion_hal_t::thread_wait(handle_t h, uint32_t time_ms)
  {
  return false;
  }

bool ion_hal_t::thread_yield(handle_t h)
  {
  return false;
  }

uint32_t ion_hal_t::thread_exit_code(handle_t h)
  {
  return 0;
  }

thread_t::status_t ion_hal_t::thread_status(handle_t h)
  {
  switch(eTaskGetState((ThreadHandle_t)h))
    {
    case eReady :
      return thread_t::creating;
    case eRunning :
      return thread_t::running;
    case eBlocked :
      return thread_t::blocked;
    case eSuspended :
      return thread_t::suspended;
    default :
    case eDeleted :
      return thread_t::terminated;
    }
  }

void ion_hal_t::thread_close(handle_t h)
  {
  vTaskDelete((ThreadHandle_t)h);
  }

void ion_hal_t::thread_terminate(handle_t h, uint32_t term_code)
  {
  }

unsigned int ion_hal_t::thread_current_id(handle_t h)
  {
  return0
  }

void ion_hal_t::set_last_error(result_t r)
  {
  _last_error = r;
  }

result_t ion_hal_t::get_last_error()
  {
  return _last_error;
  }

screen_t *ion_hal_t::screen_create(screen_t *h, const extent_t &sz)
  {
  return h->create_canvas(h, sz);
  }

screen_t *ion_hal_t::screen_create(screen_t *h,
    const bitmap_t &bm)
  {
  return h->create_canvas(h, bm);
  }

screen_t *ion_hal_t::screen_create(screen_t *h, const rect_t &r)
  {
  return h->create_canvas(h, r);
  }

void ion_hal_t::screen_close(window_t *h)
  {
  delete h;
  }

screen_t *ion_hal_t::screen()
  {
  return _screen;
  }

static uint16_t *create_framebuffer(hal_t *hal, const screen_metrics_t &metrics)
  {
  return alloc_framebuffer(metrics.screen_x * metrics.screen_y);
  }

ion_screen_t::ion_screen_t(hal_t *hal, const screen_metrics_t &metrics)
:  memory_screen_t(create_framebuffer(hal, metrics), false, metrics),
  _background_canvas(new memory_screen_t(create_framebuffer(hal, metrics), true, metrics), rect_t(0, 0, metrics.screen_x, metrics.screen_y)),
   thread_t(4096, this, do_run),
   _root_window(0)
  {
  // the worker will init the screen for us.
  resume();
  }

uint32_t ion_screen_t::do_run(void *pthis)
  {
  return reinterpret_cast<ion_screen_t *>(pthis)->run();
  }

uint32_t ion_screen_t::run()
  {
  // swap mode to orientation
  int mode;
  if(failed(hal->get_config_value("diyefis-hal", "rotation", mode)))
    mode = 0;
  
  display_mode(mode);
  
  // fill the background canvas first
  fill_rect(window_rect(), color_black);
  // update the backgrounds
   _root_window->paint_background();
  // and in
  _root_window->invalidate();

  while(!should_terminate())
    {
    // wait for a vsync.
    event_t::lock_t butler(_vsync_irq);

    if(_root_window == 0)
      continue;             // wait for a window to be created

    // this code works by waiting for a vsync (every 60hz) then on the first one
    // will ask any changed windows to update the frame buffer
    // if there was a change, on the next vsync the frame buffer is copied
    // to the display buffer.  Hopefully before the vsync ends so that the display
    // does not encounter any tearing
    // only works if the frame buffer and display buffer are identical.
    // We use memcpy as that tends to be a generic assembler routine with hardware
    // fast copy.
    switch(_state)
      {
      case fbds_checkredraw :
        if(_root_window->is_invalidated() &&
           _root_window->repaint(true))
            _state = fbds_drawsync;     // at next vsync we draw
        break;
      case fbds_drawsync :
        // since we don't know how long the previous state took we wait
        // for the vsync irq to be reset before we wait for the next
        // vsync
        state = fbds_draw;
        break;
      case fbds_draw :
        {
        // when we are in this state the foreground buffer has been drawn
        // and we now use the DMA controller to copy the foreground buffer to
        // the display buffer.
          
        // assuming a 58mhz Ts inside the S1D13L02 the minimum write time
        // for the buffer is 15.3 msec
        // The AS6C3216 SRAM has a read cycle time of 55ns or 17ms read time
        // for the complete buffer
        // minimum block transfer is 32.3msec
        
        // the tft has a hsync of 31.5khz, vsync 60hz, pixclk 25mhz
        // this means the display refresh will take 2 frames when an update
        // occurs.
        bit_blt_image(_buffer, ROOT_WINDOW_BASE, ROOT_WINDOW_SIZE)
        // we fill the foreground buffer, with the background
        bit_blt(clip_rect, window_rect(),
                _background_canvas.screen(),
                _background_canvas.window_rect(),
                _background_canvas.window_rect().top_left(),
                rop_srccopy);
        
        // max redraw speed is 64msec + update speed.  Probably
        // around 10 frames/sec
        
        _state = fbds_checkredraw;
        break;
      }
    }

  return 0;
  }

ion_screen_t::~ion_screen_t()
  {

  }

int ion_screen_t::display_mode() const
  {
  return _display_mode;
  }

void ion_screen_t::display_mode(int value)
  {
  _display_mode = value;
  
  }

void memory_screen_t::invalidate_rect(const rect_t &)
  {

  }

memory_screen_t::memory_screen_t(color_t *buffer, bool owns, const screen_metrics_t &metrics)
: _buffer(buffer),
  _owns(owns),
  raster_screen_t(metrics)
  {
  }

memory_screen_t::~memory_screen_t()
  {
  if(_owns)
    {
    delete[] _buffer;
    _buffer = 0;
    }
  }

screen_t *memory_screen_t::create_canvas(screen_t *, const extent_t &extents)
  {
  color_t *buffer = new color_t[extents.dx * extents.dy];

  return new memory_screen_t(buffer, true, screen_metrics_t(extents.dx, extents.dy, 16));
  }


screen_t *memory_screen_t::create_canvas(screen_t *, const bitmap_t &bitmap)
  {
  return new memory_screen_t(const_cast<color_t *>(bitmap.pixels), false, screen_metrics_t(bitmap.bitmap_width, bitmap.bitmap_height, bitmap.bpp));
  }

screen_t *memory_screen_t::create_canvas(screen_t *h,
    const rect_t &rect)
  {
  return create_canvas(h, rect.extents());
  }

uint8_t *memory_screen_t::point_to_address(const point_t &pt)
  {
  return reinterpret_cast<uint8_t *>(_buffer + (pt.y * screen_x) + pt.x);
  }

const uint8_t *memory_screen_t::point_to_address(const point_t &pt) const
  {
  return reinterpret_cast<const uint8_t *>(_buffer + (pt.y * screen_x) + pt.x);
  }

uint8_t *memory_screen_t::point_to_address(const point_t &pt)
  {
  gdi_dim_t pixel_offset =  ((pt.y * screen_x) + pt.x);

  return reinterpret_cast<uint8_t *>(_buffer) + (pixel_offset << 1);
  }

const uint8_t *memory_screen_t::point_to_address(const point_t &pt) const
  {
  gdi_dim_t pixel_offset =  ((pt.y * screen_x) + pt.x);

  return reinterpret_cast<const uint8_t *>(_buffer) + (pixel_offset << 1);
  }


gdi_dim_t memory_screen_t::pixel_increment() const
  {
  return 2;       // return in bytes
  }

color_t memory_screen_t::get_pixel(const uint8_t *src) const
  {
  return ::get_pixel(reinterpret_cast<uint16_t *>(src));
  }

void memory_screen_t::set_pixel(uint8_t *dest, color_t color) const
  {
  ::set_pixel(reinterpret_cast<uint16_t *>(dest), color);
  }

critical_section_t memory_screen_t::_dma6_cs;
event_t memory_screen_t::_dma6_irq;

void memory_screen_t::dma6_irq()
  {
  // the transfer is completed see if any more to do
  _dma6_irq.set();
  }
  
static void do_transfer(uint16_t *dest, uint16_t dest_size, const uint16_t *src, uint16_t src_size)
  {
  _transfer_size = dest_size;
  // now the display controller uses the dma controller channel 7
  // to transfer the framebuffer to the S1D13L02
  DCH6SSA=VirtToPhys(src); // transfer source physical address
  DCH6DSA=VirtToPhys(dest); // transfer destination physical address
  
  DCH6SSIZ = src_size;   // source size 65536 bytes
  DCH6DSIZ=dest_size;     // destination size 2 bytes
  DCH6CSIZ=src_size;     // transfer size, 65536
  DCH6INT = 0x00800000; // clear interrupts and enable source done
  DCH6CONSET = 0x80; // turn channel on
  DCH6ECONSET=0x00000080; // set CFORCE to 1
  
  event_t::lock_t doorbell(_dma6_irq);
  }

void memory_screen_t::fast_fill(uint8_t *dest, size_t words, color_t fill_color, raster_operation rop)
  {
  if(rop != rop_srccopy)
    framebuffer_screen_t::fast_fill(dest, words, fill_color, rop);
  
  critical_section::lock_t lock(_dma6_cs);
  
  uint16_t fill_color = set_pixel(fill_color);
  do_transfer(reinterpret_cast<uint16_t *>(dest), words << 1, &fill_color, 2);
  }

void memory_screen_t::fast_copy(uint8_t *dest, const raster_screen_t *src_screen, const uint8_t *src, size_t words, raster_operation rop)
  {
  if(rop != rop_srccopy)
    framebuffer_screen_t::fast_copy(dest, src_screen, src, words, rop);
  
  critical_section::lock_t lock(_dma6_cs);
  do_transfer(reinterpret_cast<uint16_t *)(dest), words << 1, reinterpret_cast<uint16_t *>(src), words << 1);
  }
