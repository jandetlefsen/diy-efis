#ifndef __ion_hal_h__
#define __ion_hal_h__


#include "../../../ion/ion.h"

#include <map>
#include "../../../gdi-lib/hal.h"

class ion_can_driver_t;
namespace kotuku {
class layout_window_t;
  };

class ion_hal_t : public kotuku::hal_t
  {
public:
  ion_hal_t();

  result_t initialize(const char *path);

  virtual time_base_t now();
  // make time from the values passed
  virtual time_base_t mktime(uint16_t year, uint8_t month,
                             uint8_t day, uint8_t hour, uint8_t minute,
                             uint8_t second, uint16_t milliseconds,
                             uint32_t nanoseconds);
  // split a time value
  virtual void gmtime(time_base_t when, uint16_t *year,
                      uint8_t *month, uint8_t *day, uint8_t *hour,
                      uint8_t *minute, uint8_t *second,
                      uint16_t *milliseconds, uint32_t *nanoseconds);

  virtual void assert_failed();
  virtual void debug_output(int level, const char *);

  // memory checking functions
  virtual result_t is_bad_read_pointer(const void *, size_t);
  virtual result_t is_bad_write_pointer(void *, size_t);
  virtual long interlocked_increment(volatile long &);
  virtual long interlocked_decrement(volatile long &);

  // event functions
  virtual handle_t event_create(bool manual_reset, bool initial_state);
  virtual void event_close(handle_t);
  virtual bool event_wait(handle_t, uint32_t timeout_ms);
  virtual void event_set(handle_t);

  // critical section functions
  virtual handle_t critical_section_create();
  virtual void critical_section_close(handle_t);
  virtual void critical_section_lock(handle_t);
  virtual void critical_section_unlock(handle_t);

  // thread functions
  virtual handle_t thread_create(thread_func, size_t stack_size, void *param,
                                 unsigned int *id);
  virtual void thread_set_priority(handle_t, uint8_t);
  virtual uint8_t thread_get_priority(handle_t);
  virtual void thread_suspend(handle_t);
  virtual void thread_resume(handle_t);
  virtual void thread_sleep(uint32_t);
  virtual bool thread_wait(handle_t, uint32_t time_ms);
  virtual uint32_t thread_exit_code(handle_t);
  virtual void thread_close(handle_t);

  virtual void thread_terminate(handle_t, uint32_t);
  virtual unsigned int thread_current_id(handle_t);
  virtual bool thread_yield(handle_t);
  virtual kotuku::thread_t::status_t thread_status(handle_t);

  // error functions
  virtual void set_last_error(result_t);
  virtual result_t get_last_error();

  // screen functions.
  virtual kotuku::screen_t *screen_create(kotuku::screen_t *, const kotuku::extent_t &);
  virtual kotuku::screen_t *screen_create(kotuku::screen_t *, const bitmap_t &);
  virtual kotuku::screen_t *screen_create(kotuku::screen_t *, const kotuku::rect_t &);
  virtual void screen_close(kotuku::window_t *);

  // can driver functions
  virtual result_t publish(const kotuku::can_msg_t &msg);
  virtual result_t set_can_provider(kotuku::canaerospace_provider_t *pdata);
  virtual result_t get_can_provider(kotuku::canaerospace_provider_t **);

  virtual int trace_level() const;

  kotuku::window_t *root_window();
  kotuku::window_t *menu_window();
  kotuku::window_t *alert_window();
protected:
  result_t _last_error;

  kotuku::canaerospace_provider_t *_provider;

  static int _trace_level;

  //  critical_section_t _cs;

  kotuku::window_t *_root_window;
  kotuku::window_t *_menu_window;
  kotuku::window_t *_alert_window;
  };

inline kotuku::window_t *ion_hal_t::root_window()
  {
  return _root_window;
  }

inline kotuku::window_t *ion_hal_t::menu_window()
  {
  return _menu_window;
  }

inline kotuku::window_t *ion_hal_t::alert_window()
  {
  return _alert_window;
  }
  
#endif
