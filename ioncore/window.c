/*
 * wmcore/window.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "global.h"
#include "window.h"
#include "focus.h"
#include "thingp.h"
#include "screen.h"


static DynFunTab window_dynfuntab[]={
	{fit_region, fit_window},
	{map_region, map_window},
	{unmap_region, unmap_window},
	{focus_region, focus_window},
	{region_rect_params, window_rect_params},
	{(DynFun*)reparent_region, (DynFun*)reparent_window},
	{(DynFun*)region_restack, (DynFun*)window_restack},
	{(DynFun*)region_lowest_win, (DynFun*)window_lowest_win},
	END_DYNFUNTAB
};


IMPLOBJ(WWindow, WRegion, deinit_window, window_dynfuntab)


/*{{{ Dynfuns */


void draw_window(WWindow *wwin, bool complete)
{
	CALL_DYN(draw_window, wwin, (wwin, complete));
}


void window_insstr(WWindow *wwin, const char *buf, size_t n)
{
	CALL_DYN(window_insstr, wwin, (wwin, buf, n));
}


int window_press(WWindow *wwin, XButtonEvent *ev, WThing **thing_ret)
{
	int area=0;
	CALL_DYN_RET(area, int, window_press, wwin, (wwin, ev, thing_ret));
	return area;
}


/*}}}*/
	

/*{{{ Init, create */


bool init_window(WWindow *wwin, WScreen *scr, Window win, WRectangle geom)
{
	init_region(&wwin->region, scr, geom);
	
	wwin->flags=0;
	wwin->win=win;
	wwin->xic=NULL;
	/*wwin->bindmap=NULL;*/
	
	XSaveContext(wglobal.dpy, win, wglobal.win_context, (XPointer)wwin);
	
	return TRUE;
}


bool init_window_new(WWindow *p, WScreen *scr, WWinGeomParams params)
{
	Window win;
	
	win=create_simple_window(scr, params);
	if(win==None)
		return FALSE;
	/* init_window above will always succeed */
	return init_window(p, scr, win, params.geom);
}


void deinit_window(WWindow *wwin)
{
	deinit_region((WRegion*)wwin);
	
	if(wwin->xic!=NULL)
		XDestroyIC(wwin->xic);

	XDeleteContext(wglobal.dpy, wwin->win, wglobal.win_context);
	XDestroyWindow(wglobal.dpy, wwin->win);
}


/*}}}*/


/*{{{ Find, X Window -> thing */


WThing *find_window(Window win)
{
	WThing *thing;
	
	if(XFindContext(wglobal.dpy, win, wglobal.win_context,
					(XPointer*)&thing)!=0)
		return NULL;
	
	return thing;
}


WThing *find_window_t(Window win, const WObjDescr *descr)
{
	WThing *thing=find_window(win);
	
	if(thing==NULL)
		return NULL;
	
	if(!wobj_is((WObj*)thing, descr))
		return NULL;
	
	return thing;
}


/*}}}*/


/*{{{ Region dynfuns */


void window_rect_params(WWindow *wwin, WRectangle geom,
						WWinGeomParams *ret)
{
	ret->geom=geom;
	ret->win=wwin->win;
	ret->win_x=geom.x;
	ret->win_y=geom.y;
}


static bool reparent_or_fit_window(WWindow *wwin, WWinGeomParams params,
								   bool rep)
{
	bool move=(REGION_GEOM(wwin).x!=params.geom.x ||
			   REGION_GEOM(wwin).y!=params.geom.y);

	if(rep){
		XReparentWindow(wglobal.dpy, wwin->win, params.win,
						params.win_x, params.win_y);
		XResizeWindow(wglobal.dpy, wwin->win, params.geom.w, params.geom.h);
	}else{
		XMoveResizeWindow(wglobal.dpy, wwin->win,
						  params.win_x, params.win_y,
						  params.geom.w, params.geom.h);
	}
	
	REGION_GEOM(wwin)=params.geom;

	if(move)
		notify_subregions_move(&(wwin->region));
	
	return TRUE;
}


bool reparent_window(WWindow *wwin, WWinGeomParams params)
{
	return reparent_or_fit_window(wwin, params, TRUE);
}


void fit_window(WWindow *wwin, WRectangle geom)
{
	WWinGeomParams params;
	WRegion *par;
	par=FIND_PARENT1(wwin, WRegion);
	if(par!=NULL){
		region_rect_params(par, geom, &params);
		reparent_or_fit_window(wwin, params, FALSE);
	}
}


void map_window(WWindow *wwin)
{
	XMapWindow(wglobal.dpy, wwin->win);
	MARK_REGION_MAPPED(wwin);
}


void unmap_window(WWindow *wwin)
{
	XUnmapWindow(wglobal.dpy, wwin->win);
	MARK_REGION_UNMAPPED(wwin);
}


void focus_window(WWindow *wwin, bool warp)
{
	if(warp)
		do_move_pointer_to((WRegion*)wwin);
	SET_FOCUS(wwin->win);
}


void do_restack_window(Window win, Window other, int stack_mode)
{
	XWindowChanges wc;
	int wcmask;
	
	wcmask=CWStackMode;
	wc.stack_mode=stack_mode;
	if((wc.sibling=other)!=None)
		wcmask|=CWSibling;

	XConfigureWindow(wglobal.dpy, win, wcmask, &wc);
}


Window window_restack(WWindow *wwin, Window other, int mode)
{
	do_restack_window(wwin->win, other, mode);
	return wwin->win;
}


Window window_lowest_win(WWindow *wwin)
{
	return wwin->win;
}


/*}}}*/


