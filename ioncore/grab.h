/*
 * ion/ioncore/grab.h
 *
 * Copyright (c) Lukas Schroeder 2002,
 *				 Tuomo Valkonen 2003.
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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

extern bool call_grab_handler(XEvent *ev);

extern void grab_establish(WRegion *reg, GrabHandler *func,
						   GrabKilledHandler *kh,long eventmask);
extern void grab_remove(GrabHandler *func);
extern void grab_holder_remove(WRegion *holder);
extern WRegion *grab_get_holder();
extern WRegion *grab_get_my_holder(GrabHandler *func);
extern bool grab_held();
extern void change_grab_cursor(int cursor);
extern void grab_confine_to(Window confine_to);

#endif /* ION_IONCORE_GRAB_H */

