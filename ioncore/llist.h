/*
 * ion/ioncore/llist.h
 *
 * Copyright (c) Tuomo Valkonen 2005. 
 * 
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_LLIST_H
#define ION_IONCORE_LLIST_H

#include <limits.h>

#include "mplex.h"
#include "mplexpholder.h"
#include "extlconv.h"


#define LLIST_L2_HIDDEN  0x0001
#define LLIST_L2_PASSIVE 0x0002
#define LLIST_L2         0x0004
#define LLIST_L2_SEMIMODAL 0x0008


DECLSTRUCT(WLListNode){
    WLListNode *next, *prev;
    int flags;
    WRegion *reg;
    WMPlexPHolder *phs;
};


typedef WLListNode *WLListIterTmp;

#define LLIST_REG(NODE) ((NODE)!=NULL ? (NODE)->reg : NULL)
#define LLIST_LAYER(NODE) ((NODE)->flags&LLIST_L2 ? 2 : 1)


#define FOR_ALL_NODES_ON_LLIST(NODE, LL) \
    LIST_FOR_ALL(LL, NODE, next, prev)

#define FOR_ALL_NODES_ON_LLIST_REV(NODE, LL) \
    LIST_FOR_ALL_REV(LL, NODE, next, prev)

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
extern void llist_link_after(WLListNode **list, 
                             WLListNode *after, WLListNode *node);
extern void llist_link_last(WLListNode **list, WLListNode *node);
extern WLListNode *llist_index_to_after(WLListNode *list, 
                                        WLListNode *current,
                                        int index);
extern void llist_unlink(WLListNode **list, WLListNode *node);

#define LLIST_INDEX_AFTER_CURRENT (INT_MIN)
#define LLIST_INDEX_LAST (-1)

#endif /* ION_IONCORE_LLIST_H */
