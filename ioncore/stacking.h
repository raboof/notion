/*
 * ion/ioncore/stacking.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006.
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_STACKING_H
#define ION_IONCORE_STACKING_H

#include "common.h"
#include "region.h"

INTRSTRUCT(WStacking);

DECLSTRUCT(WStacking){
    WRegion *reg;
    WRegion *above;
    WStacking *next, *prev;
    bool sticky;
};


typedef bool WStackingFilter(WRegion *reg, void *data);


void stacking_do_raise(WStacking **stacking, WRegion *reg, bool initial,
                       Window fb_win,
                       WStackingFilter *filt, void *filt_data);

void stacking_do_lower(WStacking **stacking, WRegion *reg, Window fb_win,
                       WStackingFilter *filt, void *filt_data);

void stacking_restack(WStacking **stacking, Window other, int mode,
                      WStacking *other_on_list,
                      WStackingFilter *filt, void *filt_data);

void stacking_stacking(WStacking *stacking, Window *bottomret, Window *topret,
                       WStackingFilter *filt, void *filt_data);

#endif /* ION_IONCORE_STACKING_H */
