/*
 * ion/ioncore/activity.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/setparam.h>
#include <libtu/minmax.h>
#include <libtu/objlist.h>
#include "common.h"
#include "global.h"
#include "region.h"
#include "activity.h"


WHook *region_activity_hook=NULL;

static ObjList *actlist=NULL;


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
    
    mgr->mgd_activity=maxof(0, mgr->mgd_activity-1);
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
        objlist_insert_last(&actlist, (Obj*)reg);
        
        if(reg->mgd_activity==0)
            propagate_activity(reg);
    }else{
        reg->flags&=~REGION_ACTIVITY;
        objlist_remove(&actlist, (Obj*)reg);
        
        if(reg->mgd_activity==0)
            propagate_clear(reg);
    }
    
    extl_protect(NULL);
    hook_call_o(region_activity_hook, (Obj*)reg);
    extl_unprotect(NULL);

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


/*EXTL_DOC
 * Return list of regions with activity/urgency bit set.
 */
EXTL_SAFE
EXTL_EXPORT
ExtlTab ioncore_activity_list()
{
    ExtlTab t=extl_create_table();
    ObjListIterTmp tmp;
    Obj *obj;
    int i=1;
    
    FOR_ALL_ON_OBJLIST(Obj*, obj, actlist, tmp){
        extl_table_seti_o(t, i, obj);
    }
    
    return t;
}


/*EXTL_DOC
 * Return first regio non activity list.
 */
EXTL_SAFE
EXTL_EXPORT
WRegion *ioncore_activity_first()
{
    if(actlist==NULL)
        return NULL;
    return (WRegion*)actlist->watch.obj;
}


/*EXTL_DOC
 * Go to first region on activity list.
 */
EXTL_EXPORT
bool ioncore_goto_activity()
{
    WRegion *reg=ioncore_activity_first();
    
    if(reg!=NULL)
        return region_goto(reg);
    else
        return FALSE;
}

