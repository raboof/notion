/*
 * ion/ioncore/fullscreen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "global.h"
#include "sizehint.h"
#include "clientwin.h"
#include "attach.h"
#include "screen.h"
#include "manage.h"
#include "fullscreen.h"
#include "mwmhints.h"


bool clientwin_check_fullscreen_request(WClientWin *cwin, int w, int h)
{
	WScreen *scr;
	WMwmHints *mwm;
	
	mwm=get_mwm_hints(cwin->win);
	if(mwm==NULL || !(mwm->flags&MWM_HINTS_DECORATIONS) ||
	   mwm->decorations!=0)
		return FALSE;
	
	FOR_ALL_SCREENS(scr){
		if(!same_rootwin((WRegion*)scr, (WRegion*)cwin))
			continue;
		/* TODO: if there are multiple possible rootwins, use the one with
		 * requested position, if any.
		 */
		if(REGION_GEOM(scr).w==w && REGION_GEOM(scr).h==h){
			return clientwin_fullscreen_scr(cwin, (WScreen*)scr,
										   clientwin_get_switchto(cwin));
		}
	}
	
	/* Catch Xinerama-unaware apps here */
	if(REGION_GEOM(ROOTWIN_OF(cwin)).w==w &&
	   REGION_GEOM(ROOTWIN_OF(cwin)).h==h){
		return clientwin_enter_fullscreen(cwin, 
										  clientwin_get_switchto(cwin));
	}
	return FALSE;
}


static void lastmgr_watchhandler(WWatch *watch, WObj *obj)
{
	WClientWinFSInfo *fsinfo=(WClientWinFSInfo*)watch;
	WRegion *r;
	
	assert(WOBJ_IS(obj, WRegion));
	
	r=region_find_rescue_manager((WRegion*)obj);
	
	if(r!=NULL)
		setup_watch(&(fsinfo->last_mgr_watch), (WObj*)r, lastmgr_watchhandler);
}


bool clientwin_fullscreen_scr(WClientWin *cwin, WScreen *scr, bool switchto)
{
	int rootx, rooty;
	bool wasfs=TRUE;
	
	if(cwin->fsinfo.last_mgr_watch.obj==NULL){
		wasfs=FALSE;
		
		if(REGION_MANAGER(cwin)!=NULL){
			setup_watch(&(cwin->fsinfo.last_mgr_watch),
						(WObj*)REGION_MANAGER(cwin),
						lastmgr_watchhandler);
		}else if(scr->mplex.current_sub!=NULL){
			setup_watch(&(cwin->fsinfo.last_mgr_watch),
						(WObj*)scr->mplex.current_sub, 
						lastmgr_watchhandler);
		}
		region_rootpos((WRegion*)cwin, &rootx, &rooty);
		cwin->fsinfo.saved_rootrel_geom.x=rootx;
		cwin->fsinfo.saved_rootrel_geom.y=rooty;
		cwin->fsinfo.saved_rootrel_geom.w=REGION_GEOM(cwin).w;
		cwin->fsinfo.saved_rootrel_geom.h=REGION_GEOM(cwin).h;
	}
	
	if(!region_add_managed_simple((WRegion*)scr, (WRegion*)cwin, switchto ? 
								  REGION_ATTACH_SWITCHTO : 0)){
		warn("Failed to enter full screen mode");
		if(!wasfs)
			reset_watch(&(cwin->fsinfo.last_mgr_watch));
		return FALSE;
	}

	return TRUE;
}


bool clientwin_enter_fullscreen(WClientWin *cwin, bool switchto)
{
	WScreen *scr=region_screen_of((WRegion*)cwin);
	
	if(scr==NULL){
		scr=rootwin_current_scr(ROOTWIN_OF(cwin));
		if(scr==NULL)
			return FALSE;
	}
	
	return clientwin_fullscreen_scr(cwin, scr, switchto);
}


bool clientwin_leave_fullscreen(WClientWin *cwin, bool switchto)
{	
	WRegion *reg;
	WAttachParams param;
	XSizeHints hnt;
	int rootx, rooty;
	bool cf;
	
	if(cwin->fsinfo.last_mgr_watch.obj==NULL)
		return FALSE;
	
	cf=region_may_control_focus((WRegion*)cwin);

	reg=(WRegion*)cwin->fsinfo.last_mgr_watch.obj;
	reset_watch(&(cwin->fsinfo.last_mgr_watch));
	assert(WOBJ_IS(reg, WRegion));
	
	/* Set up geometry hints */
	param.flags|=(REGION_ATTACH_SIZERQ|REGION_ATTACH_POSRQ|
				  REGION_ATTACH_SWITCHTO|REGION_ATTACH_SIZE_HINTS);
	
	region_rootpos(reg, &rootx, &rooty);
	param.geomrq=cwin->fsinfo.saved_rootrel_geom;
	param.geomrq.x-=rootx;
	param.geomrq.y-=rooty;
	
	hnt=cwin->size_hints;
	hnt.flags|=PWinGravity;
	hnt.win_gravity=StaticGravity;
	param.size_hints=&hnt;
	
	if(!do_add_clientwin(reg, cwin, &param)){
		warn("WClientWin failed to return from full screen mode; remaining "
			 "manager or parent from previous location refused to manage us.");
		return FALSE;
	}
	
	if(!cf)
		return TRUE;
	return region_goto((WRegion*)cwin);
}


/*EXTL_DOC
 * Toggle between full screen and normal (framed) mode.
 */
EXTL_EXPORT
bool clientwin_toggle_fullscreen(WClientWin *cwin)
{
	if(cwin->fsinfo.last_mgr_watch.obj!=NULL)
		return clientwin_leave_fullscreen(cwin, TRUE);
	
	return clientwin_enter_fullscreen(cwin, TRUE);
}


