/*
 * ion/de/font.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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

DECLSTRUCT(DEFont){
	char *pattern;
	int refcount;
	XFontSet fontset;
	XFontStruct *fontstruct;
	DEFont *next, *prev;
};

#include "brush.h"
#include "colour.h"


extern DEFont *de_load_font(const char *fontname);
extern void de_free_font(DEFont *font);


extern void debrush_draw_string(DEBrush *brush, Window win, int x, int y,
								const char *str, int len, bool needfill,
								const char *attrib);
extern void debrush_do_draw_string(DEBrush *brush, Window win, int x, int y,
								   const char *str, int len, bool needfill, 
								   DEColourGroup *colours);

extern void debrush_get_font_extents(DEBrush *brush, GrFontExtents *fnte);

extern uint debrush_get_text_width(DEBrush *brush, const char *text, uint len);

#endif /* ION_DE_FONT_H */
