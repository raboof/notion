/*
 * wmcore/window.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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
	/*{region_rect_params, window_rect_params},*/
	{(DynFun*)reparent_region, (DynFun*)reparent_window},
	{(DynFun*)region_restack, (DynFun*)window_restack},
	{(DynFun*)region_lowest_win, (DynFun*)window_lowest_win},
	END_DYNFUNTAB
};


IMPLOBJ(WWindow, WRegion, deinit_window, window_dynfuntab, NULL)


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


void window_release(WWindow *wwin)
{
	CALL_DYN(window_release, wwin, (wwin));
}


/*}}}*/
	

/*{{{ Init, create */


bool init_window(WWindow *wwin, WWindow *parent, Window win, WRectangle geom)
{
	init_region(&(wwin->region), (WRegion*)parent, geom);
	
	wwin->win=win;
#ifdef CF_XFT
	wwin->draw=XftDrawCreate(wglobal.dpy, win, 
							 DefaultVisual(wglobal.dpy, SCREEN_OF(wwin)->xscr),
							 SCREEN_OF(wwin)->default_cmap);
#else
	wwin->draw=NULL;
#endif
	wwin->xic=NULL;
	/*wwin->bindmap=NULL;*/
	
	XSaveContext(wglobal.dpy, win, wglobal.win_context, (XPointer)wwin);
	
	return TRUE;
}


bool init_window_new(WWindow *p, WWindow *parent, WRectangle geom)
{
	Window win;
	
	win=create_simple_window(SCREEN_OF(parent), parent->win, geom);
	
	if(win==None)
		return FALSE;
	
	/* init_window above will always succeed */
	return init_window(p, parent, win, geom);
}


void deinit_window(WWindow *wwin)
{
	deinit_region((WRegion*)wwin);
	
	if(wwin->xic!=NULL)
		XDestroyIC(wwin->xic);

	XDeleteContext(wglobal.dpy, wwin->win, wglobal.win_context);
#ifdef CF_XFT
	XftDrawDestroy(wwin->draw);
#endif
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


/*void window_rect_params(WWindow *wwin, WRectangle geom,
						WWinGeomParams *ret)
{
	ret->geom=geom;
	ret->win=wwin->win;
	ret->win_x=geom.x;
	ret->win_y=geom.y;
}*/


static void reparent_or_fit_window(WWindow *wwin, Window parwin,
								   WRectangle geom)
{
	bool move=(REGION_GEOM(wwin).x!=geom.x ||
			   REGION_GEOM(wwin).y!=geom.y);

	if(parwin!=None){
		XReparentWindow(wglobal.dpy, wwin->win, parwin, geom.x, geom.y);
		XResizeWindow(wglobal.dpy, wwin->win, geom.w, geom.h);
	}else{
		XMoveResizeWindow(wglobal.dpy, wwin->win, geom.x, geom.y,
						  geom.w, geom.h);
	}
	
	REGION_GEOM(wwin)=geom;

	if(move)
		notify_subregions_move(&(wwin->region));
}


bool reparent_window(WWindow *wwin, WRegion *parent, WRectangle geom)
{
	if(!WTHING_IS(parent, WWindow) || !same_screen((WRegion*)wwin, parent))
		return FALSE;
	
	region_detach((WRegion*)wwin);
	region_set_parent((WRegion*)wwin, parent);
	reparent_or_fit_window(wwin, ((WWindow*)parent)->win, geom);
	return TRUE;
}


void fit_window(WWindow *wwin, WRectangle geom)
{
	reparent_or_fit_window(wwin, None, geom);
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


