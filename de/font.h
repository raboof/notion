/*
 * ion/de/font.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_DE_FONT_H
#define ION_DE_FONT_H

#include <ioncore/common.h>
#include <ioncore/gr.h>

INTRSTRUCT(DEFont);

#include "brush.h"
#include "colour.h"
#include "style.h"

#define DE_RESET_FONT_EXTENTS(FNTE) \
   {(FNTE)->max_height=0; (FNTE)->max_width=0; (FNTE)->baseline=0;}

extern bool de_load_font_for_style(DEStyle *style, const char *fontname);
extern bool de_set_font_for_style(DEStyle *style, DEFont *font);
extern DEFont *de_load_font(const char *fontname);
extern void de_free_font(DEFont *font);

extern void debrush_draw_string(DEBrush *brush, Window win, int x, int y,
                                const char *str, int len, bool needfill,
                                const char *attrib);
extern void debrush_do_draw_string(DEBrush *brush, Window win, int x, int y,
                                   const char *str, int len, bool needfill, 
                                   DEColourGroup *colours);
extern void debrush_do_draw_string_default(DEBrush *brush, Window win,
                                           int x, int y,
                                           const char *str, int len, 
                                           bool needfill, 
                                           DEColourGroup *colours);

extern void debrush_get_font_extents(DEBrush *brush, GrFontExtents *fnte);

extern uint debrush_get_text_width(DEBrush *brush, const char *text, uint len);

extern uint defont_get_text_width(DEFont *font, const char *text, uint len);
extern void defont_get_font_extents(DEFont *font, GrFontExtents *fnte);

#endif /* ION_DE_FONT_H */
