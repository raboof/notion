/*
 * ion/ioncore/focus.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "focus.h"
#include "global.h"
#include "window.h"
#include "region.h"
#include "hooks.h"


/*{{{ Previous active region */


static WWatch prev_watch=WWATCH_INIT;


static void prev_watch_handler(WWatch *watch, WRegion *prev)
{
	WRegion *r;
	while(1){
		r=region_manager_or_parent(prev);
		if(r==NULL)
			break;
		
		if(setup_watch(&prev_watch, (WObj*)r, 
					   (WWatchHandler*)prev_watch_handler))
			break;
		prev=r;
	}
}


void set_previous_of(WRegion *reg)
{
	WRegion *r2;
	
	if(reg==NULL || wglobal.previous_protect!=0)
		return;

	if(REGION_IS_ACTIVE(reg))
		return;
	
	r2=(WRegion*)wglobal.active_screen;
	while(r2->active_sub!=NULL)
		r2=r2->active_sub;
	
	if(r2!=NULL)
		setup_watch(&prev_watch, (WObj*)r2, (WWatchHandler*)prev_watch_handler);
}


void protect_previous()
{
	wglobal.previous_protect++;
}


void unprotect_previous()
{
	assert(wglobal.previous_protect>0);
	wglobal.previous_protect--;
}


/*EXTL_DOC
 * Go to a region that had its activity state previously saved.
 */
EXTL_EXPORT
void goto_previous()
{
	WRegion *r=(WRegion*)prev_watch.obj;
	
	if(r!=NULL){
		reset_watch(&prev_watch);
		region_goto(r);
	}
}


/*}}}*/


/*{{{ set_focus, warp */


WHooklist *do_warp_alt=NULL;


bool do_warp_default(WRegion *reg)
{
	Window root, win=None, realroot=None;
	int x, y, w, h;
	int wx=0, wy=0, px=0, py=0;
	uint mask=0;
	
	D(fprintf(stderr, "do_warp %p %s\n", reg, WOBJ_TYPESTR(reg)));
	
	root=ROOT_OF(reg);
	region_rootpos(reg, &x, &y);

	if(XQueryPointer(wglobal.dpy, root, &realroot, &win,
					 &px, &py, &wx, &wy, &mask)){
		w=REGION_GEOM(reg).w;
		h=REGION_GEOM(reg).h;

		if(px>=x && py>=y && px<x+w && py<y+h)
			return TRUE;
	}
	
	XWarpPointer(wglobal.dpy, None, root, 0, 0, 0, 0,
				 x+5, y+5);
	
	return TRUE;
}


void do_warp(WRegion *reg)
{
	CALL_ALT_B_NORET(do_warp_alt, (reg));
}


void do_set_focus(WRegion *reg, bool warp)
{
	if(reg==NULL || !region_is_fully_mapped(reg))
		return;
	D(fprintf(stderr, "do_set_focus %p %s\n", reg, WOBJ_TYPESTR(reg)));
	region_set_focus_to(reg, warp);
}


void set_focus(WRegion *reg)
{
	D(fprintf(stderr, "set_focus %p %s\n", reg, WOBJ_TYPESTR(reg)));
	wglobal.focus_next=reg;
	wglobal.warp_next=FALSE;
}


void warp(WRegion *reg)
{
	D(fprintf(stderr, "warp %p %s\n", reg, WOBJ_TYPESTR(reg)));
	wglobal.focus_next=reg;
	wglobal.warp_next=wglobal.warp_enabled;
}


WRegion *set_focus_mgrctl(WRegion *freg, bool dowarp)
{
	WRegion *mgr=freg;
	WRegion *reg;
	
	while(1){
		reg=mgr;
		mgr=REGION_MANAGER(reg);
		if(mgr==NULL)
			break;
		reg=region_control_managed_focus(mgr, reg);
		if(reg!=NULL)
			freg=reg;
	}
	
	if(!REGION_IS_ACTIVE(freg)){
		if(dowarp)
			warp(freg);
		else
			set_focus(freg);
	}

	return freg;
}


/*}}}*/

