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

#include <libtu/objp.h>
#include <libtu/minmax.h>

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
#include "stacking.h"
#include "extl.h"
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


/*{{{ Destroy/create mplex */


static bool mplex_do_init(WMPlex *mplex, WWindow *parent, Window win,
                          const WFitParams *fp, bool create)
{
    mplex->flags=0;
    mplex->l1_count=0;
    mplex->l1_list=NULL;
    mplex->l1_current=NULL;
    mplex->l2_count=0;
    mplex->l2_list=NULL;
    mplex->l2_current=NULL;
    
    if(create){
        if(!window_init_new((WWindow*)mplex, parent, fp))
            return FALSE;
    }else{
        if(!window_init((WWindow*)mplex, parent, win, fp))
            return FALSE;
    }
    
    mplex->win.region.flags|=REGION_BINDINGS_ARE_GRABBED;

    region_add_bindmap((WRegion*)mplex, ioncore_mplex_bindmap);

    return TRUE;
}


bool mplex_init(WMPlex *mplex, WWindow *parent, Window win,
                const WFitParams *fp)
{
    return mplex_do_init(mplex, parent, win, fp, FALSE);
}


bool mplex_init_new(WMPlex *mplex, WWindow *parent, const WFitParams *fp)
{
    return mplex_do_init(mplex, parent, None, fp, TRUE);
}
    

void mplex_deinit(WMPlex *mplex)
{
    WRegion *reg, *next;
    
    FOR_ALL_MANAGED_ON_LIST_W_NEXT(mplex->l1_list, reg, next){
        destroy_obj((Obj*)reg);
    }
    
    FOR_ALL_MANAGED_ON_LIST_W_NEXT(mplex->l2_list, reg, next){
        destroy_obj((Obj*)reg);
    }

    window_deinit((WWindow*)mplex);
}


/*}}}*/


/*{{{ Managed list management */


static bool on_list(WRegion *list, WRegion *reg)
{
    WRegion *reg2;

    FOR_ALL_MANAGED_ON_LIST(list, reg2){
        if(reg2==reg)
            return TRUE;
    }
    
    return FALSE;
}


static bool on_l1_list(WMPlex *mplex, WRegion *reg)
{
    return on_list(mplex->l1_list, reg);
}


static bool on_l2_list(WMPlex *mplex, WRegion *reg)
{
    return on_list(mplex->l2_list, reg);
}


static WRegion *nth_on_list(WRegion *list, uint n)
{
    WRegion *reg=REGION_FIRST_MANAGED(list);
    
    while(n-->0 && reg!=NULL)
        reg=REGION_NEXT_MANAGED(list, reg);
    
    return reg;
}


WRegion *mplex_current(WMPlex *mplex)
{
    return (mplex->l2_current!=NULL ? mplex->l2_current : mplex->l1_current);
}


/*EXTL_DOC
 * Return the managed object currently active within layer 1 of \var{mplex}.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_l1_current(WMPlex *mplex)
{
    return mplex->l1_current;
}


/*EXTL_DOC
 * Return the managed object currently active within layer 2 of \var{mplex}.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_l2_current(WMPlex *mplex)
{
    return mplex->l2_current;
}


/*EXTL_DOC
 * Returns the \var{n}:th object managed by \var{mplex} on the layer 1
 * list.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_l1_nth(WMPlex *mplex, uint n)
{
    return nth_on_list(mplex->l1_list, n);
}


/*EXTL_DOC
 * Returns the \var{n}:th object managed by \var{mplex} on the layer 1
 * list.
 */
EXTL_EXPORT_MEMBER
WRegion *mplex_l2_nth(WMPlex *mplex, uint n)
{
    return nth_on_list(mplex->l2_list, n);
}


/*EXTL_DOC
 * Returns a list of regions managed by \var{mplex} on layer 1.
 */
EXTL_EXPORT_MEMBER
ExtlTab mplex_l1_list(WMPlex *mplex)
{
    return managed_list_to_table(mplex->l1_list, NULL);
}


/*EXTL_DOC
 * Returns a list of regions managed by \var{mplex} on layer 2.
 */
EXTL_EXPORT_MEMBER
ExtlTab mplex_l2_list(WMPlex *mplex)
{
    return managed_list_to_table(mplex->l2_list, NULL);
}


/*EXTL_DOC
 * Returns the number of regions managed/multiplexed by \var{mplex}
 * on layer 1.
 */
EXTL_EXPORT_MEMBER
int mplex_l1_count(WMPlex *mplex)
{
    return mplex->l1_count;
}


/*EXTL_DOC
 * Returns the number of regions managed by \var{mplex} on layer 2.
 */
EXTL_EXPORT_MEMBER
int mplex_l2_count(WMPlex *mplex)
{
    return mplex->l2_count;
}


static void link_at(WMPlex *mplex, WRegion *reg, int index)
{
    WRegion *after=NULL;
    
    if(index>0){
        after=mplex_l1_nth(mplex, index-1);
    }else if(index<0){
        if(!(mplex->flags&MPLEX_ADD_TO_END) && ioncore_g.opmode!=IONCORE_OPMODE_INIT)
            after=mplex->l1_current;
    }

    if(after==reg)
        after=NULL;
    
    if(after!=NULL){
        LINK_ITEM_AFTER(mplex->l1_list, after, reg, mgr_next, mgr_prev);
    }else if(index==0){
        LINK_ITEM_FIRST(mplex->l1_list, reg, mgr_next, mgr_prev);
    }else{
        LINK_ITEM(mplex->l1_list, reg, mgr_next, mgr_prev);
    }
}


/*EXTL_DOC
 * Set index of \var{reg} within the multiplexer to \var{index}.
 */
EXTL_EXPORT_MEMBER
void mplex_set_index(WMPlex *mplex, WRegion *reg, int index)
{
    if(index<0 || reg==NULL)
        return;

    if(!on_l1_list(mplex, reg))
        return;

    UNLINK_ITEM(mplex->l1_list, reg, mgr_next, mgr_prev);
    link_at(mplex, reg, index);
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
    WRegion *other;
    int index=0;
    
    FOR_ALL_MANAGED_ON_LIST(mplex->l1_list, other){
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
        r=mplex_l1_current(mplex);
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
        r=mplex_l1_current(mplex);
    if(r!=NULL)
        mplex_set_index(mplex, r, mplex_get_index(mplex, r)-1);
}


/*}}}*/


/*{{{ Resize and reparent */


bool mplex_fitrep(WMPlex *mplex, WWindow *par, const WFitParams *fp)
{
    bool wchg=(REGION_GEOM(mplex).w!=fp->g.w);
    bool hchg=(REGION_GEOM(mplex).h!=fp->g.h);
    bool move=(REGION_GEOM(mplex).x!=fp->g.x ||
               REGION_GEOM(mplex).y!=fp->g.y);
    int w=maxof(1, fp->g.w);
    int h=maxof(1, fp->g.h);
    
    if(par!=NULL){
        if(!region_same_rootwin((WRegion*)mplex, (WRegion*)par))
            return FALSE;
        region_detach_parent((WRegion*)mplex);
        XReparentWindow(ioncore_g.dpy, MPLEX_WIN(mplex), par->win,
                        fp->g.x, fp->g.y);
        XResizeWindow(ioncore_g.dpy, MPLEX_WIN(mplex), w, h);
        region_attach_parent((WRegion*)mplex, (WRegion*)par);
    }else{
        XMoveResizeWindow(ioncore_g.dpy, MPLEX_WIN(mplex),
                          fp->g.x, fp->g.y, w, h);
    }
    
    REGION_GEOM(mplex)=fp->g;
    
    if(move && !wchg && !hchg)
        region_notify_subregions_move(&(mplex->win.region));
    else if(wchg || hchg)
        mplex_fit_managed(mplex);
    
    if(wchg || hchg)
        mplex_size_changed(mplex, wchg, hchg);
    
    return TRUE;
}


void mplex_fit_managed(WMPlex *mplex)
{
    WRectangle geom;
    WRegion *sub;
    WFitParams fp;
    
    if(MPLEX_MGD_UNVIEWABLE(mplex))
        return;
    
    mplex_managed_geom(mplex, &(fp.g));
    
    fp.mode=REGION_FIT_EXACT;
    FOR_ALL_MANAGED_ON_LIST(mplex->l1_list, sub){
        region_fitrep(sub, NULL, &fp);
    }

    fp.mode=REGION_FIT_BOUNDS;
    FOR_ALL_MANAGED_ON_LIST(mplex->l2_list, sub){
        region_fitrep(sub, NULL, &fp);
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


/*{{{ Mapping */


void mplex_map(WMPlex *mplex)
{
    window_map((WWindow*)mplex);
    /* A lame requirement of the ICCCM is that client windows should be
     * unmapped if the parent is unmapped.
     */
    if(!MPLEX_MGD_UNVIEWABLE(mplex)){
        if(mplex->l1_current!=NULL)
            region_map(mplex->l1_current);
        /*if(mplex->l2_current!=NULL)
            region_map(mplex->l2_current);*/
    }
}


void mplex_unmap(WMPlex *mplex)
{
    window_unmap((WWindow*)mplex);
    /* A lame requirement of the ICCCM is that client windows should be
     * unmapped if the parent is unmapped.
     */
    if(!MPLEX_MGD_UNVIEWABLE(mplex)){
        if(mplex->l1_current!=NULL)
            region_unmap(mplex->l1_current);
        /*if(mplex->l2_current!=NULL)
         region_unmap(mplex->l2_current);*/
    }
}


/*}}}*/


/*{{{ Focus  */


void mplex_do_set_focus(WMPlex *mplex, bool warp)
{
    if(!MPLEX_MGD_UNVIEWABLE(mplex) && mplex->l2_current!=NULL){
        region_do_set_focus((WRegion*)mplex->l2_current, FALSE);
    }else if(!MPLEX_MGD_UNVIEWABLE(mplex) && mplex->l1_current!=NULL){
        /* Allow workspaces to position cursor to their liking. */
        if(warp && OBJ_IS(mplex->l1_current, WGenWS)){
            region_do_set_focus(mplex->l1_current, TRUE);
            return;
        }else{
            region_do_set_focus(mplex->l1_current, FALSE);
        }
    }else{
        xwindow_do_set_focus(MPLEX_WIN(mplex));
    }

    if(warp)
        region_do_warp((WRegion*)mplex);
}
    

static WRegion *mplex_managed_control_focus(WMPlex *mplex, WRegion *reg)
{
    return mplex->l2_current;
}


/*}}}*/


/*{{{ Managed region switching */


/*EXTL_DOC
 * If var \var{reg} is on the l2 list of \var{mplex} and currently shown, 
 * hide it. if \var{reg} is nil, hide all objects on the l2 list.
 */
EXTL_EXPORT_MEMBER
bool mplex_l2_hide(WMPlex *mplex, WRegion *reg)
{
    WRegion *reg2;
    WRegion *toact=NULL;
    bool changes=FALSE;
    bool fixcurrent=FALSE;
    bool mcf=region_may_control_focus((WRegion*)mplex);
    
    FOR_ALL_MANAGED_ON_LIST(mplex->l2_list, reg2){
        if(!REGION_IS_MAPPED(reg2))
            continue;
        if(reg==NULL || reg==reg2){
            if(reg2==mplex->l2_current){
                fixcurrent=TRUE;
                mplex->l2_current=NULL;
            }
            changes=TRUE;
            region_unmap(reg2);
        }else{
            toact=reg2;
        }
    }
    
    if(fixcurrent){
        if(toact!=NULL){
            mplex->l2_current=toact;
            if(mcf)
                mplex_managed_display(mplex, toact);
        }else if(mcf){
            region_set_focus((WRegion*)mplex);
        }
    }
    
    return changes;
}


/*EXTL_DOC
 * If var \var{reg} is on the l2 list of \var{mplex} and currently hidden, 
 * display it. if \var{reg} is nil, display all objects on the l2 list.
 */
EXTL_EXPORT_MEMBER
bool mplex_l2_show(WMPlex *mplex, WRegion *reg)
{
    WRegion *reg2;
    WRegion *toact=NULL;
    bool fixcurrent=TRUE;
    bool changes=FALSE;
    bool mcf=region_may_control_focus((WRegion*)mplex);
    
    FOR_ALL_MANAGED_ON_LIST(mplex->l2_list, reg2){
        if(REGION_IS_MAPPED(reg2)){
            if(reg2==mplex->l2_current)
                fixcurrent=FALSE;
            continue;
        }
        fixcurrent=TRUE;
        if(reg==NULL || reg==reg2){
            toact=reg2;
            changes=TRUE;
            region_map(reg2);
        }
    }
    
    if(fixcurrent && toact!=NULL){
        mplex->l2_current=toact;
        if(mcf)
            region_set_focus(toact);
    }
    
    return changes;
}


static bool mplex_do_managed_display(WMPlex *mplex, WRegion *sub)
{
    bool mapped;
    bool l2=FALSE;
    
    if(sub==mplex->l1_current || sub==mplex->l2_current)
        return TRUE;
    
    if(on_l2_list(mplex, sub))
        l2=TRUE;
    else if(!on_l1_list(mplex, sub))
        return FALSE;
    
    if(REGION_IS_MAPPED(mplex) && !MPLEX_MGD_UNVIEWABLE(mplex))
        region_map(sub);
    else
        region_unmap(sub);
    
    if(!l2){
        if(mplex->l1_current!=NULL && REGION_IS_MAPPED(mplex))
            region_unmap(mplex->l1_current);

        mplex->l1_current=sub;
        
        /* Many programs will get upset if the visible, although only 
         * such, client window is not the lowest window in the mplex. 
         * xprop/xwininfo will return the information for the lowest 
         * window. 'netscape -remote' will not work at all if there are
         * no visible netscape windows.
         */
        region_lower(sub);
    }else{
        UNLINK_ITEM(mplex->l2_list, sub, mgr_next, mgr_prev);
        region_raise(sub);
        LINK_ITEM(mplex->l2_list, sub, mgr_next, mgr_prev);
        mplex->l2_current=sub;
    }
    
    if(region_may_control_focus((WRegion*)mplex))
        region_warp((WRegion*)mplex);
    
    return TRUE;
}


bool mplex_managed_display(WMPlex *mplex, WRegion *sub)
{
    if(mplex_do_managed_display(mplex, sub)){
        mplex_managed_changed(mplex, MPLEX_CHANGE_SWITCHONLY, TRUE, sub);
        return TRUE;
    }
    
    return FALSE;
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
    do_switch(mplex, mplex_l1_nth(mplex, n));
}


/*EXTL_DOC
 * Have \var{mplex} display next (wrt. currently selected) object managed 
 * by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_next(WMPlex *mplex)
{
    do_switch(mplex, REGION_NEXT_MANAGED_WRAP(mplex->l1_list, 
                                              mplex->l1_current));
}


/*EXTL_DOC
 * Have \var{mplex} display previous (wrt. currently selected) object
 * managed by it.
 */
EXTL_EXPORT_MEMBER
void mplex_switch_prev(WMPlex *mplex)
{
    do_switch(mplex, REGION_PREV_MANAGED_WRAP(mplex->l1_list, 
                                              mplex->l1_current));
}


/*}}}*/


/*{{{ Attach */


typedef struct{
    int flags;
    int index;
} MPlexAttachParams;


static WRegion *mplex_do_attach(WMPlex *mplex, WRegionAttachHandler *hnd,
                                void *hnd_param, MPlexAttachParams *param)
{
    WRegion *reg;
    WFitParams fp;
    bool sw=param->flags&MPLEX_ATTACH_SWITCHTO;
    bool l2=param->flags&MPLEX_ATTACH_L2;
    
    mplex_managed_geom(mplex, &(fp.g));
    fp.mode=(l2 ? REGION_FIT_BOUNDS : REGION_FIT_EXACT);
    
    reg=hnd((WWindow*)mplex, &fp, hnd_param);
    
    if(reg==NULL)
        return NULL;
    
    if(l2){
        LINK_ITEM(mplex->l2_list, reg, mgr_next, mgr_prev);
        mplex->l2_count++;
    }else{
        link_at(mplex, reg, param->index);
        mplex->l1_count++;
    }
    
    region_set_manager(reg, (WRegion*)mplex, NULL);
    
    if(l2)
        region_keep_on_top(reg);

    if(sw || (!l2 && mplex->l1_count==1)){
        mplex_do_managed_display(mplex, reg);
        mplex_managed_changed(mplex, MPLEX_CHANGE_ADD, TRUE, reg);
    }else{
        region_unmap(reg);
        mplex_managed_changed(mplex, MPLEX_CHANGE_ADD, FALSE, reg);
    }
    
    return reg;
}


WRegion *mplex_attach_simple(WMPlex *mplex, WRegion *reg, int flags)
{
    MPlexAttachParams par;
    
    if(reg==(WRegion*)mplex)
        return FALSE;

    par.index=-1;
    par.flags=flags;
    
    return region__attach_reparent((WRegion*)mplex, reg,
                                   (WRegionDoAttachFn*)mplex_do_attach, 
                                   &par);
}


WRegion *mplex_attach_hnd(WMPlex *mplex, WRegionAttachHandler *hnd,
                          void *hnd_param, int flags)
{
    MPlexAttachParams par;
    
    par.index=-1;
    par.flags=flags;
    
    return mplex_do_attach(mplex, hnd, hnd_param, &par);
}


static void get_params(ExtlTab tab, MPlexAttachParams *par)
{
    int layer=1;
    
    par->flags=0;
    par->index=-1;
    
    extl_table_gets_i(tab, "layer", &layer);
    if(layer==2)
       par->flags|=MPLEX_ATTACH_L2;

    if(extl_table_is_bool_set(tab, "switchto"))
        par->flags|=MPLEX_ATTACH_SWITCHTO;
    
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
 *  \hline
 *  Field & Description \\
 *  \hline
 *  \var{type} & Class name (a string) of the object to be created. Mandatory. \\
 *  \var{name} & Name of the object to be created (a string). Optional. \\
 *  \var{switchto} & Should the region be switched to (boolean)? Optional. \\
 *  \var{index} & Index of the new region in \var{mplex}'s list of
 *   managed objects (integer, 0 = first). Optional. \\
 *  \var{layer} & Layer to attach on; 1 (default) or 2.
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
    WRegion *curr=mplex_l1_current(mplex);

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
            if(region_manage_clientwin(mplex->l2_current, cwin, param,
                                       MANAGE_REDIR_PREFER_YES))
                return TRUE;
        }
        if(mplex->l1_current!=NULL){
            if(region_manage_clientwin(mplex->l1_current, cwin, param,
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
    WRegion *next=NULL;
    bool l2=FALSE;
    bool sw=FALSE;
    
    if(mplex->l1_current==sub){
        next=REGION_PREV_MANAGED(mplex->l1_list, sub);
        if(next==NULL)
            next=REGION_NEXT_MANAGED(mplex->l1_list, sub);
        mplex->l1_current=NULL;
        sw=TRUE;
    }else if(mplex->l2_current==sub){
        WRegion *next2;
        l2=TRUE;
        mplex->l2_current=NULL;
        next=mplex->l1_current;
        FOR_ALL_MANAGED_ON_LIST(mplex->l2_list, next2){
            if(REGION_IS_MAPPED(next2) && next2!=sub){
                mplex->l2_current=next2;
                next=NULL;
            }
        }
    }else{
        l2=on_l2_list(mplex, sub);
    }
    
    if(l2){
        region_unset_manager(sub, (WRegion*)mplex, &(mplex->l2_list));
        mplex->l2_count--;
    }else{
        region_unset_manager(sub, (WRegion*)mplex, &(mplex->l1_list));
        mplex->l1_count--;
    }
    
    if(OBJ_IS_BEING_DESTROYED(mplex))
        return;
    
    if(next!=NULL && sw)
        mplex_do_managed_display(mplex, next);
    else if(l2 && region_may_control_focus((WRegion*)mplex))
        region_set_focus((WRegion*)mplex);
    
    mplex_managed_changed(mplex, MPLEX_CHANGE_REMOVE, sw, sub);

}


bool mplex_rescue_clientwins(WMPlex *mplex)
{
    bool ret1, ret2, ret3;
    
    ret1=region_rescue_managed_clientwins((WRegion*)mplex, mplex->l1_list);
    ret2=region_rescue_managed_clientwins((WRegion*)mplex, mplex->l2_list);
    ret3=region_rescue_child_clientwins((WRegion*)mplex);
    
    return (ret1 && ret2 && ret3);
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


/*{{{ Save/load */


ExtlTab mplex_get_base_configuration(WMPlex *mplex)
{
    WRegion *sub=NULL;
    int n=0;
    ExtlTab tab, subs;
    
    tab=region_get_base_configuration((WRegion*)mplex);
    
    subs=extl_create_table();
    extl_table_sets_t(tab, "subs", subs);
    
    FOR_ALL_MANAGED_ON_LIST(mplex->l1_list, sub){
        ExtlTab st=region_get_configuration(sub);
        if(st!=extl_table_none()){
            if(sub==mplex->l1_current)
                extl_table_sets_b(st, "switchto", TRUE);
            extl_table_seti_t(subs, ++n, st);
            extl_unref_table(st);
        }
    }

    FOR_ALL_MANAGED_ON_LIST(mplex->l2_list, sub){
        ExtlTab st=region_get_configuration(sub);
        if(st!=extl_table_none()){
            extl_table_sets_i(st, "layer", 2);
            if(REGION_IS_MAPPED(sub))
                extl_table_sets_b(st, "switchto", TRUE);
            extl_table_seti_t(subs, ++n, st);
            extl_unref_table(st);
        }
    }
    
    extl_unref_table(subs);
    
    return tab;
}


void mplex_load_contents(WMPlex *mplex, ExtlTab tab)
{
    ExtlTab substab, subtab;
    int n, i;
    
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


/*}}}*/


/*{{{ Dynfuntab and class info */


static DynFunTab mplex_dynfuntab[]={
    {region_do_set_focus, mplex_do_set_focus},
    {(DynFun*)region_managed_control_focus,
     (DynFun*)mplex_managed_control_focus},
    
    {region_managed_remove, mplex_managed_remove},
    {region_managed_rqgeom, mplex_managed_rqgeom},
    {(DynFun*)region_managed_display, (DynFun*)mplex_managed_display},

    {(DynFun*)region_handle_drop, (DynFun*)mplex_handle_drop},
    
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

    {(DynFun*)region_fitrep,
     (DynFun*)mplex_fitrep},
            
    END_DYNFUNTAB
};
                                       

IMPLCLASS(WMPlex, WWindow, mplex_deinit, mplex_dynfuntab);


/*}}}*/
