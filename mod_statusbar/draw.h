/*
 * ion/mod_statusbar/draw.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_STATUSBAR_DRAW_H
#define ION_MOD_STATUSBAR_DRAW_H

#include <libextl/extl.h>
#include "statusbar.h"

extern void statusbar_draw(WStatusBar *sb, bool complete);
extern void statusbar_calculate_xs(WStatusBar *sb);

#endif /* ION_MOD_STATUSBAR_DRAW_H */
