/*
 * ion/grab.h
 *
 * Copyright (c) Lukas Schroeder 2002.
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_GRAB_H
#define INCLUDED_GRAB_H

#include "global.h" /* for InputHandler and InputHandlerContext */
#include "common.h"
#include "thing.h"

/* GrabHandler:
   the default_keyboard_handler now simplifies access to subsequent keypresses
   when you establish a grab using grab_establish().

   if your GrabHandler returns TRUE, your grab will be removed, otherwise it's
   kept active and you get more grabbed events passed to your handler.
 */
typedef bool GrabHandler(WRegion *thing, XEvent *ev);

extern bool call_grab_handler(XEvent *ev);

extern void grab_establish(WRegion *thing, GrabHandler *func, long eventmask);
extern void grab_remove(GrabHandler *func);
extern void grab_holder_remove(WRegion *holder);
extern WRegion *grab_get_holder();
extern WRegion *grab_get_my_holder(GrabHandler *func);
extern void grab_suspend();
extern void grab_resume();
extern bool grab_held();


#endif /* INCLUDED_GRAB_H */

