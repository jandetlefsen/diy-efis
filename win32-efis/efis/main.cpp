#include "../../gdi-lib/stddef.h"
#include "../../widgets/pfd_application.h"
#include "../../widgets/watchdog.h"
#include "../../gdi-lib/can_aerospace.h"
#include "../../gdi-lib/screen.h"
#include "../../widgets/menu_window.h"
#include "gdi_screen.h"

#ifdef RGB
#undef RGB
#endif

#include "win32_hal.h"

#include <windows.h>


static kotuku::watchdog_t *watchdog;
static kotuku::pfd_application_t *_the_app = 0;

kotuku::application_t *kotuku::the_app()
  {
  return _the_app;
  }

kotuku::hal_t *kotuku::the_hal()
  {
  static kotuku::win32_hal_t *hal_impl = 0;
  if(hal_impl == 0)
    hal_impl = new kotuku::win32_hal_t();

  return hal_impl;
  }

static kotuku::canaerospace_provider_t *provider = 0;

static void send_msg(uint16_t message_id, short value)
  {
  if (provider == 0)
    kotuku::the_hal()->get_can_provider(&provider);

  if (provider != 0)
    {
    kotuku::can_msg_t msg;
    create_can_msg_short(&msg, message_id, 0, value);
    provider->receive(msg);
    }
  }

#ifdef _WIN32_WCE
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
#else
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
#endif
  {
  reinterpret_cast<kotuku::win32_hal_t *>(kotuku::the_hal())->initialize("..\\..\\configs\\efis.ini");
  _the_app = new kotuku::pfd_application_t(kotuku::the_hal()->root_window());

  watchdog = new kotuku::watchdog_t(kotuku::the_hal()->root_window(), _the_app);

  // turn on all events
  _the_app->publishing_enabled(true);

  // we need to open the menu window (not visible) so the menu's are created
  kotuku::the_hal()->menu_window()->load_menus(_the_app);

  _the_app->root_window()->paint(true);
  int metric;

  SetTimer(NULL, NULL, 200, NULL);

  MSG msg;
  while(GetMessageW(&msg, NULL, 0, 0))
    {
    switch(msg.message)
      {
      case WM_PAINT :
      case WM_TIMER :
        kotuku::gdi_screen_t::gdi_metric(0);

        _the_app->root_window()->paint(false);

        metric = kotuku::gdi_screen_t::gdi_metric();
        break;
      case WM_KEYDOWN :
        /* We use the keyboard to emulate the diy-efis board adapter.
         *
         *  F1 -> button 1
         *  F2 -> button 2
         *  F3 -> button 3
         *  F4 -> button 4
         *  UP -> decka up
         *  DOWN -> decka dn
         *  RIGHT -> deckb up
         *  LEFT -> deckb dn
         */
        switch (msg.wParam)
          {
          case VK_UP :
            send_msg(id_decka, 1);
            break;
          case VK_DOWN :
            send_msg(id_decka, -1);
            break;
          case VK_LEFT :
            send_msg(id_deckb, -1);
            break;
          case VK_RIGHT :
            send_msg(id_deckb, 1);
            break;
          case VK_F1 :
            send_msg(id_key0, 1);
            break;
          case VK_F2 :
            send_msg(id_key1, 1);
            break;
          case VK_F3 :
            send_msg(id_key2, 1);
            break;
          case VK_F4 :
            send_msg(id_key3, 1);
            break;
          }
        break;
      default :
        DispatchMessage(&msg);
        break;
      }
    }
  return 0;
  }
