 /*
 * ion/ioncore/mplex.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <limits.h>
#include <string.h>

#include "common.h"
#include <libtu/objp.h>
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
#include "stacking.h"
#include "extl.h"
#include "extlconv.h"
#include "genws.h"
#include "frame-pointer.h"
#include "bindmaps.h"
#include "regbind.h"
#include "region-iter.h"


#define MPLEX_WIN(MPLEX) ((MPLEX)->win.win)
#define MPLEX_MGD_UNVIEWABLE(MPLEX) \
            ((MPLEX)->flags&MPLEX_MANAGED_UNVIEWABLE)


/*{{{ Destroy/create mplex */


static bool mplex_do_init(WMPlex *mplex, WWindow *parent, Window win,
                          const WRectangle *geom, bool create)
{
    mplex->managed_count=0;
    mplex->managed_list=NULL;
    mplex->current_sub=NULL;
    mplex->current_input=NULL;
    mplex->flags=0;
    
    if(create){
        if(!window_init_new((WWindow*)mplex, parent, geom))
            return FALSE;
    }else{
        if(!window_init((WWindow*)mplex, parent, win, geom))
            return FALSE;
    }
    
    mplex->win.region.flags|=REGION_BINDINGS_ARE_GRABBED;

    region_add_bindmap((WRegion*)mplex, &ioncore_mplex_bindmap);

    return TRUE;
}


bool mplex_init(WMPlex *mplex, WWindow *parent, Window win,
                const WRectangle *geom)
{
    return mplex_do_init(mplex, parent, win, geom, FALSE);
}


bool mplex_init_new(WMPlex *mplex, WWindow *parent, const WRectangle *geom)
{
    return mplex_do_init(mplex, parent, None, geom, TRUE);
}
    

void mplex_deinit(WMPlex *mplex)
{
    window_deinit((WWindow*)mplex);
}


/*}}}*/


/*{{{ Ordering */


static void link_at(WMPlex *mplex, WRegion *reg, int index)
{
    WRegion *after=NULL;
    
    if(index>0){
        after=mplex_nth_managed(mplex, index-1);
    }else if(index<0){
        if(!(mplex->flags&MPLEX_ADD_TO_END) && ioncore_g.opmode!=IONCORE_OPMODE_INIT)
            after=mplex->current_sub;
    }

    if(after==reg)
        after=NULL;
    
    if(after!=NULL){
        LINK_ITEM_AFTER(mplex->managed_list, after, reg, mgr_next, mgr_prev);
    }else if(index==0){
        LINK_ITEM_FIRST(mplex->managed_list, reg, mgr_next, mgr_prev);
    }else{
        LINK_ITEM(mplex->managed_list, reg, mgr_next, mgr_prev);
    }
}


/*EXTL_DOC
 * Set index of \var{reg} within the multiplexer to \var{index}.
 */
EXTL_EXPORT_MEMBER
void mplex_set_managed_index(WMPlex *mplex, WRegion *reg, int index)
{
    if(index<0)
        return;

    if(REGION_MANAGER(reg)!=(WRegion*)mplex)
        return;

    UNLINK_ITEM(mplex->managed_list, reg, mgr_next, mgr_prev);
    link_at(mplex, reg, index);
    mplex_managed_changed(mplex, MPLEX_CHANGE_REORDER, FALSE, reg);
}


/*EXTL_DOC
 * Get index of \var{reg} within the multiplexer. The first region managed
 * by \var{mplex} has index zero. If \var{reg} is not managed by \var{mplex},
 * -1 is returned.
 */
EXTL_EXPORT_MEMBER
int mplex_get_managed_index(WMPlex *mplex, WRegion *reg)
{
    WRegion *other;
    int index=0;
    
    FOR_ALL_MANAGED_ON_LIST(mplex->managed_list, other){
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
void mplex_inc_managed_index(WMPlex *mplex, WRegion *r)
{
    if(r==NULL)
        r=mplex_current(mplex);
    if(r!=NULL){
        mplex_set_managed_index(mplex, r,
                                mplex_get_managed_index(mplex, r)+1);
    }
}


/*EXTL_DOC
 * Move \var{r} ''right'' within objects managed by \var{mplex}.
 */
EXTL_EXPORT_MEMBER
void mplex_dec_managed_index(WMPlex *mplex, WRegion *r)
{
    if(r==NULL)
        r=mplex_current(mplex);
    if(r!=NULL){
        mplex_set_managed_index(mplex, r,
                                mplex_get_managed_index(mplex, r)-1);
    }
}


/*}}}*/


/*{{{ Resize and reparent */


static void reparent_or_fit(WMPlex *mplex, const WRectangle *geom,
                            WWindow *parent)
{
    bool wchg=(REGION_GEOM(mplex).w!=geom->w);
    bool hchg=(REGION_GEOM(mplex).h!=geom->h);
    bool move=(REGION_GEOM(mplex).x!=geom->x ||
               REGION_GEOM(mplex).y!=geom->y);
    
    if(parent!=NULL){
        region_detach_parent((WRegion*)mplex);
        XReparentWindow(ioncore_g.dpy, MPLEX_WIN(mplex), parent->win,
                        geom->x, geom->y);
        XResizeWindow(ioncore_g.dpy, MPLEX_WIN(mplex), geom->w, geom->h);
        region_attach_parent((WRegion*)mplex, (WRegion*)parent);
    }else{
        XMoveResizeWindow(ioncore_g.dpy, MPLEX_WIN(mplex),
                          geom->x, geom->y, geom->w, geom->h);
    }
    
    REGION_GEOM(mplex)=*geom;
    
    if(move && !wchg && !hchg)
        region_notify_subregions_move(&(mplex->win.region));
    else if(wchg || hchg)
        mplex_fit_managed(mplex);
    
    if(wchg || hchg)
        mplex_size_changed(mplex, wchg, hchg);
}


bool mplex_reparent(WMPlex *mplex, WWindow *parent, const WRectangle *geom)
{
    if(!region_same_rootwin((WRegion*)mplex, (WRegion*)parent))
        return FALSE;
    
    reparent_or_fit(mplex, geom, parent);
    return TRUE;
}


void mplex_fit(WMPlex *mplex, const WRectangle *geom)
{
    reparent_or_fit(mplex, geom, NULL);
}


void mplex_fit_managed(WMPlex *mplex)
{
    WRectangle geom;
    WRegion *sub;
    
    if(MPLEX_MGD_UNVIEWABLE(mplex))
        return;
    
    mplex_managed_geom(mplex, &geom);
    
    FOR_ALL_MANAGED_ON_LIST(mplex->managed_list, sub){
        region_fit(sub, &geom);
    }
    
    if(mplex->current_input!=NULL)
        region_fit(mplex->current_input, &geom);
}


static void mplex_request_managed_geom(WMPlex *mplex, WRegion *sub,
                                       int flags, const WRectangle *geom, 
                                       WRectangle *geomret)
{
    WRectangle mg;
    /* Just try to give it the maximum size */
    mplex_managed_geom(mplex, &mg);
    
    if(geomret!=NULL)
        *geomret=mg;
    
    if(!(flags&REGION_RQGEOM_TRYONLY))
        region_fit(sub, &mg);
}


/*}}}*/


/*{{{ Mapping */


void mplex_map(WMPlex *mplex)
{
    window_map((WWindow*)mplex);
    /* A lame requirement of the ICCCM is that client windows should be
     * unmapped if the parent is unmapped.
     */
    if(mplex->current_sub!=NULL && !MPLEX_MGD_UNVIEWABLE(mplex))
        region_map(mplex->current_sub);
}


void mplex_unmap(WMPlex *mplex)
{
    window_unmap((WWindow*)mplex);
    /* A lame requirement of the ICCCM is that client windows should be
     * unmapped if the parent is unmapped.
     */
    if(mplex->current_sub!=NULL)
        region_unmap(mplex->current_sub);
}


/*}}}*/


/*{{{ Focus  */


void mplex_do_set_focus(WMPlex *mplex, bool warp)
{
    if(!MPLEX_MGD_UNVIEWABLE(mplex) && mplex->current_input!=NULL){
        region_do_set_focus((WRegion*)mplex->current_input, FALSE);
    }else if(!MPLEX_MGD_UNVIEWABLE(mplex) && mplex->current_sub!=NULL){
        /* Allow workspaces to position cursor to their liking. */
        if(warp && OBJ_IS(mplex->current_sub, WGenWS)){
            region_do_set_focus(mplex->current_sub, TRUE);
            return;
        }else{
            region_do_set_focus(mplex->current_sub, FALSE);
        }
    }else{
        xwindow_do_set_focus(MPLEX_WIN(mplex));
    }

    if(warp)
        region_do_warp((WRegion*)mplex);
}
    

static WRegion *mplex_managed_focus(WMPlex *mplex, WRegion *reg)
{
    return mplex->current_input;
}


/*}}}*/


/*{{{ Managed region switching */


static bool mplex_do_display_managed(WMPlex *mplex, WRegion *sub)
{
    bool mapped;
    
    if(sub==mplex->current_sub || sub==mplex->current_input)
        return TRUE;

    if(REGION_IS_MAPPED(mplex) && !MPLEX_MGD_UNVIEWABLE(mplex))
        region_map(sub);
    else
        region_unmap(sub);
    
    if(mplex->current_sub!=NULL && REGION_IS_MAPPED(mplex))
        region_unmap(mplex->current_sub);

    mplex->current_sub=sub;
    
    /* Many programs will get upset if the visible, although only such,
     * client window is not the lowest window in the mplex. xprop/xwininfo
     * will return the information for the lowest window. 'netscape -remote'
     * will not work at all if there are no visible netscape windows.
     */
    region_lower(sub);

    if(region_may_control_focus((WRegion*)mplex))
        region_warp((WRegion*)mplex);
    
    return TRUE;
}


bool mplex_display_managed(WMPlex *mplex, WRegion *sub)
{
    if(mplex_do_display_managed(mplex, sub)){
        mplex_managed_changed(mplex, MPLEX_CHANGE_SWITCHONLY, TRUE, sub);
        return TRUE;
    }
    
    return FALSE;
}


/*EXTL_DOC
 * Returns the \var{n}:th object managed by \var{mplex}.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_nth_managed(WMPlex *mplex, uint n)
{
    WRegion *reg=REGION_FIRST_MANAGED(mplex->managed_list);
    
    while(n-->0 && reg!=NULL)
        reg=REGION_NEXT_MANAGED(mplex->managed_list, reg);
    
    return reg;
}


static void do_switch(WMPlex *mplex, WRegion *sub)
{
    if(sub!=NULL)
        region_display_sp(sub);
}


/*EXTL_DOC
 * Have \var{mplex} display the \var{n}:th object managed by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_nth(WMPlex *mplex, uint n)
{
    do_switch(mplex, mplex_nth_managed(mplex, n));
}


/*EXTL_DOC
 * Have \var{mplex} display next (wrt. currently selected) object managed 
 * by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_next(WMPlex *mplex)
{
    do_switch(mplex, REGION_NEXT_MANAGED_WRAP(mplex->managed_list, 
                                              mplex->current_sub));
}


/*EXTL_DOC
 * Have \var{mplex} display previous (wrt. currently selected) object
 * managed by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_prev(WMPlex *mplex)
{
    do_switch(mplex, REGION_PREV_MANAGED_WRAP(mplex->managed_list, 
                                              mplex->current_sub));
}


/*}}}*/


/*{{{ Attach */


typedef struct{
    bool switchto;
    int index;
} MPlexAttachParams;


static WRegion *mplex_do_attach(WMPlex *mplex, WRegionAttachHandler *fn,
                                void *fnparams, MPlexAttachParams *param)
{
    bool switchto;
    WRectangle geom;
    WRegion *reg;
    
    mplex_managed_geom(mplex, &geom);
    
    reg=fn((WWindow*)mplex, &geom, fnparams);
    
    if(reg==NULL)
        return NULL;
    
    link_at(mplex, reg, param->index);
    
    region_set_manager(reg, (WRegion*)mplex, NULL);
    
    mplex->managed_count++;
    
    if(mplex->managed_count==1 || param->switchto){
        mplex_do_display_managed(mplex, reg);
        mplex_managed_changed(mplex, MPLEX_CHANGE_ADD, TRUE, reg);
    }else{
        region_unmap(reg);
        mplex_managed_changed(mplex, MPLEX_CHANGE_ADD, FALSE, reg);
    }
    
    return reg;
}


bool mplex_attach_simple(WMPlex *mplex, WRegion *reg, bool switchto)
{
    MPlexAttachParams par;
    
    if(reg==(WRegion*)mplex)
        return FALSE;

    par.index=-1;
    par.switchto=switchto;
    
    return region__attach_reparent((WRegion*)mplex, reg,
                                   (WRegionDoAttachFn*)mplex_do_attach, 
                                   &par);
}


WRegion *mplex_attach_new_simple(WMPlex *mplex, WRegionSimpleCreateFn *fn,
                                 bool switchto)
{
    MPlexAttachParams par;
    
    par.index=-1;
    par.switchto=switchto;
    
    return region__attach_new((WRegion*)mplex, fn,
                              (WRegionDoAttachFn*)mplex_do_attach, 
                              &par);
}


static void get_params(ExtlTab tab, MPlexAttachParams *par)
{
    par->switchto=extl_table_is_bool_set(tab, "switchto");
    par->index=-1;
    extl_table_gets_i(tab, "index", &(par->index));
}


/*EXTL_DOC
 * Attach and reparent existing region \var{reg} to \var{mplex}.
 * The table \var{param} may contain the fields \var{index} and
 * \var{switchto} that are interpreted as for \fnref{WMPlex.attach_new}.
 */
EXTL_EXPORT_MEMBER
bool mplex_attach(WMPlex *mplex, WRegion *reg, ExtlTab param)
{
    MPlexAttachParams par;
    get_params(param, &par);

    if(reg==(WRegion*)mplex)
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
 *  \hline
 *  Field & Description \\
 *  \hline
 *  \var{type} & Class name (a string) of the object to be created. Mandatory. \\
 *  \var{name} & Name of the object to be created (a string). Optional. \\
 *  \var{switchto} & Should the region be switched to (boolean)? Optional. \\
 *  \var{index} & Index of the new region in \var{mplex}'s list of
 *   managed objects (integer, 0 = first). Optional. \\
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
        mplex_attach_simple(mplex, reg, FALSE);
}


static bool mplex_handle_drop(WMPlex *mplex, int x, int y,
                              WRegion *dropped)
{
    if(mplex->current_sub!=NULL &&
       HAS_DYN(mplex->current_sub, region_handle_drop)){
        int rx, ry;
        region_rootpos(mplex->current_sub, &rx, &ry);
        if(rectangle_contains(&REGION_GEOM(mplex->current_sub), x-rx, y-ry)){
            if(region_handle_drop(mplex->current_sub, x, y, dropped))
                return TRUE;
        }
    }
    return mplex_attach_simple(mplex, dropped, TRUE);
}


static bool mplex_manage_clientwin(WMPlex *mplex, WRegion *reg,
                                   const WManageParams *param)
{
    return mplex_attach_simple(mplex, reg, param->switchto);
}


/*}}}*/


/*{{{ Remove */


static void mplex_do_remove(WMPlex *mplex, WRegion *sub)
{
    WRegion *next=NULL;
    
    if(mplex->current_sub==sub){
        next=REGION_PREV_MANAGED(mplex->managed_list, sub);
        if(next==NULL)
            next=REGION_NEXT_MANAGED(mplex->managed_list, sub);
        mplex->current_sub=NULL;
    }
    
    region_unset_manager(sub, (WRegion*)mplex, &(mplex->managed_list));
    mplex->managed_count--;
    
    if(!OBJ_IS_BEING_DESTROYED(mplex)){
        bool sw=(next!=NULL || mplex->managed_count==0);
        if(next!=NULL)
            mplex_do_display_managed(mplex, next);
        mplex_managed_changed(mplex,  MPLEX_CHANGE_REMOVE, sw, sub);
    }
}


void mplex_remove_managed(WMPlex *mplex, WRegion *reg)
{
    if(mplex->current_input==reg){
        region_unset_manager(reg, (WRegion*)mplex, NULL);
        mplex->current_input=NULL;
        if(region_may_control_focus((WRegion*)mplex))
            region_set_focus((WRegion*)mplex);
    }else{
        mplex_do_remove(mplex, reg);
    }
}


static bool mplex_do_rescue_clientwins(WMPlex *mplex, WRegion *dst)
{
    bool ret1, ret2;
    
    ret1=region_do_rescue_managed_clientwins((WRegion*)mplex, dst,
                                             mplex->managed_list);
    ret2=region_do_rescue_child_clientwins((WRegion*)mplex, dst);
    
    return (ret1 && ret2);
}


/*}}}*/


/*{{{ Inputs */


/*EXTL_DOC
 * Returns the currently active ''input'' (query, message etc.) in
 * \var{mplex} or nil.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_current_input(WMPlex *mplex)
{
    return mplex->current_input;
}


WRegion *mplex_add_input(WMPlex *mplex, WRegionAttachHandler *fn, void *fnp)
{
    WRectangle geom;
    WRegion *sub;
    
    if(mplex->current_input!=NULL || MPLEX_MGD_UNVIEWABLE(mplex))
        return NULL;
    
    mplex_managed_geom(mplex, &geom);
    sub=fn((WWindow*)mplex, &geom, fnp);
    
    if(sub==NULL)
        return NULL;
    
    mplex->current_input=sub;
    region_set_manager(sub, (WRegion*)mplex, NULL);
    region_keep_on_top(sub);
    region_map(sub);
    
    if(region_may_control_focus((WRegion*)mplex))
        region_set_focus(sub);

    return sub;
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Return the object managed by and currenly displayed in \var{mplex}.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_current(WMPlex *mplex)
{
    return mplex->current_sub;
}

/*EXTL_DOC
 * Returns a list of regions managed by \var{mplex}.
 */
EXTL_EXPORT_MEMBER
ExtlTab mplex_managed_list(WMPlex *mplex)
{
    return managed_list_to_table(mplex->managed_list, NULL);
}


/*EXTL_DOC
 * Returns the number of regions managed/multiplexed by \var{mplex}.
 */
EXTL_EXPORT_MEMBER
int mplex_managed_count(WMPlex *mplex)
{
    return mplex->managed_count;
}


/*}}}*/


/*{{{ Dynfuns */


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


/*{{{ Dynfuntab and class info */


static DynFunTab mplex_dynfuntab[]={
    {region_fit, mplex_fit},
    {(DynFun*)region_reparent, (DynFun*)mplex_reparent},

    {region_do_set_focus, mplex_do_set_focus},
    {(DynFun*)region_control_managed_focus,
     (DynFun*)mplex_managed_focus},
    
    {region_remove_managed, mplex_remove_managed},
    {region_request_managed_geom, mplex_request_managed_geom},
    {(DynFun*)region_display_managed, (DynFun*)mplex_display_managed},

    {(DynFun*)region_handle_drop, (DynFun*)mplex_handle_drop},
    
    {region_map, mplex_map},
    {region_unmap, mplex_unmap},
    
    {(DynFun*)region_manage_clientwin,
     (DynFun*)mplex_manage_clientwin},
    
    {(DynFun*)region_current,
     (DynFun*)mplex_current},

    {(DynFun*)region_do_rescue_clientwins,
     (DynFun*)mplex_do_rescue_clientwins},
            
    END_DYNFUNTAB
};
                                       

IMPLCLASS(WMPlex, WWindow, mplex_deinit, mplex_dynfuntab);


/*}}}*/
