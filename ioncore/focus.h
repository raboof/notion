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

#define SET_FOCUS(WIN) \
    {set_await_focus(find_window(WIN)); \
	XSetInputFocus(wglobal.dpy, WIN, RevertToParent, CurrentTime);}

extern void do_set_focus(WRegion *reg, bool warp);
extern void set_focus(WRegion *reg);
extern void warp(WRegion *reg);
extern WRegion *set_focus_mgrctl(WRegion *freg, bool dowarp);
extern void set_previous_of(WRegion *reg);
extern void goto_previous();
extern void protect_previous();
extern void unprotect_previous();
extern void goto_previous();

extern void do_warp(WRegion *reg);
extern bool do_warp_default(WRegion *reg);
extern WHooklist *do_warp_alt;

extern void set_await_focus(WRegion *reg);

#endif /* ION_IONCORE_FOCUS_H */
