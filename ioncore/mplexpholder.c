/*
 * ion/ioncore/mplexpholder.c
 *
 * Copyright (c) Tuomo Valkonen 2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include <libtu/obj.h>
#include <libtu/pointer.h>

#include "common.h"
#include "mplex.h"
#include "mplexpholder.h"
#include "mplexp.h"


static void mplex_watch_handler(Watch *watch, Obj *mplex);


/*{{{ Primitives */


static bool on_ph_list(WMPlexPHolder *ll, WMPlexPHolder *ph)
{
    WMPlexPHolder *ph2;
    
    LIST_FOR_ALL(ll, ph2, next, prev){
        if(ph==ph2)
            return TRUE;
    }
    
    return FALSE;
}


static void do_link_ph(WMPlexPHolder *ph, 
                       WMPlex *mplex,
                       WMPlexPHolder *after,
                       WLListNode *or_after, 
                       int or_layer)
{
    assert(mplex==(WMPlex*)ph->mplex_watch.obj && mplex!=NULL);
    
    if(after!=NULL){
        if(after->after!=NULL){
            LINK_ITEM_AFTER(after->after->phs, after, ph, next, prev);
        }else if(on_ph_list(mplex->l1_phs, after)){
            LINK_ITEM_AFTER(mplex->l1_phs, after, ph, next, prev);
        }else if(on_ph_list(mplex->l2_phs, after)){
            LINK_ITEM_AFTER(mplex->l2_phs, after, ph, next, prev);
        }else{
            assert(FALSE);
        }
        ph->after=after->after;
    }else if(or_after!=NULL){
        LINK_ITEM_FIRST(or_after->phs, ph, next, prev);
        ph->after=or_after;
    }else if(or_layer==1){
        LINK_ITEM_FIRST(mplex->l1_phs, ph, next, prev);
        ph->after=NULL;
    }else if(or_layer==2){
        LINK_ITEM_FIRST(mplex->l2_phs, ph, next, prev);
        ph->after=NULL;
    }else{
        assert(FALSE);
    }
}


static void do_unlink_ph(WMPlexPHolder *ph, WMPlex *mplex)
{
    if(ph->after!=NULL){
        UNLINK_ITEM(ph->after->phs, ph, next, prev);
    }else if(mplex!=NULL){
        if(on_ph_list(mplex->l1_phs, ph)){
            UNLINK_ITEM(mplex->l1_phs, ph, next, prev);
        }else{
            assert(on_ph_list(mplex->l2_phs, ph));
            UNLINK_ITEM(mplex->l2_phs, ph, next, prev);
        }
    }else{
        assert(ph->next==NULL && ph->prev==NULL);
    }
    
    ph->after=NULL;
    ph->next=NULL;
    ph->prev=NULL;
}


/*}}}*/


/*{{{ Init/deinit */


static void mplex_watch_handler(Watch *watch, Obj *mplex)
{
    WMPlexPHolder *ph=FIELD_TO_STRUCT(WMPlexPHolder, mplex_watch, watch);
    do_unlink_ph(ph, (WMPlex*)mplex);
    pholder_redirect(&(ph->ph), (WRegion*)mplex);
}


bool mplexpholder_init(WMPlexPHolder *ph, WMPlex *mplex, 
                       WMPlexPHolder *after,
                       WLListNode *or_after, 
                       int or_layer)
{
    pholder_init(&(ph->ph));

    watch_init(&(ph->mplex_watch));
    ph->after=NULL;
    ph->next=NULL;
    ph->prev=NULL;
    
    if(mplex==NULL)
        return TRUE;
    
    if(!watch_setup(&(ph->mplex_watch), (Obj*)mplex, mplex_watch_handler)){
        pholder_deinit(&(ph->ph));
        return FALSE;
    }

    do_link_ph(ph, mplex, after, or_after, or_layer);
    
    return TRUE;
}
 

WMPlexPHolder *create_mplexpholder(WMPlex *mplex, 
                                   WMPlexPHolder *after,
                                   WLListNode *or_after, 
                                   int or_layer)
{
    CREATEOBJ_IMPL(WMPlexPHolder, mplexpholder, 
                   (p, mplex, after, or_after, or_layer));
}


void mplexpholder_deinit(WMPlexPHolder *ph)
{
    do_unlink_ph(ph, (WMPlex*)ph->mplex_watch.obj);
    watch_reset(&(ph->mplex_watch));
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Move, attach, layer */


int mplexpholder_layer(WMPlexPHolder *ph)
{
    WMPlex *mplex=(WMPlex*)ph->mplex_watch.obj;
    
    if(mplex==NULL)
        return -1;
    
    if(ph->after!=NULL){
        if(llist_is_node_on(mplex->l2_list, ph->after))
            return 2;
        
        assert(llist_is_node_on(mplex->l1_list, ph->after));
        
        return 1;
    }else{
        if(on_ph_list(mplex->l2_phs, ph))
            return 2;

        assert(on_ph_list(mplex->l1_phs, ph));
        
        return 1;
    }
}


bool mplexpholder_do_attach(WMPlexPHolder *ph, WRegionAttachHandler *hnd,
                            void *hnd_param)
{
    WMPlex *mplex=(WMPlex*)ph->mplex_watch.obj;
    WLListNode *nnode;
    WMPlexAttachParams param;
    WMPlexPHolder *ph2, *ph3=NULL;
    int layer;
    
    if(mplex==NULL)
        return FALSE;
    
    layer=mplexpholder_layer(ph);
    
    param.flags=(layer==2 ? MPLEX_ATTACH_L2 : 0);
    
    nnode=mplex_do_attach_after(mplex, ph->after, &param, hnd, hnd_param);
    
    if(nnode==NULL)
        return FALSE;
    
    /* Move following placeholders or_after node */
    
    while(ph->next!=NULL){
        ph2=ph->next;
        
        do_unlink_ph(ph2, mplex);
        do_link_ph(ph2, mplex, ph3, nnode, layer);
        
        ph3=ph2;
    }

    return TRUE;
}


bool mplexpholder_move(WMPlexPHolder *ph, WMPlex *mplex, 
                       WMPlexPHolder *after,
                       WLListNode *or_after, 
                       int or_layer)

{
    do_unlink_ph(ph, (WMPlex*)ph->mplex_watch.obj);

    watch_reset(&(ph->mplex_watch));
    
    if(mplex==NULL)
        return TRUE;
    
    if(!watch_setup(&(ph->mplex_watch), (Obj*)mplex, mplex_watch_handler))
        return FALSE;

    do_link_ph(ph, mplex, after, or_after, or_layer);
    
    return TRUE;
}


bool mplexpholder_do_goto(WMPlexPHolder *ph)
{
    WMPlex *mplex=(WMPlex*)ph->mplex_watch.obj;
    
    if(mplex!=NULL)
        return region_goto((WRegion*)mplex);
    
    return FALSE;
}


/*}}}*/


/*{{{ WMPlex routines */


void mplex_move_phs(WMPlex *mplex, WLListNode *node,
                    WMPlexPHolder *after,
                    WLListNode *or_after, 
                    int or_layer)
{
    WMPlexPHolder *ph;
    
    assert(node!=or_after);
    
    while(node->phs!=NULL){
        ph=node->phs;
        
        do_unlink_ph(ph, mplex);
        do_link_ph(ph, mplex, after, or_after, or_layer);
        
        after=ph;
    }
}

static WMPlexPHolder *node_last_ph(WMPlex *mplex, 
                                   WLListNode *lnode, int layer)
{
    WMPlexPHolder *lph=NULL;
    
    if(lnode!=NULL){
        lph=LIST_LAST(lnode->phs, next, prev);
    }else if(layer==1){
        lph=LIST_LAST(mplex->l1_phs, next, prev);
    }else{
        lph=LIST_LAST(mplex->l2_phs, next, prev);
    }
    
    return lph;
}


void mplex_move_phs_before(WMPlex *mplex, WLListNode *node, int layer)
{
    WMPlexPHolder *after=NULL;
    WLListNode *or_after;
    
    or_after=(layer==2 
              ? LIST_PREV(mplex->l2_list, node, next, prev)
              : LIST_PREV(mplex->l1_list, node, next, prev));
                         
    after=node_last_ph(mplex, or_after, layer);
        
    mplex_move_phs(mplex, node, after, or_after, layer);
}


WMPlexPHolder *mplex_managed_get_pholder(WMPlex *mplex, WRegion *mgd)
{
    WLListNode *node;
    
    node=llist_find_on(mplex->l2_list, mgd);
    if(node!=NULL)
        return create_mplexpholder(mplex, NULL, node, 2);

    node=llist_find_on(mplex->l1_list, mgd);
    if(node!=NULL)
        return create_mplexpholder(mplex, NULL, node, 1);
        
    return NULL;
}


WMPlexPHolder *mplex_get_rescue_pholder_for(WMPlex *mplex, WRegion *mgd)
{
    WMPlexPHolder *ph=mplex_managed_get_pholder(mplex, mgd);
    
    if(ph==NULL){
        /* Just put stuff at the end. We don't want rescues to fail. */
        ph=create_mplexpholder(mplex, NULL,
                               LIST_LAST(mplex->l1_list, next, prev), 1);
    }
    
    return ph;
}


WMPlexPHolder *mplex_last_place_holder(WMPlex *mplex, int layer)
{
    WLListNode *lnode=NULL;
    WMPlexPHolder *lph=NULL;
    
    if(layer==1)
        lnode=LIST_LAST(mplex->l1_list, next, prev);
    else if(layer==2)
        lnode=LIST_LAST(mplex->l2_list, next, prev);

    lph=node_last_ph(mplex, lnode, layer);
    
    return create_mplexpholder(mplex, lph, lnode, layer);
}


/*}}}*/


/*{{{ Class information */


static DynFunTab mplexpholder_dynfuntab[]={
    {(DynFun*)pholder_do_attach, 
     (DynFun*)mplexpholder_do_attach},
    
    {(DynFun*)pholder_do_goto, 
     (DynFun*)mplexpholder_do_goto},

    END_DYNFUNTAB
};

IMPLCLASS(WMPlexPHolder, WPHolder, mplexpholder_deinit, 
          mplexpholder_dynfuntab);


/*}}}*/

