/*
 * ion/ioncore/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FOCUS_H
#define ION_IONCORE_FOCUS_H

#include "common.h"
#include "window.h"
#include "region.h"
#include "hooks.h"

DYNFUN void region_do_set_focus(WRegion *reg, bool warp);

/* Delayed (until return to main loop) warp/focus */
extern void region_warp(WRegion *reg);
extern void region_set_focus(WRegion *reg);
extern WRegion *region_set_focus_mgrctl(WRegion *freg, bool dowarp);

/* Immediate warp/focus */
extern void region_do_warp(WRegion *reg);
extern bool region_do_warp_default(WRegion *reg);
extern WHooklist *region_do_warp_alt;
extern void xwindow_do_set_focus(Window win);

/* Awaiting focus state */
extern void region_set_await_focus(WRegion *reg);

/* Event handling */
extern void region_got_focus(WRegion *reg);
extern void region_lost_focus(WRegion *reg);

/* May reg transfer focus to its children? */
extern bool region_may_control_focus(WRegion *reg);

/* Does reg have focus? */
extern bool region_is_active(WRegion *reg);

/* Previously active region tracking */
extern void ioncore_set_previous_of(WRegion *reg);
extern void ioncore_goto_previous();
extern void ioncore_protect_previous();
extern void ioncore_unprotect_previous();

#endif /* ION_IONCORE_FOCUS_H */
