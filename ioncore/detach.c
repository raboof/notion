/*
 * ion/ioncore/detach.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include <libtu/setparam.h>
#include <libtu/minmax.h>

#include <ioncore/common.h>
#include <ioncore/mplex.h>
#include <ioncore/focus.h>
#include <ioncore/group.h>
#include <ioncore/group-ws.h>
#include <ioncore/framedpholder.h>
#include <ioncore/return.h>


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


bool ioncore_do_detach(WRegion *reg, WGroup *grp, WFrameMode framemode)
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
        WStacking *st=ioncore_find_stacking(reg);
        
        if(st!=NULL){
            ap.szplcy=st->szplcy;
            ap.szplcy_set=TRUE;
            
            ap.level=maxof(st->level, STACKING_LEVEL_NORMAL);
            ap.level_set=TRUE;
        }
        
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
    WMPlex *mplex=REGION_MANAGER_CHK(reg, WMPlex);
    
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


bool ioncore_detach(WRegion *reg, int sp)
{
    WPHolder *ph=region_get_return(reg);
    WFrameMode mode;
    WGroup *grp;
    bool set, nset;
    
    reg=region_group_if_bottom(reg);
    
    grp=find_group(check_mplex(reg, &mode));
    
    /* reg is only considered detached if there's no higher-level group
     * to attach to, thus causing 'toggle' to cycle.
     */
    set=(grp==NULL);
    nset=libtu_do_setparam(sp, set);
    
    if(!XOR(nset, set))
        return set;

    if(!set){
        return ioncore_do_detach(reg, grp, mode);
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
 * If \var{reg} is the 'bottom' of some group, the whole group is
 * detached. If \var{reg} is a \type{WWindow}, it is put into a 
 * frame.
 */
EXTL_EXPORT_AS(ioncore, detach)
bool ioncore_detach_extl(WRegion *reg, const char *how)
{
    if(how==NULL)
        how="set";
    
    return ioncore_detach(reg, libtu_string_to_setparam(how));
}


