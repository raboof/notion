/*
 * ion/ioncore/mplexpholder.c
 *
 * Copyright (c) Tuomo Valkonen 2005-2008. 
 *
 * See the included file LICENSE for details.
 */

#include <libtu/objp.h>
#include <libtu/obj.h>
#include <libtu/pointer.h>
#include <libmainloop/defer.h>

#include "common.h"
#include "mplex.h"
#include "mplexpholder.h"
#include "llist.h"
#include "framedpholder.h"
#include "basicpholder.h"


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
    assert(mplex==(WMPlex*)ph->mplex && mplex!=NULL);
    
    if(after!=NULL){
        assert(after->after==or_after);
        
        if(after->after!=NULL){
            LINK_ITEM_AFTER(after->after->phs, after, ph, next, prev);
        }else{
            assert(on_ph_list(mplex->misc_phs, after));
            LINK_ITEM_AFTER(mplex->misc_phs, after, ph, next, prev);
        }
        ph->after=after->after;
    }else if(or_after!=NULL){
        LINK_ITEM_FIRST(or_after->phs, ph, next, prev);
        ph->after=or_after;
    }else{
        LINK_ITEM_FIRST(mplex->misc_phs, ph, next, prev);
        ph->after=NULL;
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


void mplexpholder_do_unlink(WMPlexPHolder *ph, WMPlex *mplex)
{
    if(ph->recreate_pholder!=NULL){
        if(ph->next!=NULL){
            ph->next->recreate_pholder=ph->recreate_pholder;
        }else{
            /* It might be in use in attach chain! So defer. */
            mainloop_defer_destroy((Obj*)ph->recreate_pholder);
        }
        ph->recreate_pholder=NULL;
    }
    
    if(ph->after!=NULL){
        UNLINK_ITEM(ph->after->phs, ph, next, prev);
    }else if(mplex!=NULL && on_ph_list(mplex->misc_phs, ph)){
        UNLINK_ITEM(mplex->misc_phs, ph, next, prev);
    }else if(ph->prev!=NULL){
        WMPlexPHolder *next=ph->next;

        ph->prev->next=next;

        if(next==NULL){
            next=get_head(ph);
            assert(next->prev==ph);
        }
        next->prev=ph->prev;
    }else{
        /* ph should not be on a list, if prev pointer is NULL (whereas
         * next alone can be NULL in our semi-doubly-linked lists).
         */
        assert(ph->next==NULL);
    }
    
    ph->after=NULL;
    ph->next=NULL;
    ph->prev=NULL;
}


/*}}}*/


/*{{{ Init/deinit */


static void mplex_get_attach_params(WMPlex *mplex, WStacking *st,
                                    WMPlexAttachParams *param)
{
    param->flags=(MPLEX_ATTACH_SIZEPOLICY|
                  MPLEX_ATTACH_GEOM|
                  MPLEX_ATTACH_LEVEL|
                  (st->hidden ? MPLEX_ATTACH_HIDDEN : 0)|
                  (st->lnode==NULL ? MPLEX_ATTACH_UNNUMBERED : 0));
    param->szplcy=st->szplcy;
    param->geom=REGION_GEOM(st->reg);
    param->level=st->level;
}


bool mplexpholder_init(WMPlexPHolder *ph, WMPlex *mplex, WStacking *st,
                       WMPlexAttachParams *param)
{
    WLListNode *or_after=NULL;
    WMPlexPHolder *after=NULL;
    
    pholder_init(&(ph->ph));

    ph->mplex=mplex;
    ph->after=NULL;
    ph->next=NULL;
    ph->prev=NULL;
    ph->param.flags=0;
    ph->recreate_pholder=NULL;

    if(st!=NULL){
        mplex_get_attach_params(mplex, st, &ph->param);
        
        if(st->lnode!=NULL){
            after=LIST_LAST(st->lnode->phs, next, prev);
            or_after=st->lnode;
        }
    }else{
        static WMPlexAttachParams dummy_param={0, 0, {0, 0, 0, 0}, 0, 0};
        
        if(param==NULL)
            param=&dummy_param;
        
        ph->param=*param;
        
        if(!(param->flags&MPLEX_ATTACH_UNNUMBERED)){
            int index=(param->flags&MPLEX_ATTACH_INDEX
                       ? param->index
                       : mplex_default_index(mplex));
            or_after=llist_index_to_after(mplex->mx_list, 
                                          mplex->mx_current, index);
            after=(index==LLIST_INDEX_LAST
                   ? (or_after!=NULL
                      ? LIST_LAST(or_after->phs, next, prev) 
                      : LIST_LAST(mplex->misc_phs, next, prev))
                   : NULL);
        }
    }

    mplexpholder_do_link(ph, mplex, after, or_after);
    
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
    mplexpholder_do_unlink(ph, ph->mplex);
    pholder_deinit(&(ph->ph));
}


/*}}}*/


/*{{{ Move, attach */


typedef struct{
    WMPlexPHolder *ph, *ph_head;
    WRegionAttachData *data;
    WFramedParam *param;
    WRegion *reg_ret;
} RP;


static WRegion *recreate_handler(WWindow *par, 
                                 const WFitParams *fp, 
                                 void *rp_)
{
    RP *rp=(RP*)rp_;
    WMPlexPHolder *ph=rp->ph, *ph_head=rp->ph_head, *phtmp;
    WFramedParam *param=rp->param;
    WFrame *frame;
    
    frame=create_frame(par, fp, param->mode);
    
    if(frame==NULL)
        return NULL;
    
    /* Move pholders to frame */
    frame->mplex.misc_phs=ph_head;
    
    for(phtmp=frame->mplex.misc_phs; phtmp!=NULL; phtmp=phtmp->next)
        phtmp->mplex=&frame->mplex;
        
    /* Attach */
    if(fp->mode&(REGION_FIT_BOUNDS|REGION_FIT_WHATEVER))
        ph->param.flags|=MPLEX_ATTACH_WHATEVER;
    
    rp->reg_ret=mplex_do_attach_pholder(&frame->mplex, ph, rp->data);

    ph->param.flags&=~MPLEX_ATTACH_WHATEVER;

    if(rp->reg_ret==NULL){
        /* Try to recover */
        for(phtmp=frame->mplex.misc_phs; phtmp!=NULL; phtmp=phtmp->next)
            phtmp->mplex=NULL;
        
        frame->mplex.misc_phs=NULL;
    
        destroy_obj((Obj*)frame);
        
        return NULL;
    }else{
        frame_adjust_to_initial(frame, fp, param, rp->reg_ret);
        
        return (WRegion*)frame;
    }
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
    WRegion *frame;
    RP rp;
    
    rp.ph_head=get_head(ph);
    
    assert(rp.ph_head!=NULL);
    
    fph=rp.ph_head->recreate_pholder;
    
    if(fph==NULL)
        return NULL;
    
    rp.ph=ph;
    rp.data=data;
    rp.param=&fph->param;
    rp.reg_ret=NULL;
    
    data2.type=REGION_ATTACH_NEW;
    data2.u.n.fn=recreate_handler;
    data2.u.n.param=&rp;
    
    frame=pholder_do_attach(fph->cont, flags, &data2);
    
    if(frame!=NULL){
        rp.ph_head->recreate_pholder=NULL;
        /* It might be in use in attach chain! So defer. */
        mainloop_defer_destroy((Obj*)fph);
    }
    
    return rp.reg_ret;
}


WRegion *mplexpholder_do_attach(WMPlexPHolder *ph, int flags,
                                WRegionAttachData *data)
{
    WMPlex *mplex=ph->mplex;
    
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
    mplexpholder_do_unlink(ph, ph->mplex);

    ph->mplex=mplex;
        
    if(mplex==NULL)
        return TRUE;
    
    mplexpholder_do_link(ph, mplex, after, or_after);
    
    return TRUE;
}


bool mplexpholder_do_goto(WMPlexPHolder *ph)
{
    WRegion *reg=(WRegion*)ph->mplex;
    
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
    WRegion *reg=(WRegion*)ph->mplex;
    
    if(reg!=NULL){
        return reg;
    }else{
        WFramedPHolder *fph=get_recreate_ph(ph);
        
        return (fph!=NULL
                ? pholder_do_target((WPHolder*)fph)
                : NULL);
    }
}


bool mplexpholder_stale(WMPlexPHolder *ph)
{
    WRegion *reg=(WRegion*)ph->mplex;
    
    if(reg!=NULL){
        return FALSE;
    }else{
        WFramedPHolder *fph=get_recreate_ph(ph);
        
        return (fph==NULL || pholder_stale((WPHolder*)fph));
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
           : LIST_LAST(mplex->misc_phs, next, prev));
        
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


void mplex_flatten_phs(WMPlex *mplex)
{
    WLListNode *node;
    WLListIterTmp tmp;

    FOR_ALL_NODES_ON_LLIST(node, mplex->mx_list, tmp){
        WMPlexPHolder *last=(mplex->misc_phs==NULL ? NULL : mplex->misc_phs->prev);
        mplex_move_phs(mplex, node, last, NULL);
    }
}


void mplex_migrate_phs(WMPlex *src, WMPlex *dst)
{
    WLListNode *or_after=LIST_LAST(dst->mx_list, next, prev);
    WMPlexPHolder *after=(or_after!=NULL
                          ? LIST_LAST(or_after->phs, next, prev)
                          : LIST_LAST(dst->misc_phs, next, prev));
    
    while(src->misc_phs!=NULL){
        WMPlexPHolder *ph=src->misc_phs;
        mplexpholder_move(ph, dst, after, or_after);
        after=ph;
    }
}


/*}}}*/


/*{{ Rescue */


WRegion *mplex_rescue_attach(WMPlex *mplex, int flags, WRegionAttachData *data)
{
    WMPlexAttachParams param;
    
    param.flags=0;
    
    /* Improved attach parametrisation hack for WMPlex source */
    if(data->type==REGION_ATTACH_REPARENT){
        WRegion *reg=data->u.reg;
        WMPlex *src_mplex=REGION_MANAGER_CHK(reg, WMPlex);
        if(src_mplex!=NULL){
            WStacking *st=ioncore_find_stacking(reg);
            if(st!=NULL)
                mplex_get_attach_params(src_mplex, st, &param);
        }
    }
    
    param.flags|=(MPLEX_ATTACH_INDEX|
                  (flags&PHOLDER_ATTACH_SWITCHTO ? MPLEX_ATTACH_SWITCHTO : 0));
    param.index=LLIST_INDEX_LAST;

    return mplex_do_attach(mplex, &param, data);
}


WPHolder *mplex_get_rescue_pholder_for(WMPlex *mplex, WRegion *mgd)
{
#if 0
    WStacking *st=mplex_find_stacking(mplex, mgd);
    WMPlexAttachParams param;
    
    param.flags=MPLEX_ATTACH_INDEX;
    param.index=LLIST_INDEX_LAST;
    
    return create_mplexpholder(mplex, st, &param);
#else
    return (WPHolder*)create_basicpholder((WRegion*)mplex, 
                                          (WBasicPHolderHandler*)mplex_rescue_attach);
#endif
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
     
    {(DynFun*)pholder_stale, 
     (DynFun*)mplexpholder_stale},

    END_DYNFUNTAB
};


IMPLCLASS(WMPlexPHolder, WPHolder, mplexpholder_deinit, 
          mplexpholder_dynfuntab);


/*}}}*/

