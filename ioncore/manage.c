/*
 * ion/ioncore/manage.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "global.h"
#include "common.h"
#include "region.h"
#include "attach.h"
#include "manage.h"
#include "objp.h"
#include "names.h"
#include "fullscreen.h"
#include "pointer.h"
#include "extl.h"


/*{{{ Add */


static WScreen *find_suitable_screen(WClientWin *cwin, 
									 const WManageParams *param)
{
	WScreen *scr=NULL, *found=NULL;
	bool respectpos=(param->tfor!=NULL || param->userpos);
	
	FOR_ALL_SCREENS(scr){
		if(!same_rootwin((WRegion*)scr, (WRegion*)cwin))
			continue;
		if(REGION_IS_ACTIVE(scr)){
			found=scr;
			if(!respectpos)
				break;
		}
		
		if(coords_in_rect(&REGION_GEOM(scr), param->geom.x, param->geom.y)){
			found=scr;
			if(respectpos)
				break;
		}
		
		if(found==NULL)
			found=scr;
	}
	
	return found;
}


bool add_clientwin_default(WClientWin *cwin, const WManageParams *param)
{
	WRegion *r=NULL;
	WScreen *scr=NULL;
	bool triedws=FALSE;
	
	/* Transients are managed by their transient_for client window unless the
	 * behaviour is overridden before this function.
	 */
	if(param->tfor!=NULL){
		if(clientwin_attach_transient(param->tfor, (WRegion*)cwin))
			return TRUE;
	}
	
	/* check full screen mode */
	if(clientwin_check_fullscreen_request(cwin, param->geom.w, param->geom.h)){
		if(param->switchto)
			region_goto((WRegion*)cwin);
		return TRUE;
	}

	/* Find a suitable screen */
	scr=find_suitable_screen(cwin, param);
	if(scr==NULL){
		warn("Unable to find a screen for a new client window.");
		return FALSE;
	}
	
	/* Starting from the innermost active object, find first proper
	 * workspace and see if it can manage cwin. If not, try all regions
	 * between the ws and the screen that can manage client windows
	 * falling back on the screen if all else fails.
	 */

	 r=(WRegion*)scr;
	
	while(r->active_sub!=NULL)
		r=r->active_sub;
	
	while(r!=(WRegion*)scr && r!=NULL){
		if(region_has_manage_clientwin(r) && (triedws || WOBJ_IS(r, WGenWS))){
			if(region_manage_clientwin(r, cwin, param))
				return TRUE;
		}
		
		r=region_manager_or_parent(r);
	}
	
	return region_manage_clientwin((WRegion*)scr, cwin, param);
}



/*}}}*/


/*{{{ region_manage_clientwin */


bool region_manage_clientwin(WRegion *reg, WClientWin *cwin,
							 const WManageParams *param)
{
	bool ret=FALSE;
	CALL_DYN_RET(ret, bool, region_manage_clientwin, reg, (reg, cwin, param));
	return ret;
}


bool region_has_manage_clientwin(WRegion *reg)
{
	return HAS_DYN(reg, region_manage_clientwin);
}


/*}}}*/

