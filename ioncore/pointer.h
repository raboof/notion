/*
 * ion/ioncore/pointer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_POINTER_H
#define ION_IONCORE_POINTER_H

#include "common.h"
#include "region.h"

typedef void WButtonHandler(WRegion *reg, XButtonEvent *ev);
typedef void WMotionHandler(WRegion *reg, XMotionEvent *ev, int dx, int dy);

extern bool handle_button_press(XButtonEvent *ev);
extern bool handle_button_release(XButtonEvent *ev);
extern void handle_pointer_motion(XMotionEvent *ev);

extern bool find_window_at(Window rootwin, int x, int y, Window *childret);

extern bool set_button_handler(WRegion *reg, WButtonHandler *handler);
extern bool set_drag_handlers(WRegion *reg, WMotionHandler *begin,
							  WMotionHandler *motion, WButtonHandler *end);

#endif /* ION_IONCORE_POINTER_H */
