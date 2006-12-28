/*
 * ion/ioncore/fullscreen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FULLSCREEN_H
#define ION_IONCORE_FULLSCREEN_H

#include <libtu/setparam.h>
#include "common.h"
#include "screen.h"
#include "clientwin.h"

#define REGION_IS_FULLSCREEN(REG) OBJ_IS(REGION_PARENT(REG), WScreen)

extern bool clientwin_check_fullscreen_request(WClientWin *cwin, 
                                               int w, int h, bool switchto);
extern bool region_fullscreen_scr(WRegion *reg, WScreen *vp, bool switchto);
extern bool region_enter_fullscreen(WRegion *reg, bool switchto);
extern bool region_leave_fullscreen(WRegion *reg, bool switchto);

extern bool clientwin_set_fullscreen(WClientWin *cwin, int sp);
extern bool clientwin_is_fullscreen(WClientWin *cwin);
extern bool clientwin_fullscreen_may_switchto(WClientWin *cwin);

#endif /* ION_IONCORE_FULLSCREEN_H */
