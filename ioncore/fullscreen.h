/*
 * ion/ioncore/fullscreen.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_FULLSCREEN_H
#define ION_IONCORE_FULLSCREEN_H

#include <libtu/setparam.h>
#include "common.h"
#include "screen.h"
#include "clientwin.h"

#define REGION_IS_FULLSCREEN(REG) OBJ_IS(REGION_PARENT(REG), WScreen)

extern WScreen *clientwin_fullscreen_chkrq(WClientWin *cwin, int w, int h);
extern bool clientwin_fullscreen_may_switchto(WClientWin *cwin);

extern bool region_fullscreen_scr(WRegion *reg, WScreen *vp, bool switchto);
extern bool region_enter_fullscreen(WRegion *reg, bool switchto);
extern bool region_leave_fullscreen(WRegion *reg, bool switchto);


#endif /* ION_IONCORE_FULLSCREEN_H */
