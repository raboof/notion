/*
 * ion/ioncore/netwm.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_NETWM_H
#define ION_IONCORE_NETWM_H

#include "common.h"

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */

extern void netwm_init();

extern int netwm_check_initial_fullscreen(WClientWin *cwin, bool switchto);
extern void netwm_update_state(WClientWin *cwin);
extern void netwm_state_change_rq(WClientWin *cwin, 
								  const XClientMessageEvent *ev);

#endif /* ION_IONCORE_NETWM_H */
