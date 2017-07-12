// Copyright (C) 2001 - 2008, 2011, 2013 Rob Caelers & Raymond Penners
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "UnixGrab.hh"

#include "debug.hh"
#include "utils/Platform.hh"

GdkDevice *UnixGrab::keyboard = nullptr;
GdkDevice *UnixGrab::pointer = nullptr;


UnixGrab::UnixGrab() :
  keyboard(nullptr),
  pointer(nullptr),
  grab_wanted(false),
  grabbed(false)
{

}

bool
UnixGrab::can_grab()
{
  return !Platform::running_on_wayland();
}

void
UnixGrab::grab()
{
  grab_wanted = true;
  if (!grabbed)
    {
      grabbed = grab_internal();
      if (! grabbed && !grab_retry_connection.connected())
        {
          grab_retry_connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &UniwGrab::on_grab_retry_timer), 2000);
        }
    }
}
#endif

bool
UnixGrab::grab_internal()
{
  TRACE_ENTER("UnixGrab::grab");
  bool ret = false;

  if (num_windows > 0)
    {
#if GTK_CHECK_VERSION(3, 20, 0)
      GdkGrabStatus status;

      GdkDisplay *display = gdk_display_get_default();
      GdkSeat *seat = gdk_display_get_default_seat(display);
      status = gdk_seat_grab(seat, windows[0], GDK_SEAT_CAPABILITY_ALL, TRUE, NULL, NULL, NULL, NULL);

      ret = status == GDK_GRAB_SUCCESS;
#else
      GdkDevice *device = gtk_get_current_event_device();
      if (device == nullptr)
        {
          GdkDisplay *display = gdk_window_get_display(windows[0]);
          GdkDeviceManager *device_manager = gdk_display_get_device_manager(display);
          device = gdk_device_manager_get_client_pointer(device_manager);
        }

      if (device != nullptr)
        {
          if (gdk_device_get_source(device) == GDK_SOURCE_KEYBOARD)
            {
              keyboard = device;
              pointer = gdk_device_get_associated_device(device);
            }
          else
            {
              pointer = device;
              keyboard = gdk_device_get_associated_device(device);
            }
        }

      GdkGrabStatus keybGrabStatus;
      keybGrabStatus = gdk_device_grab(keyboard, windows[0],
                                       GDK_OWNERSHIP_NONE, TRUE,
                                       (GdkEventMask) (GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK),
                                       nullptr, GDK_CURRENT_TIME);

      if (keybGrabStatus == GDK_GRAB_SUCCESS)
        {
          GdkGrabStatus pointerGrabStatus;
          pointerGrabStatus = gdk_device_grab(pointer, windows[0],
                                              GDK_OWNERSHIP_NONE, TRUE,
                                              (GdkEventMask) (GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK),
                                              nullptr, GDK_CURRENT_TIME);

          if (pointerGrabStatus != GDK_GRAB_SUCCESS)
            {
              gdk_device_ungrab(keyboard, GDK_CURRENT_TIME);
            }
          else
            {
              ret = true;
            }
        }
#endif
    }
#endif
  TRACE_EXIT();
  return ret;
}


void
UnixGrab::ungrab()
{
  grabbed = false;
  grab_wanted = false;
  grab_retry_connection.disconnect();

#if GTK_CHECK_VERSION(3, 20, 0)
  GdkDisplay *display = gdk_display_get_default();
  GdkSeat *seat = gdk_display_get_default_seat(display);
  gdk_seat_ungrab(seat);
#else
  if (keyboard != nullptr)
    {
      gdk_device_ungrab(keyboard, GDK_CURRENT_TIME);
      keyboard = nullptr;
    }
  if (pointer != nullptr)
    {
      gdk_device_ungrab(pointer, GDK_CURRENT_TIME);
      pointer = nullptr;
    }
#endif
}

bool
UnixGrab::on_grab_retry_timer()
{
  TRACE_ENTER("Grab::on_grab_retry_timer");
  bool ret = false;
  if (grab_wanted)
    {
      ret = grab();
    }
  else
    {
      ret = false;
    }
  TRACE_MSG(ret);
  TRACE_EXIT();
  return ret;
}
