/*
 * wmcore/window.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_WINDOW_H
#define WMCORE_WINDOW_H

#include "common.h"

INTROBJ(WWindow)

#include "region.h"
#include "binding.h"

#define WWINDOW_UNMAPPABLE	0x0001
#define WWINDOW_MAPPED		0x0002
#define WWINDOW_UNFOCUSABLE	0x0004
#define WWINDOW_WFORCED		0x0010
#define WWINDOW_HFORCED		0x0020

#define FIND_WINDOW_T(WIN, TYPE) (TYPE*)find_window_t(WIN, &OBJDESCR(TYPE))
#define FIND_WINDOW(WIN) find_window(WIN)


DECLOBJ(WWindow){
	WRegion region;
	int flags;
	Window win;
	XIC xic;
};


#include "screen.h"


extern bool init_window(WWindow *p, WScreen *scr, Window win, WRectangle geom);
extern bool init_window_new(WWindow *p, WScreen *scr, WWinGeomParams params);
extern void deinit_window(WWindow *win);

extern WThing *find_window(Window win);
extern WThing *find_window_t(Window win, const WObjDescr *descr);

DYNFUN void draw_window(WWindow *wwin, bool complete);
DYNFUN void window_insstr(WWindow *wwin, const char *buf, size_t n);
DYNFUN int window_press(WWindow *wwin, XButtonEvent *ev);
DYNFUN void window_release(WWindow *wwin);

/* Only to be used by regions that inherit this */
extern void map_window(WWindow *wwin);
extern void unmap_window(WWindow *wwin);
extern void focus_window(WWindow *wwin, bool warp);
extern void fit_window(WWindow *wwin, WRectangle geom);
extern bool reparent_window(WWindow *wwin, WWinGeomParams params);
extern void window_rect_params(WWindow *wwin, WRectangle geom,
							   WWinGeomParams *ret);
extern Window window_restack(WWindow *wwin, Window other, int mode);
extern Window window_lowest_win(WWindow *wwin);

extern void do_restack_window(Window win, Window other, int stack_mode);

#endif /* WMCORE_WINDOW_H */
