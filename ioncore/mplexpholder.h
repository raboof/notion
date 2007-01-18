/*
 * ion/ioncore/mplexpholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2007. 
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
#include "framedpholder.h"


DECLCLASS(WMPlexPHolder){
    WPHolder ph;
    Watch mplex_watch;
    WFramedPHolder *recreate_pholder; /* only on first of list */
    WLListNode *after;
    WMPlexPHolder *next, *prev;
    WMPlexAttachParams param;
};


/* If 'after' is set, it is used, otherwise 'or_after', 
 * and finally 'or_layer' if this is also unset
 */

extern WMPlexPHolder *create_mplexpholder(WMPlex *mplex, 
                                          WStacking *either_st,
                                          WMPlexAttachParams *or_param);
extern bool mplexpholder_init(WMPlexPHolder *ph, 
                              WMPlex *mplex, 
                              WStacking *either_st,
                              WMPlexAttachParams *or_param);
extern void mplexpholder_deinit(WMPlexPHolder *ph);

extern int mplexpholder_layer(WMPlexPHolder *ph);

extern WRegion *mplexpholder_do_attach(WMPlexPHolder *ph, int flags,
                                       WRegionAttachData *data);

extern bool mplexpholder_do_goto(WMPlexPHolder *ph);

extern WPHolder *mplexpholder_do_root(WMPlexPHolder *ph);

extern WRegion *mplexpholder_do_target(WMPlexPHolder *ph);

extern bool mplexpholder_move(WMPlexPHolder *ph, WMPlex *mplex, 
                              WMPlexPHolder *after,
                              WLListNode *or_after);

extern void mplexpholder_do_unlink(WMPlexPHolder *ph, WMPlex *mplex);

extern void mplex_move_phs(WMPlex *mplex, WLListNode *node,
                           WMPlexPHolder *after,
                           WLListNode *or_after);
extern void mplex_move_phs_before(WMPlex *mplex, WLListNode *node);

extern WMPlexPHolder *mplex_managed_get_pholder(WMPlex *mplex, 
                                                WRegion *mgd);
extern WMPlexPHolder *mplex_get_rescue_pholder_for(WMPlex *mplex, 
                                                       WRegion *mgd);

#endif /* ION_IONCORE_MPLEXPHOLDER_H */
