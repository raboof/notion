/*
 * ion/ioncore/fullscreen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_FULLSCREEN_H
#define ION_IONCORE_FULLSCREEN_H

#include "common.h"
#include "screen.h"
#include "clientwin.h"

extern bool clientwin_check_fullscreen_request(WClientWin *cwin, int w, int h);
extern bool clientwin_fullscreen_scr(WClientWin *cwin, WScreen *vp,
									bool switchto);
extern bool clientwin_toggle_fullscreen(WClientWin *cwin);
extern bool clientwin_enter_fullscreen(WClientWin *cwin, bool switchto);
extern bool clientwin_leave_fullscreen(WClientWin *cwin, bool switchto);

#endif /* ION_IONCORE_FULLSCREEN_H */
