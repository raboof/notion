/*
 * ion/ioncore/fullscreen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FULLSCREEN_H
#define ION_IONCORE_FULLSCREEN_H

#include "common.h"
#include "screen.h"
#include "clientwin.h"

extern bool clientwin_check_fullscreen_request(WClientWin *cwin, 
											   int w, int h, bool switchto);
extern bool clientwin_fullscreen_scr(WClientWin *cwin, WScreen *vp,
									 bool switchto);
extern bool clientwin_toggle_fullscreen(WClientWin *cwin);
extern bool clientwin_enter_fullscreen(WClientWin *cwin, bool switchto);
extern bool clientwin_leave_fullscreen(WClientWin *cwin, bool switchto);

#endif /* ION_IONCORE_FULLSCREEN_H */
