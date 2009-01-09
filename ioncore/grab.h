/*
 * ion/ioncore/grab.h
 *
 * Copyright (c) Lukas Schroeder 2002,
 *               Tuomo Valkonen 2003-2009.
 *
 * See the included file LICENSE for details.
 *
 * Alternatively, you may apply the Clarified Artistic License to this file,
 * since Lukas' contributions were originally under that.
 */

#ifndef ION_IONCORE_GRAB_H
#define ION_IONCORE_GRAB_H

#include "global.h" /* for InputHandler and InputHandlerContext */
#include "common.h"
#include "region.h"

/* GrabHandler:
   the default_keyboard_handler now simplifies access to subsequent keypresses
   when you establish a grab using grab_establish().

   if your GrabHandler returns TRUE, your grab will be removed, otherwise it's
   kept active and you get more grabbed events passed to your handler.
 */
typedef bool GrabHandler(WRegion *reg, XEvent *ev);
typedef void GrabKilledHandler(WRegion *reg);

extern void ioncore_grab_establish(WRegion *reg, GrabHandler *func,
                                   GrabKilledHandler *kh,long eventmask);
extern void ioncore_grab_remove(GrabHandler *func);
extern void ioncore_grab_holder_remove(WRegion *holder);
extern WRegion *ioncore_grab_get_holder();
extern WRegion *ioncore_grab_get_my_holder(GrabHandler *func);
extern bool ioncore_grab_held();
extern void ioncore_change_grab_cursor(int cursor);
extern void ioncore_grab_confine_to(Window confine_to);

extern bool ioncore_handle_grabs(XEvent *ev);

#endif /* ION_IONCORE_GRAB_H */

