/*
 * ion/ioncore/draw.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_DRAW_H
#define ION_IONCORE_DRAW_H

#include "common.h"
#include "window.h"
#include "rootwin.h"
#include "grdata.h"

extern void draw_rubberbox(WRootWin *rw, WRectangle rect);

extern void set_moveres_pos(WRootWin *rootwin, int x, int y);
extern void set_moveres_size(WRootWin *rootwin, int w, int h);

extern bool alloc_color(WRootWin *rootwin, const char *name, WColor *cret);
extern void setup_color_group(WRootWin *rootwin, WColorGroup *cg,
							  WColor hl, WColor sh, WColor bg, WColor fg);

extern void preinit_graphics(WRootWin *rootwin);
extern void postinit_graphics(WRootWin *rootwin);

extern void reread_draw_config();

#endif /* ION_IONCORE_DRAW_H */
