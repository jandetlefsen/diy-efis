/*
diy-efis
Copyright (C) 2016 Kotuku Aerospace Limited

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

If a file does not contain a copyright header, either because it is incomplete
or a binary file then the above copyright notice will apply.

Portions of this repository may have further copyright notices that may be
identified in the respective files.  In those cases the above copyright notice and
the GPL3 are subservient to that copyright notice.

Portions of this repository contain code fragments from the following
providers.


If any file has a copyright notice or portions of code have been used
and the original copyright notice is not yet transcribed to the repository
then the origional copyright notice is to be respected.

If any material is included in the repository that is not open source
it must be removed as soon as possible after the code fragment is identified.
*/
#ifndef __comm_device_h__
#define __comm_device_h__

#include "../gdi-lib/device.h"

namespace kotuku
  {
  class comm_device_t: public device_t
    {
  public:
    comm_device_t(const char *driver);

    void set_comm_state(int baud_rate);
  private:
    comm_device_t(const comm_device_t &);
    comm_device_t &operator =(const comm_device_t &);

    bool device_request(device_t::overlapped_t *);
    result_t device_cancel_io();
    result_t device_close();

    bool process_ioctl(device_t::overlapped_t *ol);
    bool process_read(device_t::overlapped_t *ol);
    bool process_write(device_t::overlapped_t *ol);

    int _hndl;

    // event handler for the driver
    event_t *_event_handler;
    };
  }

#endif
