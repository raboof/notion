/*
 * ion/ioncore/mplex.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <limits.h>
#include <string.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libtu/rb.h>
#include <libextl/extl.h>
#include <libmainloop/defer.h>

#include "common.h"
#include "window.h"
#include "global.h"
#include "rootwin.h"
#include "focus.h"
#include "event.h"
#include "attach.h"
#include "manage.h"
#include "resize.h"
#include "tags.h"
#include "sizehint.h"
#include "extlconv.h"
#include "frame-pointer.h"
#include "bindmaps.h"
#include "regbind.h"
#include "saveload.h"
#include "xwindow.h"
#include "mplexpholder.h"
#include "llist.h"
#include "names.h"
#include "sizepolicy.h"
#include "stacking.h"
#include "group.h"
#include "navi.h"


#define SUBS_MAY_BE_MAPPED(MPLEX) \
    (REGION_IS_MAPPED(MPLEX) && !MPLEX_MGD_UNVIEWABLE(MPLEX))


#define CAN_MANAGE_STDISP(REG) HAS_DYN(REG, region_manage_stdisp)


/*{{{ Stacking list stuff */


WStacking *mplex_get_stacking(WMPlex *mplex)
{
    return window_get_stacking(&mplex->win);
}


WStacking **mplex_get_stackingp(WMPlex *mplex)
{
    return window_get_stackingp(&mplex->win);
}


void mplex_iter_init(WMPlexIterTmp *tmp, WMPlex *mplex)
{
    stacking_iter_mgr_init(tmp, mplex->mgd, NULL, mplex);
}


WRegion *mplex_iter(WMPlexIterTmp *tmp)
{
    return stacking_iter_mgr(tmp);
}


WStacking *mplex_iter_nodes(WMPlexIterTmp *tmp)
{
    return stacking_iter_mgr_nodes(tmp);
}


/*}}}*/


/*{{{ Destroy/create mplex */


bool mplex_do_init(WMPlex *mplex, WWindow *parent, Window win,
                   const WFitParams *fp, bool create)
{
    mplex->flags=0;
    
    mplex->mx_list=NULL;
    mplex->mx_current=NULL;
    mplex->mx_phs=NULL;
    mplex->mx_count=0;
    
    mplex->mgd=NULL;
    
    watch_init(&(mplex->stdispwatch));
    mplex->stdispinfo.pos=MPLEX_STDISP_BL;
    mplex->stdispinfo.fullsize=FALSE;
    
    if(create){
        if(!window_init((WWindow*)mplex, parent, fp))
            return FALSE;
    }else{
        if(!window_do_init((WWindow*)mplex, parent, win, fp))
            return FALSE;
    }
    
    mplex->win.region.flags|=REGION_BINDINGS_ARE_GRABBED;
    
    window_select_input(&(mplex->win), IONCORE_EVENTMASK_CWINMGR);
    
    region_add_bindmap((WRegion*)mplex, ioncore_mplex_bindmap);
    region_add_bindmap((WRegion*)mplex, ioncore_mplex_toplevel_bindmap);
    
    region_register((WRegion*)mplex);
    
    /* Call this to set MPLEX_MANAGED_UNVIEWABLE if necessary. */
    mplex_fit_managed(mplex);
    
    return TRUE;
}


bool mplex_init(WMPlex *mplex, WWindow *parent, const WFitParams *fp)
{
    return mplex_do_init(mplex, parent, None, fp, TRUE);
}


WMPlex *create_mplex(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WMPlex, mplex, (p, parent, fp));
}


void mplex_deinit(WMPlex *mplex)
{
    WMPlexIterTmp tmp;
    WRegion *reg;
    
    FOR_ALL_MANAGED_BY_MPLEX(mplex, reg, tmp){
        destroy_obj((Obj*)reg);
    }
    
    assert(mplex->mgd==NULL);
    assert(mplex->mx_list==NULL);

    while(mplex->mx_phs!=NULL){
        assert(mplexpholder_move(mplex->mx_phs, NULL, NULL, NULL));
    }
    
    window_deinit((WWindow*)mplex);
}


bool mplex_may_destroy(WMPlex *mplex)
{
    if(mplex->mgd!=NULL){
        warn(TR("Refusing to destroy - not empty."));
        return FALSE;
    }
    
    return TRUE;
}


/*}}}*/


/*{{{ Node lookup etc. */


WStacking *mplex_find_stacking(WMPlex *mplex, WRegion *reg)
{
    WStacking *st;
    
    /* Some routines that call us expect us to this check. */
    if(reg==NULL || REGION_MANAGER(reg)!=(WRegion*)mplex)
        return NULL;
    
    st=ioncore_find_stacking(reg);

    assert(st==NULL || st->mgr_prev!=NULL);
    
    return st;
}


WStacking *mplex_current_node(WMPlex *mplex)
{
    WRegion *reg;
    
    reg=REGION_ACTIVE_SUB(mplex);
    reg=region_managed_within((WRegion*)mplex, reg);
    
    return (reg==NULL ? NULL : mplex_find_stacking(mplex, reg));
}


WRegion *mplex_current(WMPlex *mplex)
{
    WStacking *node=mplex_current_node(mplex);
    return (node==NULL ? NULL : node->reg);
}


/*}}}*/


/*{{{ Exclusive list management and exports */

/*EXTL_DOC
 * Returns the number of objects on the mutually exclusive list of \var{mplex}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
int mplex_mx_count(WMPlex *mplex)
{
    return mplex->mx_count;
}


/*EXTL_DOC
 * Returns the managed object currently active within the mutually exclusive
 * list of \var{mplex}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WRegion *mplex_mx_current(WMPlex *mplex)
{
    WLListNode *lnode=mplex->mx_current;
    return (lnode==NULL ? NULL : lnode->st->reg);
}


/*EXTL_DOC
 * Returns the \var{n}:th object managed by \var{mplex} on the
 * \var{l}:th layer.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WRegion *mplex_mx_nth(WMPlex *mplex, uint n)
{
    WLListNode *lnode=llist_nth_node(mplex->mx_list, n);
    return (lnode==NULL ? NULL : lnode->st->reg);
}


/*EXTL_DOC
 * Returns a list of regions on the numbered/mutually exclusive list of 
 * \var{mplex}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
ExtlTab mplex_mx_list(WMPlex *mplex)
{
    WLListIterTmp tmp;
    llist_iter_init(&tmp, mplex->mx_list);
    
    return extl_obj_iterable_to_table((ObjIterator*)llist_iter_regions, &tmp);
}


/*EXTL_DOC
 * Returns a list of all regions managed by \var{mplex}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
ExtlTab mplex_managed_list(WMPlex *mplex)
{
    WMPlexIterTmp tmp;
    mplex_iter_init(&tmp, mplex);
    
    return extl_obj_iterable_to_table((ObjIterator*)mplex_iter, &tmp);
}


/*EXTL_DOC
 * Set index of \var{reg} within the multiplexer to \var{index} within 
 * the mutually exclusive list. Special values for \var{index} are:
 * \begin{tabularx}{\linewidth}{lX}
 *   $-1$ & After \fnref{WMPlex.mx_current}. \\
 *   $-2$ & Last. \\
 * \end{tabularx}
 */
EXTL_EXPORT_MEMBER
void mplex_set_index(WMPlex *mplex, WRegion *reg, int index)
{
    WLListNode *lnode, *after;
    WStacking *node;
    
    node=mplex_find_stacking(mplex, reg);

    if(node==NULL)
        return;
    
    lnode=node->lnode;
    
    if(lnode==NULL){
        lnode=ALLOC(WLListNode);
        if(lnode==NULL)
            return;
        lnode->next=NULL;
        lnode->prev=NULL;
        lnode->phs=NULL;
        lnode->st=node;
        node->lnode=lnode;
        mplex->mx_count++;
    }else{
        mplex_move_phs_before(mplex, lnode);
        llist_unlink(&(mplex->mx_list), lnode);
    }
    
    /* TODO: Support remove? */
    
    after=llist_index_to_after(mplex->mx_list, mplex->mx_current, index);
    llist_link_after(&(mplex->mx_list), after, lnode);
    mplex_managed_changed(mplex, MPLEX_CHANGE_REORDER, FALSE, reg);
}


/*EXTL_DOC
 * Get index of \var{reg} within the multiplexer on list 1. The first region 
 * managed by \var{mplex} has index zero. If \var{reg} is not managed by 
 * \var{mplex}, -1 is returned.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
int mplex_get_index(WMPlex *mplex, WRegion *reg)
{
    WLListIterTmp tmp;
    WLListNode *lnode;
    int index=0;
    
    FOR_ALL_NODES_ON_LLIST(lnode, mplex->mx_list, tmp){
        if(reg==lnode->st->reg)
            return index;
        index++;
    }
    
    return -1;
}


/*EXTL_DOC
 * Move \var{r} ''right'' within objects managed by \var{mplex} on list 1.
 */
EXTL_EXPORT_MEMBER
void mplex_inc_index(WMPlex *mplex, WRegion *r)
{
    if(r==NULL)
        r=mplex_mx_current(mplex);
    if(r!=NULL)
        mplex_set_index(mplex, r, mplex_get_index(mplex, r)+1);
}


/*EXTL_DOC
 * Move \var{r} ''right'' within objects managed by \var{mplex} on list 1.
 */
EXTL_EXPORT_MEMBER
void mplex_dec_index(WMPlex *mplex, WRegion *r)
{
    if(r==NULL)
        r=mplex_mx_current(mplex);
    if(r!=NULL)
        mplex_set_index(mplex, r, mplex_get_index(mplex, r)-1);
}


/*}}}*/


/*{{{ Mapping */


static void mplex_map_mgd(WMPlex *mplex)
{
    WMPlexIterTmp tmp;
    WStacking *node;

    FOR_ALL_NODES_IN_MPLEX(mplex, node, tmp){
        if(!STACKING_IS_HIDDEN(node))
            region_map(node->reg);
    }
}


static void mplex_unmap_mgd(WMPlex *mplex)
{
    WMPlexIterTmp tmp;
    WStacking *node;

    FOR_ALL_NODES_IN_MPLEX(mplex, node, tmp){
        if(!STACKING_IS_HIDDEN(node))
            region_unmap(node->reg);
    }
}



void mplex_map(WMPlex *mplex)
{
    window_map((WWindow*)mplex);
    /* A lame requirement of the ICCCM is that client windows should be
     * unmapped if the parent is unmapped.
     */
    if(!MPLEX_MGD_UNVIEWABLE(mplex))
        mplex_map_mgd(mplex);
}


void mplex_unmap(WMPlex *mplex)
{
    window_unmap((WWindow*)mplex);
    /* A lame requirement of the ICCCM is that client windows should be
     * unmapped if the parent is unmapped.
     */
    if(!MPLEX_MGD_UNVIEWABLE(mplex))
        mplex_unmap_mgd(mplex);
}


/*}}}*/


/*{{{ Resize and reparent */


bool mplex_fitrep(WMPlex *mplex, WWindow *par, const WFitParams *fp)
{
    bool wchg=(REGION_GEOM(mplex).w!=fp->g.w);
    bool hchg=(REGION_GEOM(mplex).h!=fp->g.h);
    
    window_do_fitrep(&(mplex->win), par, &(fp->g));
    
    if(wchg || hchg){
        mplex_fit_managed(mplex);
        mplex_size_changed(mplex, wchg, hchg);
    }
    
    return TRUE;
}


void mplex_do_fit_managed(WMPlex *mplex, WFitParams *fp)
{
    WRectangle geom;
    WMPlexIterTmp tmp;
    WStacking *node;
    WFitParams fp2;
    
    if(!MPLEX_MGD_UNVIEWABLE(mplex) && (fp->g.w<=1 || fp->g.h<=1)){
        mplex->flags|=MPLEX_MANAGED_UNVIEWABLE;
        if(REGION_IS_MAPPED(mplex))
            mplex_unmap_mgd(mplex);
    }else if(MPLEX_MGD_UNVIEWABLE(mplex) && !(fp->g.w<=1 || fp->g.h<=1)){
        mplex->flags&=~MPLEX_MANAGED_UNVIEWABLE;
        if(REGION_IS_MAPPED(mplex))
            mplex_map_mgd(mplex);
    }
    
    if(!MPLEX_MGD_UNVIEWABLE(mplex)){
        FOR_ALL_NODES_IN_MPLEX(mplex, node, tmp){
            fp2=*fp;
            sizepolicy(&node->szplcy, node->reg, NULL, 0, &fp2);
            region_fitrep(node->reg, NULL, &fp2);
        }
    }
}


void mplex_fit_managed(WMPlex *mplex)
{
    WFitParams fp;
    
    fp.mode=REGION_FIT_EXACT;
    mplex_managed_geom(mplex, &(fp.g));

    mplex_do_fit_managed(mplex, &fp);
}


static void mplex_managed_rqgeom(WMPlex *mplex, WRegion *sub,
                                 int flags, const WRectangle *geom, 
                                 WRectangle *geomret)
{
    WRectangle rg;
    WFitParams fp;
    WStacking *node;

    node=mplex_find_stacking(mplex, sub);
    
    assert(node!=NULL);

    mplex_managed_geom(mplex, &fp.g);

    sizepolicy(&node->szplcy, sub, geom, flags, &fp);
    
    if(geomret!=NULL)
        *geomret=fp.g;
    
    if(!(flags&REGION_RQGEOM_TRYONLY))
        region_fitrep(sub, NULL, &fp);
}


/*}}}*/


/*{{{ Focus  */


static WRegion *mplex_do_to_focus(WMPlex *mplex, WStacking *to_try)
{
    WStacking *stacking=mplex_get_stacking(mplex);
    WStacking *st=NULL;
    
    if(stacking==NULL)
        return NULL;

    if(to_try!=NULL && (to_try->reg==NULL || !REGION_IS_MAPPED(to_try->reg)))
        to_try=NULL;

    st=stacking_find_to_focus_mapped(stacking, to_try, NULL);
    
    if(st!=NULL)
        return st->reg;
    else if(mplex->mx_current!=NULL)
        return mplex->mx_current->st->reg;
    else
        return NULL;
}


WRegion *mplex_to_focus(WMPlex *mplex)
{
    WRegion *reg=REGION_ACTIVE_SUB(mplex);
    WStacking *to_try=NULL;
    
    if(reg!=NULL)
        to_try=ioncore_find_stacking(reg);

    return mplex_do_to_focus(mplex, to_try);
}


static WRegion *mplex_do_to_focus_on(WMPlex *mplex, WStacking *node,
                                     WStacking *to_try)
{
    WStacking *stacking=mplex_get_stacking(mplex);
    WStacking *st=NULL;
    
    if(stacking==NULL)
        return NULL;

    if(to_try!=NULL && (to_try->reg==NULL || !REGION_IS_MAPPED(to_try->reg)))
        to_try=NULL;
    
    st=stacking_find_to_focus_mapped(stacking, to_try, node->reg);
    
    return (st!=NULL ? st->reg : NULL);
}


static WRegion *mplex_to_focus_on(WMPlex *mplex, WStacking *node,
                                  WStacking *to_try)
{
    WRegion *reg;
    WGroup *grp=OBJ_CAST(node->reg, WGroup);
    
    if(grp!=NULL){
        if(to_try==NULL)
            to_try=grp->current_managed;
        reg=mplex_do_to_focus_on(mplex, node, to_try);
        if(reg!=NULL || to_try!=NULL)
            return reg;
        /* We don't know whether something is blocking focus here,
         * or if there was nothing to focus (as node->reg itself
         * isn't on the stacking list).
         */
    }
    
    reg=mplex_do_to_focus(mplex, node);
    return (reg==node->reg ? reg : NULL);
}


void mplex_do_set_focus(WMPlex *mplex, bool warp)
{
    if(!MPLEX_MGD_UNVIEWABLE(mplex)){
        WRegion *reg=mplex_to_focus(mplex);
        
        if(reg!=NULL){
            region_do_set_focus(reg, warp);
            return;
        }
    }
    
    window_do_set_focus((WWindow*)mplex, warp);
}


/*}}}*/


/*{{{ Switch */


static void mplex_do_remanage_stdisp(WMPlex *mplex, WRegion *sub)
{
    WRegion *stdisp=(WRegion*)(mplex->stdispwatch.obj);

    /* Move stdisp */
    if(sub!=NULL && CAN_MANAGE_STDISP(sub)){
        if(stdisp!=NULL){
            WRegion *mgr=region_managed_within((WRegion*)mplex, stdisp);
            if(mgr!=sub){
                if(mgr!=NULL){
                    if(CAN_MANAGE_STDISP(mgr))
                        region_unmanage_stdisp(mgr, FALSE, FALSE);
                    region_detach_manager(stdisp);
                }
                
                region_manage_stdisp(sub, stdisp, 
                                     &(mplex->stdispinfo));
            }
        }else{
            region_unmanage_stdisp(sub, TRUE, FALSE);
        }
    }else if(stdisp!=NULL){
        region_detach_manager(stdisp);
        region_unmap(stdisp);
    }
}


void mplex_remanage_stdisp(WMPlex *mplex)
{
    mplex_do_remanage_stdisp(mplex, (mplex->mx_current!=NULL
                                     ? mplex->mx_current->st->reg
                                     : NULL));
}


static void mplex_do_node_display(WMPlex *mplex, WStacking *node,
                                  bool call_changed)
{
    WRegion *sub=node->reg;
    WLListNode *mxc=mplex->mx_current;

    if(!STACKING_IS_HIDDEN(node))
        return;
    
    if(node->lnode!=NULL && node->lnode!=mxc)
        mplex_do_remanage_stdisp(mplex, sub);

    node->hidden=FALSE;
    
    if(SUBS_MAY_BE_MAPPED(mplex))
        region_map(sub);
    else
        region_unmap(sub);
    
    if(node->lnode!=NULL){
        if(mxc!=NULL){
            /* Hide current mx region. We do it after mapping the
             * new one to avoid flicker.
             */
            if(REGION_IS_MAPPED(mplex))
                region_unmap(mxc->st->reg);
            mxc->st->hidden=TRUE;
        }
        
        mplex->mx_current=node->lnode;
        
        /* Ugly hack:
         * Many programs will get upset if the visible, although only 
         * such, client window is not the lowest window in the mplex. 
         * xprop/xwininfo will return the information for the lowest 
         * window. 'netscape -remote' will not work at all if there are
         * no visible netscape windows.
         */
        {
            #warning "TODO: less ugly hack"
            WGroup *grp=(WGroup*)OBJ_CAST(sub, WGroupCW);
            if(grp!=NULL && grp->bottom!=NULL){
                region_managed_rqorder((WRegion*)grp, grp->bottom->reg, 
                                       REGION_ORDER_BACK);
            }
        }
        
        if(call_changed)
            mplex_managed_changed(mplex, MPLEX_CHANGE_SWITCHONLY, TRUE, sub);
    }
}


static bool mplex_do_node_goto(WMPlex *mplex, WStacking *node,
                               bool call_changed, int flags)
{
    bool ret=TRUE;
    
    mplex_do_node_display(mplex, node, call_changed);
    
    if(flags&REGION_GOTO_FOCUS){
        WRegion *foc=mplex_to_focus_on(mplex, node, NULL);
        
        if(foc==NULL){
            ret=FALSE;
            foc=mplex_to_focus(mplex);
        }
        
        if(foc!=NULL && !REGION_IS_ACTIVE(foc))
            region_maybewarp(foc, !(flags&REGION_GOTO_NOWARP));
    }
    
    return ret;
}


static bool mplex_do_node_goto_sw(WMPlex *mplex, WStacking *node,
                                  bool call_changed)
{
    bool mcf=region_may_control_focus((WRegion*)mplex);
    int flags=(mcf ? REGION_GOTO_FOCUS : 0)/*|REGION_GOTO_NOWARP*/;
    return mplex_do_node_goto(mplex, node, call_changed, flags);
}


bool mplex_do_prepare_focus(WMPlex *mplex, WStacking *node,
                            WStacking *sub, int flags, 
                            WPrepareFocusResult *res)
{
    WRegion *foc;
    
    if(sub==NULL && node==NULL)
        return FALSE;
    
    /* Display the node in any case */
    if(node!=NULL && !(flags&REGION_GOTO_ENTERWINDOW))
        mplex_do_node_display(mplex, node, TRUE);
    
    if(!region_prepare_focus((WRegion*)mplex, flags, res))
        return FALSE;

    if(node!=NULL)
        foc=mplex_to_focus_on(mplex, node, sub);
    else
        foc=mplex_do_to_focus(mplex, sub);

    if(foc!=NULL){
        res->reg=foc;
        res->flags=flags;
        
        if(sub==NULL)
            return (foc==node->reg);
        else
            return (foc==sub->reg);
    }else{
        return FALSE;
    }
}


bool mplex_managed_prepare_focus(WMPlex *mplex, WRegion *disp, 
                                 int flags, WPrepareFocusResult *res)
{
    WStacking *node=mplex_find_stacking(mplex, disp);
    
    if(node==NULL)
        return FALSE;
    else
        return mplex_do_prepare_focus(mplex, node, NULL, flags, res);
}


static void mplex_refocus(WMPlex *mplex, bool warp)
{
    WRegion *reg=mplex_to_focus(mplex);
    
    if(reg!=NULL && reg!=REGION_ACTIVE_SUB(mplex))
        region_maybewarp(reg, warp);
}

/*}}}*/


/*{{{ Switch exports */


static void do_switch(WMPlex *mplex, WLListNode *node)
{
    if(node!=NULL)
        mplex_do_node_goto_sw(mplex, node->st, TRUE);
}


/*EXTL_DOC
 * Have \var{mplex} display the \var{n}:th object managed by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_nth(WMPlex *mplex, uint n)
{
    do_switch(mplex, llist_nth_node(mplex->mx_list, n));
}


/*EXTL_DOC
 * Have \var{mplex} display next (wrt. currently selected) object managed 
 * by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_next(WMPlex *mplex)
{
    do_switch(mplex, LIST_NEXT_WRAP(mplex->mx_list, mplex->mx_current, 
                                    next, prev));
}


/*EXTL_DOC
 * Have \var{mplex} display previous (wrt. currently selected) object
 * managed by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_prev(WMPlex *mplex)
{
    do_switch(mplex, LIST_PREV_WRAP(mplex->mx_list, mplex->mx_current,
                                    next, prev));
}


bool mplex_set_hidden(WMPlex *mplex, WRegion *reg, int sp)
{
    bool mcf=region_may_control_focus((WRegion*)mplex);
    WStacking *node=mplex_find_stacking(mplex, reg);
    bool hidden, nhidden;
    
    if(node==NULL)
        return FALSE;
    
    hidden=STACKING_IS_HIDDEN(node);
    nhidden=libtu_do_setparam(sp, hidden);
    
    if(!hidden && nhidden){
        node->hidden=TRUE;
        
        if(REGION_IS_MAPPED(mplex) && !MPLEX_MGD_UNVIEWABLE(mplex))
            region_unmap(reg);
        
        if(mcf)
            mplex_refocus(mplex, TRUE);
    }else if(hidden && !nhidden){
        mplex_do_node_goto(mplex, node, TRUE,
                           (mcf ? REGION_GOTO_FOCUS : 0));
    }
    
    return STACKING_IS_HIDDEN(node);
}


/*EXTL_DOC
 * Set the visibility of the region \var{reg} on \var{mplex}
 * as specified with the parameter \var{how} (set/unset/toggle).
 * The resulting state is returned.
 */
EXTL_EXPORT_AS(WMPlex, set_hidden)
bool mplex_set_hidden_extl(WMPlex *mplex, WRegion *reg, const char *how)
{
    return mplex_set_hidden(mplex, reg, libtu_string_to_setparam(how));
}


/*EXTL_DOC
 * Is \var{reg} on within \var{mplex} and hidden?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool mplex_is_hidden(WMPlex *mplex, WRegion *reg)
{
    WStacking *node=mplex_find_stacking(mplex, reg);
    
    return (node!=NULL && STACKING_IS_HIDDEN(node));
}


/*}}}*/


/*{{{ Navigation */


static WStacking *mplex_nxt(WMPlex *mplex, WStacking *st, bool wrap)
{
    return (st->mgr_next!=NULL 
            ? st->mgr_next 
            : (wrap ? mplex->mgd : NULL));
}


static WStacking *mplex_prv(WMPlex *mplex, WStacking *st, bool wrap)
{
    return (st!=mplex->mgd
            ? st->mgr_prev
            : (wrap ? st->mgr_prev : NULL));
}


typedef WStacking *NxtFn(WMPlex *mplex, WStacking *st, bool wrap);


static WRegion *do_navi(WMPlex *mplex, WStacking *sti, 
                        NxtFn *fn, WRegionNaviData *data, bool sti_ok)
{
    WStacking *st, *stacking;
    uint min_level=0;
    bool wrap=FALSE;
    
    stacking=mplex_get_stacking(mplex);
    
    if(stacking!=NULL)
        min_level=stacking_min_level_mapped(stacking);

    st=sti;
    while(1){
        st=fn(mplex, st, wrap); 
        
        if(st==NULL || (st==sti && !sti_ok))
            break;
        
        if(!st->hidden){
            if(OBJ_IS(st->reg, WGroup)){
                /* WGroup navigation code should respect modal stuff. */
                WRegion *res=region_navi_cont((WRegion*)mplex, st->reg, data);
                if(res!=NULL){
                    if(res!=st->reg){
                        return res;
                    }else{
                        #warning "TODO: What to do?"
                    }
                }
            }else{
                if(st->level>=min_level && !(st->reg->flags&REGION_SKIP_FOCUS))
                    return region_navi_cont((WRegion*)mplex, st->reg, data);
            }
        }
	        
        if(st==sti)
            break;
    }
    
    return NULL;
}
                        

WRegion *mplex_navi_first(WMPlex *mplex, WRegionNavi nh,
                          WRegionNaviData *data)
{
    WStacking *lst=mplex->mgd;

    if(lst==NULL)
        return region_navi_cont((WRegion*)mplex, NULL, data);
    
    if(nh==REGION_NAVI_ANY){
        /* ? */
    }
    
    if(nh==REGION_NAVI_ANY || nh==REGION_NAVI_END || 
       nh==REGION_NAVI_BOTTOM || nh==REGION_NAVI_RIGHT){
        return do_navi(mplex, lst, mplex_prv, data, TRUE);
    }else{
        return do_navi(mplex, lst->mgr_prev, mplex_nxt, data, TRUE);
    }
}

    
WRegion *mplex_navi_next(WMPlex *mplex, WRegion *rel, WRegionNavi nh,
                         WRegionNaviData *data)
{
    WStacking *st;
    
    if(rel!=NULL){
        st=mplex_find_stacking(mplex, rel);
        if(st==NULL)
            return NULL;
    }else if(mplex->mx_current!=NULL){
        st=mplex->mx_current->st;
    }else{
        return mplex_navi_first(mplex, nh, data);
    }
        
    if(nh==REGION_NAVI_ANY){
        /* ? */
    }
    
    if(nh==REGION_NAVI_ANY || nh==REGION_NAVI_END || 
       nh==REGION_NAVI_BOTTOM || nh==REGION_NAVI_RIGHT){
        return do_navi(mplex, st, mplex_nxt, data, FALSE);
    }else{
        return do_navi(mplex, st, mplex_prv, data, FALSE);
    }
}


/*}}}*/


/*{{{ Stacking */


bool mplex_managed_rqorder(WMPlex *mplex, WRegion *reg, WRegionOrder order)
{
    WStacking **stackingp=mplex_get_stackingp(mplex);
    WStacking *st;

    if(stackingp==NULL || *stackingp==NULL)
        return FALSE;

    st=mplex_find_stacking(mplex, reg);
    
    if(st==NULL)
        return FALSE;
    
    stacking_restack(stackingp, st, None, NULL, NULL,
                     (order!=REGION_ORDER_FRONT));
    
    return TRUE;
}


/*}}}*/


/*{{{ Attach */


static bool mplex_stack(WMPlex *mplex, WStacking *st)
{
    WStacking *tmp=NULL;
    Window bottom=None, top=None;
    WStacking **stackingp=mplex_get_stackingp(mplex);
    
    if(stackingp==NULL)
        return FALSE;

    LINK_ITEM_FIRST(tmp, st, next, prev);
    stacking_weave(stackingp, &tmp, FALSE);
    assert(tmp==NULL);
    
    return TRUE;
}


static void mplex_unstack(WMPlex *mplex, WStacking *st)
{
    WStacking *stacking;
    
    stacking=mplex_get_stacking(mplex);
    
    stacking_unstack(&mplex->win, st);
}


/* WMPlexWPHolder is used for position marking in order to allow
 * WLListNodes be safely removed in the attach handler hnd, that
 * could remove something this mplex is managing.
 */
bool mplex_do_attach_final(WMPlex *mplex, WRegion *reg, WMPlexPHolder *ph)
{
    WStacking *node=NULL;
    WLListNode *lnode=NULL;
    WMPlexAttachParams *param=&ph->param;
    bool mx_was_empty, sw, modal;
    uint level;
    
    mx_was_empty=(mplex->mx_list==NULL);
    
    modal=(param->flags&MPLEX_ATTACH_LEVEL
           ? param->level>=STACKING_LEVEL_MODAL1
           : param->flags&MPLEX_ATTACH_MODAL);
    
    level=(param->flags&MPLEX_ATTACH_LEVEL
           ? param->level
           : (param->flags&MPLEX_ATTACH_MODAL
              ? STACKING_LEVEL_MODAL1
              : (param->flags&MPLEX_ATTACH_UNNUMBERED
                 ? STACKING_LEVEL_NORMAL
                 : STACKING_LEVEL_BOTTOM)));
    
    sw=((param->flags&MPLEX_ATTACH_SWITCHTO)
        || (param->flags&MPLEX_ATTACH_UNNUMBERED
            ? modal && !(param->flags&MPLEX_ATTACH_HIDDEN)
            : mx_was_empty || !(param->flags&MPLEX_ATTACH_HIDDEN)));
    
    node=create_stacking();
    
    if(node==NULL)
        return FALSE;
    
    if(!(param->flags&MPLEX_ATTACH_UNNUMBERED)){
        lnode=ALLOC(WLListNode);
        if(lnode==NULL){
            stacking_free(node);
            return FALSE;
        }
        lnode->next=NULL;
        lnode->prev=NULL;
        lnode->phs=NULL;
        lnode->st=node;
        node->lnode=lnode;
    }
    
    if(!stacking_assoc(node, reg)){
        if(lnode!=NULL){
            node->lnode=NULL;
            free(lnode);
        }
        stacking_free(node);
        return FALSE;
    }

    node->hidden=TRUE;
    node->szplcy=param->szplcy;
    node->level=level;
    
    if(lnode!=NULL){
        llist_link_after(&(mplex->mx_list), 
                         (ph!=NULL ? ph->after : NULL), 
                         lnode);
        mplex->mx_count++;
        
        /* Move following placeholders after new node */
        while(ph->next!=NULL){
            WMPlexPHolder *ph2=ph->next;
            mplexpholder_do_unlink(ph2, mplex);
            LINK_ITEM_FIRST(lnode->phs, ph2, next, prev);
        }
    }
    
    LINK_ITEM(mplex->mgd, node, mgr_next, mgr_prev);

    if(!OBJ_IS(reg, WGroup))
        mplex_stack(mplex, node);
    
    region_set_manager(reg, (WRegion*)mplex);

    if(sw)
        mplex_do_node_goto_sw(mplex, node, FALSE);
    else if(lnode==NULL && !(param->flags&MPLEX_ATTACH_HIDDEN))
        mplex_do_node_display(mplex, node, FALSE);
    else
        region_unmap(reg);

    if(lnode!=NULL)
        mplex_managed_changed(mplex, MPLEX_CHANGE_ADD, sw, reg);
    
    return TRUE;
}


WRegion *mplex_do_attach_pholder(WMPlex *mplex, WMPlexPHolder *ph,
                                 WRegionAttachData *data)
{
    WMPlexAttachParams *param=&(ph->param);
    WSizePolicy szplcy=param->szplcy;
    WFitParams fp;
    WRegion *reg;
    
    param->szplcy=(param->flags&MPLEX_ATTACH_SIZEPOLICY &&
                   param->szplcy!=SIZEPOLICY_DEFAULT
                   ? param->szplcy
                   : (param->flags&MPLEX_ATTACH_UNNUMBERED
                      ? SIZEPOLICY_FULL_BOUNDS
                      : SIZEPOLICY_FULL_EXACT));
    
    mplex_managed_geom(mplex, &(fp.g));
    
    sizepolicy(&param->szplcy, NULL, 
               (param->flags&MPLEX_ATTACH_GEOM 
                ? &(param->geom)
                : NULL),
               0, &fp);
    
    if(param->flags&MPLEX_ATTACH_WHATEVER)
        fp.mode|=REGION_FIT_WHATEVER;
    
    reg=region_attach_helper((WRegion*)mplex, 
                             (WWindow*)mplex, &fp,
                             (WRegionDoAttachFn*)mplex_do_attach_final,
                             (void*)ph, data);
    
    /* restore */
    ph->param.szplcy=szplcy;
    
    return reg;
}


WRegion *mplex_do_attach(WMPlex *mplex, WMPlexAttachParams *param,
                         WRegionAttachData *data)
{
    WMPlexPHolder *ph;
    WRegion *reg;
    
    ph=create_mplexpholder(mplex, NULL, param);
    
    if(ph==NULL)
        return NULL;

    reg=mplex_do_attach_pholder(mplex, ph, data);
    
    destroy_obj((Obj*)ph);
    
    return reg;
}


WRegion *mplex_do_attach_new(WMPlex *mplex, WMPlexAttachParams *param,
                             WRegionCreateFn *fn, void *fn_param)
{
    WRegionAttachData data;
    
    data.type=REGION_ATTACH_NEW;
    data.u.n.fn=fn;
    data.u.n.param=fn_param;
    
    return mplex_do_attach(mplex, param, &data);
}


#define MPLEX_ATTACH_SET_FLAGS (MPLEX_ATTACH_GEOM|       \
                                MPLEX_ATTACH_SIZEPOLICY| \
                                MPLEX_ATTACH_INDEX)


WRegion *mplex_attach_simple(WMPlex *mplex, WRegion *reg, int flags)
{
    WMPlexAttachParams param;
    WRegionAttachData data;
    
    param.flags=flags&~MPLEX_ATTACH_SET_FLAGS;
    
    data.type=REGION_ATTACH_REPARENT;
    data.u.reg=reg;
    
    return mplex_do_attach(mplex, &param, &data);
}


static void get_params(WMPlex *mplex, ExtlTab tab, WMPlexAttachParams *par)
{
    int layer=1;
    int tmp;
    
    par->flags=0;
    
    if(extl_table_gets_i(tab, "layer", &tmp)){
        /* backwards compatibility */
        if(tmp==2){
            par->flags|=MPLEX_ATTACH_UNNUMBERED;
            if(!extl_table_is_bool_set(tab, "passive"))
                par->flags|=MPLEX_ATTACH_MODAL;
        }
    }

    if(extl_table_gets_i(tab, "level", &tmp)){
        if(tmp>=0){
            par->flags|=MPLEX_ATTACH_LEVEL;
            par->level=tmp;
        }
    }
    
    if(extl_table_is_bool_set(tab, "modal"))
        par->flags|=MPLEX_ATTACH_MODAL;

    if(extl_table_is_bool_set(tab, "unnumbered"))
        par->flags|=MPLEX_ATTACH_UNNUMBERED;
    
    if(extl_table_is_bool_set(tab, "switchto"))
        par->flags|=MPLEX_ATTACH_SWITCHTO;

    if(extl_table_is_bool_set(tab, "hidden"))
        par->flags|=MPLEX_ATTACH_HIDDEN;

    if(extl_table_gets_i(tab, "index", &(par->index)))
        par->flags|=MPLEX_ATTACH_INDEX;

    if(extl_table_gets_i(tab, "sizepolicy", &tmp)){
        par->flags|=MPLEX_ATTACH_SIZEPOLICY;
        par->szplcy=tmp;
    }
    
    if(extl_table_gets_rectangle(tab, "geom", &par->geom))
        par->flags|=MPLEX_ATTACH_GEOM;
}


/*EXTL_DOC
 * Attach and reparent existing region \var{reg} to \var{mplex}.
 * The table \var{param} may contain the fields \var{index} and
 * \var{switchto} that are interpreted as for \fnref{WMPlex.attach_new}.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_attach(WMPlex *mplex, WRegion *reg, ExtlTab param)
{
    WMPlexAttachParams par;
    WRegionAttachData data;

    if(reg==NULL)
        return NULL;
    
    get_params(mplex, param, &par);
    
    data.type=REGION_ATTACH_REPARENT;
    data.u.reg=reg;
    
    return mplex_do_attach(mplex, &par, &data);
}


/*EXTL_DOC
 * Create a new region to be managed by \var{mplex}. At least the following
 * fields in \var{param} are understood (all but \var{type} are optional).
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *  \tabhead{Field & Description}
 *  \var{type} & (string) Class name (a string) of the object to be created. \\
 *  \var{name} & (string) Name of the object to be created (a string). \\
 *  \var{switchto} & (boolean) Should the region be switched to (boolean)? \\
 *  \var{unnumbered} & (boolean) Do not put on the numbered mutually 
 *                     exclusive list. \\
 *  \var{index} & (integer) Index on this list, same as for 
 *                \fnref{WMPlex.set_index}. \\
 *  \var{level} & (integer) Stacking level. \\
 *  \var{modal} & (boolean) Shortcut for modal stacking level. \\
 *  \var{hidden} & (boolean) Start hidden (some interplay with 
 *                  \var{switchto}).
 *  \var{sizepolicy} & (integer) Size policy.
 *                     (TODO: document them somewhere.) \\
 *  \var{geom} & (table) Geometry specification. \\
 * \end{tabularx}
 * 
 * In addition parameters to the region to be created are passed in this 
 * same table.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_attach_new(WMPlex *mplex, ExtlTab param)
{
    WMPlexAttachParams par;
    WRegionAttachData data;
    
    get_params(mplex, param, &par);
    
    data.type=REGION_ATTACH_LOAD;
    data.u.tab=param;
    
    return mplex_do_attach(mplex, &par, &data);
}


/*EXTL_DOC
 * Attach all tagged regions to \var{mplex}.
 */
EXTL_EXPORT_MEMBER
void mplex_attach_tagged(WMPlex *mplex)
{
    WRegion *reg;
    
    while((reg=ioncore_tags_take_first())!=NULL)
        mplex_attach_simple(mplex, reg, 0);
}


static bool mplex_handle_drop(WMPlex *mplex, int x, int y,
                              WRegion *dropped)
{
    WRegion *curr=mplex_mx_current(mplex);
    
    /* This code should handle dropping tabs on floating workspaces. */
    if(curr && HAS_DYN(curr, region_handle_drop)){
        int rx, ry;
        region_rootpos(curr, &rx, &ry);
        if(rectangle_contains(&REGION_GEOM(curr), x-rx, y-ry)){
            if(region_handle_drop(curr, x, y, dropped))
                return TRUE;
        }
    }
    
    return (NULL!=mplex_attach_simple(mplex, dropped, MPLEX_ATTACH_SWITCHTO));
}


WPHolder *mplex_prepare_manage(WMPlex *mplex, const WClientWin *cwin,
                               const WManageParams *param, int redir)
{
    WMPlexAttachParams ap;
    WPHolder *ph=NULL;
    WMPlexPHolder *mph;
    WLListNode *after;
    
    if(redir==MANAGE_REDIR_STRICT_YES || redir==MANAGE_REDIR_PREFER_YES){
        WStacking *cur=mplex_current_node(mplex);
        
        if(cur!=NULL){
            ph=region_prepare_manage(cur->reg, cwin, param,
                                     MANAGE_REDIR_PREFER_YES);
            if(ph!=NULL)
                return ph;
        }
        
        if(mplex->mx_current!=NULL && mplex->mx_current->st!=cur){
            ph=region_prepare_manage(mplex->mx_current->st->reg, 
                                     cwin, param, 
                                     MANAGE_REDIR_PREFER_YES);
            if(ph!=NULL)
                return ph;
        }
    }
    
    if(redir==MANAGE_REDIR_STRICT_YES)
        return NULL;
    
    ap.flags=((param->switchto ? MPLEX_ATTACH_SWITCHTO : 0)
              |MPLEX_ATTACH_SIZEPOLICY);
    ap.szplcy=SIZEPOLICY_FULL_BOUNDS;
    
    mph=create_mplexpholder(mplex, NULL, &ap);
    
    if(mph!=NULL)
        mph->initial=TRUE;
    
    return (WPHolder*)mph;
}


/*}}}*/


/*{{{ Remove */


void mplex_managed_remove(WMPlex *mplex, WRegion *sub)
{
    bool mx=FALSE, hadfocus=FALSE, mcf;
    WRegion *stdisp=(WRegion*)(mplex->stdispwatch.obj);
    WStacking *node, *next=NULL;
 
    mcf=region_may_control_focus((WRegion*)mplex);
    
    if(stdisp!=NULL){
        if(CAN_MANAGE_STDISP(sub) && 
           region_managed_within((WRegion*)mplex, stdisp)==sub){
            region_unmanage_stdisp(sub, TRUE, TRUE);
            region_detach_manager(stdisp);
        }
    }
    
    node=mplex_find_stacking(mplex, sub);
    
    if(node==NULL)
        return;
    
    hadfocus=(mplex_current_node(mplex)==node);
    
    if(node->lnode!=NULL){
        if(mplex->mx_current==node->lnode){
            WLListNode *lnext;
            
            mplex->mx_current=NULL;
            lnext=LIST_PREV(mplex->mx_list, node->lnode, next, prev);
            if(lnext==NULL){
                lnext=LIST_NEXT(mplex->mx_list, node->lnode, next, prev);
                if(lnext==node->lnode)
                    lnext=NULL;
            }
            if(lnext!=NULL)
                next=lnext->st;
        }
        
        mplex_move_phs_before(mplex, node->lnode);
        llist_unlink(&(mplex->mx_list), node->lnode);
        mplex->mx_count--;

        free(node->lnode);
        node->lnode=NULL;
        mx=TRUE;
    }
    
    UNLINK_ITEM(mplex->mgd, node, mgr_next, mgr_prev);
    
    mplex_unstack(mplex, node);
    
    stacking_unassoc(node);
    stacking_free(node);

    region_unset_manager(sub, (WRegion*)mplex);
    
    if(OBJ_IS_BEING_DESTROYED(mplex))
        return;
    
    if(next!=NULL){
        if(hadfocus)
            mplex_do_node_goto_sw(mplex, next, FALSE);
        else
            mplex_do_node_display(mplex, next, FALSE);
    }else if(hadfocus && mcf){
        mplex_refocus(mplex, TRUE);
    }

    if(mx)
        mplex_managed_changed(mplex, MPLEX_CHANGE_REMOVE, next!=NULL, sub);
}


bool mplex_rescue_clientwins(WMPlex *mplex, WPHolder *ph)
{
    bool ret1, ret2;
    WMPlexIterTmp tmp;
    
    mplex_iter_init(&tmp, mplex);
    ret1=region_rescue_some_clientwins((WRegion*)mplex, ph,
                                       (WRegionIterator*)mplex_iter,
                                       &tmp);
    
    ret2=region_rescue_child_clientwins((WRegion*)mplex, ph);
    
    return (ret1 && ret2);
}



void mplex_child_removed(WMPlex *mplex, WRegion *sub)
{
    if(sub!=NULL && sub==(WRegion*)(mplex->stdispwatch.obj)){
        watch_reset(&(mplex->stdispwatch));
        mplex_set_stdisp(mplex, NULL, NULL);
    }
}


/*}}}*/


/*{{{ Status display support */

#ifndef offsetof
# define offsetof(T,F) ((size_t)((char*)&((T*)0L)->F-(char*)0L))
#endif

#define STRUCTOF(T, F, FADDR) \
        ((T*)((char*)(FADDR)-offsetof(T, F)))


static void stdisp_watch_handler(Watch *watch, Obj *obj)
{
    /*WMPlex *mplex=STRUCTOF(WMPlex, stdispinfo, 
     STRUCTOF(WMPlexSTDispInfo, regwatch, watch));
     WMPlexSTDispInfo *di=&(mplex->stdispinfo);
     WGenWS *ws=OBJ_CAST(REGION_MANAGER(obj), WGenWS);
     * 
     if(ioncore_g.opmode!=IONCORE_OPMODE_DEINIT && ws!=NULL)
     genws_unmanage_stdisp(ws, TRUE, FALSE);*/
}


bool mplex_set_stdisp(WMPlex *mplex, WRegion *reg, 
                      const WMPlexSTDispInfo *din)
{
    WRegion *oldstdisp=(WRegion*)(mplex->stdispwatch.obj);
    WRegion *mgr=NULL;
    
    assert(reg==NULL || (reg==oldstdisp) ||
           (REGION_MANAGER(reg)==NULL && 
            REGION_PARENT(reg)==(WWindow*)mplex));
    
    if(oldstdisp!=NULL){
        mgr=region_managed_within((WRegion*)mplex, oldstdisp);
        
        if(!CAN_MANAGE_STDISP(mgr))
            mgr=NULL;
        
        if(oldstdisp!=reg){
            mainloop_defer_destroy((Obj*)oldstdisp);
            watch_reset(&(mplex->stdispwatch));
        }
    }
    
    if(din!=NULL)
        mplex->stdispinfo=*din;
    
    if(reg==NULL){
        if(mgr!=NULL){
            region_unmanage_stdisp(mgr, TRUE, FALSE);
            if(oldstdisp!=NULL)
                region_detach_manager(oldstdisp);
        }
    }else{
        watch_setup(&(mplex->stdispwatch), (Obj*)reg, stdisp_watch_handler);
        
        mplex_remanage_stdisp(mplex);
    }
    
    return TRUE;
}


void mplex_get_stdisp(WMPlex *mplex, WRegion **reg, WMPlexSTDispInfo *di)
{
    *di=mplex->stdispinfo;
    *reg=(WRegion*)mplex->stdispwatch.obj;
}


static StringIntMap pos_map[]={
    {"tl", MPLEX_STDISP_TL},
    {"tr", MPLEX_STDISP_TR},
    {"bl", MPLEX_STDISP_BL},
    {"br", MPLEX_STDISP_BR},
    {NULL, 0}
};


static bool do_attach_stdisp(WRegion *mplex, WRegion *reg, void *unused)
{
    /* We do not actually manage the stdisp. */
    return TRUE;
}


/*EXTL_DOC
 * Set/create status display for \var{mplex}. Table is a standard
 * description of the object to be created (as passed to e.g. 
 * \fnref{WMPlex.attach_new}). In addition, the following fields are
 * recognised:
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *   \tabhead{Field & Description}
 *   \var{pos} & The corner of the screen to place the status display
 *               in. One of \code{tl}, \code{tr}, \var{bl} or \var{br}. \\
 *   \var{action} & If this field is set to \code{keep}, \var{corner}
 *                  and \var{orientation} are changed for the existing
 *                  status display. If this field is set to \var{remove},
 *                  the existing status display is removed. If this
 *                  field is not set or is set to \code{replace}, a 
 *                  new status display is created and the old, if any,
 *                  removed. \\
 * \end{tabularx}
 */
EXTL_EXPORT_AS(WMPlex, set_stdisp)
WRegion *mplex_set_stdisp_extl(WMPlex *mplex, ExtlTab t)
{
    WRegion *stdisp=NULL;
    WMPlexSTDispInfo din=mplex->stdispinfo;
    char *s;
    
    if(extl_table_gets_s(t, "pos", &s)){
        din.pos=stringintmap_value(pos_map, s, -1);
        if(din.pos<0){
            warn(TR("Invalid position setting."));
            return NULL;
        }
    }
    
    extl_table_gets_b(t, "fullsize", &(din.fullsize));
    
    s=NULL;
    extl_table_gets_s(t, "action", &s);
    
    if(s==NULL || strcmp(s, "replace")==0){
        WRegionAttachData data;
        WFitParams fp;
        int o2;
        
        fp.g.x=0;
        fp.g.y=0;
        fp.g.w=REGION_GEOM(mplex).w;
        fp.g.h=REGION_GEOM(mplex).h;
        fp.mode=REGION_FIT_BOUNDS|REGION_FIT_WHATEVER;
        
        /* Full mplex size is stupid so use saved geometry initially
         * if there's one.
         */
        extl_table_gets_rectangle(t, "geom", &(fp.g));
        
        data.type=REGION_ATTACH_LOAD;
        data.u.tab=t;
            
        stdisp=region_attach_helper((WRegion*)mplex, 
                                    (WWindow*)mplex, &fp,
                                    do_attach_stdisp, NULL,
                                    &data);
        
        if(stdisp==NULL)
            return NULL;
        
    }else if(strcmp(s, "keep")==0){
        stdisp=(WRegion*)(mplex->stdispwatch.obj);
    }else if(strcmp(s, "remove")!=0){
        warn(TR("Invalid action setting."));
        return FALSE;
    }
    
    if(!mplex_set_stdisp(mplex, stdisp, &din)){
        destroy_obj((Obj*)stdisp);
        return NULL;
    }
    
    return stdisp;
}


static ExtlTab mplex_do_get_stdisp_extl(WMPlex *mplex, bool fullconfig)
{
    WRegion *reg=(WRegion*)mplex->stdispwatch.obj;
    ExtlTab t;
    
    if(reg==NULL)
        return extl_table_none();
    
    if(fullconfig){
        t=region_get_configuration(reg);
        extl_table_sets_rectangle(t, "geom", &REGION_GEOM(reg));
    }else{
        t=extl_create_table();
        extl_table_sets_o(t, "reg", (Obj*)reg);
    }
    
    if(t!=extl_table_none()){
        WMPlexSTDispInfo *di=&(mplex->stdispinfo);
        extl_table_sets_s(t, "pos", stringintmap_key(pos_map, di->pos, NULL));
        extl_table_sets_b(t, "fullsize", di->fullsize);
    }
    return t;
}


/*EXTL_DOC
 * Get status display information. See \fnref{WMPlex.get_stdisp} for
 * information on the fields.
 */
EXTL_SAFE
EXTL_EXPORT_AS(WMPlex, get_stdisp)
ExtlTab mplex_get_stdisp_extl(WMPlex *mplex)
{
    return mplex_do_get_stdisp_extl(mplex, FALSE);
}


/*}}}*/


/*{{{ Dynfuns */


void mplex_managed_geom_default(const WMPlex *mplex, WRectangle *geom)
{
    geom->x=0;
    geom->y=0;
    geom->w=REGION_GEOM(mplex).w;
    geom->h=REGION_GEOM(mplex).h;
}


void mplex_managed_geom(const WMPlex *mplex, WRectangle *geom)
{
    CALL_DYN(mplex_managed_geom, mplex, (mplex, geom));
}


void mplex_size_changed(WMPlex *mplex, bool wchg, bool hchg)
{
    CALL_DYN(mplex_size_changed, mplex, (mplex, wchg, hchg));
}


void mplex_managed_changed(WMPlex *mplex, int mode, bool sw, WRegion *mgd)
{
    CALL_DYN(mplex_managed_changed, mplex, (mplex, mode, sw, mgd));
}


int mplex_default_index(WMPlex *mplex)
{
    int idx=LLIST_INDEX_LAST;
    CALL_DYN_RET(idx, int, mplex_default_index, mplex, (mplex));
    return idx;
}


/* For regions managing stdisps */

void region_manage_stdisp(WRegion *reg, WRegion *stdisp, 
                          const WMPlexSTDispInfo *info)
{
    CALL_DYN(region_manage_stdisp, reg, (reg, stdisp, info));
}


void region_unmanage_stdisp(WRegion *reg, bool permanent, bool nofocus)
{
    CALL_DYN(region_unmanage_stdisp, reg, (reg, permanent, nofocus));
}


/*}}}*/


/*{{{ Changed hook helper */


static const char *mode2str(int mode)
{
    if(mode==MPLEX_CHANGE_SWITCHONLY)
        return "switchonly";
    else if(mode==MPLEX_CHANGE_REORDER)
        return "reorder";
    else if(mode==MPLEX_CHANGE_ADD)
        return "add";
    else if(mode==MPLEX_CHANGE_REMOVE)
        return "remove";
    return NULL;
}
    

static bool mrsh_chg(ExtlFn fn, WMPlexChangedParams *p)
{
    ExtlTab t=extl_create_table();
    bool ret;
    
    extl_table_sets_o(t, "reg", (Obj*)p->reg);
    extl_table_sets_s(t, "mode", mode2str(p->mode));
    extl_table_sets_b(t, "sw", p->sw);
    extl_table_sets_o(t, "sub", (Obj*)p->sub);
    
    extl_protect(NULL);
    ret=extl_call(fn, "t", NULL, t);
    extl_unprotect(NULL);
    
    extl_unref_table(t);
    
    return ret;
}


void mplex_call_changed_hook(WMPlex *mplex, WHook *hook, 
                             int mode, bool sw, WRegion *reg)
{
    WMPlexChangedParams p;
    
    p.reg=mplex;
    p.mode=mode;
    p.sw=sw;
    p.sub=reg;

    hook_call_p(hook, &p, (WHookMarshallExtl*)mrsh_chg);
}


/*}}} */


/*{{{ Save/load */


static void save_node(WMPlex *mplex, ExtlTab subs, int *n, 
                      WStacking *node, bool unnumbered)
{
    ExtlTab st, g;
    
    st=region_get_configuration(node->reg);
    
    if(st!=extl_table_none()){
        /*"TODO: better switchto saving? */
        if(mplex->mx_current!=NULL && node==mplex->mx_current->st)
            extl_table_sets_b(st, "switchto", TRUE);
        extl_table_sets_i(st, "sizepolicy", node->szplcy);
        extl_table_sets_i(st, "level", node->level);
        g=extl_table_from_rectangle(&REGION_GEOM(node->reg));
        extl_table_sets_t(st, "geom", g);
        extl_unref_table(g);
        if(STACKING_IS_HIDDEN(node))
            extl_table_sets_b(st, "hidden", TRUE);
        if(unnumbered)
            extl_table_sets_b(st, "unnumbered", TRUE);
        
        extl_table_seti_t(subs, ++(*n), st);
        extl_unref_table(st);
    }
}


ExtlTab mplex_get_configuration(WMPlex *mplex)
{
    ExtlTab tab, subs, stdisptab;
    WMPlexIterTmp tmp;
    WLListIterTmp ltmp;
    WLListNode *lnode;
    WStacking *node;
    int n=0;
    
    tab=region_get_base_configuration((WRegion*)mplex);
    
    subs=extl_create_table();
    extl_table_sets_t(tab, "subs", subs);
    
    /* First the numbered/mutually exclusive nodes */
    FOR_ALL_NODES_ON_LLIST(lnode, mplex->mx_list, ltmp){
        save_node(mplex, subs, &n, lnode->st, FALSE);
    }

    FOR_ALL_NODES_IN_MPLEX(mplex, node, tmp){
        if(node->lnode==NULL)
            save_node(mplex, subs, &n, node, TRUE);
    }

    extl_unref_table(subs);
    
    /*stdisptab=mplex_do_get_stdisp_extl(mplex, TRUE);
    if(stdisptab!=extl_table_none()){
        extl_table_sets_t(tab, "stdisp", stdisptab);
        extl_unref_table(stdisptab);
    }*/
    
    return tab;
}


static WMPlex *tmp_mplex=NULL;
static WMPlexAttachParams *tmp_par=NULL;

static WPHolder *pholder_callback()
{
    assert(tmp_mplex!=NULL);
    return (WPHolder*)create_mplexpholder(tmp_mplex, NULL, tmp_par);
}


void mplex_load_contents(WMPlex *mplex, ExtlTab tab)
{
    ExtlTab substab, subtab;
    int n, i;

    /*if(extl_table_gets_t(tab, "stdisp", &subtab)){
        mplex_set_stdisp_extl(mplex, subtab);
        extl_unref_table(subtab);
    }*/
    
    if(extl_table_gets_t(tab, "subs", &substab)){
        n=extl_table_get_n(substab);
        for(i=1; i<=n; i++){
            if(extl_table_geti_t(substab, i, &subtab)){
                /*mplex_attach_new(mplex, subtab);*/
                WMPlexAttachParams par;
                WRegionAttachData data;
                char *tmp=NULL;
                
                get_params(mplex, subtab, &par);
                
                par.flags|=MPLEX_ATTACH_INDEX;
                par.index=LLIST_INDEX_LAST;
                
                tmp_par=&par;
                tmp_mplex=mplex;
                
                data.type=REGION_ATTACH_LOAD;
                data.u.tab=subtab;
                
                ioncore_set_sm_pholder_callback(pholder_callback);
    
                mplex_do_attach(mplex, &par, &data);

                tmp_mplex=NULL;
                
                extl_unref_table(subtab);
            }
        }
        extl_unref_table(substab);
    }
}


WRegion *mplex_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WMPlex *mplex=create_mplex(par, fp);
    if(mplex!=NULL)
        mplex_load_contents(mplex, tab);
    return (WRegion*)mplex;
}


/*}}}*/


/*{{{ Dynfuntab and class info */


static DynFunTab mplex_dynfuntab[]={
    {region_do_set_focus, 
     mplex_do_set_focus},
    
    {region_managed_remove, 
     mplex_managed_remove},
    
    {region_managed_rqgeom,
     mplex_managed_rqgeom},
    
    {(DynFun*)region_managed_prepare_focus,
     (DynFun*)mplex_managed_prepare_focus},
    
    {(DynFun*)region_handle_drop, 
     (DynFun*)mplex_handle_drop},
    
    {region_map, mplex_map},
    {region_unmap, mplex_unmap},
    
    {(DynFun*)region_prepare_manage,
     (DynFun*)mplex_prepare_manage},
    
    {(DynFun*)region_current,
     (DynFun*)mplex_current},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)mplex_rescue_clientwins},
    
    {(DynFun*)region_get_configuration,
     (DynFun*)mplex_get_configuration},
    
    {mplex_managed_geom, 
     mplex_managed_geom_default},
    
    {(DynFun*)region_fitrep,
     (DynFun*)mplex_fitrep},
    
    {region_child_removed,
     mplex_child_removed},
    
    {(DynFun*)region_managed_get_pholder,
     (DynFun*)mplex_managed_get_pholder},

    {(DynFun*)region_get_rescue_pholder_for,
     (DynFun*)mplex_get_rescue_pholder_for},

    {(DynFun*)region_may_destroy,
     (DynFun*)mplex_may_destroy},
    
    {(DynFun*)region_navi_first,
     (DynFun*)mplex_navi_first},
    
    {(DynFun*)region_navi_next,
     (DynFun*)mplex_navi_next},
    
    {(DynFun*)region_managed_rqorder,
     (DynFun*)mplex_managed_rqorder},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WMPlex, WWindow, mplex_deinit, mplex_dynfuntab);


/*}}}*/

