/*
 * ion/ioncore/mplexp.h
 *
 * Copyright (c) Tuomo Valkonen 2005. 
 * 
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_MPLEXP_H
#define ION_IONCORE_MPLEXP_H

#include "mplex.h"
#include "extlconv.h"


#define LLIST_L2_HIDDEN 0x0001
#define LLIST_L2_PASSIVE 0x0002


DECLSTRUCT(WLListNode){
    WLListNode *next, *prev;
    int flags;
    WRegion *reg;
    WMPlexPHolder *phs;
};


typedef WLListNode *WLListIterTmp;

#define LLIST_REG(NODE) ((NODE)!=NULL ? (NODE)->reg : NULL)

#define FOR_ALL_NODES_ON_LLIST(NODE, LL) \
    LIST_FOR_ALL(LL, NODE, next, prev)

#define FOR_ALL_NODES_ON_LLIST_W_NEXT(NODE, LL, TMP) \
    FOR_ALL_ITER(llist_iter_init, llist_iter, NODE, LL, &(TMP))

#define FOR_ALL_REGIONS_ON_LLIST(REG, LL, TMP) \
    FOR_ALL_ITER(llist_iter_init, llist_iter_regions, REG, LL, &(TMP))


extern void llist_iter_init(WLListIterTmp *tmp, WLListNode *llist);
extern WLListNode *llist_iter(WLListIterTmp *tmp);
extern WRegion *llist_iter_regions(WLListIterTmp *tmp);
extern WLListNode *llist_find_on(WLListNode *list, WRegion *reg);
extern bool llist_is_on(WLListNode *list, WRegion *reg);
extern bool llist_is_node_on(WLListNode *list, WLListNode *node);
extern WLListNode *llist_nth_node(WLListNode *list, uint n);
extern WRegion *llist_nth_region(WLListNode *list, uint n);
extern ExtlTab llist_to_table(WLListNode *list);

#define INDEX_AFTER_CURRENT (INT_MIN)
#define DEFAULT_INDEX(MPLEX) \
    ((MPLEX)->flags&MPLEX_ADD_TO_END ? -1 : INDEX_AFTER_CURRENT)

#define MPLEX_MGD_UNVIEWABLE(MPLEX) \
    ((MPLEX)->flags&MPLEX_MANAGED_UNVIEWABLE)

#endif /* ION_IONCORE_MPLEXP_H */
