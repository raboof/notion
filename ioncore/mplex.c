/*
 * ion/ioncore/mplex.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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
#include "region-iter.h"
#include "saveload.h"
#include "xwindow.h"


#define MPLEX_WIN(MPLEX) ((MPLEX)->win.win)
#define MPLEX_MGD_UNVIEWABLE(MPLEX) \
            ((MPLEX)->flags&MPLEX_MANAGED_UNVIEWABLE)

    
/*{{{ Layer list stuff */


#define MGD_L2_HIDDEN 0x0001
#define MGD_L2_PASSIVE 0x0002

#define INDEX_AFTER_CURRENT (INT_MIN)
#define DEFAULT_INDEX(MPLEX) \
    ((MPLEX)->flags&MPLEX_ADD_TO_END ? -1 : INDEX_AFTER_CURRENT)


#define GET_REG(NODE) ((NODE)!=NULL ? (NODE)->reg : NULL)


#define FOR_ALL_NODES(LL, NODE) \
    LIST_FOR_ALL(LL, NODE, next, prev)

#define FOR_ALL_NODES_W_NEXT(LL, NODE) \
    LIST_FOR_ALL_W_NEXT(LL, NODE, NEXT, next, prev)

#define FOR_ALL_REGIONS(LL, NODE, REG)          \
   for(REG=(LL==NULL ? NULL : (LL)->reg),       \
       NODE=(LL==NULL ? NULL : (LL)->next);     \
       REG!=NULL;                               \
       REG=(NODE==NULL ? NULL : (NODE)->reg),   \
       NODE=(NODE==NULL ? NULL : (NODE)->next))

        

static WMPlexManaged *find_on_list(WMPlexManaged *list, WRegion *reg)
{
    WMPlexManaged *node;
    
    FOR_ALL_NODES(list, node){
        if(node->reg==reg)
            return node;
    }
    
    return NULL;
}


static bool on_list(WMPlexManaged *list, WRegion *reg)
{
    return (find_on_list(list, reg)!=NULL);
}


static WMPlexManaged *nth_node_on_list(WMPlexManaged *list, uint n)
{
    WMPlexManaged *node;
    
    FOR_ALL_NODES(list, node){
        if(n==0)
            return node;
        n--;
    }
            
    return NULL;
}


static WRegion *nth_region_on_list(WMPlexManaged *list, uint n)
{
    WMPlexManaged *node=nth_node_on_list(list, n);
    return (node!=NULL ? node->reg : NULL);
}


static void do_link_at(WMPlexManaged **list, WMPlexManaged *node,
                       WMPlexManaged *after, bool nullfirst)
{
    if(after!=NULL){
        LINK_ITEM_AFTER(*list, after, node, next, prev);
    }else if(nullfirst){
        LINK_ITEM_FIRST(*list, node, next, prev);
    }else{
        LINK_ITEM(*list, node, next, prev);
    }
}


static void link_at(WMPlexManaged **list, WMPlexManaged *node, int index)
{
    WMPlexManaged *after=NULL;
    
    if(index>0){
        after=nth_node_on_list(*list, index-1);
        if(after==node)
            return;
    }
    
    do_link_at(list, node, after, index==0);
}


static void unlink(WMPlexManaged **list, WMPlexManaged *node)
{
    UNLINK_ITEM(*list, node, next, prev);
}


static ExtlTab llist_to_table(WMPlexManaged *list)
{
    WMPlexManaged *node;
    int i=1;
    ExtlTab t=extl_create_table();
    
    FOR_ALL_NODES(list, node){
        if(extl_table_seti_o(t, i, (Obj*)node->reg))
            i++;
    }
    
    return t;
}

/*}}}*/


/*{{{ Destroy/create mplex */


bool mplex_do_init(WMPlex *mplex, WWindow *parent, Window win,
                   const WFitParams *fp, bool create)
{
    mplex->flags=0;
    mplex->l1_count=0;
    mplex->l1_list=NULL;
    mplex->l1_current=NULL;
    mplex->l2_count=0;
    mplex->l2_list=NULL;
    mplex->l2_current=NULL;
    watch_init(&(mplex->stdispinfo.regwatch));
    mplex->stdispinfo.pos=MPLEX_STDISP_BL;
    
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
    WMPlexManaged *nextnode;
    WRegion *reg;
    
    FOR_ALL_REGIONS(mplex->l1_list, nextnode, reg){
        destroy_obj((Obj*)reg);
    }

    FOR_ALL_REGIONS(mplex->l2_list, nextnode, reg){
        destroy_obj((Obj*)reg);
    }

    assert(mplex->l1_list==NULL);
    assert(mplex->l2_list==NULL);
    
    window_deinit((WWindow*)mplex);
}


/*}}}*/


/*{{{ Higher-level layer list stuff */


static bool on_l1_list(WMPlex *mplex, WRegion *reg)
{
    return on_list(mplex->l1_list, reg);
}


static bool on_l2_list(WMPlex *mplex, WRegion *reg)
{
    return on_list(mplex->l2_list, reg);
}


WRegion *mplex_current(WMPlex *mplex)
{
    return (mplex->l2_current!=NULL 
            ? mplex->l2_current->reg
            : GET_REG(mplex->l1_current));
}


/*EXTL_DOC
 * Returns the layer \var{reg} is on \var{mplex} or $-1$ if \var{reg}
 * is not managed by \var{mplex}.
 */
EXTL_EXPORT_MEMBER
int mplex_layer(WMPlex *mplex, WRegion *reg)
{
    if(on_l1_list(mplex, reg))
        return 1;
    if(on_l2_list(mplex, reg))
        return 2;
    return -1;
}


/*EXTL_DOC
 * Return the managed object currently active within layer \var{l} of
 * \var{mplex}.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_lcurrent(WMPlex *mplex, uint l)
{
    return (l==1 
            ? GET_REG(mplex->l1_current)
            : (l==2
               ? GET_REG(mplex->l2_current)
               : 0));
}


/*EXTL_DOC
 * Returns the \var{n}:th object managed by \var{mplex} on the the
 * \var{l}:th layer..
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_lnth(WMPlex *mplex, uint l, uint n)
{
    return (l==1 
            ? nth_region_on_list(mplex->l1_list, n)
            : (l==2
               ? nth_region_on_list(mplex->l2_list, n)
               : 0));
}


/*EXTL_DOC
 * Returns a list of regions managed by \var{mplex} on layer \var{l}.
 */
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
    WMPlexManaged *node;
    
    if(reg==NULL)
        return;
    
    node=find_on_list(mplex->l1_list, reg);
    
    if(node==NULL)
        return;
    
    unlink(&(mplex->l1_list), node);
    link_at(&(mplex->l1_list), node, index);
    
    mplex_managed_changed(mplex, MPLEX_CHANGE_REORDER, FALSE, reg);
}


/*EXTL_DOC
 * Get index of \var{reg} within the multiplexer. The first region managed
 * by \var{mplex} has index zero. If \var{reg} is not managed by \var{mplex},
 * -1 is returned.
 */
EXTL_EXPORT_MEMBER
int mplex_get_index(WMPlex *mplex, WRegion *reg)
{
    WMPlexManaged *nextnode;
    WRegion *other;
    int index=0;
    
    FOR_ALL_REGIONS(mplex->l1_list, nextnode, other){
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


/*{{{ Mapping */


static void mplex_map_mgd(WMPlex *mplex)
{
    WMPlexManaged *node;

    if(mplex->l1_current!=NULL)
        region_map(mplex->l1_current->reg);
    
    FOR_ALL_NODES(mplex->l2_list, node){
        if(!(node->flags&MGD_L2_HIDDEN))
            region_map(node->reg);
    }
}


static void mplex_unmap_mgd(WMPlex *mplex)
{
    WMPlexManaged *node;
    
    if(mplex->l1_current!=NULL)
        region_unmap(mplex->l1_current->reg);
    
    FOR_ALL_NODES(mplex->l2_list, node){
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


void mplex_fit_managed(WMPlex *mplex)
{
    WRectangle geom;
    WRegion *sub;
    WFitParams fp;
    WMPlexManaged *nextnode;
    
    mplex_managed_geom(mplex, &(fp.g));
    
    if(!MPLEX_MGD_UNVIEWABLE(mplex) && (fp.g.w<=1 || fp.g.h<=1)){
        mplex->flags|=MPLEX_MANAGED_UNVIEWABLE;
        if(REGION_IS_MAPPED(mplex))
            mplex_unmap_mgd(mplex);
    }else if(MPLEX_MGD_UNVIEWABLE(mplex) && !(fp.g.w<=1 || fp.g.h<=1)){
        mplex->flags&=~MPLEX_MANAGED_UNVIEWABLE;
        if(REGION_IS_MAPPED(mplex))
            mplex_map_mgd(mplex);
    }
    
    if(!MPLEX_MGD_UNVIEWABLE(mplex)){
        fp.mode=REGION_FIT_EXACT;
        FOR_ALL_REGIONS(mplex->l1_list, nextnode, sub){
            region_fitrep(sub, NULL, &fp);
        }
        
        fp.mode=REGION_FIT_BOUNDS;
        FOR_ALL_REGIONS(mplex->l2_list, nextnode, sub){
            region_fitrep(sub, NULL, &fp);
        }
    }
}


static void mplex_managed_rqgeom(WMPlex *mplex, WRegion *sub,
                                 int flags, const WRectangle *geom, 
                                 WRectangle *geomret)
{
    WRectangle mg, rg;
    
    mplex_managed_geom(mplex, &mg);
    
    if(on_l2_list(mplex, sub)){
        /* allow changes but constrain with managed area */
        rg=*geom;
        rectangle_constrain(&rg, &mg);
    }else{
        rg=mg;
    }
    
    if(geomret!=NULL)
        *geomret=rg;
    
    if(!(flags&REGION_RQGEOM_TRYONLY))
        region_fit(sub, &rg, REGION_FIT_EXACT);
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


/*}}}*/


/*{{{ Managed region switching */


void mplex_managed_activated(WMPlex *mplex, WRegion *reg)
{
    WMPlexManaged *node=find_on_list(mplex->l2_list, reg);

    if(node!=NULL){
        mplex->l2_current=node;
    }else if(mplex->l2_current!=NULL && 
             mplex->l2_current->flags&MGD_L2_PASSIVE){
        mplex->l2_current=NULL;
    }
}


/*EXTL_DOC
 * Is \var{reg} on the layer2 of \var{mplex}, but hidden?
 */
EXTL_EXPORT_MEMBER
bool mplex_l2_hidden(WMPlex *mplex, WRegion *reg)
{
    WMPlexManaged *node=find_on_list(mplex->l2_list, reg);
    
    return (node!=NULL && node->flags&MGD_L2_HIDDEN);
}


/*EXTL_DOC
 * If var \var{reg} is on the l2 list of \var{mplex} and currently shown, 
 * hide it. 
 */
EXTL_EXPORT_MEMBER
bool mplex_l2_hide(WMPlex *mplex, WRegion *reg)
{
    bool mcf=region_may_control_focus((WRegion*)mplex);
    WMPlexManaged *node=find_on_list(mplex->l2_list, reg);
    
    if(node==NULL || node->flags&MGD_L2_HIDDEN)
        return FALSE;
    
    node->flags|=MGD_L2_HIDDEN;
    
    if(REGION_IS_MAPPED(mplex) && !MPLEX_MGD_UNVIEWABLE(mplex))
        region_unmap(reg);
    
    if(mplex->l2_current==node){
        WMPlexManaged *node2;
        mplex->l2_current=NULL;
        FOR_ALL_NODES(mplex->l2_list, node2){
            if((node2->flags&(MGD_L2_HIDDEN|MGD_L2_PASSIVE))==0)
                mplex->l2_current=node2;
        }
    }
    
    if(mcf)
        region_warp((WRegion*)mplex);
    
    return TRUE;
}


/*EXTL_DOC
 * If var \var{reg} is on the l2 list of \var{mplex} and currently hidden, 
 * display it. 
 */
EXTL_EXPORT_MEMBER
bool mplex_l2_show(WMPlex *mplex, WRegion *reg)
{
    bool mcf=region_may_control_focus((WRegion*)mplex);
    WMPlexManaged *node=find_on_list(mplex->l2_list, reg);

    if(node==NULL || !(node->flags&MGD_L2_HIDDEN))
        return FALSE;
    
    return mplex_managed_goto(mplex, reg, (mcf ? REGION_GOTO_FOCUS : 0));
}


static bool mplex_do_managed_display(WMPlex *mplex, WRegion *sub,
                                     bool call_changed)
{
    bool l2=FALSE;
    WRegion *stdisp;
    WMPlexManaged *node;
    
    node=find_on_list(mplex->l2_list, sub);
    
    if(node!=NULL){
        l2=TRUE;
        if(node==mplex->l2_current)
            return TRUE;
    }else{
        node=find_on_list(mplex->l1_list, sub);
        if(node==NULL)
            return FALSE;
        if(node==mplex->l1_current)
            return TRUE;
    }

    stdisp=(WRegion*)(mplex->stdispinfo.regwatch.obj);
    
    if(!l2 && OBJ_IS(sub, WGenWS)){
        if(stdisp!=NULL){
            WRegion *mgr=REGION_MANAGER(stdisp);
            if(mgr!=sub){
                if(OBJ_IS(mgr, WGenWS)){
                    genws_unmanage_stdisp((WGenWS*)mgr, FALSE, FALSE);
                    region_detach_manager(stdisp);
                }
            
                genws_manage_stdisp((WGenWS*)sub, stdisp, 
                                    mplex->stdispinfo.pos);
            }
        }else{
            genws_unmanage_stdisp((WGenWS*)sub, TRUE, FALSE);
        }
    }
    
    if(REGION_IS_MAPPED(mplex) && !MPLEX_MGD_UNVIEWABLE(mplex))
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
        if(OBJ_IS(sub, WClientWin))
            region_lower(sub);
        
        /* This call should be unnecessary... */
        mplex_managed_activated(mplex, sub);
    }else{
        unlink(&(mplex->l2_list), node);
        region_raise(sub);
        do_link_at(&(mplex->l2_list), node, NULL, TRUE);
        
        node->flags&=~MGD_L2_HIDDEN;
        if(!(node->flags&MGD_L2_PASSIVE))
            mplex->l2_current=node;
    }
    
    if(!l2){
        mplex_managed_changed(mplex, MPLEX_CHANGE_SWITCHONLY, TRUE, sub);
        return mplex->l2_current==NULL;
    }else{
        return mplex->l2_current==node;
    }
}


static bool mplex_do_managed_goto(WMPlex *mplex, WRegion *sub,
                                  bool call_changed, int flags)
{
    if(!mplex_do_managed_display(mplex, sub, call_changed))
        return FALSE;
    
    if(flags&REGION_GOTO_FOCUS)
        region_maybewarp((WRegion*)mplex, !(flags&REGION_GOTO_NOWARP));
    
    return TRUE;
}


static bool mplex_do_managed_goto_sw(WMPlex *mplex, WRegion *sub,
                                     bool call_changed)
{
    bool mcf=region_may_control_focus((WRegion*)mplex);
    return mplex_do_managed_goto(mplex, sub, FALSE, 
                                 (mcf ? REGION_GOTO_FOCUS : 0)
                                 |REGION_GOTO_NOWARP);
}


bool mplex_managed_goto(WMPlex *mplex, WRegion *sub, int flags)
{
    return mplex_do_managed_goto(mplex, sub, TRUE, flags);
}


static void do_switch(WMPlex *mplex, WMPlexManaged *node)
{
    if(node!=NULL){
        bool mcf=region_may_control_focus((WRegion*)mplex);
        if(mcf)
            ioncore_set_previous_of(node->reg);
        region_managed_goto((WRegion*)mplex, node->reg,
                            (mcf ? REGION_GOTO_FOCUS : 0));
    }
}


/*EXTL_DOC
 * Have \var{mplex} display the \var{n}:th object managed by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_nth(WMPlex *mplex, uint n)
{
    do_switch(mplex, nth_node_on_list(mplex->l1_list, n));
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


/*}}}*/


/*{{{ Attach */


static WRegion *mplex_do_attach(WMPlex *mplex, WRegionAttachHandler *hnd,
                                void *hnd_param, MPlexAttachParams *param)
{
    WRegion *reg;
    WFitParams fp;
    bool sw=param->flags&MPLEX_ATTACH_SWITCHTO;
    bool l2=param->flags&MPLEX_ATTACH_L2;
    WMPlexManaged *node;
    
    mplex_managed_geom(mplex, &(fp.g));
    if(l2 && param->flags&MPLEX_ATTACH_L2_GEOM){
        WRectangle tmp=param->l2geom;
        rectangle_constrain(&tmp, &(fp.g));
        fp.g=tmp;
        fp.mode=REGION_FIT_EXACT;
    }else if(l2){
        fp.mode=REGION_FIT_BOUNDS;
    }else{
        fp.mode=REGION_FIT_EXACT;
    }
    
    node=ALLOC(WMPlexManaged);
    
    if(node==NULL)
        return NULL;
    
    reg=hnd((WWindow*)mplex, &fp, hnd_param);
    
    if(reg==NULL){
        free(node);
        return NULL;
    }
    
    node->reg=reg;
    node->flags=0;
    
    if(l2){
        do_link_at(&(mplex->l2_list), node, NULL, TRUE);
        mplex->l2_count++;
    }else{
        if(mplex->l1_current!=NULL && param->index==INDEX_AFTER_CURRENT)
            do_link_at(&(mplex->l1_list), node, mplex->l1_current, FALSE);
        else
            link_at(&(mplex->l1_list), node, param->index);
        mplex->l1_count++;
    }
    
    region_set_manager(reg, (WRegion*)mplex, NULL);
    
    if(l2){
        if(!sw)
            node->flags|=MGD_L2_HIDDEN;
        if(param->flags&MPLEX_ATTACH_L2_PASSIVE)
            node->flags|=MGD_L2_PASSIVE;
    }
    
    if(!l2 && mplex->l1_count==1)
        sw=TRUE;
    
    if(sw)
        mplex_do_managed_goto_sw(mplex, reg, FALSE);
    else
        region_unmap(reg);

    if(!l2)
        mplex_managed_changed(mplex, MPLEX_CHANGE_ADD, sw, reg);
    
    return reg;
}


WRegion *mplex_attach_simple(WMPlex *mplex, WRegion *reg, int flags)
{
    MPlexAttachParams par;
    
    if(reg==(WRegion*)mplex)
        return FALSE;
    
    par.index=DEFAULT_INDEX(mplex);
    par.flags=flags;
    
    return region__attach_reparent((WRegion*)mplex, reg,
                                   (WRegionDoAttachFn*)mplex_do_attach, 
                                   &par);
}


WRegion *mplex_attach_hnd(WMPlex *mplex, WRegionAttachHandler *hnd,
                          void *hnd_param, int flags)
{
    MPlexAttachParams par;
    
    par.index=DEFAULT_INDEX(mplex);
    par.flags=flags&~MPLEX_ATTACH_L2_GEOM;
    
    return mplex_do_attach(mplex, hnd, hnd_param, &par);
}


static void get_params(ExtlTab tab, MPlexAttachParams *par)
{
    int layer=1;
    
    par->flags=0;
    par->index=-1;
    
    extl_table_gets_i(tab, "layer", &layer);
    if(layer==2){
        par->flags|=MPLEX_ATTACH_L2;
        
        if(extl_table_gets_rectangle(tab, "geom", &(par->l2geom)))
            par->flags|=MPLEX_ATTACH_L2_GEOM;
    }
    
    if(extl_table_is_bool_set(tab, "switchto"))
        par->flags|=MPLEX_ATTACH_SWITCHTO;

    if(extl_table_is_bool_set(tab, "passive"))
        par->flags|=MPLEX_ATTACH_L2_PASSIVE;
    
    extl_table_gets_i(tab, "index", &(par->index));
}


/*EXTL_DOC
 * Attach and reparent existing region \var{reg} to \var{mplex}.
 * The table \var{param} may contain the fields \var{index} and
 * \var{switchto} that are interpreted as for \fnref{WMPlex.attach_new}.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_attach(WMPlex *mplex, WRegion *reg, ExtlTab param)
{
    MPlexAttachParams par;
    get_params(param, &par);
    
    /* region__attach_reparent should do better checks. */
    if(reg==NULL || reg==(WRegion*)mplex)
        return FALSE;
    
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
    MPlexAttachParams par;
    get_params(param, &par);
    
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


bool mplex_manage_clientwin(WMPlex *mplex, WClientWin *cwin,
                            const WManageParams *param, int redir)
{
    int swf=(param->switchto ? MPLEX_ATTACH_SWITCHTO : 0);
    
    if(redir==MANAGE_REDIR_STRICT_YES || redir==MANAGE_REDIR_PREFER_YES){
        if(mplex->l2_current!=NULL){
            if(region_manage_clientwin(mplex->l2_current->reg, cwin, param,
                                       MANAGE_REDIR_PREFER_YES))
                return TRUE;
        }
        if(mplex->l1_current!=NULL){
            if(region_manage_clientwin(mplex->l1_current->reg, cwin, param,
                                       MANAGE_REDIR_PREFER_YES))
                return TRUE;
        }
    }
    
    if(redir==MANAGE_REDIR_STRICT_YES)
        return FALSE;
    
    return (NULL!=mplex_attach_simple(mplex, (WRegion*)cwin, swf));
}


bool mplex_manage_rescue(WMPlex *mplex, WClientWin *cwin, WRegion *from)
{
    return (NULL!=mplex_attach_simple(mplex, (WRegion*)cwin, 0));
}


/*}}}*/


/*{{{ Remove */


void mplex_managed_remove(WMPlex *mplex, WRegion *sub)
{
    bool l2=FALSE;
    bool sw=FALSE;
    WRegion *stdisp=(WRegion*)(mplex->stdispinfo.regwatch.obj);
    WMPlexManaged *node, *next=NULL;
    
    if(OBJ_IS(sub, WGenWS) && stdisp!=NULL && REGION_MANAGER(stdisp)==sub){
        genws_unmanage_stdisp((WGenWS*)sub, TRUE, TRUE);
        region_detach_manager(stdisp);
    }
    
    region_unset_manager(sub, (WRegion*)mplex, NULL);

    node=find_on_list(mplex->l2_list, sub);
    if(node!=NULL){
        l2=TRUE;
        unlink(&(mplex->l2_list), node);
        mplex->l2_count--;

        assert(node!=mplex->l1_current);

        if(mplex->l2_current==node){
            WMPlexManaged *node2;
            mplex->l2_current=NULL;
            FOR_ALL_NODES(mplex->l2_list, node2){
                if((node2->flags&(MGD_L2_HIDDEN|MGD_L2_PASSIVE))==0)
                    mplex->l2_current=node2;
            }
        }
    }else{
        node=find_on_list(mplex->l1_list, sub);
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
        
        unlink(&(mplex->l1_list), node);
        mplex->l1_count--;
    }
    
    free(node);
    
    if(OBJ_IS_BEING_DESTROYED(mplex))
        return;
    
    if(next!=NULL && sw)
        mplex_do_managed_goto_sw(mplex, next->reg, FALSE);
    else if(l2 && region_may_control_focus((WRegion*)mplex))
        region_warp((WRegion*)mplex);
    
    if(!l2)
        mplex_managed_changed(mplex, MPLEX_CHANGE_REMOVE, sw, sub);
}


bool mplex_rescue_clientwins(WMPlex *mplex)
{
    /*bool ret1, ret2, ret3;*/
    
#warning "TOFIX"    
    /*ret1=region_rescue_managed_clientwins((WRegion*)mplex, mplex->l1_list);
    ret2=region_rescue_managed_clientwins((WRegion*)mplex, mplex->l2_list);
    ret3=region_rescue_child_clientwins((WRegion*)mplex);
    
    return (ret1 && ret2 && ret3);*/
    return FALSE;
}


void mplex_child_removed(WMPlex *mplex, WRegion *sub)
{
    WMPlexSTDispInfo *di=&(mplex->stdispinfo);
    
    if(sub==(WRegion*)(di->regwatch.obj)){
        watch_reset(&(di->regwatch));
        mplex_set_stdisp(mplex, NULL, di->pos);
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


bool mplex_set_stdisp(WMPlex *mplex, WRegion *reg, int pos)
{
    WMPlexSTDispInfo *di=&(mplex->stdispinfo);
    WRegion *oldstdisp=(WRegion*)(di->regwatch.obj);
    WGenWS *mgr=NULL;
    
    assert(reg==NULL || (Obj*)reg==di->regwatch.obj ||
           (REGION_MANAGER(reg)==NULL && 
            REGION_PARENT(reg)==(WWindow*)mplex));
    
    if(oldstdisp!=NULL){
        mgr=OBJ_CAST(REGION_MANAGER(oldstdisp), WGenWS);
        
        if(oldstdisp!=reg){
            mainloop_defer_destroy(di->regwatch.obj);
            watch_reset(&(di->regwatch));
        }
    }
    
    di->pos=pos;
    
    if(reg==NULL){
        if(mgr!=NULL){
            genws_unmanage_stdisp((WGenWS*)mgr, TRUE, FALSE);
            region_detach_manager(oldstdisp);
        }
    }else{
        watch_setup(&(di->regwatch), (Obj*)reg, stdisp_watch_handler);
        
        if(mplex->l1_current!=NULL
           && OBJ_IS(mplex->l1_current->reg, WGenWS)
           && mgr!=(WGenWS*)(mplex->l1_current->reg)){
            if(mgr!=NULL){
                genws_unmanage_stdisp(mgr, FALSE, TRUE);
                region_detach_manager(oldstdisp);
            }
            mgr=(WGenWS*)(mplex->l1_current->reg);
        }
        if(mgr!=NULL)
            genws_manage_stdisp(mgr, reg, di->pos);
        else
            region_unmap(reg);
    }
    
    return TRUE;
}


void mplex_get_stdisp(WMPlex *mplex, WRegion **reg, int *pos)
{
    WMPlexSTDispInfo *di=&(mplex->stdispinfo);
    
    *reg=(WRegion*)di->regwatch.obj;
    *pos=di->pos;
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
    int p=mplex->stdispinfo.pos;
    char *s;
    
    if(extl_table_gets_s(t, "pos", &s)){
        p=stringintmap_value(pos_map, s, -1);
        if(p<0){
            warn(TR("Invalid position setting."));
            return NULL;
        }
    }
    
    s=NULL;
    extl_table_gets_s(t, "action", &s);
    
    if(s==NULL || strcmp(s, "replace")==0){
        WFitParams fp;
        int o2;
        
        fp.g.x=0;
        fp.g.y=0;
        fp.g.w=REGION_GEOM(mplex).w;
        fp.g.h=REGION_GEOM(mplex).h;
        fp.mode=REGION_FIT_BOUNDS;
        
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
        stdisp=(WRegion*)(mplex->stdispinfo.regwatch.obj);
    }else if(strcmp(s, "remove")!=0){
        warn(TR("Invalid action setting."));
        return FALSE;
    }
    
    if(!mplex_set_stdisp(mplex, stdisp, p)){
        destroy_obj((Obj*)stdisp);
        return NULL;
    }
    
    return stdisp;
}


static ExtlTab mplex_do_get_stdisp_extl(WMPlex *mplex, bool fullconfig)
{
    WMPlexSTDispInfo *di=&(mplex->stdispinfo);
    ExtlTab t;
    
    if(di->regwatch.obj==NULL)
        return extl_table_none();
    
    if(fullconfig){
        t=region_get_configuration((WRegion*)di->regwatch.obj);
        extl_table_sets_rectangle(t, "geom", &REGION_GEOM(di->regwatch.obj));
    }else{
        t=extl_create_table();
        extl_table_sets_o(t, "reg", di->regwatch.obj);
    }
    
    if(t!=extl_table_none())
        extl_table_sets_s(t, "pos", stringintmap_key(pos_map, di->pos, NULL));
    
    return t;
}


/*EXTL_DOC
 * Get status display information. See \fnref{WMPlex.get_stdisp} for
 * information on the fields.
 */
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
    
    ret=extl_call(fn, "t", NULL, t);
    
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
    WMPlexManaged *node;
    ExtlTab tab, subs, stdisptab, g;
    int n=0;
    
    tab=region_get_base_configuration((WRegion*)mplex);
    
    subs=extl_create_table();
    extl_table_sets_t(tab, "subs", subs);
    
    FOR_ALL_NODES(mplex->l1_list, node){
        ExtlTab st=region_get_configuration(node->reg);
        if(st!=extl_table_none()){
            if(node==mplex->l1_current)
                extl_table_sets_b(st, "switchto", TRUE);
            extl_table_seti_t(subs, ++n, st);
            extl_unref_table(st);
        }
    }
    
    FOR_ALL_NODES(mplex->l1_list, node){
        ExtlTab st=region_get_configuration(node->reg);
        if(st!=extl_table_none()){
            extl_table_sets_i(st, "layer", 2);
            extl_table_sets_b(st, "switchto", !(node->flags&MGD_L2_HIDDEN));
            extl_table_sets_b(st, "passive", node->flags&MGD_L2_PASSIVE);
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
                mplex_attach_new(mplex, subtab);
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
    
    {(DynFun*)region_manage_clientwin,
     (DynFun*)mplex_manage_clientwin},
    
    {(DynFun*)region_current,
     (DynFun*)mplex_current},
    
    {(DynFun*)region_rescue_clientwins,
     (DynFun*)mplex_rescue_clientwins},
    
    {(DynFun*)region_manage_rescue,
     (DynFun*)mplex_manage_rescue},
    
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
    
    END_DYNFUNTAB
};


IMPLCLASS(WMPlex, WWindow, mplex_deinit, mplex_dynfuntab);


/*}}}*/

