/*
 * ion/ioncore/pointer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_POINTER_H
#define ION_IONCORE_POINTER_H

#include "common.h"
#include "region.h"
#include "grab.h"

typedef void WButtonHandler(WRegion *reg, XButtonEvent *ev);
typedef void WMotionHandler(WRegion *reg, XMotionEvent *ev, int dx, int dy);

extern bool handle_button_press(XButtonEvent *ev);
extern bool handle_button_release(XButtonEvent *ev);
extern void handle_pointer_motion(XMotionEvent *ev);

extern XEvent *p_current_event();

extern bool p_set_drag_handlers(WRegion *reg, 
								WMotionHandler *begin,
								WMotionHandler *motion, 
								WButtonHandler *end,
								GrabHandler *handler,
								GrabKilledHandler *killhandler);

extern bool find_window_at(Window rootwin, int x, int y, Window *childret);
extern bool coords_in_rect(const WRectangle *g, int x, int y);

extern WRegion *pointer_grab_region();

#endif /* ION_IONCORE_POINTER_H */
