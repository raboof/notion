/*
 * wmcore/event.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_EVENT_H
#define WMCORE_EVENT_H

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
					 StructureNotifyMask)


extern void get_event(XEvent *ev);
extern void get_event_mask(XEvent *ev, long mask);
extern void do_grab_kb_ptr(Window win, WRegion *reg, long eventmask);
extern void grab_kb_ptr(WRegion *reg);
extern void ungrab_kb_ptr();

#endif /* WMCORE_EVENT_H */
