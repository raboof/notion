/*
 * ion/ioncore/mplexpholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_MPLEXPHOLDER_H
#define ION_IONCORE_MPLEXPHOLDER_H

#include "common.h"
#include "pholder.h"
#include "mplex.h"
#include "attach.h"

DECLCLASS(WMPlexPHolder){
    WPHolder ph;
    Watch mplex_watch;
    WLListNode *after;
    WMPlexPHolder *next, *prev;
    int szplcy;
    bool initial;
};


/* If 'after' is set, it is used, otherwise 'or_after', 
 * and finally 'or_layer' if this is also unset
 */

extern WMPlexPHolder *create_mplexpholder(WMPlex *mplex, 
                                          WMPlexPHolder *after,
                                          WLListNode *or_after, 
                                          int or_layer);
extern bool mplexpholder_init(WMPlexPHolder *ph, WMPlex *mplex, 
                              WMPlexPHolder *after,
                              WLListNode *or_after, 
                              int or_layer);
extern void mplexpholder_deinit(WMPlexPHolder *ph);

extern int mplexpholder_layer(WMPlexPHolder *ph);

extern bool mplexpholder_do_attach(WMPlexPHolder *ph, 
                                   WRegionAttachHandler *hnd, void *hnd_param, 
                                   int flags);

extern bool mplexpholder_do_goto(WMPlexPHolder *ph);

extern WRegion *mplexpholder_do_target(WMPlexPHolder *ph);

extern bool mplexpholder_move(WMPlexPHolder *ph, WMPlex *mplex, 
                              WMPlexPHolder *after,
                              WLListNode *or_after, 
                              int or_layer);

extern void mplex_move_phs(WMPlex *mplex, WLListNode *node,
                           WMPlexPHolder *after,
                           WLListNode *or_after, 
                           int or_layer);
extern void mplex_move_phs_before(WMPlex *mplex, WLListNode *node);

extern WMPlexPHolder *mplex_managed_get_pholder(WMPlex *mplex, 
                                                WRegion *mgd);
extern WMPlexPHolder *mplex_get_rescue_pholder_for(WMPlex *mplex, 
                                                       WRegion *mgd);

extern WMPlexPHolder *mplex_last_place_holder(WMPlex *mplex, int layer);

#endif /* ION_IONCORE_MPLEXPHOLDER_H */
