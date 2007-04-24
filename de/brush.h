/*
 * ion/de/brush.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_DE_BRUSH_H
#define ION_DE_BRUSH_H

#include <libextl/extl.h>

#include <ioncore/common.h>
#include <ioncore/gr.h>
#include <ioncore/rectangle.h>

INTRCLASS(DEBrush);

#include "style.h"
#include "colour.h"


typedef void DEBrushExtrasFn(DEBrush *brush, 
                             const WRectangle *g, 
                             DEColourGroup *cg,
                             const GrBorderWidths *bdw,
                             const GrFontExtents *fnte,
                             const GrStyleSpec *a1,
                             const GrStyleSpec *a2,
                             bool pre, int index);

DECLCLASS(DEBrush){
    GrBrush grbrush;
    DEStyle *d;
    DEBrushExtrasFn *extras_fn;
    int indicator_w;
    Window win;
    bool clip_set;
    
    GrStyleSpec current_attr;
};

extern DEBrush *de_get_brush(Window win, WRootWin *rootwin,
                             const char *style);

extern DEBrush *create_debrush(Window win, 
                               const GrStyleSpec *spec, DEStyle *style);
extern bool debrush_init(DEBrush *brush, Window win,
                         const GrStyleSpec *spec, DEStyle *style);
extern void debrush_deinit(DEBrush *brush);

extern DEBrush *debrush_get_slave(DEBrush *brush, WRootWin *rootwin, 
                                  const char *style);

extern void debrush_release(DEBrush *brush);


extern DEColourGroup *debrush_get_colour_group2(DEBrush *brush, 
                                                const GrStyleSpec *a1,
                                                const GrStyleSpec *a2);

extern DEColourGroup *debrush_get_colour_group(DEBrush *brush, 
                                               const GrStyleSpec *attr);

extern DEColourGroup *debrush_get_current_colour_group(DEBrush *brush);

/* Begin/end */

extern void debrush_begin(DEBrush *brush, const WRectangle *geom, int flags);
extern void debrush_end(DEBrush *brush);

extern void debrush_init_attr(DEBrush *brush, const GrStyleSpec *spec);
extern void debrush_set_attr(DEBrush *brush, GrAttr attr);
extern void debrush_unset_attr(DEBrush *brush, GrAttr attr);
extern GrStyleSpec *debrush_get_current_attr(DEBrush *brush);

/* Information */

extern void debrush_get_border_widths(DEBrush *brush, GrBorderWidths *bdw);
extern bool debrush_get_extra(DEBrush *brush, const char *key, char type, 
                              void *data);

/* Borders & boxes */

extern void debrush_draw_border(DEBrush *brush, 
                                const WRectangle *geom);
extern void debrush_draw_borderline(DEBrush *brush, const WRectangle *geom,
                                    GrBorderLine line);

extern void debrush_draw_textbox(DEBrush *brush, const WRectangle *geom,
                                 const char *text, bool needfill);

extern void debrush_draw_textboxes(DEBrush *brush, const WRectangle *geom, 
                                   int n, const GrTextElem *elem, 
                                   bool needfill);

extern DEBrushExtrasFn debrush_tab_extras;
extern DEBrushExtrasFn debrush_menuentry_extras;

/* Misc */

extern void debrush_set_window_shape(DEBrush *brush, bool rough,
                                     int n, const WRectangle *rects);

extern void debrush_enable_transparency(DEBrush *brush, GrTransparency mode);

extern void debrush_fill_area(DEBrush *brush, const WRectangle *geom);
extern void debrush_clear_area(DEBrush *brush, const WRectangle *geom);


#endif /* ION_DE_BRUSH_H */
