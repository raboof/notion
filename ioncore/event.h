/*
 * ion/ioncore/event.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_EVENT_H
#define ION_IONCORE_EVENT_H

#include "common.h"
#include "region.h"

#define GRAB_POINTER_MASK (ButtonPressMask|ButtonReleaseMask|\
						   ButtonMotionMask)

#define ROOT_MASK	(SubstructureRedirectMask|          \
					 ColormapChangeMask|                \
					 ButtonPressMask|ButtonReleaseMask| \
					 PropertyChangeMask|KeyPressMask|   \
					 FocusChangeMask|EnterWindowMask)

#define FRAME_MASK	(FocusChangeMask|          \
					 ButtonPressMask|          \
					 ButtonReleaseMask|        \
					 KeyPressMask|             \
					 EnterWindowMask|          \
					 ExposureMask|             \
					 SubstructureRedirectMask)

#define CLIENT_MASK (ColormapChangeMask| \
					 PropertyChangeMask|FocusChangeMask| \
					 StructureNotifyMask|EnterWindowMask)


extern void get_event(XEvent *ev);
extern void get_event_mask(XEvent *ev, long mask);
extern void do_grab_kb_ptr(Window win, Window confine_to, int cursor,
						   long eventmask);
/*extern void grab_kb_ptr(WRegion *reg);*/
extern void ungrab_kb_ptr();
extern void update_timestamp(XEvent *ev);
extern Time get_timestamp();

#endif /* ION_IONCORE_EVENT_H */
