/*
 * ion/ioncore/pholder.c
 *
 * Copyright (c) Tuomo Valkonen 2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include "common.h"
#include "pholder.h"


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


bool pholder_do_attach(WPHolder *ph, 
                       WRegionAttachHandler *hnd, void *hnd_param)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, pholder_do_attach, ph, (ph, hnd, hnd_param));
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


bool pholder_attach(WPHolder *ph, WRegion *reg)
{
    if(ph->redirect!=NULL){
        return pholder_attach(ph->redirect, reg);
    }else{
        return pholder_do_attach(ph, (WRegionAttachHandler*)add_fn_reparent, 
                                 reg);
    }
}


bool pholder_do_goto(WPHolder *ph)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, pholder_do_goto, ph, (ph));
    return ret;
}


bool pholder_goto(WPHolder *ph)
{
    if(ph->redirect!=NULL)
        return pholder_goto(ph->redirect);
    else
        return pholder_do_goto(ph);
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


IMPLCLASS(WPHolder, Obj, pholder_deinit, NULL);

