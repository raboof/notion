/*
 * ion/ioncore/window.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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
};


extern bool window_init(WWindow *p, WWindow *parent, 
                        const WFitParams *fp);
extern bool window_do_init(WWindow *p, WWindow *parent, Window win, 
                           const WFitParams *fp);
extern void window_deinit(WWindow *win);

DYNFUN void window_draw(WWindow *wwin, bool complete);
DYNFUN void window_insstr(WWindow *wwin, const char *buf, size_t n);
DYNFUN int window_press(WWindow *wwin, XButtonEvent *ev, WRegion **reg_ret);
DYNFUN void window_release(WWindow *wwin);

/* Only to be used by regions that inherit this */
extern void window_map(WWindow *wwin);
extern void window_unmap(WWindow *wwin);

extern void window_do_set_focus(WWindow *wwin, bool warp);

extern void window_do_fitrep(WWindow *wwin, WWindow *parent,
                             const WRectangle *geom);
extern bool window_fitrep(WWindow *wwin, WWindow *parent, 
                          const WFitParams *fp);
extern void window_notify_subs_move(WWindow *wwin);

extern void window_restack(WWindow *wwin, Window other, int mode);

#endif /* ION_IONCORE_WINDOW_H */
