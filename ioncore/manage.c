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
#include "netwm.h"


/*{{{ Add */


WScreen *find_suitable_screen(WClientWin *cwin, const WManageParams *param)
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


/* Try the deepest WGenWS first and then anything before it on the path
 * given by region_current() calls starting from chosen screen.
 */
static bool try_manage(WRegion *reg, WClientWin *cwin,
					   const WManageParams *param, bool *triedws)
{
	WRegion *r2=region_current(reg);
	
	if(r2!=NULL){
		if(try_manage(r2, cwin, param, triedws))
			return TRUE;
	}

	if(WOBJ_IS(reg, WGenWS))
		*triedws=TRUE;
	
	if(!*triedws)
		return FALSE;
	
	return region_manage_clientwin(reg, cwin, param);
}


bool add_clientwin_default(WClientWin *cwin, const WManageParams *param)
{
	WRegion *r=NULL, *r2;
	WScreen *scr=NULL;
	bool triedws=FALSE;
	int fs;
	
	/* Transients are managed by their transient_for client window unless the
	 * behaviour is overridden before this function.
	 */
	if(param->tfor!=NULL){
		if(clientwin_attach_transient(param->tfor, (WRegion*)cwin))
			return TRUE;
	}
	
	/* Check fullscreen mode */
	
	fs=netwm_check_initial_fullscreen(cwin, param->switchto);
	
	if(fs<0){
		fs=clientwin_check_fullscreen_request(cwin, 
											  param->geom.w,
											  param->geom.h,
											  param->switchto);
	}

	if(fs>0)
		return TRUE;

	/* Find a suitable screen */
	scr=find_suitable_screen(cwin, param);
	if(scr==NULL){
		warn("Unable to find a screen for a new client window.");
		return FALSE;
	}

	if(try_manage((WRegion*)scr, cwin, param, &triedws))
		return TRUE;
	
	if(clientwin_get_transient_mode(cwin)==TRANSIENT_MODE_CURRENT){
		WRegion *r=mplex_current((WMPlex*)scr);
		if(r!=NULL && WOBJ_IS(r, WClientWin)){
			if(clientwin_attach_transient((WClientWin*)r, (WRegion*)cwin)){
				return TRUE;
			}
		}
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

