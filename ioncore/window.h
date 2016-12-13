/*
 * ion/ioncore/window.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
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
    long event_mask;
    WStacking *stacking;
};


extern bool window_init(WWindow *p, /*@notnull@*/ WWindow *parent,
                        const WFitParams *fp, const char *name);
/**
 * @param parent required when 'win' is 'None'.
 * @param win    the window to initialize. A new window is created when 'win'
 *               is 'None'.
 * @param name   the name of the newly created Window
 */
extern bool window_do_init(WWindow *p, WWindow *parent,
                           const WFitParams *fp, Window win, const char *name);
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

extern void window_select_input(WWindow *wwin, long event_mask);

#endif /* ION_IONCORE_WINDOW_H */
