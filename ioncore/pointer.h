/*
 * ion/ioncore/pointer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
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
