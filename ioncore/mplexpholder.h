/*
 * ion/ioncore/mplexpholder.h
 *
 * Copyright (c) Tuomo Valkonen 2005. 
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
    WMPlexManaged *after;
    WMPlexPHolder *next, *prev;
};


/* If 'after' is set, it is used, otherwise 'or_after', 
 * and finally 'or_layer' if this is also unset
 */

extern WMPlexPHolder *create_mplexpholder(WMPlex *mplex, 
                                          WMPlexPHolder *after,
                                          WMPlexManaged *or_after, 
                                          int or_layer);
extern bool mplexpholder_init(WMPlexPHolder *ph, WMPlex *mplex, 
                              WMPlexPHolder *after,
                              WMPlexManaged *or_after, 
                              int or_layer);
extern void mplexpholder_deinit(WMPlexPHolder *ph);

extern int mplexpholder_layer(WMPlexPHolder *ph);

extern bool mplexpholder_stale(WMPlexPHolder *ph);

extern bool mplexpholder_do_attach(WMPlexPHolder *ph, 
                                   WRegionAttachHandler *hnd,
                                   void *hnd_param);

extern bool mplexpholder_move(WMPlexPHolder *ph, WMPlex *mplex, 
                              WMPlexPHolder *after,
                              WMPlexManaged *or_after, 
                              int or_layer);

extern void mplex_move_phs(WMPlex *mplex, WMPlexManaged *node,
                           WMPlexPHolder *after,
                           WMPlexManaged *or_after, 
                           int or_layer);
extern void mplex_move_phs_before(WMPlex *mplex, WMPlexManaged *node, 
                                  int layer);

extern WMPlexPHolder *mplex_managed_get_pholder(WMPlex *mplex, 
                                                WRegion *mgd);
extern WMPlexPHolder *mplex_get_rescue_pholder_for(WMPlex *mplex, 
                                                       WRegion *mgd);

#endif /* ION_IONCORE_MPLEXPHOLDER_H */
