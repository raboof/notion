/*
 * ion/ioncore/focus.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "focus.h"
#include "global.h"
#include "window.h"
#include "region.h"


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
	
	r2=region_get_active_leaf((WRegion*)wglobal.active_rootwin);
	
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


static void pointer_rootpos(Window rootwin, int *xret, int *yret)
{
	Window root, win;
	int x, y, wx, wy;
	uint mask;
	
	XQueryPointer(wglobal.dpy, rootwin, &root, &win,
				  xret, yret, &wx, &wy, &mask);
}


void do_move_pointer_to(WRegion *reg)
{
	int x, y, w, h, px, py;
	Window root;
	
	root=ROOT_OF(reg);
	
	pointer_rootpos(root, &px, &py);
	region_rootpos(reg, &x, &y);
	w=REGION_GEOM(reg).w;
	h=REGION_GEOM(reg).h;
	
	if(px<x || py<y || px>=x+w || py>=y+h){
		XWarpPointer(wglobal.dpy, None, root, 0, 0, 0, 0,
					 x+5, y+5);
	}
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


/*}}}*/

