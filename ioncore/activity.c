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


#include "common.h"
#include "global.h"
#include "region.h"
#include "activity.h"


bool region_activity(WRegion *reg)
{
    return (reg->flags&REGION_ACTIVITY || reg->mgd_activity!=0);
}


static void propagate_activity(WRegion *reg)
{
    WRegion *mgr=region_manager(reg);
    bool mgr_marked;
    
    if(mgr==NULL)
        return;
    
    mgr_marked=region_activity(mgr);
    mgr->mgd_activity++;
    region_managed_notify(mgr, reg);
    
    if(!mgr_marked)
        propagate_activity(mgr);
}


/*EXTL_DOC
 * Notify of activity in \var{reg}.
 */
EXTL_EXPORT_MEMBER
void region_notify_activity(WRegion *reg)
{
    if(reg->flags&REGION_ACTIVITY || REGION_IS_ACTIVE(reg))
        return;
    
    reg->flags|=REGION_ACTIVITY;
    
    if(reg->mgd_activity==0)
        propagate_activity(reg);
}


static void propagate_clear(WRegion *reg)
{
    WRegion *mgr=region_manager(reg);
    bool mgr_notify_always;
    
    if(mgr==NULL)
        return;
    
    mgr->mgd_activity--;
    region_managed_notify(mgr, reg);
    
    if(!region_activity(mgr))
        propagate_clear(mgr);
}

    
/*EXTL_DOC
 * Clear notification of activity in \var{reg}. If \var{reg} is a client window
 * and its urgency hint is set, \var{force} must be set to clear the state.
 */
EXTL_EXPORT_MEMBER
void region_clear_activity(WRegion *reg, bool force)
{
    if(!(reg->flags&REGION_ACTIVITY))
        return;
    
    if(!force && OBJ_IS(reg, WClientWin)){
        XWMHints *hints=XGetWMHints(ioncore_g.dpy, ((WClientWin*)reg)->win);
        if(hints!=NULL){
            if(hints->flags&XUrgencyHint){
                XFree(hints);
                return;
            }
            XFree(hints);
        }
    }
    
    reg->flags&=~REGION_ACTIVITY;
    
    if(reg->mgd_activity==0)
        propagate_clear(reg);
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

