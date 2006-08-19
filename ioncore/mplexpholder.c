/*
 * ion/ioncore/mplexpholder.c
 *
 * Copyright (c) Tuomo Valkonen 2005-2006. 
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
#include "llist.h"
#include "group-cw.h"


static void mplex_watch_handler(Watch *watch, Obj *mplex);


/*{{{ Primitives */


static bool on_ph_list(WMPlexPHolder *ll, WMPlexPHolder *ph)
{
    return ph->prev!=NULL;
}


static void do_link_ph(WMPlexPHolder *ph, 
                       WMPlex *mplex,
                       WMPlexPHolder *after,
                       WLListNode *or_after)
{
    assert(mplex==(WMPlex*)ph->mplex_watch.obj && mplex!=NULL);
    
    if(after!=NULL){
        if(after->after!=NULL){
            LINK_ITEM_AFTER(after->after->phs, after, ph, next, prev);
        }else{
            assert(on_ph_list(mplex->mx_phs, ph));
            LINK_ITEM_AFTER(mplex->mx_phs, after, ph, next, prev);
        }
        ph->after=after->after;
    }else if(or_after!=NULL){
        LINK_ITEM_FIRST(or_after->phs, ph, next, prev);
        ph->after=or_after;
    }else{
        LINK_ITEM_FIRST(mplex->mx_phs, ph, next, prev);
        ph->after=NULL;
    }
}


static void do_unlink_ph(WMPlexPHolder *ph, WMPlex *mplex)
{
    if(ph->after!=NULL){
        UNLINK_ITEM(ph->after->phs, ph, next, prev);
    }else if(mplex!=NULL){
        assert(on_ph_list(mplex->mx_phs, ph));
        UNLINK_ITEM(mplex->mx_phs, ph, next, prev);
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


static WMPlexAttachParams dummy_param={0, 0, {0, 0, 0, 0}, 0, 0};


bool mplexpholder_init(WMPlexPHolder *ph, WMPlex *mplex, WStacking *st,
                       WMPlexAttachParams *param)
{
    pholder_init(&(ph->ph));

    watch_init(&(ph->mplex_watch));
    ph->after=NULL;
    ph->next=NULL;
    ph->prev=NULL;
    ph->initial=FALSE;
    ph->param.flags=0;
    
    if(!watch_setup(&(ph->mplex_watch), (Obj*)mplex, mplex_watch_handler)){
        pholder_deinit(&(ph->ph));
        return FALSE;
    }

    if(param==NULL)
        param=&dummy_param;
    
    if(st!=NULL){
        if(st->lnode!=NULL)
            do_link_ph(ph, mplex, 
                       LIST_LAST(st->lnode->phs, next, prev), 
                       st->lnode);
        else
            ph->param.flags|=MPLEX_ATTACH_UNNUMBERED;
        
        ph->param.flags|=(MPLEX_ATTACH_SIZEPOLICY|
                          MPLEX_ATTACH_GEOM|
                          MPLEX_ATTACH_LEVEL|
                          (st->hidden ? MPLEX_ATTACH_HIDDEN : 0));
        ph->param.szplcy=st->szplcy;
        ph->param.geom=REGION_GEOM(st->reg);
        ph->param.level=st->level;
    }else{
        ph->param=*param;
        
        if(!(param->flags&MPLEX_ATTACH_UNNUMBERED)){
            int index=(param->flags&MPLEX_ATTACH_INDEX
                       ? param->index
                       : mplex_default_index(mplex));
            WLListNode *or_after=llist_index_to_after(mplex->mx_list, 
                                                      mplex->mx_current, 
                                                      index);
            WMPlexPHolder *after=(index==LLIST_INDEX_LAST
                                  ? (or_after!=NULL
                                     ? LIST_LAST(or_after->phs, next, prev) 
                                     : LIST_LAST(mplex->mx_phs, next, prev))
                                  : NULL);

            do_link_ph(ph, mplex, after, or_after);
        }
    }
    
    return TRUE;
}
 

WMPlexPHolder *create_mplexpholder(WMPlex *mplex,
                                   WStacking *st,
                                   WMPlexAttachParams *param)
{
    CREATEOBJ_IMPL(WMPlexPHolder, mplexpholder, (p, mplex, st, param));
}


void mplexpholder_deinit(WMPlexPHolder *ph)
{
    do_unlink_ph(ph, (WMPlex*)ph->mplex_watch.obj);
    watch_reset(&(ph->mplex_watch));
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Move, attach, layer */


typedef struct{
    WRegionAttachHandler *trs_fn;
    void *trs_fnparams;
} GroupedParam;


WRegion *grouped_handler(WWindow *par, 
                         const WFitParams *fp, 
                         void *frp_)
{
    GroupedParam *frp=(GroupedParam*)frp_;
    WGroupAttachParams param=GROUPATTACHPARAMS_INIT;
    WGroupCW *cwg;
    WRegion *reg;
    WStacking *st;

    cwg=create_groupcw(par, fp);
    
    if(cwg==NULL)
        return NULL;
    
    param.level_set=1;
    param.level=STACKING_LEVEL_BOTTOM;
    param.switchto_set=1;
    param.switchto=1;
    param.bottom=1;
    
    if(!(fp->mode&REGION_FIT_WHATEVER)){
        param.geom_set=1;
        param.geom=fp->g;
        param.szplcy=SIZEPOLICY_FULL_EXACT;
        param.szplcy_set=TRUE;
    }
    
    reg=group_do_attach(&cwg->grp, frp->trs_fn, frp->trs_fnparams, &param);
    
    if(reg==NULL){
        destroy_obj((Obj*)cwg);
        return NULL;
    }
    
    st=group_find_stacking(&cwg->grp, reg);
    
    if(st!=NULL){
        st->szplcy=SIZEPOLICY_FULL_EXACT;
        REGION_GEOM(cwg)=REGION_GEOM(reg);
    }
    
    return (WRegion*)cwg;
}


bool mplexpholder_do_attach(WMPlexPHolder *ph, 
                            WRegionAttachHandler *hnd, void *hnd_param, 
                            int flags)
{
    WMPlex *mplex=(WMPlex*)ph->mplex_watch.obj;
    WStacking *nnode=NULL;
    WMPlexPHolder *ph2, *ph3=NULL;
    
    if(mplex==NULL)
        return FALSE;
    
    if(flags&PHOLDER_ATTACH_SWITCHTO)
        ph->param.flags|=MPLEX_ATTACH_SWITCHTO;
    else
        ph->param.flags&=~MPLEX_ATTACH_SWITCHTO;
    
    if(ph->initial){
        GroupedParam frp;
        frp.trs_fn=hnd;
        frp.trs_fnparams=hnd_param;
        
        nnode=mplex_do_attach_after(mplex, ph->after, 
                                    &ph->param, grouped_handler, &frp);
    }
    
    if(nnode==NULL)
        nnode=mplex_do_attach_after(mplex, ph->after, 
                                    &ph->param, hnd, hnd_param);
    
    if(nnode==NULL)
        return FALSE;
    
    if(nnode->lnode!=NULL){
        /* Move following placeholders after new node */
        while(ph->next!=NULL){
            ph2=ph->next;
            do_unlink_ph(ph2, mplex);
            LINK_ITEM_FIRST(nnode->lnode->phs, ph2, next, prev);
        }
    }

    return TRUE;
}


bool mplexpholder_move(WMPlexPHolder *ph, WMPlex *mplex, 
                       WMPlexPHolder *after,
                       WLListNode *or_after)

{
    do_unlink_ph(ph, (WMPlex*)ph->mplex_watch.obj);

    watch_reset(&(ph->mplex_watch));
    
    if(mplex==NULL)
        return TRUE;
    
    if(!watch_setup(&(ph->mplex_watch), (Obj*)mplex, mplex_watch_handler))
        return FALSE;

    do_link_ph(ph, mplex, after, or_after);
    
    return TRUE;
}


bool mplexpholder_do_goto(WMPlexPHolder *ph)
{
    WMPlex *mplex=(WMPlex*)ph->mplex_watch.obj;
    
    if(mplex!=NULL)
        return region_goto((WRegion*)mplex);
    
    return FALSE;
}


WRegion *mplexpholder_do_target(WMPlexPHolder *ph)
{
    return (WRegion*)ph->mplex_watch.obj;
}


/*}}}*/


/*{{{ WMPlex routines */


void mplex_move_phs(WMPlex *mplex, WLListNode *node,
                    WMPlexPHolder *after,
                    WLListNode *or_after)
{
    WMPlexPHolder *ph;
    
    assert(node!=or_after);
    
    while(node->phs!=NULL){
        ph=node->phs;
        
        do_unlink_ph(ph, mplex);
        do_link_ph(ph, mplex, after, or_after);
        
        after=ph;
    }
}

void mplex_move_phs_before(WMPlex *mplex, WLListNode *node)
{
    WMPlexPHolder *after=NULL;
    WLListNode *or_after;
    
    or_after=LIST_PREV(mplex->mx_list, node, next, prev);
                         
    after=(or_after!=NULL
           ? LIST_LAST(or_after->phs, next, prev)
           : LIST_LAST(mplex->mx_phs, next, prev));
        
    mplex_move_phs(mplex, node, after, or_after);
}


WMPlexPHolder *mplex_managed_get_pholder(WMPlex *mplex, WRegion *mgd)
{
    WStacking *st=mplex_find_stacking(mplex, mgd);
    
    if(st==NULL)
        return NULL;
    else
        return create_mplexpholder(mplex, st, NULL);
}


WMPlexPHolder *mplex_get_rescue_pholder_for(WMPlex *mplex, WRegion *mgd)
{
    WStacking *st=mplex_find_stacking(mplex, mgd);
    
    return create_mplexpholder(mplex, st, NULL);
}


/*}}}*/


/*{{{ Class information */


static DynFunTab mplexpholder_dynfuntab[]={
    {(DynFun*)pholder_do_attach, 
     (DynFun*)mplexpholder_do_attach},
    
    {(DynFun*)pholder_do_goto, 
     (DynFun*)mplexpholder_do_goto},

    {(DynFun*)pholder_do_target, 
     (DynFun*)mplexpholder_do_target},

    END_DYNFUNTAB
};


IMPLCLASS(WMPlexPHolder, WPHolder, mplexpholder_deinit, 
          mplexpholder_dynfuntab);


/*}}}*/

