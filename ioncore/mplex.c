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
#include <libtu/rb.h>

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
#include "extl.h"
#include "extlconv.h"
#include "genws.h"
#include "frame-pointer.h"
#include "bindmaps.h"
#include "regbind.h"
#include "region-iter.h"
#include "saveload.h"
#include "xwindow.h"
#include "defer.h"


#define MPLEX_WIN(MPLEX) ((MPLEX)->win.win)
#define MPLEX_MGD_UNVIEWABLE(MPLEX) \
            ((MPLEX)->flags&MPLEX_MANAGED_UNVIEWABLE)


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
    
    XSelectInput(ioncore_g.dpy, MPLEX_WIN(mplex), IONCORE_EVENTMASK_FRAME);
    
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


/*{{{ Hidden L2 objects RB-tree */ 

#define MGD_L2_HIDDEN 0x0001
#define MGD_L2_PASSIVE 0x0002

static Rb_node mgd_flag_rb=NULL;


static bool mgd_set_flags(WRegion *reg, int flag)
{
    Rb_node nd;
    
    if(mgd_flag_rb==NULL){
        mgd_flag_rb=make_rb();
        if(mgd_flag_rb==NULL)
            return FALSE;
    }else{
        int found=0;
        nd=rb_find_pkey_n(mgd_flag_rb, reg, &found);
        if(found){
            nd->v.ival|=flag;
            return TRUE;
        }
    }
    
    nd=rb_insertp(mgd_flag_rb, reg, NULL);
    if(nd!=NULL)
        nd->v.ival=flag;
    
    return (nd!=NULL);
}


static void mgd_unset_flags(WRegion *reg, int flag)
{
    int found=0;
    Rb_node nd;
    
    if(mgd_flag_rb==NULL)
        return;
    
    nd=rb_find_pkey_n(mgd_flag_rb, reg, &found);
    if(!found)
        return;
    
    nd->v.ival&=~flag;
    
    if(nd->v.ival==0)
        rb_delete_node(nd);
}


static int mgd_flags(WRegion *reg)
{
    int found=0;
    Rb_node nd;
    
    if(mgd_flag_rb==NULL)
        return 0;
    
    nd=rb_find_pkey_n(mgd_flag_rb, reg, &found);
    
    return (found ? nd->v.ival : 0);
}


static bool l2_is_hidden(WRegion *reg)
{
    return mgd_flags(reg)&MGD_L2_HIDDEN;
}


static bool l2_mark_hidden(WRegion *reg)
{
    return mgd_set_flags(reg, MGD_L2_HIDDEN);
}


static void l2_unmark_hidden(WRegion *reg)
{
    mgd_unset_flags(reg, MGD_L2_HIDDEN);
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
            ? mplex->l1_current
            : (l==2
               ? mplex->l2_current
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
            ? nth_on_list(mplex->l1_list, n)
            : (l==2
               ? nth_on_list(mplex->l2_list, n)
               : 0));
}


/*EXTL_DOC
 * Returns a list of regions managed by \var{mplex} on layer \var{l}.
 */
EXTL_EXPORT_MEMBER
ExtlTab mplex_llist(WMPlex *mplex, uint l)
{
    return (l==1 
            ? managed_list_to_table(mplex->l1_list, NULL)
            : (l==2
               ? managed_list_to_table(mplex->l2_list, NULL)
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


static void link_at(WMPlex *mplex, WRegion *reg, int index)
{
    WRegion *after=NULL;
    
    if(index>0){
        after=mplex_lnth(mplex, 1, index-1);
    }else if(index<0){
        if(!(mplex->flags&MPLEX_ADD_TO_END) && 
           ioncore_g.opmode!=IONCORE_OPMODE_INIT){
            after=mplex->l1_current;
        }
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
    WRegion *reg;
    
    if(mplex->l1_current!=NULL)
        region_map(mplex->l1_current);
    
    FOR_ALL_MANAGED_ON_LIST(mplex->l2_list, reg){
        if(!l2_is_hidden(reg))
            region_map(reg);
    }
}


static void mplex_unmap_mgd(WMPlex *mplex)
{
    WRegion *reg;
    
    if(mplex->l1_current!=NULL)
        region_unmap(mplex->l1_current);
    
    FOR_ALL_MANAGED_ON_LIST(mplex->l2_list, reg){
        region_unmap(reg);
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
        FOR_ALL_MANAGED_ON_LIST(mplex->l1_list, sub){
            region_fitrep(sub, NULL, &fp);
        }
        
        fp.mode=REGION_FIT_BOUNDS;
        FOR_ALL_MANAGED_ON_LIST(mplex->l2_list, sub){
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
    
    if(!MPLEX_MGD_UNVIEWABLE(mplex)){
        if(mplex->l2_current!=NULL){
            region_do_set_focus((WRegion*)mplex->l2_current, warp);
            return;
        }else if(mplex->l1_current!=NULL){
            /* Allow workspaces to position cursor to their liking. */
            if(warp && OBJ_IS(mplex->l1_current, WGenWS)){
                region_do_set_focus(mplex->l1_current, TRUE);
                return;
            }else{
                region_do_set_focus(mplex->l1_current, FALSE);
                focset=TRUE;
            }
        }
    }
    
    if(!focset)
        xwindow_do_set_focus(MPLEX_WIN(mplex));
    
    if(warp)
        region_do_warp((WRegion*)mplex);
}


static WRegion *mplex_managed_control_focus(WMPlex *mplex, WRegion *reg)
{
    if(mplex->l2_current!=NULL && 
       (mgd_flags(mplex->l2_current)&MGD_L2_PASSIVE)==0){
        return mplex->l2_current;
    }
    return NULL;
}


/*}}}*/


/*{{{ Managed region switching */


void mplex_managed_activated(WMPlex *mplex, WRegion *reg)
{
    if(mplex->l2_current!=NULL && mplex->l2_current!=reg &&
       mgd_flags(mplex->l2_current)&MGD_L2_PASSIVE){
        mplex->l2_current=NULL;
    }else if(on_l2_list(mplex, reg)){
        mplex->l2_current=reg;
    }
}


/*EXTL_DOC
 * Is \var{reg} on the layer2 of \var{mplex}, but hidden?
 */
EXTL_EXPORT_MEMBER
bool mplex_l2_hidden(WMPlex *mplex, WRegion *reg)
{
    return (REGION_MANAGER(reg)==(WRegion*)mplex
            && l2_is_hidden(reg));
}


/*EXTL_DOC
 * If var \var{reg} is on the l2 list of \var{mplex} and currently shown, 
 * hide it. if \var{reg} is nil, hide all objects on the l2 list.
 */
EXTL_EXPORT_MEMBER
bool mplex_l2_hide(WMPlex *mplex, WRegion *reg)
{
    WRegion *reg2, *toact=NULL;
    bool mcf=region_may_control_focus((WRegion*)mplex);
    
    if(REGION_MANAGER(reg)!=(WRegion*)mplex)
        return FALSE;
    
    if(l2_is_hidden(reg))
        return FALSE;
    
    if(!l2_mark_hidden(reg))
        return FALSE;
    
    if(REGION_IS_MAPPED(mplex) && !MPLEX_MGD_UNVIEWABLE(mplex))
        region_unmap(reg);
    
    if(mplex->l2_current==reg){
        mplex->l2_current=NULL;
        FOR_ALL_MANAGED_ON_LIST(mplex->l2_list, reg2){
            /*if(!l2_is_hidden(reg2))*/
            if((mgd_flags(reg2)&(MGD_L2_HIDDEN|MGD_L2_PASSIVE))==0)
                mplex->l2_current=reg2;
        }
    }
    
    if(mcf)
        region_warp((WRegion*)mplex);
    
    return TRUE;
}


/*EXTL_DOC
 * If var \var{reg} is on the l2 list of \var{mplex} and currently hidden, 
 * display it. if \var{reg} is nil, display all objects on the l2 list.
 */
EXTL_EXPORT_MEMBER
bool mplex_l2_show(WMPlex *mplex, WRegion *reg)
{
    WRegion *reg2, *toact=NULL;
    bool mcf=region_may_control_focus((WRegion*)mplex);

    if(REGION_MANAGER(reg)!=(WRegion*)mplex)
        return FALSE;
    
    if(!l2_is_hidden(reg))
        return FALSE;
    
#if 1
    return mplex_managed_display(mplex, reg);
#else    
    l2_unmark_hidden(reg);
    if(REGION_IS_MAPPED(mplex) && !MPLEX_MGD_UNVIEWABLE(mplex))
        region_map(reg);
    
    FOR_ALL_MANAGED_ON_LIST(mplex->l2_list, reg2){
        /*if(!l2_is_hidden(reg2))*/
        if((mgd_flags(reg2)&(MGD_L2_HIDDEN|MGD_L2_PASSIVE))==0)
            toact=reg2;
    }
    
    mplex->l2_current=toact;
    
    if(mcf)
        region_warp((WRegion*)mplex);

    return TRUE;
#endif    
}


static bool mplex_do_managed_display(WMPlex *mplex, WRegion *sub,
                                     bool call_changed)
{
    bool l2=FALSE;
    WRegion *stdisp;
    
    if(sub==mplex->l1_current || sub==mplex->l2_current)
        return TRUE;
    
    if(on_l2_list(mplex, sub))
        l2=TRUE;
    else if(!on_l1_list(mplex, sub))
        return FALSE;
    
    stdisp=(WRegion*)(mplex->stdispinfo.regwatch.obj);
    
    if(!l2 && OBJ_IS(sub, WGenWS)){
        if(stdisp!=NULL){
            WRegion *mgr=REGION_MANAGER(stdisp);
            if(mgr!=sub){
                if(OBJ_IS(mgr, WGenWS)){
                    genws_unmanage_stdisp((WGenWS*)mgr, FALSE, TRUE);
                    region_detach_manager(stdisp);
                }
            
                genws_manage_stdisp((WGenWS*)sub, stdisp, 
                                    mplex->stdispinfo.pos);
            }
        }else{
            genws_unmanage_stdisp((WGenWS*)sub, TRUE, TRUE);
        }
    }
    
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
        if(OBJ_IS(sub, WClientWin))
            region_lower(sub);
        
        /* This call should be unnecessary... */
        mplex_managed_activated(mplex, sub);
    }else{
        UNLINK_ITEM(mplex->l2_list, sub, mgr_next, mgr_prev);
        region_raise(sub);
        LINK_ITEM(mplex->l2_list, sub, mgr_next, mgr_prev);
        if(l2_is_hidden(sub))
            l2_unmark_hidden(sub);
        mplex->l2_current=sub;
    }
    
    if(region_may_control_focus((WRegion*)mplex))
        region_warp((WRegion*)mplex);

    if(!l2)
        mplex_managed_changed(mplex, MPLEX_CHANGE_SWITCHONLY, TRUE, sub);
    
    return TRUE;
}


bool mplex_managed_display(WMPlex *mplex, WRegion *sub)
{
    return mplex_do_managed_display(mplex, sub, TRUE);
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
    do_switch(mplex, mplex_lnth(mplex, 1, n));
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
    
    if(l2){
        if(!sw)
            sw=!l2_mark_hidden(reg);
        if(param->flags&MPLEX_ATTACH_L2_PASSIVE)
            mgd_set_flags(reg, MGD_L2_PASSIVE);
    }
    
    if(!l2 && mplex->l1_count==1)
        sw=TRUE;
    
    if(sw)
        mplex_do_managed_display(mplex, reg, FALSE);
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
 *  \hline
 *  Field & Description \\
 *  \hline
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
    WRegion *stdisp=(WRegion*)(mplex->stdispinfo.regwatch.obj);
    
    if(OBJ_IS(sub, WGenWS) && stdisp!=NULL && REGION_MANAGER(stdisp)==sub){
        genws_unmanage_stdisp((WGenWS*)sub, TRUE, TRUE);
        region_detach_manager(stdisp);
    }
    
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
        FOR_ALL_MANAGED_ON_LIST(mplex->l2_list, next2){
            if(next2!=sub &&
               (mgd_flags(next2)&(MGD_L2_HIDDEN|MGD_L2_PASSIVE))==0){
                mplex->l2_current=next2;
            }
        }
    }else{
        l2=on_l2_list(mplex, sub);
    }
    
    mgd_unset_flags(sub, ~0);
    
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
        mplex_do_managed_display(mplex, next, FALSE);
    else if(l2 && region_may_control_focus((WRegion*)mplex))
        region_warp((WRegion*)mplex);
    
    if(!l2)
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
           (REGION_MANAGER(reg)==NULL && REGION_PARENT(reg)==(WRegion*)mplex));
    
    if(oldstdisp!=NULL){
        mgr=OBJ_CAST(REGION_MANAGER(oldstdisp), WGenWS);
        
        if(oldstdisp!=reg){
            ioncore_defer_destroy(di->regwatch.obj);
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
        
        if(mplex->l1_current!=NULL && OBJ_IS(mplex->l1_current, WGenWS) &&
           mgr!=(WGenWS*)(mplex->l1_current)){
            if(mgr!=NULL){
                genws_unmanage_stdisp(mgr, FALSE, TRUE);
                region_detach_manager(oldstdisp);
            }
            mgr=(WGenWS*)(mplex->l1_current);
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
 * \begin{tabularx}{\linewidth}{lX}
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


/*{{{ Save/load */


ExtlTab mplex_get_configuration(WMPlex *mplex)
{
    WRegion *sub=NULL;
    int n=0;
    ExtlTab tab, subs, stdisptab;
    
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
            int flags=mgd_flags(sub);
            extl_table_sets_i(st, "layer", 2);
            extl_table_sets_b(st, "switchto", !(flags&MGD_L2_HIDDEN));
            extl_table_sets_b(st, "passive", flags&MGD_L2_PASSIVE);
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
    
    {(DynFun*)region_managed_control_focus,
     (DynFun*)mplex_managed_control_focus},
    
    {region_managed_remove, 
     mplex_managed_remove},
    
    {region_managed_rqgeom,
     mplex_managed_rqgeom},
    
    {(DynFun*)region_managed_display,
     (DynFun*)mplex_managed_display},
    
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

