/*
 * ion/ioncore/window.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_WINDOW_H
#define ION_IONCORE_WINDOW_H

#include "common.h"

INTROBJ(WWindow);

#include "region.h"
#include "binding.h"

#define FIND_WINDOW_T(WIN, TYPE) (TYPE*)find_window_t(WIN, &OBJDESCR(TYPE))
#define FIND_WINDOW(WIN) find_window(WIN)


DECLOBJ(WWindow){
	WRegion region;
	Window win;
	XIC xic;
	WRegion *keep_on_top_list;
};


extern bool window_init_new(WWindow *p, WWindow *parent, 
							const WRectangle *geom);
extern bool window_init(WWindow *p, WWindow *parent, Window win, 
						const WRectangle *geom);
extern void window_deinit(WWindow *win);

extern WRegion *find_window(Window win);
extern WRegion *find_window_t(Window win, const WObjDescr *descr);

DYNFUN void window_draw(WWindow *wwin, bool complete);
DYNFUN void window_insstr(WWindow *wwin, const char *buf, size_t n);
DYNFUN int window_press(WWindow *wwin, XButtonEvent *ev, WRegion **reg_ret);
DYNFUN void window_release(WWindow *wwin);

/* Only to be used by regions that inherit this */
extern void window_map(WWindow *wwin);
extern void window_unmap(WWindow *wwin);
extern void window_set_focus_to(WWindow *wwin, bool warp);
extern void window_fit(WWindow *wwin, const WRectangle *geom);
extern bool reparent_window(WWindow *wwin, WWindow *parent, 
							const WRectangle *geom);

extern Window window_restack(WWindow *wwin, Window other, int mode);
extern void do_restack_window(Window win, Window other, int stack_mode);

DYNFUN bool reparent_region(WRegion *reg, WWindow *target, 
							const WRectangle *geom);

#endif /* ION_IONCORE_WINDOW_H */
