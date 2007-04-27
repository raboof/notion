/*
 * ion/mod_statusbar/draw.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_STATUSBAR_DRAW_H
#define ION_MOD_STATUSBAR_DRAW_H

#include <libextl/extl.h>
#include "statusbar.h"

extern void statusbar_draw(WStatusBar *sb, bool complete);
extern void statusbar_calculate_xs(WStatusBar *sb);

#endif /* ION_MOD_STATUSBAR_DRAW_H */
