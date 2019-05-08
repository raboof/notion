/*
 * ion/ioncore/rectangle.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_RECTANGLE_H
#define ION_IONCORE_RECTANGLE_H

#include <stdio.h>
#include "common.h"
#include <libextl/extl.h>

INTRSTRUCT(WRectangle);

DECLSTRUCT(WRectangle){
    int x, y;
    int w, h;
};

#define RECTANGLE_SAME   0x00
#define RECTANGLE_X_DIFF 0x01
#define RECTANGLE_Y_DIFF 0x02
#define RECTANGLE_POS_DIFF (RECTANGLE_X_DIFF|RECTANGLE_Y_DIFF)
#define RECTANGLE_W_DIFF 0x04
#define RECTANGLE_H_DIFF 0x08
#define RECTANGLE_SZ_DIFF (RECTANGLE_W_DIFF|RECTANGLE_H_DIFF)

extern int rectangle_compare(const WRectangle *g, const WRectangle *h);
extern bool rectangle_contains(const WRectangle *g, int x, int y);
extern void rectangle_constrain(WRectangle *g, const WRectangle *bounds);
extern void rectangle_clamp_or_center(WRectangle *g, const WRectangle *bounds);

extern void rectange_debugprint(const WRectangle *g, const char *n);

#endif /* ION_IONCORE_RECTANGLE_H */

