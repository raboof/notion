/*
 * wmcore/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_FOCUS_H
#define WMCORE_FOCUS_H

#include "common.h"
#include "thing.h"
#include "window.h"

#define SET_FOCUS(WIN) \
	XSetInputFocus(wglobal.dpy, WIN, RevertToParent, CurrentTime);

extern void do_move_pointer_to(WRegion *reg);
extern void do_set_focus(WRegion *reg, bool warp);
extern void set_focus(WRegion *reg);
extern void warp(WRegion *reg);
extern void set_previous_of(WRegion *reg);
extern void goto_previous();
extern void protect_previous();
extern void unprotect_previous();
extern void goto_previous();

#endif /* WMCORE_FOCUS_H */
