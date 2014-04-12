/*
 * ion/ioncore/pholder.c
 *
 * Copyright (c) Tuomo Valkonen 2005-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <libtu/objp.h>
#include "common.h"
#include "attach.h"
#include "pholder.h"
#include "focus.h"
#include "utildefines.h"


bool pholder_init(WPHolder *UNUSED(ph))
{
    return TRUE;
}


void pholder_deinit(WPHolder *UNUSED(ph))
{
}


WRegion *pholder_do_attach(WPHolder *ph, int flags,
                           WRegionAttachData *data)

{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, pholder_do_attach, ph, (ph, flags, data));
    return ret;
}


bool pholder_attach(WPHolder *ph, int flags, WRegion *reg)
{
    WRegionAttachData data;
        
    data.type=REGION_ATTACH_REPARENT;
    data.u.reg=reg;
    
    return (pholder_do_attach(ph, flags, &data)!=NULL);
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
    return pholder_do_target(ph);
}


static bool pholder_do_check_reparent_default(WPHolder *ph, WRegion *reg)
{
    WRegion *target=pholder_do_target(ph);
    
    return (target==NULL 
            ? FALSE
            : region_ancestor_check(target, reg));
}


DYNFUN bool pholder_do_check_reparent(WPHolder *ph, WRegion *reg)
{
    bool ret=FALSE;
    CALL_DYN_RET(ret, bool, pholder_do_check_reparent, ph, (ph, reg));
    return ret;
}


bool pholder_check_reparent(WPHolder *ph, WRegion *reg)
{
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
    return pholder_do_goto(ph);
}


bool pholder_stale_default(WPHolder *ph)
{
    return (pholder_target(ph)==NULL);
}


bool pholder_stale(WPHolder *ph)
{
    bool ret=TRUE;
    CALL_DYN_RET(ret, bool, pholder_stale, ph, (ph));
    return ret;
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
     
    {(DynFun*)pholder_stale, 
     (DynFun*)pholder_stale_default},

    END_DYNFUNTAB
};


IMPLCLASS(WPHolder, Obj, pholder_deinit, pholder_dynfuntab);

