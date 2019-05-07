/*
 * ion/ioncore/netwm.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_NETWM_H
#define ION_IONCORE_NETWM_H

#include "common.h"
#include "rootwin.h"
#include "screen.h"

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */

extern void netwm_init();
extern void netwm_init_rootwin(WRootWin *rw);

extern bool netwm_check_initial_fullscreen(WClientWin *cwin);
extern void netwm_update_state(WClientWin *cwin);
extern void netwm_update_allowed_actions(WClientWin *cwin);
extern void netwm_delete_state(WClientWin *cwin);
extern void netwm_set_active(WRegion *reg);
extern char **netwm_get_name(WClientWin *cwin);

extern void netwm_handle_client_message(const XClientMessageEvent *ev);
extern bool netwm_handle_property(WClientWin *cwin, const XPropertyEvent *ev);

extern void ioncore_screens_updated(WRootWin *rw);

#endif /* ION_IONCORE_NETWM_H */
