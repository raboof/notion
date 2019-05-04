/*
 * libtu/objlist.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include "obj.h"
#include "types.h"
#include "objlist.h"
#include "dlist.h"
#include "misc.h"


static ObjList *reuse_first(ObjList **objlist)
{
    ObjList *node=*objlist;
    
    if(node!=NULL && node->watch.obj==NULL){
        UNLINK_ITEM(*objlist, node, next, prev);
        return node;
    }
    
    return NULL;
}


static ObjList *reuse(ObjList **objlist)
{
    ObjList *first=reuse_first(objlist);
    ObjList *last=reuse_first(objlist);
    
    if(first==NULL){
        return last;
    }else{
        if(last!=NULL)
            free(last);
        return first;
    }
}


static void optimise(ObjList **objlist)
{
    ObjList *first=reuse_first(objlist);
    ObjList *last=reuse_first(objlist);
    
    if(first!=NULL)
        free(first);
    if(last!=NULL)
        free(last);
}


void watch_handler(Watch *watch, Obj *UNUSED(obj))
{
    ObjList *node=(ObjList*)watch;
    
    assert(node->prev!=NULL);
    
    if(node->next==NULL){
        /* Last item - can't free */
    }else if(node->prev->next==NULL){
        /* First item - can't free cheaply */
    }else{
        ObjList *tmp=node->prev;
        node->next->prev=node->prev;
        tmp->next=node->next;
        free(node);
    }
}


static void free_node(ObjList **objlist, ObjList *node)
{
    watch_reset(&(node->watch));
    UNLINK_ITEM(*objlist, node, next, prev);
    free(node);
}


static ObjList *mknode(void *obj)
{
    ObjList *node;
    
    if(obj==NULL)
        return NULL;
    
    node=ALLOC(ObjList);
    
    if(node==NULL)
        return FALSE;
        
    watch_init(&(node->watch));
    
    if(!watch_setup(&(node->watch), obj, watch_handler)){
        free(node);
        return NULL;
    }
    
    return node;
}


static ObjList *objlist_find_node(ObjList *objlist, Obj *obj)
{
    ObjList *node=objlist;
    
    while(node!=NULL){
        if(node->watch.obj==obj)
            break;
        node=node->next;
    }
    
    return node;
}


bool objlist_contains(ObjList *objlist, Obj *obj)
{
    return (objlist_find_node(objlist, obj)!=NULL);
}


bool objlist_insert_last(ObjList **objlist, Obj *obj)
{
    ObjList *node=reuse(objlist);
    
    if(node==NULL)
        node=mknode(obj);
    
    if(node==NULL)
        return FALSE;
    
    LINK_ITEM_LAST(*objlist, node, next, prev);
    
    return TRUE;
}


bool objlist_insert_first(ObjList **objlist, Obj *obj)
{
    ObjList *node=reuse(objlist);
    
    if(node==NULL)
        node=mknode(obj);
    
    if(node==NULL)
        return FALSE;
    
    LINK_ITEM_FIRST(*objlist, node, next, prev);
    
    return TRUE;
}


bool objlist_reinsert_last(ObjList **objlist, Obj *obj)
{
    ObjList *node;
    
    optimise(objlist);
    
    node=objlist_find_node(*objlist, obj);
    
    if(node==NULL)
        return objlist_insert_last(objlist, obj);
    
    UNLINK_ITEM(*objlist, node, next, prev);
    LINK_ITEM_LAST(*objlist, node, next, prev);
    
    return TRUE;
}


bool objlist_reinsert_first(ObjList **objlist, Obj *obj)
{
    ObjList *node;

    optimise(objlist);
    
    node=objlist_find_node(*objlist, obj);
    
    if(node==NULL)
        return objlist_insert_first(objlist, obj);
    
    UNLINK_ITEM(*objlist, node, next, prev);
    LINK_ITEM_FIRST(*objlist, node, next, prev);
    
    return TRUE;
}


bool objlist_remove(ObjList **objlist, Obj *obj)
{
    ObjList *node=objlist_find_node(*objlist, obj);
    
    if(node!=NULL)
        free_node(objlist, node);
    
    optimise(objlist);
    
    return (node!=NULL);
}


void objlist_clear(ObjList **objlist)
{
    while(*objlist!=NULL)
        free_node(objlist, *objlist);
}


ObjListIterTmp objlist_iter_tmp=NULL;


void objlist_iter_init(ObjListIterTmp *state, ObjList *objlist)
{
    *state=objlist;
}


Obj *objlist_iter(ObjListIterTmp *state)
{
    Obj *obj=NULL;
    
    while(obj==NULL && *state!=NULL){
        obj=(*state)->watch.obj;
        (*state)=(*state)->next;
    }
    
    return obj;
}


void objlist_iter_rev_init(ObjListIterTmp *state, ObjList *objlist)
{
    *state=(objlist==NULL ? NULL : objlist->prev);
}


Obj *objlist_iter_rev(ObjListIterTmp *state)
{
    Obj *obj=NULL;
    
    while(obj==NULL && *state!=NULL){
        obj=(*state)->watch.obj;
        *state=(*state)->prev;
        if((*state)->next==NULL)
            *state=NULL;
    }
    
    return obj;
}


bool objlist_empty(ObjList *objlist)
{
    ObjListIterTmp tmp;
    Obj *obj;
    
    FOR_ALL_ON_OBJLIST(Obj*, obj, objlist, tmp){
        return FALSE;
    }
    
    return TRUE;
}


Obj *objlist_take_first(ObjList **objlist)
{
    ObjList *node;
    Obj*obj;
    
    optimise(objlist);
    
    node=*objlist;
    
    if(node==NULL)
        return NULL;
    
    obj=node->watch.obj;
    
    assert(obj!=NULL);
    
    free_node(objlist, node);
    
    return obj;
}
    
        
Obj *objlist_take_last(ObjList **objlist)
{
    ObjList *node;
    Obj*obj;
    
    optimise(objlist);
    
    node=*objlist;
    
    if(node==NULL)
        return NULL;
    
    node=node->prev;
    
    obj=node->watch.obj;

    assert(obj!=NULL);
    
    free_node(objlist, node);
    
    return obj;
}
