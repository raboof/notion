/*
 * ion/ioncore/netwm.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_NETWM_H
#define ION_IONCORE_NETWM_H

#include "common.h"
#include "rootwin.h"

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */

extern void netwm_init();
extern void netwm_init_rootwin(WRootWin *rw);

extern int netwm_check_initial_fullscreen(WClientWin *cwin, bool switchto);
extern void netwm_update_state(WClientWin *cwin);
extern void netwm_delete_state(WClientWin *cwin);
extern void netwm_set_active(WRegion *reg);
extern char **netwm_get_name(WClientWin *cwin);

extern void netwm_handle_client_message(const XClientMessageEvent *ev);
extern void netwm_handle_property(WClientWin *cwin, const XPropertyEvent *ev);

#endif /* ION_IONCORE_NETWM_H */
