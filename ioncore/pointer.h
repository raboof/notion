/*
 * ion/ioncore/pointer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
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

extern bool ioncore_do_handle_buttonpress(XButtonEvent *ev);
extern bool ioncore_do_handle_buttonrelease(XButtonEvent *ev);
extern void ioncore_do_handle_motionnotify(XMotionEvent *ev);

extern XEvent *ioncore_current_pointer_event();
extern WRegion *ioncore_pointer_grab_region();

extern bool ioncore_set_drag_handlers(WRegion *reg, 
                                      WMotionHandler *begin,
                                      WMotionHandler *motion, 
                                      WButtonHandler *end,
                                      GrabHandler *handler,
                                      GrabKilledHandler *killhandler);

#endif /* ION_IONCORE_POINTER_H */
