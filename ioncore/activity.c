/*
 * ion/ioncore/activity.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
 */

#include <libtu/setparam.h>
#include <libtu/minmax.h>
#include <libtu/objlist.h>
#include "common.h"
#include "global.h"
#include "region.h"
#include "activity.h"


static ObjList *actlist=NULL;


void region_mark_mgd_activity(WRegion *mgr)
{
    bool mgr_marked;
    
    if(mgr==NULL)
        return;
    
    mgr_marked=region_is_activity_r(mgr);
    mgr->mgd_activity++;
    
    if(!mgr_marked){
        region_notify_change(mgr, ioncore_g.notifies.sub_activity);
        region_mark_mgd_activity(REGION_MANAGER(mgr));
    }
}


void region_clear_mgd_activity(WRegion *mgr)
{
    if(mgr==NULL)
        return;
    
    mgr->mgd_activity=maxof(0, mgr->mgd_activity-1);
    
    if(!region_is_activity_r(mgr)){
        region_notify_change(mgr, ioncore_g.notifies.sub_activity);
        region_clear_mgd_activity(REGION_MANAGER(mgr));
    }
}
    
    
static void propagate_activity(WRegion *reg)
{
    region_mark_mgd_activity(REGION_MANAGER(reg));
}


static void propagate_clear(WRegion *reg)
{
    region_clear_mgd_activity(REGION_MANAGER(reg));
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
    
    region_notify_change(reg, ioncore_g.notifies.activity);
    
    return nset;
}


/*EXTL_DOC
 * Set activity flag of \var{reg}. The \var{how} parameter must be
 * one of \codestr{set}, \codestr{unset} or \codestr{toggle}.
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
 * Iterate over activity list until \var{iterfn} returns \code{false}.
 * The function is called in protected mode.
 * This routine returns \code{true} if it reaches the end of list
 * without this happening.
 */
EXTL_SAFE
EXTL_EXPORT
bool ioncore_activity_i(ExtlFn iterfn)
{
    return extl_iter_objlist(iterfn, actlist);
}


/*EXTL_DOC
 * Returns first region on activity list.
 */
EXTL_SAFE
EXTL_EXPORT
WRegion *ioncore_activity_first()
{
    if(actlist==NULL)
        return NULL;
    return (WRegion*)actlist->watch.obj;
}


ObjList *ioncore_activity_list()
{
    return actlist;
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

