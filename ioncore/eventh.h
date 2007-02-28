/*
 * ion/ioncore/eventh.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_EVENTH_H
#define ION_IONCORE_EVENTH_H

#include "common.h"

extern bool ioncore_handle_event(XEvent *ev);
extern void ioncore_handle_expose(const XExposeEvent *ev);
extern void ioncore_handle_map_request(const XMapRequestEvent *ev);
extern void ioncore_handle_configure_request(XConfigureRequestEvent *ev);
extern void ioncore_handle_enter_window(XEvent *ev);
extern void ioncore_handle_unmap_notify(const XUnmapEvent *ev);
extern void ioncore_handle_destroy_notify(const XDestroyWindowEvent *ev);
extern void ioncore_handle_client_message(const XClientMessageEvent *ev);
extern void ioncore_handle_focus_in(const XFocusChangeEvent *ev);
extern void ioncore_handle_focus_out(const XFocusChangeEvent *ev);
extern void ioncore_handle_property(const XPropertyEvent *ev);
extern void ioncore_handle_buttonpress(XEvent *ev);
extern void ioncore_handle_keyboard(XEvent *ev);
extern void ioncore_handle_mapping_notify(XEvent *ev);

#endif /* ION_IONCORE_EVENTH_H */
