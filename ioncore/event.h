/*
 * ion/ioncore/event.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include "hooks.h"

#define IONCORE_EVENTMASK_PTRGRAB (ButtonPressMask|ButtonReleaseMask| \
                                   ButtonMotionMask)

#define IONCORE_EVENTMASK_PTRLOOP (IONCORE_EVENTMASK_PTRGRAB|ExposureMask| \
                                   KeyPressMask|KeyReleaseMask|            \
					               EnterWindowMask|FocusChangeMask)

#define IONCORE_EVENTMASK_ROOT	(SubstructureRedirectMask|          \
                                 ColormapChangeMask|                \
                                 ButtonPressMask|ButtonReleaseMask| \
                                 PropertyChangeMask|KeyPressMask|   \
                                 FocusChangeMask|EnterWindowMask)

#define IONCORE_EVENTMASK_FRAME	(FocusChangeMask|          \
                                 ButtonPressMask|          \
                                 ButtonReleaseMask|        \
                                 KeyPressMask|             \
                                 EnterWindowMask|          \
                                 ExposureMask|             \
                                 SubstructureRedirectMask)

#define IONCORE_EVENTMASK_CLIENTWIN (ColormapChangeMask|                  \
                                     PropertyChangeMask|FocusChangeMask|  \
                                     StructureNotifyMask|EnterWindowMask)

#define IONCORE_EVENTMASK_INPUT (ExposureMask|KeyPressMask|         \
                                 ButtonPressMask|ButtonReleaseMask| \
                                 FocusChangeMask)

extern void ioncore_mainloop();
extern void ioncore_get_event(XEvent *ev);
extern void ioncore_get_event_mask(XEvent *ev, long mask);
extern void ioncore_update_timestamp(XEvent *ev);
extern Time ioncore_get_timestamp();

extern WHooklist *ioncore_handle_event_alt;

#endif /* ION_IONCORE_EVENT_H */
