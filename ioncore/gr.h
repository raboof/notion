/*
 * ion/ioncore/gr.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GR_H
#define ION_IONCORE_GR_H

#include <libtu/stringstore.h>

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

typedef StringId GrAttr;

#define GRATTR_NONE STRINGID_NONE

#define GR_STYLESPEC_INIT {0, NULL}

typedef struct{
    GrAttr attr;
    uint score;
} GrAttrScore;

typedef struct{
    uint n;
    GrAttrScore *attrs;
} GrStyleSpec;

#define GR_TEXTELEM_INIT {NULL, 0, GR_STYLESPEC_INIT}

typedef struct{
    char *text;
    int iw;
    GrStyleSpec attr;
    void /*cairo_surface_t*/ *icon;
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
#define GRBRUSH_KEEP_ATTR   0x0010

/* Engines etc. */

typedef GrBrush *GrGetBrushFn(Window win, WRootWin *rootwin,
                              const char *style);

extern bool gr_register_engine(const char *engine,  GrGetBrushFn *fn);
extern void gr_unregister_engine(const char *engine);
extern bool gr_select_engine(const char *engine);
extern void gr_refresh();
extern void gr_read_config();

 /* Every star ('*') element of 'spec' increases score by one.
  * Every other element of 'spec' found in 'attr' increases the score by the
  * number set in attr times two. Any element of 'spec' (other than star),
  *  not found in 'attr', forces score to zero.
  */
extern uint gr_stylespec_score(const GrStyleSpec *spec, const GrStyleSpec *attr);
extern uint gr_stylespec_score2(const GrStyleSpec *spec, const GrStyleSpec *attr,
                                const GrStyleSpec *attr2);

extern void gr_stylespec_init(GrStyleSpec *spec);
extern bool gr_stylespec_set(GrStyleSpec *spec, GrAttr a);
extern bool gr_stylespec_isset(const GrStyleSpec *spec, GrAttr a);
extern void gr_stylespec_unset(GrStyleSpec *spec, GrAttr a);
extern bool gr_stylespec_add(GrStyleSpec *spec, GrAttr a, uint score);
extern bool gr_stylespec_append(GrStyleSpec *dst, const GrStyleSpec *src);
extern void gr_stylespec_unalloc(GrStyleSpec *spec);
extern bool gr_stylespec_equals(const GrStyleSpec *s1, const GrStyleSpec *s2);
extern bool gr_stylespec_load(GrStyleSpec *spec, const char *str);
extern bool gr_stylespec_load_(GrStyleSpec *spec, const char *str,
                               bool no_order_score);

/* GrBrush */

extern GrBrush *gr_get_brush(Window win, WRootWin *rootwin,
                             const char *style);

DYNFUN GrBrush *grbrush_get_slave(GrBrush *brush, WRootWin *rootwin,
                                  const char *style);

extern void grbrush_release(GrBrush *brush);

extern bool grbrush_init(GrBrush *brush);
extern void grbrush_deinit(GrBrush *brush);

DYNFUN void grbrush_begin(GrBrush *brush, const WRectangle *geom,
                          int flags);
DYNFUN void grbrush_end(GrBrush *brush);

/* Attributes */

DYNFUN void grbrush_init_attr(GrBrush *brush, const GrStyleSpec *spec);
DYNFUN void grbrush_set_attr(GrBrush *brush, GrAttr attr);
DYNFUN void grbrush_unset_attr(GrBrush *brush, GrAttr attr);

/* Border drawing */

DYNFUN void grbrush_get_border_widths(GrBrush *brush, GrBorderWidths *bdi);

DYNFUN void grbrush_draw_border(GrBrush *brush, const WRectangle *geom);
DYNFUN void grbrush_draw_borderline(GrBrush *brush, const WRectangle *geom,
                                    GrBorderLine line);

/* String drawing */

DYNFUN void grbrush_get_font_extents(GrBrush *brush, GrFontExtents *fnti);

DYNFUN uint grbrush_get_text_width(GrBrush *brush, const char *text, uint len);

DYNFUN void grbrush_draw_string(GrBrush *brush, int x, int y,
                                const char *str, int len, bool needfill);

/* Textbox drawing */

DYNFUN void grbrush_draw_textbox(GrBrush *brush, const WRectangle *geom,
                                 const char *text, bool needfill);

DYNFUN void grbrush_draw_textboxes(GrBrush *brush, const WRectangle *geom,
                                   int n, const GrTextElem *elem,
                                   bool needfill);

/* Misc */

/* Behaviour of the following two functions for "slave brushes" is undefined.
 * If the parameter rough to grbrush_set_window_shape is set, the actual
 * shape may be changed for corner smoothing and other superfluous effects.
 * (This feature is only used by floatframes.)
 */
DYNFUN void grbrush_set_window_shape(GrBrush *brush, bool rough,
                                     int n, const WRectangle *rects);

DYNFUN void grbrush_enable_transparency(GrBrush *brush, GrTransparency mode);

DYNFUN void grbrush_fill_area(GrBrush *brush, const WRectangle *geom);
DYNFUN void grbrush_clear_area(GrBrush *brush, const WRectangle *geom);

DYNFUN bool grbrush_get_extra(GrBrush *brush, const char *key,
                              char type, void *data);

#endif /* ION_IONCORE_GR_H */
