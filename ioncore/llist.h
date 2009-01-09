/*
 * ion/ioncore/llist.h
 *
 * Copyright (c) Tuomo Valkonen 2005-2009. 
 * 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_LLIST_H
#define ION_IONCORE_LLIST_H

#include <limits.h>

#include "mplex.h"
#include "mplexpholder.h"
#include "extlconv.h"
#include "stacking.h"


DECLSTRUCT(WLListNode){
    WLListNode *next, *prev;
    WMPlexPHolder *phs;
    WStacking *st;
};

typedef WLListNode *WLListIterTmp;


#define FOR_ALL_NODES_ON_LLIST(NODE, LL, TMP) \
    FOR_ALL_ITER(llist_iter_init, llist_iter, NODE, LL, &(TMP))

#define FOR_ALL_REGIONS_ON_LLIST(NODE, LL, TMP) \
    FOR_ALL_ITER(llist_iter_init, llist_iter_regions, NODE, LL, &(TMP))


extern void llist_iter_init(WLListIterTmp *tmp, WLListNode *llist);
extern WLListNode *llist_iter(WLListIterTmp *tmp);
extern WRegion *llist_iter_regions(WLListIterTmp *tmp);
extern WLListNode *llist_nth_node(WLListNode *list, uint n);
extern ExtlTab llist_to_table(WLListNode *list);
extern void llist_link_after(WLListNode **list, 
                             WLListNode *after, WLListNode *node);
extern void llist_link_last(WLListNode **list, WLListNode *node);
extern WLListNode *llist_index_to_after(WLListNode *list, 
                                        WLListNode *current,
                                        int index);
extern void llist_unlink(WLListNode **list, WLListNode *node);

#define LLIST_INDEX_LAST              (-1)
#define LLIST_INDEX_AFTER_CURRENT     (-2)
#define LLIST_INDEX_AFTER_CURRENT_ACT (-3)

#endif /* ION_IONCORE_LLIST_H */
