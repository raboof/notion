/*
 * ion/ioncore/grab.h
 *
 * Based on the contributed code "(c) Lukas Schroeder 2002".
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

extern bool call_grab_handler(XEvent *ev);

extern void grab_establish(WRegion *reg, GrabHandler *func, long eventmask);
extern void grab_remove(GrabHandler *func);
extern void grab_holder_remove(WRegion *holder);
extern WRegion *grab_get_holder();
extern WRegion *grab_get_my_holder(GrabHandler *func);
extern bool grab_held();
extern void change_grab_cursor(int cursor);
extern void grab_confine_to(Window confine_to);

#endif /* ION_IONCORE_GRAB_H */

