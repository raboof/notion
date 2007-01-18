/*
 * ion/ioncore/gr.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GR_H
#define ION_IONCORE_GR_H

#include "common.h"
#include "rectangle.h"


INTRCLASS(GrBrush);
DECLCLASS(GrBrush){
    Obj obj;
};


#include "rootwin.h"

/* Types */

#define GR_FONT_EXTENTS_INIT {0, 0, 0}

typedef struct{
    uint max_height;
    uint max_width;
    uint baseline;
} GrFontExtents;

#define GR_BORDER_WIDTHS_INIT {0, 0, 0, 0, 0, 0, 0}

typedef struct{
    uint top, bottom, left, right;
    uint tb_ileft, tb_iright;
    uint spacing;
} GrBorderWidths;

typedef struct{
    char *text;
    int iw;
    char *attr;
} GrTextElem;

typedef enum{
    GR_TRANSPARENCY_NO,
    GR_TRANSPARENCY_YES,
    GR_TRANSPARENCY_DEFAULT
} GrTransparency;

typedef enum{
    GR_BORDERLINE_NONE,
    GR_BORDERLINE_LEFT,
    GR_BORDERLINE_RIGHT,
    GR_BORDERLINE_TOP,
    GR_BORDERLINE_BOTTOM
} GrBorderLine;

/* Flags to grbrush_begin */
#define GRBRUSH_AMEND       0x0001
#define GRBRUSH_NEED_CLIP   0x0004
#define GRBRUSH_NO_CLEAR_OK 0x0008 /* implied by GRBRUSH_AMEND */

/* Engines etc. */

typedef GrBrush *GrGetBrushFn(Window win, WRootWin *rootwin,
                              const char *style);

extern bool gr_register_engine(const char *engine,  GrGetBrushFn *fn);
extern void gr_unregister_engine(const char *engine);
extern bool gr_select_engine(const char *engine);
extern void gr_refresh();
extern void gr_read_config();

/* Stylespecs are of the from attr1-attr2-etc. We require that each attr in
 * 'spec' matches the one at same index in 'attrib' when '*' matches anything.
 * The score increment for exact match is 2*3^index and 1*3^index for '*' 
 * match. If all elements of 'spec' match those of 'attrib' exactly, the 
 * accumulated score is returned. Otherwise the matching fails and zero is
 * returned. For example:
 *  
 *  spec        attrib            score
 *     foo-*-baz     foo-bar-baz        2+1*3+2*3^2 = 23
 *  foo-bar          foo-bar-baz        2+2*3         = 8
 *  foo-baz          foo-bar-baz        0
 * 
 * gr_stylespec_score2 continues matching from attrib_p2 (if not NULL) when
 * it has reached end of attrib.
 */
extern uint gr_stylespec_score(const char *spec, const char *attrib);
extern uint gr_stylespec_score2(const char *spec, const char *attrib, 
                                const char *attrib_p2);

/* GrBrush */

extern GrBrush *gr_get_brush(Window win, WRootWin *rootwin,
                             const char *style);

extern GrBrush *grbrush_get_slave(GrBrush *brush, WRootWin *rootwin, 
                                  const char *style);

extern void grbrush_release(GrBrush *brush);

extern bool grbrush_init(GrBrush *brush);
extern void grbrush_deinit(GrBrush *brush);

extern void grbrush_begin(GrBrush *brush, const WRectangle *geom,
                          int flags);
extern void grbrush_end(GrBrush *brush);

/* Border drawing */

DYNFUN void grbrush_get_border_widths(GrBrush *brush, GrBorderWidths *bdi);

DYNFUN void grbrush_draw_border(GrBrush *brush, const WRectangle *geom,
                                const char *attrib);
DYNFUN void grbrush_draw_borderline(GrBrush *brush, const WRectangle *geom,
                                    const char *attrib, GrBorderLine line);

/* String drawing */

DYNFUN void grbrush_get_font_extents(GrBrush *brush, GrFontExtents *fnti);

DYNFUN uint grbrush_get_text_width(GrBrush *brush, const char *text, uint len);

DYNFUN void grbrush_draw_string(GrBrush *brush, int x, int y,
                                const char *str, int len, bool needfill,
                                const char *attrib);

/* Textbox drawing */

DYNFUN void grbrush_draw_textbox(GrBrush *brush, const WRectangle *geom,
                                 const char *text, const char *attr,
                                 bool needfill);

DYNFUN void grbrush_draw_textboxes(GrBrush *brush, const WRectangle *geom,
                                   int n, const GrTextElem *elem, 
                                   bool needfill, const char *common_attrib);

/* Misc */

/* Behaviour of the following two functions for "slave brushes" is undefined. 
 * If the parameter rough to grbrush_set_window_shape is set, the actual 
 * shape may be changed for corner smoothing and other superfluous effects.
 * (This feature is only used by floatframes.)
 */
DYNFUN void grbrush_set_window_shape(GrBrush *brush, bool rough,
                                     int n, const WRectangle *rects);

DYNFUN void grbrush_enable_transparency(GrBrush *brush, GrTransparency mode);

DYNFUN void grbrush_fill_area(GrBrush *brush, const WRectangle *geom, 
                              const char *attr);
DYNFUN void grbrush_clear_area(GrBrush *brush, const WRectangle *geom);

DYNFUN bool grbrush_get_extra(GrBrush *brush, const char *key, 
                              char type, void *data);

#endif /* ION_IONCORE_GR_H */
