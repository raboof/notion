/*
 * ion/ioncore/activity.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */


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
	region_notify_managed_change(mgr, reg);
	
	if(!mgr_marked)
		propagate_activity(mgr);
}


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
	region_notify_managed_change(mgr, reg);
	
	if(!region_activity(mgr))
		propagate_clear(mgr);
}

	
void region_clear_activity(WRegion *reg)
{
	if(!(reg->flags&REGION_ACTIVITY))
		return;
	
	reg->flags&=~REGION_ACTIVITY;
	
	if(reg->mgd_activity==0)
		propagate_clear(reg);
}
