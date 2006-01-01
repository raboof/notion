/*
 * ion/ioncore/frame.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FRAME_H
#define ION_IONCORE_FRAME_H

#include <libtu/stringstore.h>
#include <libtu/setparam.h>
#include <libextl/extl.h>

#include "common.h"
#include "window.h"
#include "attach.h"
#include "mplex.h"
#include "gr.h"
#include "rectangle.h"

#define FRAME_TAB_HIDE    0x0004
#define FRAME_SAVED_VERT  0x0008
#define FRAME_SAVED_HORIZ 0x0010
#define FRAME_SHADED      0x0020
#define FRAME_SETSHADED   0x0040
#define FRAME_BAR_OUTSIDE 0x0080
#define FRAME_DEST_EMPTY  0x0100
#define FRAME_MAXED_VERT  0x0200
#define FRAME_MAXED_HORIZ 0x0400
#define FRAME_MIN_HORIZ   0x0800
#define FRAME_SZH_USEMINMAX 0x1000

typedef void WFrameStyleFn(const char **, const char **);

DECLCLASS(WFrame){
    WMPlex mplex;
    
    int flags;
    int saved_w, saved_h;
    int saved_x, saved_y;
    
    int tab_dragged_idx;
    
    StringId style;
    GrBrush *brush;
    GrBrush *bar_brush;
    GrTransparency tr_mode;
    int bar_h;
    GrTextElem *titles;
    int titles_n;
};


/* Create/destroy */
extern WFrame *create_frame(WWindow *parent, const WFitParams *fp,
                            const char *style);
extern bool frame_init(WFrame *frame, WWindow *parent, const WFitParams *fp,
                       const char *style);
extern void frame_deinit(WFrame *frame);
extern bool frame_rqclose(WFrame *frame);

/* Resize and reparent */
extern bool frame_fitrep(WFrame *frame, WWindow *par, const WFitParams *fp);
extern void frame_resize_hints(WFrame *frame, XSizeHints *hints_ret);

/* Focus */
extern void frame_activated(WFrame *frame);
extern void frame_inactivated(WFrame *frame);

/* Tabs */
extern int frame_nth_tab_w(const WFrame *frame, int n);
extern int frame_nth_tab_iw(const WFrame *frame, int n);
extern int frame_nth_tab_x(const WFrame *frame, int n);
extern int frame_tab_at_x(const WFrame *frame, int x);
extern void frame_update_attr_nth(WFrame *frame, int i);

extern bool frame_set_tabbar(WFrame *frame, int sp);
extern bool frame_is_tabbar(WFrame *frame);
extern bool frame_set_shaded(WFrame *frame, int sp);
extern bool frame_is_shaded(WFrame *frame);

extern int frame_default_index(WFrame *frame);

/* Misc */
extern void frame_managed_notify(WFrame *frame, WRegion *sub);
extern void frame_managed_remove(WFrame *frame, WRegion *reg);

/* Save/load */
extern ExtlTab frame_get_configuration(WFrame *frame);
extern WRegion *frame_load(WWindow *par, const WFitParams *fp, ExtlTab tab);
extern void frame_do_load(WFrame *frame, ExtlTab tab);

/* This hook has WFrameChangedParams* (see below) as parameter. */
extern WHook *frame_managed_changed_hook;


#endif /* ION_IONCORE_FRAME_H */
