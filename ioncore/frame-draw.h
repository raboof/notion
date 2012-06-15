/*
 * ion/ioncore/frame-draw.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_FRAME_DRAW_H
#define ION_IONCORE_FRAME_DRAW_H

#include "frame.h"
#include "rectangle.h"

extern void frame_draw(const WFrame *frame, bool complete);
extern void frame_draw_bar(const WFrame *frame, bool complete);
extern void frame_recalc_bar(WFrame *frame);
extern void frame_bar_geom(const WFrame *frame, WRectangle *geom);
extern void frame_border_geom(const WFrame *frame, WRectangle *geom);
extern void frame_border_inner_geom(const WFrame *frame, WRectangle *geom);
extern void frame_brushes_updated(WFrame *frame);
extern void frame_managed_geom(const WFrame *frame, WRectangle *geom);

extern void frame_initialise_gr(WFrame *frame);
extern void frame_release_brushes(WFrame *frame);
extern bool frame_set_background(WFrame *frame, bool set_always);
extern void frame_updategr(WFrame *frame);

extern void frame_set_shape(WFrame *frame);
extern void frame_clear_shape(WFrame *frame);

extern const char *framemode_get_style(WFrameMode mode);
extern const char *framemode_get_tab_style(WFrameMode mode);

extern void frame_update_attr(WFrame *frame, int i, WRegion *reg);

extern void frame_setup_dragwin_style(WFrame *frame, GrStyleSpec *spec, int tab);

extern void frame_inactivated(WFrame *frame);
extern void frame_activated(WFrame *frame);
extern void frame_quasiactivity_change(WFrame *frame);

/* 'elastic tab sizes' logic 
 *
 * bar_w: total width of the bar. The width of the tabs of the children should
 * add up to this.
 */
extern void frame_get_max_width_and_elastic(WFrame * frame,int bar_w,int *maxw,
    int *elastic, int *minw);

#endif /* ION_IONCORE_FRAME_DRAW_H */
