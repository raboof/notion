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
#include "colormap.h"
#include "activity.h"


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
	WRegion *r2=NULL, *r3=NULL;
	
	if(reg==NULL || wglobal.previous_protect!=0)
		return;

	if(REGION_IS_ACTIVE(reg))
		return;
	
    r3=(WRegion*)wglobal.active_screen;
	
	while(r3!=NULL){
		r2=r3;
		r3=r2->active_sub;
		if(r3==NULL)
			r3=region_current(r2);
	}

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


/*{{{ Await focus */


static WWatch await_watch=WWATCH_INIT;


static void await_watch_handler(WWatch *watch, WRegion *prev)
{
	WRegion *r;
	while(1){
		r=region_parent(prev);
		if(r==NULL)
			break;
		
		if(setup_watch(&await_watch, (WObj*)r, 
					   (WWatchHandler*)await_watch_handler))
			break;
		prev=r;
	}
}


void set_await_focus(WRegion *reg)
{
    if(reg!=NULL){
        setup_watch(&await_watch, (WObj*)reg,
                    (WWatchHandler*)await_watch_handler);
    }
}


static bool is_await(WRegion *reg)
{
    WRegion *aw=(WRegion*)await_watch.obj;
    
    while(aw!=NULL){
        if(aw==reg)
            return TRUE;
        aw=region_parent(aw);
    }
    
    return FALSE;
}


/* Only keep await status if focus event is to an ancestor of the await 
 * region.
 */
static void check_clear_await(WRegion *reg)
{
    if(is_await(reg) && reg!=(WRegion*)await_watch.obj)
        return;
    
    reset_watch(&await_watch);
}


/*}}}*/


/*{{{ Region stuff */


bool region_may_control_focus(WRegion *reg)
{
	WRegion *par, *r2;
	
	if(WOBJ_IS_BEING_DESTROYED(reg))
		return FALSE;

	if(REGION_IS_ACTIVE(reg))
		return TRUE;
	
    if(is_await(reg))
        return TRUE;
    
	par=region_parent(reg);
	
	if(par==NULL || !REGION_IS_ACTIVE(par))
		return FALSE;
	
	r2=par->active_sub;
	while(r2!=NULL && r2!=par){
		if(r2==reg)
			return TRUE;
		r2=REGION_MANAGER(r2);
	}

	return FALSE;
}


void region_got_focus(WRegion *reg)
{
	WRegion *r;
	
    check_clear_await(reg);
    
	region_clear_activity(reg);
	
	if(!REGION_IS_ACTIVE(reg)){
		D(fprintf(stderr, "got focus (inact) %s [%p]\n", WOBJ_TYPESTR(reg), reg);)
		reg->flags|=REGION_ACTIVE;
		
		r=region_parent(reg);
		if(r!=NULL)
			r->active_sub=reg;
		
		region_activated(reg);
		
		r=REGION_MANAGER(reg);
		if(r!=NULL)
			region_managed_activated(r, reg);
	}else{
		D(fprintf(stderr, "got focus (act) %s [%p]\n", WOBJ_TYPESTR(reg), reg);)
    }

	/* Install default colour map only if there is no active subregion;
	 * their maps should come first. WClientWins will install their maps
	 * in region_activated. Other regions are supposed to use the same
	 * default map.
	 */
	if(reg->active_sub==NULL && !WOBJ_IS(reg, WClientWin))
		install_cmap(ROOTWIN_OF(reg), None); 
}


void region_lost_focus(WRegion *reg)
{
	WRegion *r;
	
	if(!REGION_IS_ACTIVE(reg)){
		D(fprintf(stderr, "lost focus (inact) %s [%p:]\n", WOBJ_TYPESTR(reg), reg);)
		return;
	}
	
	D(fprintf(stderr, "lost focus (act) %s [%p:]\n", WOBJ_TYPESTR(reg), reg);)
	
	reg->flags&=~REGION_ACTIVE;
	region_inactivated(reg);
	r=REGION_MANAGER(reg);
	if(r!=NULL)
		region_managed_inactivated(r, reg);
}


/*EXTL_DOC
 * Is \var{reg} active/does it or one of it's children of focus?
 */
EXTL_EXPORT_MEMBER
bool region_is_active(WRegion *reg)
{
	return REGION_IS_ACTIVE(reg);
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

