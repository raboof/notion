/*
 * ion/ioncore/activity.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/setparam.h>
#include <libtu/minmax.h>
#include "common.h"
#include "global.h"
#include "region.h"
#include "activity.h"


static void propagate_activity(WRegion *reg)
{
    WRegion *mgr=region_manager(reg);
    bool mgr_marked;
    
    if(mgr==NULL)
        return;
    
    mgr_marked=region_is_activity_r(mgr);
    mgr->mgd_activity++;
    region_managed_notify(mgr, reg);
    
    if(!mgr_marked)
        propagate_activity(mgr);
}


static void propagate_clear(WRegion *reg)
{
    WRegion *mgr=region_manager(reg);
    bool mgr_notify_always;
    
    if(mgr==NULL)
        return;
    
    mgr->mgd_activity=minof(0, mgr->mgd_activity-1);
    region_managed_notify(mgr, reg);
    
    if(!region_is_activity_r(mgr))
        propagate_clear(mgr);
}


bool region_set_activity(WRegion *reg, int sp)
{
    bool set=(reg->flags&REGION_ACTIVITY);
    bool nset=libtu_do_setparam(sp, set);
    
    if(!XOR(set, nset))
        return nset;

    if(nset){
        if(REGION_IS_ACTIVE(reg))
            return FALSE;
    
        reg->flags|=REGION_ACTIVITY;
        
        if(reg->mgd_activity==0)
            propagate_activity(reg);
    }else{
        reg->flags&=~REGION_ACTIVITY;
        if(reg->mgd_activity==0)
            propagate_clear(reg);
    }
    
    return nset;
}


/*EXTL_DOC
 * Set activity flag of \var{reg}. The \var{how} parameter most be
 * one of (set/unset/toggle).
 */
EXTL_EXPORT_AS(WRegion, set_activity)
bool region_set_activity_extl(WRegion *reg, const char *how)
{
    return region_set_activity(reg, libtu_string_to_setparam(how));
}


/*EXTL_DOC
 * Is activity notification set on \var{reg}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool region_is_activity(WRegion *reg)
{
    return (reg->flags&REGION_ACTIVITY);
}


bool region_is_activity_r(WRegion *reg)
{
    return (reg->flags&REGION_ACTIVITY || reg->mgd_activity!=0);
}
