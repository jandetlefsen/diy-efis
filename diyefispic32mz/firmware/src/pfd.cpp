#include "../../../widgets/watchdog.h"
#include "../../../widgets/pfd_application.h"
#include "../../../widgets/layout_window.h"
#include "ion_hal.h"
#include "../../../widgets/layout_window.h"

static kotuku::layout_window_t *root_window;
static kotuku::watchdog_t *watchdog;
static kotuku::pfd_application_t *_the_app = 0;
static ion_hal_t *hal_impl = 0;

kotuku::application_t *kotuku::the_app()
  {
  return _the_app;
  }

kotuku::hal_t *kotuku::the_hal()
  {
  if(hal_impl == 0)
    hal_impl = new ion_hal_t();

  return hal_impl;
  }

static bool needs_vsync;

static bool watchdog_callback(void *parg)
  {
  return true;
  }

extern "C" void run()
  {
  kotuku::the_hal()->initialize("/a/efis.ini");
  _the_app = new kotuku::pfd_application_t(reinterpret_cast<layout_window_t *>(kotuku::the_hal()->root_window()));

  watchdog = new kotuku::watchdog_t(reinterpret_cast<layout_window_t *>(kotuku::the_hal()->root_window()), _the_app);

  // enable the application can thread
  _the_app->resume();

  // turn on all events
  _the_app->publishing_enabled(true);

  // start the application
  watchdog->run(watchdog_callback, 0, 17);
  }

