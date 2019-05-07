/*
 * ion/ioncore/sizehint.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_SIZEHINT_H
#define ION_IONCORE_SIZEHINT_H

#include "common.h"
#include "region.h"

INTRSTRUCT(WSizeHints);

DECLSTRUCT(WSizeHints){
    uint min_set:1;
    uint max_set:1;
    uint inc_set:1;
    uint base_set:1;
    uint aspect_set:1;
    uint no_constrain:1;

    int min_width, min_height;
    int max_width, max_height;
    int width_inc, height_inc;
    struct {
        int x;
        int y;
    } min_aspect, max_aspect;
    int base_width, base_height;
};

extern void xsizehints_to_sizehints(const XSizeHints *xh,
                                    WSizeHints *hints);

extern void xsizehints_sanity_adjust(XSizeHints *hints);

extern void sizehints_clear(WSizeHints *hints);

extern void sizehints_adjust_for(WSizeHints *hints, WRegion *reg);

extern void sizehints_correct(const WSizeHints *hints, int *wp, int *hp,
                              bool min, bool override_no_constrain);

extern int xgravity_deltax(int gravity, int left, int right);
extern int xgravity_deltay(int gravity, int top, int bottom);
extern void xgravity_translate(int gravity, WRegion *reg, WRectangle *geom);

#endif /* ION_IONCORE_SIZEHINT_H */
