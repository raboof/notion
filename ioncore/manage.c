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


/*{{{ Static helper functions */


static bool ws_ok(WRegion *r)
{
	return (r!=NULL && WOBJ_IS(r, WGenWS) && HAS_DYN(r, genws_add_clientwin));
}
	
	
static WGenWS *find_suitable_workspace(WScreen *scr)
{
	WRegion *r=(WRegion*)scr;
	WGenWS *ws=NULL;
	
	while(r!=NULL){
		if(ws_ok(r)){
			ws=(WGenWS*)r;
			break; /* Can't continue */
		}else if(WOBJ_IS(r, WMPlex)){
			if(ws_ok(((WMPlex*)r)->current_sub))
				ws=(WGenWS*)((WMPlex*)r)->current_sub;
		}
		r=region_active_sub(r);
	}

	return ws;
	
	/*
	if(ws!=NULL)
		return ws;

	if(ws==NULL){
		FOR_ALL_MANAGED_ON_LIST(scr->mplex.managed_list, r){
			if(ws_ok(r))
				return (WGenWS*)r;
		}
	}

	return NULL;
	*/
}


static WScreen *find_suitable_screen(WClientWin *cwin, 
									 const WAttachParams *param)
{
	WScreen *scr=NULL, *found=NULL;
	bool respectpos=(cwin->transient_for!=None ||
					 cwin->size_hints.flags&USPosition);

	FOR_ALL_SCREENS(scr){
		if(!same_rootwin((WRegion*)scr, (WRegion*)cwin))
		   continue;
		if(REGION_IS_ACTIVE(scr)){
			found=scr;
			if(!respectpos)
				break;
		}
		
		if(coords_in_rect(&REGION_GEOM(scr), 
						  param->geomrq.x, param->geomrq.y)){
			found=scr;
			if(respectpos)
				break;
		}
	}
	
	return found;
}


/*}}}*/


/*{{{ Add */


bool add_clientwin_default(WClientWin *cwin, const WAttachParams *param)
{
	WGenWS *ws=NULL;
	WScreen *scr=NULL;
	int tm;

	/* Transients are managed by their transient_for client window 
	 * (unless overridden before call to this function).
	 */
	if(param->flags&REGION_ATTACH_TFOR){
		/* The flag should only be set if transient_mode is ...NORMAL. */
		if(finish_add_clientwin((WRegion*)param->tfor, cwin, param))
			return TRUE;
	}
	
	/* check full screen mode */
	if(param->flags&REGION_ATTACH_POSRQ){ /* flag should always be set */
		if(clientwin_check_fullscreen_request(cwin, param->geomrq.w,
											  param->geomrq.h)){
			/*if(clientwin_get_switchto(cwin))*/
			if(param->flags&REGION_ATTACH_SWITCHTO)
				region_goto((WRegion*)cwin);
			return TRUE;
		}
	}

	/* Need to find a workspace and let it handle this. */
	scr=find_suitable_screen(cwin, param);
	if(scr==NULL){
		warn("Unable to find a screen for a new client window.");
		return FALSE;
	}
	
	ws=find_suitable_workspace(scr);
	
	if(ws!=NULL){
		if(genws_add_clientwin(ws, cwin, param))
			return TRUE;
	}
	
	/* All else failed, try full screen mode */
	return clientwin_fullscreen_scr(cwin, scr, FALSE);
}


bool genws_add_clientwin(WGenWS *reg, WClientWin *cwin,
						 const WAttachParams *param)
{
	bool ret=FALSE;
	CALL_DYN_RET(ret, bool, genws_add_clientwin, reg, (reg, cwin, param));
	return ret;
	
}


/*}}}*/


/*{{{ finish_add_clientwin */


bool do_add_clientwin(WRegion *reg, WClientWin *cwin,
					  const WAttachParams *param)
{
	if(HAS_DYN(reg, genws_add_clientwin)){
		return genws_add_clientwin((WGenWS*)reg, cwin, param);
	}else{
		/* Can't use region_x_window(reg)==None because WFloatWS:s have
		 * a dummy window.
		 */
		if(!WOBJ_IS(reg, WWindow) && !WOBJ_IS(reg, WClientWin)){
			warn("Attaching a WClientWin to a non-WWindow non-WClientWin "
				 "WRegion with region_add_managed... "
				 "probably not a good idea.");
		}
		return region_add_managed(reg, (WRegion*)cwin, param);
	}
}


bool finish_add_clientwin(WRegion *reg, WClientWin *cwin,
						  const WAttachParams *param)
{
	WAttachParams param2;

	param2.flags=0;
	
	assert(reg!=NULL);
	
	if(!same_rootwin(reg, (WRegion*)cwin)){
		warn("Trying to add client window to a region not on same screen.");
		return FALSE;
	}
		
	/*if(clientwin_get_switchto(cwin))*/
	if(param->flags&REGION_ATTACH_SWITCHTO)
		param2.flags|=REGION_ATTACH_SWITCHTO;
	
	if(param->flags&REGION_ATTACH_SIZERQ){
		param2.flags|=REGION_ATTACH_SIZERQ;
		param2.geomrq.w=param->geomrq.w;
		param2.geomrq.h=param->geomrq.h;
	}
	
	if(param->flags&REGION_ATTACH_INITSTATE &&
	   param->init_state==IconicState)
		param2.flags=0;
	
	if(do_add_clientwin(reg, cwin, &param2)){
		/* region_add_managed should only change focus if the region may
		 * control the focus.
		 */
		/*if(param->flags&REGION_ATTACH_SWITCHTO)
			region_goto((WRegion*)cwin);*/
		return TRUE;
	}
	return FALSE;
}


/*}}}*/

