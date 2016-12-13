/*
 * ion/ioncore/eventh.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
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
