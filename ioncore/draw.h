/*
 * ion/ioncore/draw.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_DRAW_H
#define ION_IONCORE_DRAW_H

#include "common.h"
#include "screen.h"
#include "grdata.h"

extern void draw_rubberband(WScreen *scr, WRectangle rect,
							bool vertical);
extern void draw_rubberbox(WScreen *scr, WRectangle rect);

extern void set_moveres_pos(WScreen *scr, int x, int y);
extern void set_moveres_size(WScreen *scr, int w, int h);

extern bool alloc_color(WScreen *scr, const char *name, WColor *cret);
extern void setup_color_group(WScreen *scr, WColorGroup *cg,
							  WColor hl, WColor sh, WColor bg, WColor fg);

extern void preinit_graphics(WScreen *scr);
extern void postinit_graphics(WScreen *scr);

extern void reread_draw_config();

#endif /* ION_IONCORE_DRAW_H */
