/*
 * ion/ioncore/frame-draw.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FRAME_DRAW_H
#define ION_IONCORE_FRAME_DRAW_H

#include "frame.h"
#include "rectangle.h"

DYNFUN void frame_draw_bar(const WFrame *frame, bool complete);
DYNFUN void frame_recalc_bar(WFrame *frame);
DYNFUN void frame_bar_geom(const WFrame *frame, WRectangle *geom);
DYNFUN void frame_border_geom(const WFrame *frame, WRectangle *geom);
DYNFUN void frame_border_inner_geom(const WFrame *frame, WRectangle *geom);
DYNFUN void frame_brushes_updated(WFrame *frame);

extern void frame_draw_default(const WFrame *frame, bool complete);
extern void frame_draw_bar_default(const WFrame *frame, bool complete);
extern void frame_recalc_bar_default(WFrame *frame);
extern void frame_border_geom_default(const WFrame *frame, WRectangle *geom);
extern void frame_border_inner_geom_default(const WFrame *frame, WRectangle *geom);
extern void frame_bar_geom_default(const WFrame *frame, WRectangle *geom);
extern void frame_managed_geom_default(const WFrame *frame, WRectangle *geom);
extern void frame_brushes_updated_default(WFrame *frame);

extern void frame_initialise_gr(WFrame *frame);
extern void frame_release_brushes(WFrame *frame);
extern bool frame_set_background(WFrame *frame, bool set_always);
extern void frame_updategr(WFrame *frame);

extern void frame_set_shape(WFrame *frame);
extern void frame_clear_shape(WFrame *frame);

extern const char *framemode_get_style(WFrameMode mode);
extern const char *framemode_get_tab_style(WFrameMode mode);
extern bool framemode_from_style(WFrameMode *mode, const char *str);

#endif /* ION_IONCORE_FRAME_DRAW_H */
