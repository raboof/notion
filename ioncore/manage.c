/*
 * ion/ioncore/manage.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "global.h"
#include "common.h"
#include "region.h"
#include "attach.h"
#include "property.h"
#include "targetid.h"
#include "manage.h"
#include "mwmhints.h"
#include "objp.h"
#include "names.h"
#include "extl.h"


/* This file contains the add_clientwin_default handler for managing
 * clientwins. It should be called whenever no other handler knows
 * a special way to handle the window. This will then check if there
 * are any properties set indicating where to place the window and if
 * not, pass on the trouble to a suitable workspace (the current).
 */


/*{{{ Static helper functions */

static bool ws_ok(WRegion *r)
{
	return (r!=NULL && WOBJ_IS(r, WGenWS) && HAS_DYN(r, genws_add_clientwin));
}
	
	
static WGenWS *find_suitable_workspace(WViewport *vp)
{
	WRegion *r=vp->current_ws;
	
	if(ws_ok(r))
		return (WGenWS*)r;

	FOR_ALL_MANAGED_ON_LIST(vp->ws_list, r){
		if(ws_ok(r))
			return (WGenWS*)r;
	}
	
	return NULL;
}


static void find_prop_target(WClientWin *cwin, WRegion **target,
							 WGenWS **ws)
{
	WRegion *r;
	char *target_name;
	
	if(!extl_table_gets_s(cwin->proptab, "target", &target_name))
		return;
	
	/* Potential problem: numbering */
	r=lookup_region(target_name);
	
	free(target_name);
	
	if(r!=NULL && SCREEN_OF(r)==SCREEN_OF(cwin)){
		if(ws!=NULL && ws_ok(r))
			*ws=(WGenWS*)r;
		else if(target!=NULL && HAS_DYN(r, region_add_managed))
			*target=r;
	}
}


static WViewport *find_suitable_viewport(WClientWin *cwin, 
										 const WAttachParams *param)
{
	WScreen *scr=SCREEN_OF(cwin);
	WViewport *vp;

	if(param->flags&REGION_ATTACH_GEOMRQ){
		FOR_ALL_TYPED_CHILDREN(scr, vp, WViewport){
			WRectangle geom=REGION_GEOM(vp);
			if(param->geomrq.x>=geom.x && param->geomrq.x<geom.x+geom.w &&
			   param->geomrq.y>=geom.y && param->geomrq.y<geom.y+geom.h)
				return vp;
		}
	}
	
	if(scr->current_viewport!=NULL)
		return scr->current_viewport;
	
	return scr->default_viewport;
}


/*}}}*/


/*{{{ Add */


bool add_clientwin_default(WClientWin *cwin, WAttachParams *param)
{
	int target_id=0;
	WRegion *target=NULL;
	WGenWS *ws=NULL;
	WViewport *vp=NULL;
	int tm;
	
	/* check full screen mode */
	
	if(param->flags&REGION_ATTACH_GEOMRQ){ /* flag should always be set */
		if(clientwin_check_fullscreen_request(cwin, param->geomrq.w,
											  param->geomrq.h))
			return TRUE;

	}

	/* Check transient mode */
	
	tm=clientwin_get_transient_mode(cwin);
	
	if(tm!=TRANSIENT_MODE_CURRENT){
		if(tm==TRANSIENT_MODE_NORMAL){
			/* Is it a transient to some other window? */
			if(XGetTransientForHint(wglobal.dpy, cwin->win,
									&(cwin->transient_for))){
				param->tfor=find_clientwin(cwin->transient_for);
				if(param->tfor==cwin){
					param->tfor=NULL;
					cwin->transient_for=None;
				}else{
					param->flags|=REGION_ATTACH_TFOR;
				}
			}
		}
	}
	
	/* Check target id property and target winprop for non-transients */
	
	if(!(param->flags&REGION_ATTACH_TFOR)){
		get_integer_property(cwin->win, wglobal.atom_frame_id, &target_id);
		
		if(target_id!=0)
			target=find_target_by_id(target_id);
		
		find_prop_target(cwin, target==NULL ? &target : NULL, &ws);
		
		if(target!=NULL){
			if(!region_supports_add_managed(target)){
				warn("Target region of window %#x does not support primitive "
					 "add_managed method", cwin->win);
				target=NULL;
			}else if(SCREEN_OF(target)!=SCREEN_OF(cwin)){
				warn("The target id property of window %#x is set to "
					 "a frame on different screen", cwin->win);
				target=NULL;
			}
		}
		
		/* Found a specific target frame or such? */
		if(target!=NULL){
			if(finish_add_clientwin(target, cwin, param))
				return TRUE;
		}
	}
	
	/* Need to find a workspace and let it handle this. */
	
	if(ws==NULL){
		vp=find_suitable_viewport(cwin, param);
		if(vp!=NULL)
			ws=find_suitable_workspace(vp);
	}

	if(ws!=NULL){
		if(genws_add_clientwin(ws, cwin, param))
			return TRUE;
	}
	
	/* If there was no workspace or the workspace failed to handle this,
	 * try to have the transient_for clientwin manage transients.
	 */
	if(param->flags&REGION_ATTACH_TFOR){
		if(finish_add_clientwin((WRegion*)param->tfor, cwin, param))
			return TRUE;
	}
	
	/* All else failed, try full screen mode */
	
	return clientwin_fullscreen_vp(cwin, vp, FALSE);
}


bool genws_add_clientwin(WGenWS *reg, WClientWin *cwin,
						 const WAttachParams *param)
{
	bool ret=FALSE;
	CALL_DYN_RET(ret, bool, genws_add_clientwin, reg, (reg, cwin, param));
	return ret;
	
}


bool finish_add_clientwin(WRegion *reg, WClientWin *cwin,
						  const WAttachParams *param)
{
	WAttachParams param2;

	param2.flags=0;
	
	assert(reg!=NULL);
	
	if(clientwin_get_switchto(cwin))
		param2.flags|=REGION_ATTACH_SWITCHTO;
	
	if(param->flags&REGION_ATTACH_INITSTATE &&
	   param->init_state==IconicState)
		param2.flags=0;
	
	return region_add_managed(reg, (WRegion*)cwin, &param2);
}


/*}}}*/

