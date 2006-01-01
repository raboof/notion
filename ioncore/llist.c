/*
 * ion/ioncore/llist.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "llist.h"


/*{{{ WMPlex Layer list stuff */


void llist_iter_init(WLListIterTmp *tmp, WLListNode *llist)
{
    *tmp=llist;
}


WLListNode *llist_iter(WLListIterTmp *tmp)
{
    WLListNode *mgd=*tmp;
    if(mgd!=NULL)
        *tmp=mgd->next;
    return mgd;
}


WRegion *llist_iter_regions(WLListIterTmp *tmp)
{
    WLListNode *mgd=*tmp;
    if(mgd!=NULL){
        *tmp=mgd->next;
        return mgd->reg;
    }
    return NULL;
}


static bool reg_is(WLListNode *node, WRegion *reg)
{
    return (node->reg==reg);
}


WLListNode *llist_find_on(WLListNode *list, WRegion *reg)
{
    WLListIterTmp tmp;
    llist_iter_init(&tmp, list);
    return (WLListNode*)iterable_find((BoolFilter*)reg_is, reg, 
                                      (VoidIterator*)llist_iter, &tmp);
}


bool llist_is_on(WLListNode *list, WRegion *reg)
{
    WLListIterTmp tmp;
    llist_iter_init(&tmp, list);
    return iterable_is_on(reg, (VoidIterator*)llist_iter_regions, &tmp);
}


bool llist_is_node_on(WLListNode *list, WLListNode *node)
{
    WLListIterTmp tmp;
    llist_iter_init(&tmp, list);
    return iterable_is_on(node, (VoidIterator*)llist_iter, &tmp);
}


WLListNode *llist_nth_node(WLListNode *list, uint n)
{
    WLListIterTmp tmp;
    llist_iter_init(&tmp, list);
    return (WLListNode*)iterable_nth(n, (VoidIterator*)llist_iter, &tmp);
}


WRegion *llist_nth_region(WLListNode *list, uint n)
{
    WLListIterTmp tmp;
    llist_iter_init(&tmp, list);
    return (WRegion*)iterable_nth(n, (VoidIterator*)llist_iter_regions, &tmp);
}


ExtlTab llist_to_table(WLListNode *list)
{
    WLListIterTmp tmp;
    llist_iter_init(&tmp, list);
    return extl_obj_iterable_to_table((ObjIterator*)llist_iter_regions, &tmp);
}


void llist_link_after(WLListNode **list, 
                      WLListNode *after, WLListNode *node)
{
    if(after!=NULL){
        LINK_ITEM_AFTER(*list, after, node, next, prev);
    }else{
        LINK_ITEM_FIRST(*list, node, next, prev);
    }
}


void llist_link_last(WLListNode **list, WLListNode *node)
{
    LINK_ITEM_LAST(*list, node, next, prev);
}


WLListNode *llist_index_to_after(WLListNode *list, 
                                 WLListNode *current,
                                 int index)
{
    WLListNode *after=NULL;
    
    if(index==LLIST_INDEX_AFTER_CURRENT){
        return current;
    }else if(index<0){
        return (list!=NULL ? list->prev : NULL);
    }else if(index==0){
        return NULL;
    }else{ /* index>0 */
        return llist_nth_node(list, index-1);
    }
}


void llist_unlink(WLListNode **list, WLListNode *node)
{
    UNLINK_ITEM(*list, node, next, prev);
}


/*}}}*/
