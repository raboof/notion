/*
 * wmcore/focus.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "focus.h"
#include "thingp.h"
#include "global.h"
#include "window.h"


/*{{{ Previous active region */


void set_previous_of(WRegion *reg)
{
	bool flag=FALSE;
	WRegion *r2;
	
	if(reg==NULL || wglobal.previous_protect!=0)
		return;

	if(REGION_IS_ACTIVE(reg))
		return;

	/* Trace back to the root to remove all previous-references on
	 * the path to reg. We do this so the node previous to reg can
	 * be found starting from both reg and the root.
	 */
	while(1){
		r2=FIND_PARENT1(reg, WRegion);
		if(r2==NULL)
			break;
		r2->previous_act=NULL;
		reg=r2;
	}
	
	wglobal.previous_screen=wglobal.active_screen;
	r2=(WRegion*)wglobal.active_screen;
	
	/* Create a new previous-path from the root passing through all
	 * the currently active nodes.
	 */
	while(r2!=NULL){
		r2->previous_act=r2->active_sub;
		r2=r2->active_sub;
	}
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


void goto_previous()
{
	WRegion *reg=(WRegion*)wglobal.previous_screen;

	if(reg==NULL)
		reg=(WRegion*)wglobal.active_screen;
	
	if(reg==NULL)
		reg=(WRegion*)wglobal.screens;
	
	while(reg->previous_act!=NULL)
		reg=reg->previous_act;
	
	goto_region(reg);
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

	focus_region(reg, warp);
}


void set_focus(WRegion *reg)
{
	wglobal.focus_next=reg;
	wglobal.warp_next=FALSE;
}


void warp(WRegion *reg)
{
	wglobal.focus_next=reg;
	wglobal.warp_next=wglobal.warp_enabled;
}


/*}}}*/

