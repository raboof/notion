/*
 * ion/ioncore/pholder.c
 *
 * Copyright (c) Tuomo Valkonen 2005-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include "common.h"
#include "attach.h"
#include "pholder.h"
#include "focus.h"


bool pholder_init(WPHolder *ph)
{
    ph->redirect=NULL;
    return TRUE;
}


void pholder_deinit(WPHolder *ph)
{
    if(ph->redirect!=NULL)
        destroy_obj((Obj*)ph->redirect);
}


WRegion *pholder_do_attach(WPHolder *ph, int flags,
                           WRegionAttachData *data)

{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, pholder_do_attach, ph, (ph, flags, data));
    return ret;
}


static WRegion *add_fn_reparent(WWindow *par, const WFitParams *fp, 
                                WRegion *reg)
{
    if(!region_fitrep(reg, par, fp)){
        warn(TR("Unable to reparent."));
        return NULL;
    }
    region_detach_manager(reg);
    return reg;
}


WRegion *pholder_attach_(WPHolder *ph, int flags, WRegionAttachData *data)
{
    WPHolder *root=pholder_root(ph);
    
    /* Use the root, so that extra containers are not added from
     * stale chains.
     */
    
    return (root==NULL
            ? NULL
            : pholder_do_attach(root, flags, data));
}


bool pholder_attach(WPHolder *ph, int flags, WRegion *reg)
{
    WRegionAttachData data;
        
    data.type=REGION_ATTACH_REPARENT;
    data.u.reg=reg;
    
    return (pholder_attach_(ph, flags, &data)!=NULL);
}


bool pholder_attach_mcfgoto(WPHolder *ph, int flags, WRegion *reg)
{
    bool cf=region_may_control_focus(reg);
    
    if(!pholder_attach(ph, flags, reg))
        return FALSE;
        
    if(cf)
        region_goto(reg);
    
    return TRUE;
}


WRegion *pholder_do_target(WPHolder *ph)
{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, pholder_do_target, ph, (ph));
    return ret;
}


WRegion *pholder_target(WPHolder *ph)
{
    return (ph->redirect!=NULL
            ? pholder_target(ph->redirect)
            : pholder_do_target(ph));
}


static bool pholder_do_check_reparent_default(WPHolder *ph, WRegion *reg)
{
    WRegion *target=pholder_do_target(ph);
    
    if(target==NULL)
        return FALSE;
    else
        return region_attach_reparent_check(target, reg);
}


DYNFUN bool pholder_do_check_reparent(WPHolder *ph, WRegion *reg)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, pholder_do_check_reparent, ph, (ph, reg));
    return ret;
}


bool pholder_check_reparent(WPHolder *ph, WRegion *reg)
{
    if(ph->redirect!=NULL)
        return pholder_check_reparent(ph->redirect, reg);
    else
        return pholder_do_check_reparent(ph, reg);
}
    

bool pholder_do_goto(WPHolder *ph)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, pholder_do_goto, ph, (ph));
    return ret;
}


bool pholder_goto(WPHolder *ph)
{
    return (ph->redirect!=NULL
            ? pholder_goto(ph->redirect)
            : pholder_do_goto(ph));
}


WPHolder *pholder_do_root_default(WPHolder *ph)
{
    return ph;
}


WPHolder *pholder_do_root(WPHolder *ph)
{
    WPHolder *ret=NULL;
    CALL_DYN_RET(ret, WPHolder*, pholder_do_root, ph, (ph));
    return ret;
}


WPHolder *pholder_root(WPHolder *ph)
{
    return (ph->redirect!=NULL
            ? pholder_root(ph->redirect)
            : pholder_do_root(ph));
}


bool pholder_stale(WPHolder *ph)
{
    return (pholder_root(ph)!=ph);
}


bool pholder_redirect(WPHolder *ph, WRegion *old_target)
{
    WPHolder *ph2=region_get_rescue_pholder(old_target);
    
    if(ph2==NULL)
        return FALSE;
    
    if(ph->redirect!=NULL)
        destroy_obj((Obj*)ph->redirect);

    ph->redirect=ph2;
    
    return TRUE;
}


WPHolder *region_managed_get_pholder(WRegion *reg, WRegion *mgd)
{
    WPHolder *ret=NULL;
    CALL_DYN_RET(ret, WPHolder*, region_managed_get_pholder, 
                 reg, (reg, mgd));
    return ret;
}


WPHolder *region_get_rescue_pholder_for(WRegion *reg, WRegion *mgd)
{
    if(OBJ_IS_BEING_DESTROYED(reg) || reg->flags&REGION_CWINS_BEING_RESCUED){
        return FALSE;
    }else{
        WPHolder *ret=NULL;
    
        CALL_DYN_RET(ret, WPHolder*, region_get_rescue_pholder_for,
                     reg, (reg, mgd));
        return ret;
    }
}


WPHolder *region_get_rescue_pholder(WRegion *reg)
{
    WRegion *mgr;
    WPHolder *ph=NULL;

    while(1){
        mgr=region_manager_or_parent(reg);
        if(mgr==NULL)
            break;
        ph=region_get_rescue_pholder_for(mgr, reg);
        if(ph!=NULL)
            break;
        reg=mgr;
    }
    
    return ph;
}


WPHolder *pholder_either(WPHolder *a, WPHolder *b)
{
    return (a!=NULL ? a : b);
}


static DynFunTab pholder_dynfuntab[]={
    {(DynFun*)pholder_do_check_reparent, 
     (DynFun*)pholder_do_check_reparent_default},
     
    {(DynFun*)pholder_do_root, 
     (DynFun*)pholder_do_root_default},

    END_DYNFUNTAB
};


IMPLCLASS(WPHolder, Obj, pholder_deinit, pholder_dynfuntab);

