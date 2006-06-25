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
#include "genws.h"
#include "frame-pointer.h"
#include "bindmaps.h"
#include "regbind.h"
#include "saveload.h"
#include "xwindow.h"
#include "mplexpholder.h"
#include "llist.h"
#include "names.h"
#include "sizepolicy.h"


#define SUBS_MAY_BE_MAPPED(MPLEX) \
    (REGION_IS_MAPPED(MPLEX) && !MPLEX_MGD_UNVIEWABLE(MPLEX))


#define MANAGES_STDISP(REG) HAS_DYN(REG, region_manage_stdisp)


/*{{{ Destroy/create mplex */


bool mplex_do_init(WMPlex *mplex, WWindow *parent, Window win,
                   const WFitParams *fp, bool create)
{
    mplex->flags=0;
    
    mplex->l1_count=0;
    mplex->l1_list=NULL;
    mplex->l1_current=NULL;
    mplex->l1_phs=NULL;
    
    mplex->l2_count=0;
    mplex->l2_list=NULL;
    mplex->l2_current=NULL;
    mplex->l2_phs=NULL;
    
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
    
    region_register((WRegion*)mplex);
    
    mplex->win.region.flags|=REGION_BINDINGS_ARE_GRABBED;
    
    window_select_input(&(mplex->win), IONCORE_EVENTMASK_CWINMGR);
    
    region_add_bindmap((WRegion*)mplex, ioncore_mplex_bindmap);
    region_add_bindmap((WRegion*)mplex, ioncore_mplex_toplevel_bindmap);
    
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
    WLListIterTmp tmp;
    WRegion *reg;
    
    FOR_ALL_REGIONS_ON_LLIST(reg, mplex->l1_list, tmp){
        destroy_obj((Obj*)reg);
    }

    FOR_ALL_REGIONS_ON_LLIST(reg, mplex->l2_list, tmp){
        destroy_obj((Obj*)reg);
    }

    assert(mplex->l1_list==NULL);
    assert(mplex->l2_list==NULL);

    while(mplex->l1_phs!=NULL){
        assert(mplexpholder_move(mplex->l1_phs, NULL, NULL, NULL, 1));
    }

    while(mplex->l2_phs!=NULL){
        assert(mplexpholder_move(mplex->l2_phs, NULL, NULL, NULL, 1));
    }
    
    window_deinit((WWindow*)mplex);
}


bool mplex_may_destroy(WMPlex *mplex)
{
    if(mplex->l1_list!=NULL || mplex->l2_list!=NULL){
        warn(TR("Refusing to destroy - not empty."));
        return FALSE;
    }
    
    return TRUE;
}


/*}}}*/


/*{{{ Higher-level layer list stuff */


WLListNode *mplex_find_node(WMPlex *mplex, WRegion *reg)
{
    WLListNode *node=llist_find_on(mplex->l2_list, reg);
    
    if(node==NULL)
        node=llist_find_on(mplex->l1_list, reg);
    
    return node;
}



WRegion *mplex_current(WMPlex *mplex)
{
    return (mplex->l2_current!=NULL 
            ? mplex->l2_current->reg
            : LLIST_REG(mplex->l1_current));
}


/*EXTL_DOC
 * Returns the layer \var{reg} is on \var{mplex} or $-1$ if \var{reg}
 * is not managed by \var{mplex}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
int mplex_layer(WMPlex *mplex, WRegion *reg)
{
    WLListNode *node=mplex_find_node(mplex, reg);
    
    if(node==NULL)
        return -1;
    else
        return LLIST_LAYER(node);
}


/*EXTL_DOC
 * Return the managed object currently active within layer \var{l} of
 * \var{mplex}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WRegion *mplex_lcurrent(WMPlex *mplex, uint l)
{
    return (l==1 
            ? LLIST_REG(mplex->l1_current)
            : (l==2
               ? LLIST_REG(mplex->l2_current)
               : 0));
}


/*EXTL_DOC
 * Returns the \var{n}:th object managed by \var{mplex} on the the
 * \var{l}:th layer..
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
WRegion *mplex_lnth(WMPlex *mplex, uint l, uint n)
{
    return (l==1 
            ? llist_nth_region(mplex->l1_list, n)
            : (l==2
               ? llist_nth_region(mplex->l2_list, n)
               : 0));
}


/*EXTL_DOC
 * Returns a list of regions managed by \var{mplex} on layer \var{l}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
ExtlTab mplex_llist(WMPlex *mplex, uint l)
{
    return (l==1 
            ? llist_to_table(mplex->l1_list)
            : (l==2
               ? llist_to_table(mplex->l2_list)
               : extl_table_none()));
}


/*EXTL_DOC
 * Returns the number of regions managed by \var{mplex} on layer \var{l}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
int mplex_lcount(WMPlex *mplex, uint l)
{
    return (l==1 
            ? mplex->l1_count
            : (l==2
               ? mplex->l2_count
               : 0));
}


/*EXTL_DOC
 * Set index of \var{reg} within the multiplexer to \var{index}.
 */
EXTL_EXPORT_MEMBER
void mplex_set_index(WMPlex *mplex, WRegion *reg, int index)
{
    WLListNode *node, *after;
    
    if(reg==NULL)
        return;
    
    node=llist_find_on(mplex->l1_list, reg);
    
    if(node==NULL)
        return;
    
    mplex_move_phs_before(mplex, node);
    llist_unlink(&(mplex->l1_list), node);
    
    after=llist_index_to_after(mplex->l1_list, mplex->l1_current, index);
    llist_link_after(&(mplex->l1_list), after, node);
    
    mplex_managed_changed(mplex, MPLEX_CHANGE_REORDER, FALSE, reg);
}


/*EXTL_DOC
 * Get index of \var{reg} within the multiplexer. The first region managed
 * by \var{mplex} has index zero. If \var{reg} is not managed by \var{mplex},
 * -1 is returned.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
int mplex_get_index(WMPlex *mplex, WRegion *reg)
{
    WLListIterTmp tmp;
    WRegion *other;
    int index=0;
    
    FOR_ALL_REGIONS_ON_LLIST(other, mplex->l1_list, tmp){
        if(reg==other)
            return index;
        index++;
    }
    
    return -1;
}


/*EXTL_DOC
 * Move \var{r} ''right'' within objects managed by \var{mplex}.
 */
EXTL_EXPORT_MEMBER
void mplex_inc_index(WMPlex *mplex, WRegion *r)
{
    if(r==NULL)
        r=mplex_lcurrent(mplex, 1);
    if(r!=NULL)
        mplex_set_index(mplex, r, mplex_get_index(mplex, r)+1);
}


/*EXTL_DOC
 * Move \var{r} ''right'' within objects managed by \var{mplex}.
 */
EXTL_EXPORT_MEMBER
void mplex_dec_index(WMPlex *mplex, WRegion *r)
{
    if(r==NULL)
        r=mplex_lcurrent(mplex, 1);
    if(r!=NULL)
        mplex_set_index(mplex, r, mplex_get_index(mplex, r)-1);
}


/*}}}*/


/*{{{ Mapping */


static void mplex_map_mgd(WMPlex *mplex)
{
    WLListNode *node;

    if(mplex->l1_current!=NULL)
        region_map(mplex->l1_current->reg);
    
    FOR_ALL_NODES_ON_LLIST(node, mplex->l2_list){
        if(!(node->flags&LLIST_L2_HIDDEN))
            region_map(node->reg);
    }
}


static void mplex_unmap_mgd(WMPlex *mplex)
{
    WLListNode *node;
    
    if(mplex->l1_current!=NULL)
        region_unmap(mplex->l1_current->reg);
    
    FOR_ALL_NODES_ON_LLIST(node, mplex->l2_list){
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
    WLListNode *node;
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
        FOR_ALL_NODES_ON_LLIST(node, mplex->l1_list){
            fp2=*fp;
            sizepolicy(&node->szplcy, node->reg, NULL, 0, &fp2);
            region_fitrep(node->reg, NULL, &fp2);
        }
        
        FOR_ALL_NODES_ON_LLIST(node, mplex->l2_list){
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
    WLListNode *node;

    node=mplex_find_node(mplex, sub);
    
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


void mplex_do_set_focus(WMPlex *mplex, bool warp)
{
    bool focset=FALSE;
    bool warped=FALSE;
    
    if(!MPLEX_MGD_UNVIEWABLE(mplex)){
        if(mplex->l2_current!=NULL){
            region_do_set_focus(mplex->l2_current->reg, warp);
            return;
        }else if(mplex->l1_current!=NULL){
            region_do_set_focus(mplex->l1_current->reg, warp);
            return;
        }
    }
    
    window_do_set_focus((WWindow*)mplex, warp);
}


void mplex_managed_activated(WMPlex *mplex, WRegion *reg)
{
    WLListNode *node=llist_find_on(mplex->l2_list, reg);

    if(node==NULL){
        if(mplex->l2_current!=NULL && 
           mplex->l2_current->flags&LLIST_L2_PASSIVE){
            mplex->l2_current=NULL;
        }
    }else if(!(node->flags&LLIST_L2_HIDDEN)){
        mplex->l2_current=node;
    }
}


/*}}}*/


/*{{{ Managed region switching */


static bool mplex_l2_try_make_passive(WMPlex *mplex, bool nohide)
{
    WLListNode *node;
    bool tohide=FALSE;
    
    FOR_ALL_NODES_ON_LLIST(node, mplex->l2_list){
        if((node->flags&(LLIST_L2_HIDDEN|LLIST_L2_PASSIVE))==0){
            if(node->flags&LLIST_L2_SEMIMODAL || nohide)
                return FALSE;
            tohide=TRUE;
        }
    }
     
    if(tohide){
        FOR_ALL_NODES_ON_LLIST(node, mplex->l2_list){
            if((node->flags&(LLIST_L2_HIDDEN|LLIST_L2_PASSIVE))==0){
                node->flags|=LLIST_L2_HIDDEN;
                if(SUBS_MAY_BE_MAPPED(mplex))
                    region_unmap(node->reg);
                if(mplex->l2_current==node)
                    mplex->l2_current=NULL;
            }
        }
    }
    
    return TRUE;
}


static bool mplex_do_node_display(WMPlex *mplex, WLListNode *node,
                                  bool call_changed, bool nohide)
{
    WRegion *sub=node->reg;
    bool l2=node->flags&LLIST_L2;
    
    if(l2){
        if(node==mplex->l2_current)
            return TRUE;
    }else{
        WRegion *stdisp=(WRegion*)(mplex->stdispwatch.obj);

        if(node==mplex->l1_current)
            return mplex_l2_try_make_passive(mplex, nohide);
        
        /* Move stdisp */
        if(MANAGES_STDISP(sub)){
            if(stdisp!=NULL){
                WRegion *mgr=REGION_MANAGER(stdisp);
                if(mgr!=sub){
                    if(MANAGES_STDISP(mgr)){
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
            region_unmap(stdisp);
        }
    }
    
    if(SUBS_MAY_BE_MAPPED(mplex))
        region_map(sub);
    else
        region_unmap(sub);
    
    if(!l2){
        if(mplex->l1_current!=NULL && REGION_IS_MAPPED(mplex))
            region_unmap(mplex->l1_current->reg);
        
        mplex->l1_current=node;
        
        /* Many programs will get upset if the visible, although only 
         * such, client window is not the lowest window in the mplex. 
         * xprop/xwininfo will return the information for the lowest 
         * window. 'netscape -remote' will not work at all if there are
         * no visible netscape windows.
         */
        /*if(OBJ_IS(sub, WClientWin))*/{
            /* Lower everytyhing to put l1 stuff under l2 stuff */
            region_lower(sub);
        }
        
        /* This call should be unnecessary... */
        mplex_managed_activated(mplex, sub);

        mplex_managed_changed(mplex, MPLEX_CHANGE_SWITCHONLY, TRUE, sub);
        return mplex_l2_try_make_passive(mplex, nohide);
    }else{
        mplex_move_phs_before(mplex, node);
        llist_unlink(&(mplex->l2_list), node);
        
        region_raise(sub);
        
        llist_link_last(&(mplex->l2_list), node);
        
        node->flags&=~LLIST_L2_HIDDEN;
        /*if(!(node->flags&LLIST_L2_PASSIVE))
            mplex->l2_current=node;*/
        return TRUE;
    }
}


static bool mplex_do_node_goto(WMPlex *mplex, WLListNode *node,
                               bool call_changed, bool nohide, int flags)
{
    if(!mplex_do_node_display(mplex, node, call_changed, nohide))
        return FALSE;
    
    if(flags&REGION_GOTO_FOCUS)
        region_maybewarp(node->reg, !(flags&REGION_GOTO_NOWARP));
    
    return TRUE;
}


static bool mplex_do_node_goto_sw(WMPlex *mplex, WLListNode *node,
                                  bool call_changed, bool nohide)
{
    bool mcf=region_may_control_focus((WRegion*)mplex);
    int flags=(mcf ? REGION_GOTO_FOCUS : 0)|REGION_GOTO_NOWARP;
    return mplex_do_node_goto(mplex, node, call_changed, nohide, flags);
}


bool mplex_managed_goto(WMPlex *mplex, WRegion *sub, 
                        WManagedGotoCont *p, int flags)
{
    WLListNode *node=mplex_find_node(mplex, sub);
    bool nohide=(flags&REGION_GOTO_ENTERWINDOW);

    if(!mplex_do_node_display(mplex, node, TRUE, flags))
        sub=mplex_current(mplex);
    if(sub==NULL)
        sub=(WRegion*)mplex;
    
    return region_managed_goto_cont(sub, p, flags);
}


static void mplex_refocus(WMPlex *mplex, bool warp)
{
    WRegion *reg;
    
    if(mplex->l2_current!=NULL)
        reg=mplex->l2_current->reg;
    else if(mplex->l1_current!=NULL)
        reg=mplex->l1_current->reg;
    else
        reg=(WRegion*)mplex;
    
    region_maybewarp(reg, warp);
}


static void do_switch(WMPlex *mplex, WLListNode *node)
{
    if(node!=NULL){
        bool mcf=region_may_control_focus((WRegion*)mplex);

        mplex_do_node_goto(mplex, node, TRUE, TRUE, 
                           (mcf ? REGION_GOTO_FOCUS : 0));
    }
}


/*EXTL_DOC
 * Have \var{mplex} display the \var{n}:th object managed by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_nth(WMPlex *mplex, uint n)
{
    do_switch(mplex, llist_nth_node(mplex->l1_list, n));
}


/*EXTL_DOC
 * Have \var{mplex} display next (wrt. currently selected) object managed 
 * by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_next(WMPlex *mplex)
{
    do_switch(mplex, LIST_NEXT_WRAP(mplex->l1_list, mplex->l1_current, 
                                    next, prev));
}


/*EXTL_DOC
 * Have \var{mplex} display previous (wrt. currently selected) object
 * managed by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_prev(WMPlex *mplex)
{
    do_switch(mplex, LIST_PREV_WRAP(mplex->l1_list, mplex->l1_current,
                                    next, prev));
}


bool mplex_l2_set_hidden(WMPlex *mplex, WRegion *reg, bool sp)
{
    bool mcf=region_may_control_focus((WRegion*)mplex);
    WLListNode *node=llist_find_on(mplex->l2_list, reg);
    bool hidden, nhidden;
    
    if(node==NULL)
        return FALSE;
    
    hidden=(node->flags&LLIST_L2_HIDDEN);
    nhidden=libtu_do_setparam(sp, hidden);
    
    if(!hidden && nhidden){
        node->flags|=LLIST_L2_HIDDEN;
        
        if(REGION_IS_MAPPED(mplex) && !MPLEX_MGD_UNVIEWABLE(mplex))
            region_unmap(reg);
        
        if(mplex->l2_current==node){
            WLListNode *node2;
            mplex->l2_current=NULL;
            FOR_ALL_NODES_ON_LLIST(node2, mplex->l2_list){
                if((node2->flags&(LLIST_L2_HIDDEN|LLIST_L2_PASSIVE))==0)
                    mplex->l2_current=node2;
            }
        }
        
        if(mcf)
            mplex_refocus(mplex, TRUE);
    }else if(hidden && !nhidden){
        bool psv=node->flags&LLIST_L2_PASSIVE;
        mplex_do_node_goto(mplex, node, TRUE, TRUE,
                           (mcf && !psv ? REGION_GOTO_FOCUS : 0));
    }
    
    return (node->flags&LLIST_L2_HIDDEN);
}


/*EXTL_DOC
 * Set the visibility of the layer2 region \var{reg} on \var{mplex}
 * as specified with the parameter \var{how} (set/unset/toggle).
 * The resulting state is returned.
 */
EXTL_EXPORT_AS(WMPlex, l2_set_hidden)
bool mplex_l2_set_hidden_extl(WMPlex *mplex, WRegion *reg, const char *how)
{
    return mplex_l2_set_hidden(mplex, reg, libtu_string_to_setparam(how));
}


/*EXTL_DOC
 * Is \var{reg} on the layer2 of \var{mplex} and hidden?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool mplex_l2_is_hidden(WMPlex *mplex, WRegion *reg)
{
    WLListNode *node=llist_find_on(mplex->l2_list, reg);
    
    return (node!=NULL && node->flags&LLIST_L2_HIDDEN);
}


bool mplex_l2_set_passive(WMPlex *mplex, WRegion *reg, bool sp)
{
    bool mcf=region_may_control_focus((WRegion*)mplex);
    WLListNode *node=llist_find_on(mplex->l2_list, reg);
    bool passive, npassive;
    
    if(node==NULL)
        return FALSE;
    
    passive=(node->flags&LLIST_L2_PASSIVE);
    npassive=libtu_do_setparam(sp, passive);
    
    if(!passive && npassive){
        node->flags|=LLIST_L2_PASSIVE;
    }else if(passive && !npassive){
        WLListNode *node2;
        
        node->flags&=~LLIST_L2_PASSIVE;

        FOR_ALL_NODES_ON_LLIST_REV(node2, mplex->l2_list){
            if((node2->flags&(LLIST_L2_HIDDEN|LLIST_L2_PASSIVE))==0)
                break;
        }
        
        if(node2==node){
            mplex->l2_current=node;
            if(mcf)
                mplex_refocus(mplex, TRUE);
        }
    }

    return (node->flags&LLIST_L2_PASSIVE);
}


/*EXTL_DOC
 * Set the passivity of the layer2 region \var{reg} on \var{mplex}
 * as specified with the parameter \var{how} (set/unset/toggle).
 * The resulting state is returned.
 */
EXTL_EXPORT_AS(WMPlex, l2_set_passive)
bool mplex_l2_set_passive_extl(WMPlex *mplex, WRegion *reg, const char *how)
{
    return mplex_l2_set_passive(mplex, reg, libtu_string_to_setparam(how));
}

/*EXTL_DOC
 * Is \var{reg} on the layer2 of \var{mplex} and passive?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool mplex_l2_is_passive(WMPlex *mplex, WRegion *reg)
{
    WLListNode *node=llist_find_on(mplex->l2_list, reg);
    
    return (node!=NULL && node->flags&LLIST_L2_PASSIVE);
}


/*}}}*/


/*{{{ Attach */


WLListNode *mplex_do_attach_after(WMPlex *mplex, 
                                  WLListNode *after, 
                                  WMPlexAttachParams *param,
                                  WRegionAttachHandler *hnd,
                                  void *hnd_param)
{
    WRegion *reg;
    WFitParams fp;
    bool sw=param->flags&MPLEX_ATTACH_SWITCHTO;
    bool l2=param->flags&MPLEX_ATTACH_L2;
    bool semimodal=param->flags&MPLEX_ATTACH_L2_SEMIMODAL;
    WLListNode *node;
    WSizePolicy szplcy;
    
    assert(!(semimodal && param->flags&MPLEX_ATTACH_L2_HIDDEN));
    
    szplcy=(param->flags&MPLEX_ATTACH_SIZEPOLICY &&
            param->szplcy!=SIZEPOLICY_DEFAULT
            ? param->szplcy
            : (l2 
               ? SIZEPOLICY_FULL_BOUNDS
               : SIZEPOLICY_FULL_EXACT));
    
    mplex_managed_geom(mplex, &(fp.g));
    
    sizepolicy(&szplcy, NULL, 
               (param->flags&MPLEX_ATTACH_GEOM 
                ? &(param->geom)
                : NULL),
               0, &fp);

    node=ALLOC(WLListNode);
    
    if(node==NULL)
        return NULL;
    
    if(param->flags&MPLEX_ATTACH_WHATEVER)
        fp.mode|=REGION_FIT_WHATEVER;
        
    reg=hnd((WWindow*)mplex, &fp, hnd_param);
    
    if(reg==NULL){
        free(node);
        return NULL;
    }
    
    node->reg=reg;
    node->flags=(l2 ? LLIST_L2 : 0) | (semimodal ? LLIST_L2_SEMIMODAL : 0);
    node->phs=NULL;
    node->szplcy=szplcy;
               
    if(l2){
        llist_link_after(&(mplex->l2_list), after, node);
        mplex->l2_count++;
    }else{
        llist_link_after(&(mplex->l1_list), after, node);
        mplex->l1_count++;
    }
    
    region_set_manager(reg, (WRegion*)mplex);
    
    if(l2){
        if(param->flags&MPLEX_ATTACH_L2_HIDDEN){
            node->flags|=LLIST_L2_HIDDEN;
            sw=FALSE;
        }
        if(param->flags&MPLEX_ATTACH_L2_PASSIVE){
            node->flags|=LLIST_L2_PASSIVE;
        }
    }else if(mplex->l1_count==1){
        sw=TRUE;
    }
    
    if(sw)
        mplex_do_node_goto_sw(mplex, node, FALSE, TRUE);
    else if(l2 && !(node->flags&LLIST_L2_HIDDEN))
        mplex_do_node_display(mplex, node, FALSE, TRUE);
    else
        region_unmap(reg);

    if(!l2)
        mplex_managed_changed(mplex, MPLEX_CHANGE_ADD, sw, reg);
    
    return node;
}


static WLListNode *mplex_get_after(WMPlex *mplex, bool l2,
                                   bool index_given, int index)
{
    if(!index_given)
        index=mplex_default_index(mplex);

    if(l2)
        return llist_index_to_after(mplex->l2_list, mplex->l2_current, index);
    else
        return llist_index_to_after(mplex->l1_list, mplex->l1_current, index);
}
    

WRegion *mplex_do_attach(WMPlex *mplex, WRegionAttachHandler *hnd,
                         void *hnd_param, WMPlexAttachParams *param)
{
    WLListNode *node, *after;
    
    after=mplex_get_after(mplex, param->flags&MPLEX_ATTACH_L2, 
                          param->flags&MPLEX_ATTACH_INDEX, param->index);

    node=mplex_do_attach_after(mplex, after, param, hnd, hnd_param);
    
    return (node!=NULL ? node->reg : NULL);
}


#define MPLEX_ATTACH_SET_FLAGS (MPLEX_ATTACH_GEOM|       \
                                MPLEX_ATTACH_SIZEPOLICY| \
                                MPLEX_ATTACH_INDEX)


WRegion *mplex_attach_simple(WMPlex *mplex, WRegion *reg, int flags)
{
    WMPlexAttachParams par;
    
    if(reg==(WRegion*)mplex)
        return FALSE;
    
    par.flags=flags&~MPLEX_ATTACH_SET_FLAGS;
    
    return region__attach_reparent((WRegion*)mplex, reg,
                                   (WRegionDoAttachFn*)mplex_do_attach, 
                                   &par);
}


static void get_params(WMPlex *mplex, ExtlTab tab, WMPlexAttachParams *par)
{
    int layer=1;
    int tmp;
    
    par->flags=0;
    
    extl_table_gets_i(tab, "layer", &layer);
    if(layer==2){
        par->flags|=MPLEX_ATTACH_L2;
        
        if(extl_table_gets_rectangle(tab, "geom", &(par->geom)))
            par->flags|=MPLEX_ATTACH_GEOM;
    }
    
    if(extl_table_is_bool_set(tab, "switchto"))
        par->flags|=MPLEX_ATTACH_SWITCHTO;

    if(extl_table_is_bool_set(tab, "hidden"))
        par->flags|=MPLEX_ATTACH_L2_HIDDEN;

    if(extl_table_is_bool_set(tab, "passive"))
        par->flags|=MPLEX_ATTACH_L2_PASSIVE;
    
    if(extl_table_gets_i(tab, "index", &(par->index)))
        par->flags|=MPLEX_ATTACH_INDEX;

    if(extl_table_gets_i(tab, "sizepolicy", &tmp)){
        par->flags|=MPLEX_ATTACH_SIZEPOLICY;
        par->szplcy=tmp;
    }
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
    get_params(mplex, param, &par);
    
    /* region__attach_reparent should do better checks. */
    if(reg==NULL || reg==(WRegion*)mplex)
        return NULL;
    
    return region__attach_reparent((WRegion*)mplex, reg,
                                   (WRegionDoAttachFn*)mplex_do_attach, 
                                   &par);
}


/*EXTL_DOC
 * Create a new region to be managed by \var{mplex}. At least the following
 * fields in \var{param} are understood:
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *  \tabhead{Field & Description}
 *  \var{type} & Class name (a string) of the object to be created. Mandatory. \\
 *  \var{name} & Name of the object to be created (a string). Optional. \\
 *  \var{switchto} & Should the region be switched to (boolean)? Optional. \\
 *  \var{index} & Index of the new region in \var{mplex}'s list of
 *   managed objects (integer, 0 = first). Optional. \\
 *  \var{layer} & Layer to attach on; 1 (default) or 2. \\
 *  \var{passive} & Is a layer 2 object passive/skipped when deciding
 *      object to gives focus to (boolean)? Optional.
 * \end{tabularx}
 * 
 * In addition parameters to the region to be created are passed in this 
 * same table.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_attach_new(WMPlex *mplex, ExtlTab param)
{
    WMPlexAttachParams par;
    get_params(mplex, param, &par);
    
    return region__attach_load((WRegion*)mplex, param,
                               (WRegionDoAttachFn*)mplex_do_attach, 
                               &par);
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
    WRegion *curr=mplex_lcurrent(mplex, 1);
    
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
    int swf=(param->switchto ? MPLEX_ATTACH_SWITCHTO : 0);
    WPHolder *ph=NULL;
    WMPlexPHolder *mph;
    WLListNode *after;
    
    if(redir==MANAGE_REDIR_STRICT_YES || redir==MANAGE_REDIR_PREFER_YES){
        if(mplex->l2_current!=NULL){
            ph=region_prepare_manage(mplex->l2_current->reg, cwin, param,
                                     MANAGE_REDIR_PREFER_YES);
            if(ph!=NULL)
                return ph;
        }
        if(mplex->l1_current!=NULL){
            ph=region_prepare_manage(mplex->l1_current->reg, cwin, param,
                                     MANAGE_REDIR_PREFER_YES);
            if(ph!=NULL)
                return ph;
        }
    }
    
    if(redir==MANAGE_REDIR_STRICT_YES)
        return NULL;
    
    after=mplex_get_after(mplex, FALSE, FALSE, 0);
    
    mph=create_mplexpholder(mplex, NULL, after, 1);
    
    if(mph!=NULL){
        mph->szplcy=SIZEPOLICY_FULL_BOUNDS;
        mph->initial=TRUE;
    }
        
    return (WPHolder*)mph;
}


/*}}}*/


/*{{{ Remove */


void mplex_managed_remove(WMPlex *mplex, WRegion *sub)
{
    bool l2=FALSE;
    bool sw=FALSE;
    WRegion *stdisp=(WRegion*)(mplex->stdispwatch.obj);
    WLListNode *node, *next=NULL;
    
    if(MANAGES_STDISP(sub) && stdisp!=NULL && REGION_MANAGER(stdisp)==sub){
        region_unmanage_stdisp(sub, TRUE, TRUE);
        region_detach_manager(stdisp);
    }
    
    region_unset_manager(sub, (WRegion*)mplex);

    node=llist_find_on(mplex->l2_list, sub);
    if(node!=NULL){
        l2=TRUE;
        
        mplex_move_phs_before(mplex, node);
        llist_unlink(&(mplex->l2_list), node);
        mplex->l2_count--;

        assert(node!=mplex->l1_current);

        if(mplex->l2_current==node){
            WLListNode *node2;
            mplex->l2_current=NULL;
            FOR_ALL_NODES_ON_LLIST(node2, mplex->l2_list){
                if((node2->flags&(LLIST_L2_HIDDEN|LLIST_L2_PASSIVE))==0)
                    mplex->l2_current=node2;
            }
        }
    }else{
        node=llist_find_on(mplex->l1_list, sub);
        if(node==NULL)
            return;

        assert(node!=mplex->l2_current);
        
        if(mplex->l1_current==node){
            mplex->l1_current=NULL;
            sw=TRUE;
            next=LIST_PREV(mplex->l1_list, node, next, prev);
            if(next==NULL){
                next=LIST_NEXT(mplex->l1_list, node, next, prev);
                if(next==node)
                    next=NULL;
            }
        }
        
        mplex_move_phs_before(mplex, node);
        llist_unlink(&(mplex->l1_list), node);
        mplex->l1_count--;
    }
    
    free(node);
    
    if(OBJ_IS_BEING_DESTROYED(mplex))
        return;
    
    if(next!=NULL && sw)
        mplex_do_node_goto_sw(mplex, next, FALSE, TRUE);
    else if(l2 && region_may_control_focus((WRegion*)mplex))
        region_warp((WRegion*)mplex);
    
    if(!l2)
        mplex_managed_changed(mplex, MPLEX_CHANGE_REMOVE, sw, sub);
}


bool mplex_rescue_clientwins(WMPlex *mplex, WPHolder *ph)
{
    bool ret1, ret2, ret3;
    WLListIterTmp tmp;
    
    llist_iter_init(&tmp, mplex->l1_list);
    ret1=region_rescue_some_clientwins((WRegion*)mplex, ph,
                                       (WRegionIterator*)llist_iter_regions,
                                       &tmp);

    llist_iter_init(&tmp, mplex->l2_list);
    ret2=region_rescue_some_clientwins((WRegion*)mplex, ph,
                                       (WRegionIterator*)llist_iter_regions,
                                       &tmp);
    
    ret3=region_rescue_child_clientwins((WRegion*)mplex, ph);
    
    return (ret1 && ret2 && ret3);
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
        mgr=REGION_MANAGER(oldstdisp);
        if(!MANAGES_STDISP(mgr))
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
        
        if(mplex->l1_current!=NULL
           && MANAGES_STDISP(mplex->l1_current->reg)
           && mgr!=mplex->l1_current->reg){
            if(mgr!=NULL){
                region_unmanage_stdisp(mgr, FALSE, TRUE);
                region_detach_manager(oldstdisp);
            }
            mgr=mplex->l1_current->reg;
        }
        if(mgr!=NULL)
            region_manage_stdisp(mgr, reg, &(mplex->stdispinfo));
        else
            region_unmap(reg);
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


static WRegion *do_attach_stdisp(WMPlex *mplex, WRegionAttachHandler *handler,
                                 void *handlerparams, const WFitParams *fp)
{
    return handler((WWindow*)mplex, fp, handlerparams);
    /* We do not manage the stdisp, so manager is not set in this
     * function unlike a true "attach" function.
     */
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
        
        stdisp=region__attach_load((WRegion*)mplex, t,
                                   (WRegionDoAttachFn*)do_attach_stdisp, 
                                   (void*)&fp);
        
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


ExtlTab mplex_get_configuration(WMPlex *mplex)
{
    WLListNode *node;
    ExtlTab tab, subs, stdisptab, g;
    int n=0;
    
    tab=region_get_base_configuration((WRegion*)mplex);
    
    subs=extl_create_table();
    extl_table_sets_t(tab, "subs", subs);
    
    FOR_ALL_NODES_ON_LLIST(node, mplex->l1_list){
        ExtlTab st=region_get_configuration(node->reg);
        if(st!=extl_table_none()){
            if(node==mplex->l1_current)
                extl_table_sets_b(st, "switchto", TRUE);
            extl_table_seti_t(subs, ++n, st);
            extl_table_sets_i(st, "sizepolicy", node->szplcy);
            extl_unref_table(st);
        }
    }
    
    FOR_ALL_NODES_ON_LLIST(node, mplex->l2_list){
        ExtlTab st;
        
        /* Don't save menus, queries and such even if they'd support it. */
        if(node->flags&LLIST_L2_SEMIMODAL)
            continue;
        
        st=region_get_configuration(node->reg);
        if(st!=extl_table_none()){
            extl_table_sets_i(st, "layer", 2);
            if(node==mplex->l2_current){
                extl_table_sets_b(st, "switchto", 
                                  !(node->flags&LLIST_L2_PASSIVE));
            }
            extl_table_sets_i(st, "sizepolicy", node->szplcy);
            extl_table_sets_b(st, "passive", node->flags&LLIST_L2_PASSIVE);
            extl_table_sets_b(st, "hidden", node->flags&LLIST_L2_HIDDEN);
            g=extl_table_from_rectangle(&REGION_GEOM(node->reg));
            extl_table_sets_t(st, "geom", g);
            extl_unref_table(g);
            extl_table_seti_t(subs, ++n, st);
            extl_unref_table(st);
        }
    }
    
    extl_unref_table(subs);
    
    /*stdisptab=mplex_do_get_stdisp_extl(mplex, TRUE);
    if(stdisptab!=extl_table_none()){
        extl_table_sets_t(tab, "stdisp", stdisptab);
        extl_unref_table(stdisptab);
    }*/
    
    return tab;
}


static int tmp_layer=0;
static WMPlex *tmp_mplex=NULL;


static WPHolder *pholder_callback()
{
    assert(tmp_mplex!=NULL);
    return (WPHolder*)mplex_last_place_holder(tmp_mplex, tmp_layer);
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
                char *tmp=NULL;
                
                get_params(mplex, subtab, &par);
                
                par.flags|=MPLEX_ATTACH_INDEX;
                par.index=LLIST_INDEX_LAST;

                tmp_layer=(par.flags&MPLEX_ATTACH_L2 ? 2 : 1);
                tmp_mplex=mplex;
                
                ioncore_set_sm_pholder_callback(pholder_callback);
    
                region__attach_load((WRegion*)mplex, subtab,
                                    (WRegionDoAttachFn*)mplex_do_attach, 
                                    &par);

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
    
    {(DynFun*)region_managed_goto,
     (DynFun*)mplex_managed_goto},
    
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
    
    {region_managed_activated,
     mplex_managed_activated},

    {(DynFun*)region_managed_get_pholder,
     (DynFun*)mplex_managed_get_pholder},

    {(DynFun*)region_get_rescue_pholder_for,
     (DynFun*)mplex_get_rescue_pholder_for},

    {(DynFun*)region_may_destroy,
     (DynFun*)mplex_may_destroy},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WMPlex, WWindow, mplex_deinit, mplex_dynfuntab);


/*}}}*/

