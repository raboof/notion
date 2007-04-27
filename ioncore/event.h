/*
 * ion/ioncore/event.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_EVENT_H
#define ION_IONCORE_EVENT_H

#include <libmainloop/hooks.h>
#include "common.h"
#include "region.h"

#define IONCORE_EVENTMASK_PTRGRAB (ButtonPressMask|ButtonReleaseMask| \
                                   ButtonMotionMask)

#define IONCORE_EVENTMASK_PTRLOOP (IONCORE_EVENTMASK_PTRGRAB|ExposureMask| \
                                   KeyPressMask|KeyReleaseMask|            \
                                   EnterWindowMask|FocusChangeMask)

#define IONCORE_EVENTMASK_NORMAL (ExposureMask|KeyPressMask|         \
                                  ButtonPressMask|ButtonReleaseMask| \
                                  FocusChangeMask|EnterWindowMask)

#define IONCORE_EVENTMASK_CWINMGR (IONCORE_EVENTMASK_NORMAL| \
                                   SubstructureRedirectMask)

#define IONCORE_EVENTMASK_ROOT (IONCORE_EVENTMASK_CWINMGR| \
                                PropertyChangeMask|ColormapChangeMask)

#define IONCORE_EVENTMASK_CLIENTWIN (ColormapChangeMask|                  \
                                     PropertyChangeMask|FocusChangeMask|  \
                                     StructureNotifyMask|EnterWindowMask)

#define IONCORE_EVENTMASK_SCREEN (FocusChangeMask|EnterWindowMask|   \
                                  KeyPressMask|KeyReleaseMask|       \
                                  ButtonPressMask|ButtonReleaseMask)

extern void ioncore_x_connection_handler(int conn, void *unused);
extern void ioncore_flush();
extern void ioncore_get_event(XEvent *ev, long mask);

extern void ioncore_update_timestamp(XEvent *ev);
extern Time ioncore_get_timestamp();

/* Handlers to this hook should take XEvent* as parameter. */
extern WHook *ioncore_handle_event_alt;

extern void ioncore_mainloop();

#endif /* ION_IONCORE_EVENT_H */
