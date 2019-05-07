/*
 * libtu/ptrlist.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include "obj.h"
#include "ptrlist.h"
#include "types.h"
#include "dlist.h"
#include "misc.h"


static void free_node(PtrList **ptrlist, PtrList *node)
{
    UNLINK_ITEM(*ptrlist, node, next, prev);
    free(node);
}


static PtrList *mknode(void *ptr)
{
    PtrList *node;

    if(ptr==NULL)
        return NULL;

    node=ALLOC(PtrList);

    if(node==NULL)
        return FALSE;

    node->ptr=ptr;

    return node;
}


static PtrList *ptrlist_find_node(PtrList *ptrlist, void *ptr)
{
    PtrList *node=ptrlist;

    while(node!=NULL){
        if(node->ptr==ptr)
            break;
        node=node->next;
    }

    return node;
}


bool ptrlist_contains(PtrList *ptrlist, void *ptr)
{
    return (ptrlist_find_node(ptrlist, ptr)!=NULL);
}


bool ptrlist_insert_last(PtrList **ptrlist, void *ptr)
{
    PtrList *node=mknode(ptr);

    if(node==NULL)
        return FALSE;

    LINK_ITEM_LAST(*ptrlist, node, next, prev);

    return TRUE;
}


bool ptrlist_insert_first(PtrList **ptrlist, void *ptr)
{
    PtrList *node=mknode(ptr);

    if(node==NULL)
        return FALSE;

    LINK_ITEM_FIRST(*ptrlist, node, next, prev);

    return TRUE;
}


bool ptrlist_reinsert_last(PtrList **ptrlist, void *ptr)
{
    PtrList *node=ptrlist_find_node(*ptrlist, ptr);

    if(node==NULL)
        return ptrlist_insert_last(ptrlist, ptr);

    UNLINK_ITEM(*ptrlist, node, next, prev);
    LINK_ITEM_LAST(*ptrlist, node, next, prev);

    return TRUE;
}


bool ptrlist_reinsert_first(PtrList **ptrlist, void *ptr)
{
    PtrList *node=ptrlist_find_node(*ptrlist, ptr);

    if(node==NULL)
        return ptrlist_insert_first(ptrlist, ptr);

    UNLINK_ITEM(*ptrlist, node, next, prev);
    LINK_ITEM_FIRST(*ptrlist, node, next, prev);

    return TRUE;
}


bool ptrlist_remove(PtrList **ptrlist, void *ptr)
{
    PtrList *node=ptrlist_find_node(*ptrlist, ptr);

    if(node!=NULL)
        free_node(ptrlist, node);

    return (node!=NULL);
}


void ptrlist_clear(PtrList **ptrlist)
{
    while(*ptrlist!=NULL)
        free_node(ptrlist, *ptrlist);
}


PtrListIterTmp ptrlist_iter_tmp=NULL;


void ptrlist_iter_init(PtrListIterTmp *state, PtrList *ptrlist)
{
    *state=ptrlist;
}


void *ptrlist_iter(PtrListIterTmp *state)
{
    void *ptr=NULL;

    if(*state!=NULL){
        ptr=(*state)->ptr;
        (*state)=(*state)->next;
    }

    return ptr;
}


void ptrlist_iter_rev_init(PtrListIterTmp *state, PtrList *ptrlist)
{
    *state=(ptrlist==NULL ? NULL : ptrlist->prev);
}


void *ptrlist_iter_rev(PtrListIterTmp *state)
{
    void *ptr=NULL;

    if(*state!=NULL){
        ptr=(*state)->ptr;
        *state=(*state)->prev;
        if((*state)->next==NULL)
            *state=NULL;
    }

    return ptr;
}


void *ptrlist_take_first(PtrList **ptrlist)
{
    PtrList *node=*ptrlist;
    void *ptr;

    if(node==NULL)
        return NULL;

    ptr=node->ptr;

    free_node(ptrlist, node);

    return ptr;
}


void *ptrlist_take_last(PtrList **ptrlist)
{
    PtrList *node=*ptrlist;
    void *ptr;

    if(node==NULL)
        return NULL;

    node=node->prev;

    ptr=node->ptr;

    free_node(ptrlist, node);

    return ptr;
}
