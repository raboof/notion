/*
 * ion/ioncore/window.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "objp.h"
#include "global.h"
#include "window.h"
#include "focus.h"
#include "rootwin.h"
#include "region.h"
#include "stacking.h"


/*{{{ Dynfuns */


void window_draw(WWindow *wwin, bool complete)
{
	CALL_DYN(window_draw, wwin, (wwin, complete));
}


void window_insstr(WWindow *wwin, const char *buf, size_t n)
{
	CALL_DYN(window_insstr, wwin, (wwin, buf, n));
}


int window_press(WWindow *wwin, XButtonEvent *ev, WRegion **reg_ret)
{
	int area=0;
	CALL_DYN_RET(area, int, window_press, wwin, (wwin, ev, reg_ret));
	return area;
}


void window_release(WWindow *wwin)
{
	CALL_DYN(window_release, wwin, (wwin));
}


bool region_reparent(WRegion *reg, WWindow *par, const WRectangle *geom)
{
	bool ret=FALSE;
	CALL_DYN_RET(ret, bool, region_reparent, reg, (reg, par, geom));
	return ret;
}


/*}}}*/
	

/*{{{ Init, create */


bool window_init(WWindow *wwin, WWindow *parent, Window win,
				 const WRectangle *geom)
{
	wwin->win=win;
	wwin->xic=NULL;
	wwin->keep_on_top_list=NULL;
	region_init(&(wwin->region), (WRegion*)parent, geom);
	if(win!=None){
		XSaveContext(ioncore_g.dpy, win, ioncore_g.win_context, (XPointer)wwin);
		if(parent!=NULL)
			window_init_sibling_stacking(parent, win);
	}
	return TRUE;
}


bool window_init_new(WWindow *wwin, WWindow *parent, const WRectangle *geom)
{
	Window win;
	
	win=create_xwindow(region_rootwin_of((WRegion*)parent), parent->win, geom);
	if(win==None)
		return FALSE;
	/* window_init does not fail */
	return window_init(wwin, parent, win, geom);
}


void window_deinit(WWindow *wwin)
{
	region_deinit((WRegion*)wwin);
	
	if(wwin->xic!=NULL)
		XDestroyIC(wwin->xic);

	if(wwin->win!=None){
		XDeleteContext(ioncore_g.dpy, wwin->win, ioncore_g.win_context);
		XDestroyWindow(ioncore_g.dpy, wwin->win);
	}
}


/*}}}*/


/*{{{ Find, X Window -> WRegion */


WRegion *xwindow_region_of(Window win)
{
	WRegion *reg;
	
	if(XFindContext(ioncore_g.dpy, win, ioncore_g.win_context,
					(XPointer*)&reg)!=0)
		return NULL;
	
	return reg;
}


WRegion *xwindow_region_of_t(Window win, const WClassDescr *descr)
{
	WRegion *reg=xwindow_region_of(win);
	
	if(reg==NULL)
		return NULL;
	
	if(!obj_is((WObj*)reg, descr))
		return NULL;
	
	return reg;
}


/*}}}*/


/*{{{ Region dynfuns */


static void reparent_or_fit_window(WWindow *wwin, WWindow *parent,
								   const WRectangle *geom)
{
	bool move=(REGION_GEOM(wwin).x!=geom->x ||
			   REGION_GEOM(wwin).y!=geom->y);

	if(parent!=NULL){
		region_detach_parent((WRegion*)wwin);
		XReparentWindow(ioncore_g.dpy, wwin->win, parent->win, geom->x, geom->y);
		XResizeWindow(ioncore_g.dpy, wwin->win, geom->w, geom->h);
		region_attach_parent((WRegion*)wwin, (WRegion*)parent);
	}else{
		XMoveResizeWindow(ioncore_g.dpy, wwin->win, geom->x, geom->y,
						  geom->w, geom->h);
	}
	
	REGION_GEOM(wwin)=*geom;

	if(move)
		region_notify_subregions_move(&(wwin->region));
}


bool window_reparent(WWindow *wwin, WWindow *parent, const WRectangle *geom)
{
	if(!region_same_rootwin((WRegion*)wwin, (WRegion*)parent))
		return FALSE;
	reparent_or_fit_window(wwin, parent, geom);
	return TRUE;
}


void window_fit(WWindow *wwin, const WRectangle *geom)
{
	reparent_or_fit_window(wwin, NULL, geom);
}


void window_map(WWindow *wwin)
{
	XMapWindow(ioncore_g.dpy, wwin->win);
	REGION_MARK_MAPPED(wwin);
}


void window_unmap(WWindow *wwin)
{
	XUnmapWindow(ioncore_g.dpy, wwin->win);
	REGION_MARK_UNMAPPED(wwin);
}


void window_do_set_focus(WWindow *wwin, bool warp)
{
	if(warp)
		region_do_warp((WRegion*)wwin);
	xwindow_do_set_focus(wwin->win);
}


void xwindow_restack(Window win, Window other, int stack_mode)
{
	XWindowChanges wc;
	int wcmask;
	
	wcmask=CWStackMode;
	wc.stack_mode=stack_mode;
	if((wc.sibling=other)!=None)
		wcmask|=CWSibling;

	XConfigureWindow(ioncore_g.dpy, win, wcmask, &wc);
}


Window window_restack(WWindow *wwin, Window other, int mode)
{
	xwindow_restack(wwin->win, other, mode);
	return wwin->win;
}


Window window_xwindow(const WWindow *wwin)
{
	return wwin->win;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab window_dynfuntab[]={
	{region_fit, window_fit},
	{region_map, window_map},
	{region_unmap, window_unmap},
	{region_do_set_focus, window_do_set_focus},
	{(DynFun*)region_reparent, (DynFun*)window_reparent},
	{(DynFun*)region_restack, (DynFun*)window_restack},
	{(DynFun*)region_xwindow, (DynFun*)window_xwindow},
	END_DYNFUNTAB
};


IMPLCLASS(WWindow, WRegion, window_deinit, window_dynfuntab);

	
/*}}}*/

