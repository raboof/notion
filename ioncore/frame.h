/*
 * ion/ioncore/frame.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FRAME_H
#define ION_IONCORE_FRAME_H

#include "common.h"
#include "window.h"
#include "attach.h"
#include "mplex.h"
#include "gr.h"
#include "extl.h"
#include "rectangle.h"

#define FRAME_TAB_HIDE    0x0004
#define FRAME_SAVED_VERT  0x0008
#define FRAME_SAVED_HORIZ 0x0010
#define FRAME_SHADED      0x0020
#define FRAME_SETSHADED      0x0040
#define FRAME_BAR_OUTSIDE 0x0080

DECLCLASS(WFrame){
    WMPlex mplex;
    
    int flags;
    int saved_w, saved_h;
    int saved_x, saved_y;
    
    int tab_dragged_idx;
    
    GrBrush *brush;
    GrBrush *bar_brush;
    GrTransparency tr_mode;
    int bar_h;
    GrTextElem *titles;
    int titles_n;
};


/* Create/destroy */
extern WFrame *create_frame(WWindow *parent, const WRectangle *geom);
extern bool frame_init(WFrame *frame, WWindow *parent,
                          const WRectangle *geom);
extern void frame_deinit(WFrame *frame);
extern void frame_close(WFrame *frame);

/* Resize and reparent */
extern bool frame_reparent(WFrame *frame, WWindow *parent,
                              const WRectangle *geom);
extern void frame_fit(WFrame *frame, const WRectangle *geom);
extern void frame_resize_hints(WFrame *frame, XSizeHints *hints_ret,
                                  uint *relw_ret, uint *relh_ret);

/* Focus */
extern void frame_activated(WFrame *frame);
extern void frame_inactivated(WFrame *frame);

/* Tabs */
extern int frame_nth_tab_w(const WFrame *frame, int n);
extern int frame_nth_tab_iw(const WFrame *frame, int n);
extern int frame_nth_tab_x(const WFrame *frame, int n);
extern int frame_tab_at_x(const WFrame *frame, int x);
extern void frame_update_attr_nth(WFrame *frame, int i);

extern bool frame_toggle_tabbar(WFrame *frame);
extern bool frame_toggle_shade(WFrame *frame);
extern bool frame_is_shaded(WFrame *frame);
extern bool frame_has_tabbar(WFrame *frame);

/* Misc */
extern ExtlTab frame_get_configuration(WFrame *frame);
extern WRegion *frame_load(WWindow *par, const WRectangle *geom, ExtlTab tab);
extern void frame_do_load(WFrame *frame, ExtlTab tab);

#endif /* ION_IONCORE_FRAME_H */
