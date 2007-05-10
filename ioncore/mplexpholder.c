/*
 * ion/ioncore/mplexpholder.c
 *
 * Copyright (c) Tuomo Valkonen 2005-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <libtu/objp.h>
#include <libtu/obj.h>
#include <libtu/pointer.h>

#include "common.h"
#include "mplex.h"
#include "mplexpholder.h"
#include "llist.h"
#include "framedpholder.h"


static void mplex_watch_handler(Watch *watch, Obj *mplex);


/*{{{ Primitives */


static bool on_ph_list(WMPlexPHolder *ll, WMPlexPHolder *ph)
{
    return ph->prev!=NULL;
}


static void mplexpholder_do_link(WMPlexPHolder *ph, 
                                 WMPlex *mplex,
                                 WMPlexPHolder *after,
                                 WLListNode *or_after)
{
    assert(mplex==(WMPlex*)ph->mplex_watch.obj && mplex!=NULL);
    
    if(after!=NULL){
        assert(after->after==or_after);
        
        if(after->after!=NULL){
            LINK_ITEM_AFTER(after->after->phs, after, ph, next, prev);
        }else{
            assert(on_ph_list(mplex->mx_phs, after));
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


void mplexpholder_do_unlink(WMPlexPHolder *ph, WMPlex *mplex)
{
    if(ph->recreate_pholder!=NULL){
        if(ph->prev!=NULL)
            ph->prev->recreate_pholder=ph->recreate_pholder;
        else
            destroy_obj((Obj*)ph->recreate_pholder);
        ph->recreate_pholder=NULL;
    }
    
    if(ph->after!=NULL){
        UNLINK_ITEM(ph->after->phs, ph, next, prev);
    }else if(mplex!=NULL && on_ph_list(mplex->mx_phs, ph)){
        UNLINK_ITEM(mplex->mx_phs, ph, next, prev);
    }else{
        WMPlexPHolder *next=ph->next;
    
        assert((ph->next==NULL && ph->prev==NULL)
               || ph->mplex_watch.obj==NULL);
        
        if(ph->next!=NULL)
            ph->next->prev=ph->prev;
        if(ph->prev!=NULL)
            ph->prev->next=next;
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
    mplexpholder_do_unlink(ph, (WMPlex*)mplex);
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
    ph->param.flags=0;
    ph->recreate_pholder=NULL;
    
    if(!watch_setup(&(ph->mplex_watch), (Obj*)mplex, mplex_watch_handler)){
        pholder_deinit(&(ph->ph));
        return FALSE;
    }

    if(param==NULL)
        param=&dummy_param;
    
    if(st!=NULL){
        if(st->lnode!=NULL)
            mplexpholder_do_link(ph, mplex, 
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

            mplexpholder_do_link(ph, mplex, after, or_after);
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
    mplexpholder_do_unlink(ph, (WMPlex*)ph->mplex_watch.obj);
    watch_reset(&(ph->mplex_watch));
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Move, attach */


typedef struct{
    WMPlexPHolder *ph, *ph_head;
    WRegionAttachData *data;
    WFramedParam *param;
} RP;


WRegion *recreate_handler(WWindow *par, 
                          const WFitParams *fp, 
                          void *rp_)
{
    RP *rp=(RP*)rp_;
    WMPlexPHolder *ph=rp->ph, *ph_head=rp->ph_head, *phtmp;
    WFramedParam *param=rp->param;
    WFrame *frame;
    WRegion *reg;
    
    frame=create_frame(par, fp, param->mode);
    
    if(frame==NULL)
        return NULL;
    
    /* Move pholders to frame */
    frame->mplex.mx_phs=ph_head;
    
    for(phtmp=frame->mplex.mx_phs; phtmp!=NULL; phtmp=phtmp->next)
        watch_setup(&(phtmp->mplex_watch), (Obj*)frame, mplex_watch_handler);
        
    /* Attach */
    if(fp->mode&(REGION_FIT_BOUNDS|REGION_FIT_WHATEVER))
        ph->param.flags|=MPLEX_ATTACH_WHATEVER;
    
    reg=mplex_do_attach_pholder(&frame->mplex, ph, rp->data);

    ph->param.flags&=~MPLEX_ATTACH_WHATEVER;

    if(reg==NULL){
        /* Try to recover */
        for(phtmp=frame->mplex.mx_phs; phtmp!=NULL; phtmp=phtmp->next)
            watch_reset(&(phtmp->mplex_watch));
        frame->mplex.mx_phs=NULL;
    
        destroy_obj((Obj*)frame);
        
        return NULL;
    }else{
        frame_adjust_to_initial(frame, fp, param, reg);
        
        return (WRegion*)frame;
    }
}


static WMPlexPHolder *get_head(WMPlexPHolder *ph)
{
    while(1){
        /* ph->prev==NULL should not happen.. */
        if(ph->prev==NULL || ph->prev->next==NULL)
            break;
        ph=ph->prev;
    }
    
    return ph;
}


static WFramedPHolder *get_recreate_ph(WMPlexPHolder *ph)
{
    return get_head(ph)->recreate_pholder;
}

    
static WRegion *mplexpholder_attach_recreate(WMPlexPHolder *ph, int flags,
                                             WRegionAttachData *data)
{
    WRegionAttachData data2;
    WFramedPHolder *fph;
    WPHolder *root;
    WRegion *reg;
    RP rp;
    
    rp.ph_head=get_head(ph);
    
    assert(rp.ph_head!=NULL);
    
    fph=rp.ph_head->recreate_pholder;
    
    if(fph==NULL)
        return NULL;
    
    rp.ph=ph;
    rp.data=data;
    rp.param=&fph->param;
    
    data2.type=REGION_ATTACH_NEW;
    data2.u.n.fn=recreate_handler;
    data2.u.n.param=&rp;
    
    reg=pholder_do_attach(fph->cont, flags, &data2); /* == frame */
    
    if(reg!=NULL){
        destroy_obj((Obj*)fph);
        rp.ph_head->recreate_pholder=NULL;
    }

    return reg;
}


WRegion *mplexpholder_do_attach(WMPlexPHolder *ph, int flags,
                                WRegionAttachData *data)
{
    WMPlex *mplex=(WMPlex*)ph->mplex_watch.obj;
    
    if(mplex==NULL)
        return mplexpholder_attach_recreate(ph, flags, data);
    
    if(flags&PHOLDER_ATTACH_SWITCHTO)
        ph->param.flags|=MPLEX_ATTACH_SWITCHTO;
    else
        ph->param.flags&=~MPLEX_ATTACH_SWITCHTO;
    
    return mplex_do_attach_pholder(mplex, ph, data);
}


bool mplexpholder_move(WMPlexPHolder *ph, WMPlex *mplex, 
                       WMPlexPHolder *after,
                       WLListNode *or_after)

{
    mplexpholder_do_unlink(ph, (WMPlex*)ph->mplex_watch.obj);

    watch_reset(&(ph->mplex_watch));
    
    if(mplex==NULL)
        return TRUE;
    
    if(!watch_setup(&(ph->mplex_watch), (Obj*)mplex, mplex_watch_handler))
        return FALSE;

    mplexpholder_do_link(ph, mplex, after, or_after);
    
    return TRUE;
}


bool mplexpholder_do_goto(WMPlexPHolder *ph)
{
    WRegion *reg=(WRegion*)ph->mplex_watch.obj;
    
    if(reg!=NULL){
        return region_goto(reg);
    }else{
        WFramedPHolder *fph=get_recreate_ph(ph);
        
        return (fph!=NULL
                ? pholder_do_goto((WPHolder*)fph)
                : FALSE);
    }
}


WRegion *mplexpholder_do_target(WMPlexPHolder *ph)
{
    WRegion *reg=(WRegion*)ph->mplex_watch.obj;
    
    if(reg!=NULL){
        return reg;
    }else{
        WFramedPHolder *fph=get_recreate_ph(ph);
        
        return (fph!=NULL
                ? pholder_do_target((WPHolder*)fph)
                : NULL);
    }
}


WPHolder *mplexpholder_do_root(WMPlexPHolder *ph)
{
    WRegion *reg=(WRegion*)ph->mplex_watch.obj;
    
    if(reg!=NULL){
        return &ph->ph;
    }else{
        WFramedPHolder *fph=get_recreate_ph(ph);
        WPHolder *root;
        
        if(fph==NULL)
            return NULL;
    
        root=pholder_root((WPHolder*)fph);
    
        return (root!=(WPHolder*)fph
                ? root
                : &ph->ph);
    }
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
        
        mplexpholder_do_unlink(ph, mplex);
        mplexpholder_do_link(ph, mplex, after, or_after);
        
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
     
    {(DynFun*)pholder_do_root, 
     (DynFun*)mplexpholder_do_root},

    END_DYNFUNTAB
};


IMPLCLASS(WMPlexPHolder, WPHolder, mplexpholder_deinit, 
          mplexpholder_dynfuntab);


/*}}}*/

