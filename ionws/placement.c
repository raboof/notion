/*
 * ion/ionws/placement.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/clientwin.h>
#include <ioncore/attach.h>
#include <ioncore/manage.h>
#include <ioncore/extl.h>
#include <ioncore/framep.h>
#include <ioncore/names.h>
#include <ioncore/region-iter.h>
#include "placement.h"
#include "ionframe.h"
#include "splitframe.h"
#include "ionws.h"


#define REG_OK(R) region_has_manage_clientwin(R)


static WRegion *find_suitable_target(WIonWS *ws)
{
	WRegion *r=ionws_current(ws);
	
	if(r!=NULL && REG_OK(r))
		return r;
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, r){
		if(REG_OK(r))
			return r;
	}
	
	return NULL;
}


static bool try_current(WIonWS *ws, WClientWin *cwin)
{
	WRegion *target;

	target=find_suitable_target(ws);
	
	if(target==NULL || !OBJ_IS(target, WFrame))
		return FALSE;
		
	target=FRAME_CURRENT(target);
	
	if(target==NULL || !OBJ_IS(target, WClientWin))
		return FALSE;
	
	
	return clientwin_attach_transient((WClientWin*)target, (WRegion*)cwin);
}


bool ionws_manage_clientwin(WIonWS *ws, WClientWin *cwin,
							const WManageParams *param)
{
	WRegion *target=NULL;
	
	if(clientwin_get_transient_mode(cwin)==TRANSIENT_MODE_CURRENT){
		if(try_current(ws, cwin))
			return TRUE;
	}
		
	extl_call_named("ionws_placement_method", "oob", "o", 
					ws, cwin, param->userpos, &target);
	
	if(target!=NULL){
		/* Only allow regions managed by us so the scripts shouldn't be able
		 * to cause loops and so on.
		 */
		if(!REG_OK(target) || REGION_MANAGER(target)!=(WRegion*)ws)
			target=NULL;
	}

	if(target==NULL)
		target=find_suitable_target(ws);
	
	if(target==NULL){
		warn("Ooops... could not find a region to attach client window to "
			 "on workspace %s.", region_name((WRegion*)ws));
		return FALSE;
	}
	
	return region_manage_clientwin(target, cwin, param);
}

