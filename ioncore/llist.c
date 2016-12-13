/*
 * ion/ioncore/llist.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "llist.h"
#include "activity.h"


/*{{{ WMPlex numbered/mut.ex. list stuff */


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
    WLListNode *lnode=llist_iter(tmp);
    return (lnode==NULL ? NULL : lnode->st->reg);
}


WLListNode *llist_nth_node(WLListNode *list, uint n)
{
    WLListIterTmp tmp;
    llist_iter_init(&tmp, list);
    return (WLListNode*)iterable_nth(n, (VoidIterator*)llist_iter, &tmp);
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
    if(index==LLIST_INDEX_AFTER_CURRENT_ACT){
        WLListNode *after=current;
        while(after!=NULL){
            WLListNode *nxt=after->next;
            if(nxt==NULL || nxt->st==NULL || nxt->st->reg==NULL)
                break;
            if(!region_is_activity_r(nxt->st->reg))
                break;
            after=nxt;
        }
        return after;
    }else if(index==LLIST_INDEX_AFTER_CURRENT){
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
