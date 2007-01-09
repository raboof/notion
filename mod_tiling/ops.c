/*
 * ion/mod_tiling/ops.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include <libtu/setparam.h>

#include <ioncore/common.h>
#include <ioncore/mplex.h>
#include <ioncore/focus.h>
#include <ioncore/group.h>
#include <ioncore/group-ws.h>
#include <ioncore/framedpholder.h>
#include <ioncore/return.h>
#include "tiling.h"



static void get_relative_geom(WRectangle *g, WRegion *reg, WRegion *mgr)
{
    WWindow *rel=REGION_PARENT(mgr), *w;
    
    *g=REGION_GEOM(reg);
    
    for(w=REGION_PARENT(reg); 
        w!=rel && (WRegion*)w!=mgr; 
        w=REGION_PARENT(w)){
        
        g->x+=REGION_GEOM(w).x;
        g->y+=REGION_GEOM(w).y;
    }
}


bool mod_tiling_do_detach(WRegion *reg, WGroup *grp, WFrameMode framemode)
{
    WGroupAttachParams ap=GROUPATTACHPARAMS_INIT;
    WRegionAttachData data;
    WPHolder *ph;
    bool newph=FALSE;
    bool ret;
    
    ap.switchto_set=TRUE;
    ap.switchto=region_may_control_focus(reg);
    
    data.type=REGION_ATTACH_REPARENT;
    data.u.reg=reg;
    
    ph=region_unset_get_return(reg);
    
    if(ph==NULL){
        ph=region_make_return_pholder(reg);
        newph=TRUE;
    }
    
    if(framemode!=FRAME_MODE_UNKNOWN){
        WFramedParam fpa=FRAMEDPARAM_INIT;
        
        fpa.mode=framemode;
        fpa.inner_geom_gravity_set=TRUE;
        fpa.gravity=ForgetGravity;
        
        ap.geom_weak_set=TRUE;
        ap.geom_weak=0;
        
        get_relative_geom(&fpa.inner_geom, reg, (WRegion*)grp);

        ret=(region_attach_framed((WRegion*)grp, &fpa,
                                  (WRegionAttachFn*)group_do_attach,
                                  &ap, &data)!=NULL);
    }else{
        ap.geom_set=TRUE;
        get_relative_geom(&ap.geom, reg, (WRegion*)grp);
        
        ret=(group_do_attach(grp, &ap, &data)!=NULL);
    }
    
    if(!ret && newph)
        destroy_obj((Obj*)ph);
    else if(!region_do_set_return(reg, ph))
        destroy_obj((Obj*)ph);
    
    return ret;
}


static WRegion *check_mplex(WRegion *reg, WFrameMode *mode)
{
    WMPlex *mplex=OBJ_CAST(REGION_MANAGER(reg), WMPlex);
    
    if(OBJ_IS(reg, WWindow) || mplex==NULL){
        *mode=FRAME_MODE_UNKNOWN;
        return reg;
    }
        
    *mode=FRAME_MODE_FLOATING;
    
    if(OBJ_IS(mplex, WFrame)
       && frame_mode((WFrame*)mplex)==FRAME_MODE_TRANSIENT){
        *mode=FRAME_MODE_TRANSIENT;
    }
    
    return (WRegion*)mplex;
}


static WGroup *find_group(WRegion *reg)
{
    WRegion *mgr=REGION_MANAGER(reg);
    
    while(mgr!=NULL){
        mgr=REGION_MANAGER(mgr);
        if(OBJ_IS(mgr, WGroup))
            break;
    }
    
    return (WGroup*)mgr;
}



bool mod_tiling_detach(WRegion *reg, int sp)
{
    WPHolder *ph=region_get_return(reg);
    WFrameMode mode;
    WGroup *grp;
    bool set, nset;
    
    grp=find_group(check_mplex(reg, &mode));
    
    /* reg is only considered detached if there's no higher-level group
     * to attach to, thus causing 'toggle' to cycle.
     */
    set=(grp==NULL);
    nset=libtu_do_setparam(sp, set);
    
    if(!XOR(nset, set))
        return set;

    if(!set){
        return mod_tiling_do_detach(reg, grp, mode);
    }else{
        WPHolder *ph=region_get_return(reg);
        
        if(ph!=NULL){
            if(!pholder_attach_mcfgoto(ph, PHOLDER_ATTACH_SWITCHTO, reg)){
                warn(TR("Failed to reattach."));    
                return TRUE;
            }
            region_unset_return(reg);
        }
        
        return FALSE;
    }
}


/*EXTL_DOC
 * Detach or reattach \var{reg}, depending on whether \var{how}
 * is 'set'/'unset'/'toggle'. (Detaching means making \var{reg} 
 * managed by its nearest ancestor \type{WGroup}, framed if \var{reg} is
 * not itself \type{WFrame}. Reattaching means making it managed where
 * it used to be managed, if a return-placeholder exists.)
 */
EXTL_EXPORT_AS(mod_tiling, detach)
bool mod_tiling_detach_extl(WRegion *reg, const char *how)
{
    if(how==NULL)
        how="set";
    
    return mod_tiling_detach(reg, libtu_string_to_setparam(how));
}


static WRegion *mkbottom_fn(WWindow *parent, const WFitParams *fp, 
                            void *param)
{
    WRegion *reg=(WRegion*)param;
    WTiling *tiling;
    WSplitRegion *node=NULL;
    
    if(!region_fitrep(reg, parent, fp))
        return NULL;
    
    tiling=create_tiling(parent, fp, NULL, FALSE);
    
    if(tiling==NULL)
        return NULL;
    
    node=create_splitregion(&REGION_GEOM(tiling), reg);
    if(node!=NULL){
        tiling->split_tree=(WSplit*)node;
        tiling->split_tree->ws_if_root=tiling;
        
        region_detach_manager(reg);
        
        if(tiling_managed_add(tiling, reg))
            return (WRegion*)tiling;
        
        #warning "TODO: reattach?"
        
        destroy_obj((Obj*)tiling->split_tree);
        tiling->split_tree=NULL;
    }
    
    destroy_obj((Obj*)tiling);
    return NULL;
}


/*EXTL_DOC
 * Create a new \type{WTiling} 'bottom' for the group of \var{reg},
 * consisting of \var{reg}.
 */
EXTL_EXPORT
bool mod_tiling_mkbottom(WRegion *reg)
{
    WGroup *grp=REGION_MANAGER_CHK(reg, WGroup);
    WGroupAttachParams ap=GROUPATTACHPARAMS_INIT;
    WRegionAttachData data;
    WRegion *tiling;
    
    if(grp==NULL){
        warn(TR("Not member of a group"));
        return FALSE;
    }
    
    if(grp->bottom!=NULL){
        warn(TR("Manager group already has bottom"));
        return FALSE;
    }
    
    ap.level_set=TRUE;
    ap.level=STACKING_LEVEL_BOTTOM;
    
    ap.szplcy_set=TRUE;
    ap.szplcy=SIZEPOLICY_FULL_EXACT;
    
    ap.switchto_set=TRUE;
    ap.switchto=region_may_control_focus(reg);
    
    ap.bottom=TRUE;

    data.type=REGION_ATTACH_NEW;
    data.u.n.fn=mkbottom_fn;
    data.u.n.param=reg;
    
    /* kele... poisto samalla kuin attach */
    return (group_do_attach(grp, &ap, &data)!=NULL);
}
