/*
 * ion/de/brush.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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

#define MATCHES(S, A) (gr_stylespec_score(S, A)>0)
#define MATCHES2(S, A1, A2) (gr_stylespec_score2(S, A1, A2)>0)


typedef void DEBrushExtrasFn(DEBrush *brush, 
                             const WRectangle *g, DEColourGroup *cg,
                             GrBorderWidths *bdw,
                             GrFontExtents *fnte,
                             const char *a1, const char *a2,
                             bool pre);

DECLCLASS(DEBrush){
    GrBrush grbrush;
    DEStyle *d;
    DEBrushExtrasFn *extras_fn;
    int indicator_w;
    Window win;
    bool clip_set;
};

extern DEBrush *de_get_brush(Window win, WRootWin *rootwin,
                             const char *style);

extern DEBrush *create_debrush(Window win, 
                               const char *stylename, DEStyle *style);
extern bool debrush_init(DEBrush *brush, Window win,
                         const char *stylename, DEStyle *style);
extern void debrush_deinit(DEBrush *brush);

extern DEBrush *debrush_get_slave(DEBrush *brush, WRootWin *rootwin, 
                                  const char *style);

extern void debrush_release(DEBrush *brush);


extern DEColourGroup *debrush_get_colour_group2(DEBrush *brush, 
                                                const char *attr_p1,
                                                const char *attr_p2);

extern DEColourGroup *debrush_get_colour_group(DEBrush *brush, 
                                               const char *attr);


/* Begin/end */

extern void debrush_begin(DEBrush *brush, const WRectangle *geom, int flags);
extern void debrush_end(DEBrush *brush);

/* Information */

extern void debrush_get_border_widths(DEBrush *brush, GrBorderWidths *bdw);
extern bool debrush_get_extra(DEBrush *brush, const char *key, char type, 
                              void *data);

/* Borders & boxes */

extern void debrush_draw_border(DEBrush *brush, 
                                const WRectangle *geom,
                                const char *attrib);
extern void debrush_draw_borderline(DEBrush *brush, const WRectangle *geom,
                                    const char *attrib, GrBorderLine line);

extern void debrush_draw_textbox(DEBrush *brush, const WRectangle *geom,
                                 const char *text, const char *attr,
                                 bool needfill);

extern void debrush_draw_textboxes(DEBrush *brush, const WRectangle *geom, 
                                   int n, const GrTextElem *elem, 
                                   bool needfill, const char *common_attrib);

extern DEBrushExtrasFn debrush_tab_extras;
extern DEBrushExtrasFn debrush_menuentry_extras;

/* Misc */

extern void debrush_set_window_shape(DEBrush *brush, bool rough,
                                     int n, const WRectangle *rects);

extern void debrush_enable_transparency(DEBrush *brush, GrTransparency mode);

extern void debrush_fill_area(DEBrush *brush, const WRectangle *geom, 
                              const char *attr);
extern void debrush_clear_area(DEBrush *brush, const WRectangle *geom);


#endif /* ION_DE_BRUSH_H */
