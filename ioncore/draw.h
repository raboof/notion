/*
 * wmcore/draw.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_DRAW_H
#define WMCORE_DRAW_H

#include "common.h"
#include "screen.h"
#include "grdata.h"

extern void draw_rubberband(WScreen *scr, WRectangle rect,
							bool vertical);
extern void draw_rubberbox(WScreen *scr, WRectangle rect);

extern void set_moveres_pos(WScreen *scr, int x, int y);
extern void set_moveres_size(WScreen *scr, int w, int h);

extern bool alloc_color(WScreen *scr, const char *name, Pixel *cret);
extern void setup_color_group(WScreen *scr, WColorGroup *cg,
							  Pixel hl, Pixel sh, Pixel bg, Pixel fg);

extern void preinit_graphics(WScreen *scr);
extern void postinit_graphics(WScreen *scr);

#endif /* WMCORE_DRAW_H */
