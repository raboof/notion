/*
 * ion/ioncore/window.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_WINDOW_H
#define ION_IONCORE_WINDOW_H

#include "common.h"

INTROBJ(WWindow)

#include "region.h"
#include "binding.h"

#define FIND_WINDOW_T(WIN, TYPE) (TYPE*)find_window_t(WIN, &OBJDESCR(TYPE))
#define FIND_WINDOW(WIN) find_window(WIN)


DECLOBJ(WWindow){
	WRegion region;
	Window win;
	WExtraDrawInfo *draw;
	XIC xic;
};


#include "screen.h"


extern bool init_window(WWindow *p, WWindow *parent,
						Window win, WRectangle geom);
extern bool init_window_new(WWindow *p, WWindow *parent, WRectangle geom);
extern void deinit_window(WWindow *win);

extern WThing *find_window(Window win);
extern WThing *find_window_t(Window win, const WObjDescr *descr);

DYNFUN void draw_window(WWindow *wwin, bool complete);
DYNFUN void window_insstr(WWindow *wwin, const char *buf, size_t n);
DYNFUN int window_press(WWindow *wwin, XButtonEvent *ev, WThing **thing_ret);
DYNFUN void window_release(WWindow *wwin);

/* Only to be used by regions that inherit this */
extern void map_window(WWindow *wwin);
extern void unmap_window(WWindow *wwin);
extern void focus_window(WWindow *wwin, bool warp);
extern void fit_window(WWindow *wwin, WRectangle geom);
extern bool reparent_window(WWindow *wwin, WWindow *parent, WRectangle geom);

extern Window window_restack(WWindow *wwin, Window other, int mode);
extern Window window_lowest_win(WWindow *wwin);
extern void do_restack_window(Window win, Window other, int stack_mode);

DYNFUN bool reparent_region(WRegion *reg, WWindow *target, WRectangle geom);

#endif /* ION_IONCORE_WINDOW_H */
