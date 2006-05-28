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


DECLSTRUCT(WStacking){
    WRegion *reg;
    WStacking *next, *prev;
    WStacking *above;
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

WStacking **window_get_stackingp(WWindow *wwin);
WStacking *window_get_stacking(WWindow *wwin);

WRegion *stacking_remove(WWindow *par, WRegion *reg);
/* Returns the topmost node with 'above' pointing to st. */
WStacking *stacking_unlink(WWindow *par, WStacking *st);

typedef struct{
    WStacking *st;
    WStackingFilter *filt;
    void *filt_data;
} WStackingIterTmp;

void stacking_iter_init(WStackingIterTmp *tmp,
                        WStacking *st,
                        WStackingFilter *filt,
                        void *filt_data);
WRegion *stacking_iter(WStackingIterTmp *tmp);
WStacking *stacking_iter_nodes(WStackingIterTmp *tmp);

WStacking *stacking_find(WStacking *st, WRegion *reg);

#endif /* ION_IONCORE_STACKING_H */
