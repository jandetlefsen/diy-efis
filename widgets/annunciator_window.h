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
#ifndef __annunciator_window_h__
#define __annunciator_window_h__

#include "widget.h"

namespace kotuku {
class annunciator_window_t : public widget_t
  {
public:
  annunciator_window_t(widget_t &parent, const char *section);

  // minutes the airctaft has been operating
  uint32_t hours() const;
  short qnh() const;
private:
  void update_window();
  virtual bool ev_msg(const msg_t &);

  void draw_annunciator_background(const point_t &pt, const char *label);
  void draw_annunciator_detail(const point_t &pt, const char *value, size_t len);

  uint32_t _hours; // hobbs hours, stored in AHRS as hours * 100
  short _qnh; // qnh, stored in AHRS
  short _clock;
  short _oat;
  short _cas;

  canvas_t _background_canvas;
  };
  };

#endif
