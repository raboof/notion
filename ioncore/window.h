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
#include "region.h"
#include "binding.h"
#include "rectangle.h"


DECLCLASS(WWindow){
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

DYNFUN void window_draw(WWindow *wwin, bool complete);
DYNFUN void window_insstr(WWindow *wwin, const char *buf, size_t n);
DYNFUN int window_press(WWindow *wwin, XButtonEvent *ev, WRegion **reg_ret);
DYNFUN void window_release(WWindow *wwin);

/* Only to be used by regions that inherit this */
extern void window_map(WWindow *wwin);
extern void window_unmap(WWindow *wwin);
extern void window_do_set_focus(WWindow *wwin, bool warp);
extern void window_fit(WWindow *wwin, const WRectangle *geom);
extern bool window_reparent(WWindow *wwin, WWindow *parent, 
							const WRectangle *geom);

extern Window window_restack(WWindow *wwin, Window other, int mode);

DYNFUN bool region_reparent(WRegion *reg, WWindow *target, 
							const WRectangle *geom);


#define XWINDOW_REGION_OF_T(WIN, TYPE) (TYPE*)xwindow_region_of_t(WIN, &CLASSDESCR(TYPE))
#define XWINDOW_REGION_OF(WIN) xwindow_region_of(WIN)

extern WRegion *xwindow_region_of(Window win);
extern WRegion *xwindow_region_of_t(Window win, const ClassDescr *descr);

extern void xwindow_restack(Window win, Window other, int stack_mode);

#endif /* ION_IONCORE_WINDOW_H */
