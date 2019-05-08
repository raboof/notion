/*
 * notion/de/font.h
 *
 * Copyright (c) the Notion team 2013
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef NOTION_DE_FONT_H
#define NOTION_DE_FONT_H

#include <ioncore/common.h>
#include <ioncore/gr.h>
#ifdef HAVE_X11_XFT
#include <X11/Xft/Xft.h>
#endif /* HAVE_X11_XFT */

INTRSTRUCT(DEFont);

#include "brush.h"
#include "colour.h"
#include "style.h"

#define DE_RESET_FONT_EXTENTS(FNTE) \
   {(FNTE)->max_height=0; (FNTE)->max_width=0; (FNTE)->baseline=0;}

DECLSTRUCT(DEFont){
    char *pattern;
    int refcount;
#ifdef HAVE_X11_BMF
    XFontSet fontset;
    XFontStruct *fontstruct;
#endif /* HAVE_X11_BMF */
#ifdef HAVE_X11_XFT /* HAVE_X11_XFT */
    XftFont *font;
#endif /* HAVE_X11_XFT */
    DEFont *next, *prev;
};

extern const char *de_default_fontname();
extern bool de_load_font_for_style(DEStyle *style, const char *fontname);
extern bool de_set_font_for_style(DEStyle *style, DEFont *font);
extern DEFont *de_load_font(const char *fontname);
extern void de_free_font(DEFont *font);

extern void debrush_draw_string(DEBrush *brush, int x, int y,
                                const char *str, int len, bool needfill);
extern void debrush_do_draw_string(DEBrush *brush, int x, int y,
                                   const char *str, int len, bool needfill,
                                   DEColourGroup *colours);
extern void debrush_do_draw_string_default(DEBrush *brush, int x, int y,
                                           const char *str, int len,
                                           bool needfill,
                                           DEColourGroup *colours);

extern void debrush_get_font_extents(DEBrush *brush, GrFontExtents *fnte);

extern uint debrush_get_text_width(DEBrush *brush, const char *text, uint len);

extern uint defont_get_text_width(DEFont *font, const char *text, uint len);
extern void defont_get_font_extents(DEFont *font, GrFontExtents *fnte);

#endif /* NOTION_DE_FONT_H */
