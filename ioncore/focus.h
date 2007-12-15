/*
 * ion/ioncore/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_FOCUS_H
#define ION_IONCORE_FOCUS_H

#include <libmainloop/hooks.h>
#include "common.h"
#include "window.h"
#include "region.h"


/* Delayed (until return to main loop) warp/focus */
extern void region_maybewarp(WRegion *reg, bool warp);
/* warp/focus now; do not skip enter window events etc. in mainloop */
extern void region_maybewarp_now(WRegion *reg, bool warp);

extern void region_warp(WRegion *reg); /* maybewarp TRUE */
extern void region_set_focus(WRegion *reg); /* maybewarp FALSE */

extern void region_finalise_focusing(WRegion* reg, Window win, bool warp);

DYNFUN void region_do_set_focus(WRegion *reg, bool warp);
extern void region_do_warp(WRegion *reg);
extern bool region_do_warp_default(WRegion *reg);

/* Awaiting focus state */
extern void region_set_await_focus(WRegion *reg);
extern WRegion *ioncore_await_focus();

/* Event handling */
extern void region_got_focus(WRegion *reg);
extern void region_lost_focus(WRegion *reg);

/* May reg transfer focus to its children? */
extern bool region_may_control_focus(WRegion *reg);

/* Does reg have focus? */
extern bool region_is_active(WRegion *reg, bool pseudoact_ok);

/* Focus history */
extern void region_focuslist_remove_with_mgrs(WRegion *reg);
extern void region_focuslist_push(WRegion *reg);
extern void region_focuslist_move_after(WRegion *reg, WRegion *after);
extern void region_focuslist_deinit(WRegion *reg);

extern WRegion *ioncore_goto_previous();

/* Handlers to this hook should take WRegion* as parameter. */
extern WHook *region_do_warp_alt;

/* Misc. */
extern bool region_skip_focus(WRegion *reg);
WRegion *ioncore_current();

extern void region_pointer_focus_hack(WRegion *reg);

#endif /* ION_IONCORE_FOCUS_H */
