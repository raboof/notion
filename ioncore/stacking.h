/*
 * ion/ioncore/stacking.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
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
#include "sizepolicy.h"


#define STACKING_LEVEL_BOTTOM 0
#define STACKING_LEVEL_NORMAL 1
#define STACKING_LEVEL_ON_TOP 2
#define STACKING_LEVEL_MODAL1 1024


#define STACKING_IS_HIDDEN(ST) ((ST)->hidden)


DECLSTRUCT(WStacking){
    WRegion *reg;
    WStacking *next, *prev;
    WStacking *above;
    uint level;
    WSizePolicy szplcy;
    WStacking *mgr_next, *mgr_prev;
    
    /* flags */
    uint to_unweave:2;
    uint hidden:1;
    
    /* WMPlex stuff */
    WLListNode *lnode;
};


typedef bool WStackingFilter(WStacking *st, void *data);
typedef WStacking *WStackingIterator(void *data);


DECLSTRUCT(WStackingIterTmp){
    WStacking *st;
    WStackingFilter *filt;
    void *filt_data;
};


WStacking **window_get_stackingp(WWindow *wwin);
WStacking *window_get_stacking(WWindow *wwin);


WStacking *create_stacking();

void stacking_free(WStacking *st);

/* Returns the topmost node with 'above' pointing to st. */
WStacking *stacking_unstack(WWindow *par, WStacking *st);

void stacking_iter_init(WStackingIterTmp *tmp,
                        WStacking *st,
                        WStackingFilter *filt,
                        void *filt_data);
WRegion *stacking_iter(WStackingIterTmp *tmp);
WStacking *stacking_iter_nodes(WStackingIterTmp *tmp);

void stacking_iter_mgr_init(WStackingIterTmp *tmp,
                            WStacking *st,
                            WStackingFilter *filt,
                            void *filt_data);
WRegion *stacking_iter_mgr(WStackingIterTmp *tmp);
WStacking *stacking_iter_mgr_nodes(WStackingIterTmp *tmp);

void stacking_weave(WStacking **stacking, WStacking **np, bool below);
WStacking *stacking_unweave(WStacking **stacking, 
                            WStackingFilter *filt, void *filt_data);

void stacking_restack(WStacking **stacking, WStacking *st, Window fb_win,
                      WStackingFilter *filt, void *filt_data, bool lower);

WStacking *stacking_find_to_focus(WStacking *stacking, WStacking *to_try,
                                  WStackingFilter *include_filt, 
                                  WStackingFilter *approve_filt, 
                                  void *filt_data);
WStacking *stacking_find_to_focus_mapped(WStacking *stacking, 
                                         WStacking *to_try,
                                         WRegion *mgr);

uint stacking_min_level(WStacking *stacking, 
                        WStackingFilter *include_filt, 
                        void *filt_data);

uint stacking_min_level_mapped(WStacking *stacking);


WStacking *ioncore_find_stacking(WRegion *reg);
void stacking_unassoc(WStacking *stacking);
bool stacking_assoc(WStacking *stacking, WRegion *reg);

#endif /* ION_IONCORE_STACKING_H */
