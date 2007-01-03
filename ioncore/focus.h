/*
 * ion/ioncore/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FOCUS_H
#define ION_IONCORE_FOCUS_H

#include <libmainloop/hooks.h>
#include "common.h"
#include "window.h"
#include "region.h"

DYNFUN void region_do_set_focus(WRegion *reg, bool warp);

/* Delayed (until return to main loop) warp/focus */
extern void region_warp(WRegion *reg);
extern void region_set_focus(WRegion *reg);
extern void region_maybewarp(WRegion *reg, bool warp);

/* Immediate warp/focus */
extern void region_do_warp(WRegion *reg);
extern bool region_do_warp_default(WRegion *reg);

extern void region_finalise_focusing(WRegion* reg, Window win, bool warp);

/* Awaiting focus state */
extern void region_set_await_focus(WRegion *reg);
extern bool ioncore_await_focus();

/* Event handling */
extern void region_got_focus(WRegion *reg);
extern void region_lost_focus(WRegion *reg);

/* May reg transfer focus to its children? */
extern bool region_may_control_focus(WRegion *reg);

/* Does reg have focus? */
extern bool region_is_active(WRegion *reg);

/* Focus history */
extern void region_focuslist_remove_with_mgrs(WRegion *reg);
extern void region_focuslist_push(WRegion *reg);
extern void region_focuslist_move_after(WRegion *reg, WRegion *after);
extern void region_focuslist_deinit(WRegion *reg);

extern WRegion *ioncore_goto_previous();

/* Handlers to these shook should take WRegion* as parameter. */
extern WHook *region_do_warp_alt;
extern WHook *region_activated_hook;
extern WHook *region_inactivated_hook;

/* Misc. */
extern bool region_skip_focus(WRegion *reg);
WRegion *ioncore_current();

#endif /* ION_IONCORE_FOCUS_H */
