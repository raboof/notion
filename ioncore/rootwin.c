/*
 * ion/ioncore/rootwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
/*#include <X11/Xmu/Error.h>*/
#ifndef CF_NO_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include "common.h"
#include "objp.h"
#include "rootwin.h"
#include "cursor.h"
#include "global.h"
#include "event.h"
#include "grdata.h"
#include "conf-draw.h"
#include "clientwin.h"
#include "property.h"
#include "focus.h"
#include "regbind.h"
#include "screen.h"
#include "draw.h"
#include "screen.h"
#include "bindmaps.h"


/*{{{ Error handling */


static bool redirect_error=FALSE;
static bool ignore_badwindow=TRUE;


static int my_redirect_error_handler(Display *dpy, XErrorEvent *ev)
{
	redirect_error=TRUE;
	return 0;
}


static int my_error_handler(Display *dpy, XErrorEvent *ev)
{
	static char msg[128], request[64], num[32];
	
	/* Just ignore bad window and similar errors; makes the rest of
	 * the code simpler.
	 */
	if((ev->error_code==BadWindow ||
		(ev->error_code==BadMatch && ev->request_code==X_SetInputFocus) ||
		(ev->error_code==BadDrawable && ev->request_code==X_GetGeometry)) &&
	   ignore_badwindow)
		return 0;

#if 0
	XmuPrintDefaultErrorMessage(dpy, ev, stderr);
#else
	XGetErrorText(dpy, ev->error_code, msg, 128);
	snprintf(num, 32, "%d", ev->request_code);
	XGetErrorDatabaseText(dpy, "XRequest", num, "", request, 64);

	if(request[0]=='\0')
		snprintf(request, 64, "<unknown request>");

	if(ev->minor_code!=0){
		warn("[%d] %s (%d.%d) %#lx: %s", ev->serial, request,
			 ev->request_code, ev->minor_code, ev->resourceid,msg);
	}else{
		warn("[%d] %s (%d) %#lx: %s", ev->serial, request,
			 ev->request_code, ev->resourceid,msg);
	}
#endif

	kill(getpid(), SIGTRAP);
	
	return 0;
}


/*}}}*/


/*{{{ Utility functions */


Window create_simple_window_bg(const WGRData *grdata, Window par,
							   WRectangle geom, WColor background)
{
	return XCreateSimpleWindow(wglobal.dpy, par,
							   geom.x, geom.y, geom.w, geom.h, 0, 0,
							   COLOR_PIXEL(background));
}


Window create_simple_window(const WGRData *grdata, Window par, 
							WRectangle geom)
{
	return create_simple_window_bg(grdata, par, geom, 
								   grdata->frame_colors.bg);
}


/*}}}*/


/*{{{ Init/deinit */


static void scan_initial_windows(WRootWin *rootwin)
{
	Window dummy_root, dummy_parent, *wins=NULL;
	uint nwins=0, i, j;
	XWMHints *hints;
	
	XQueryTree(wglobal.dpy, rootwin->root, &dummy_root, &dummy_parent,
			   &wins, &nwins);
	
	for(i=0; i<nwins; i++){
		if(wins[i]==None)
			continue;
		hints=XGetWMHints(wglobal.dpy, wins[i]);
		if(hints!=NULL && hints->flags&IconWindowHint){
			for(j=0; j<nwins; j++){
				if(wins[j]==hints->icon_window){
					wins[j]=None;
					break;
				}
			}
		}
		if(hints!=NULL)
			XFree((void*)hints);
	}
	
	rootwin->tmpwins=wins;
	rootwin->tmpnwins=nwins;
}


void rootwin_manage_initial_windows(WRootWin *rootwin)
{
	Window *wins=rootwin->tmpwins;
	Window tfor=None;
	int i, nwins=rootwin->tmpnwins;

	rootwin->tmpwins=NULL;
	rootwin->tmpnwins=0;
	
	for(i=0; i<nwins; i++){
		if(FIND_WINDOW(wins[i])!=NULL)
			wins[i]=None;
		if(wins[i]==None)
			continue;
		if(XGetTransientForHint(wglobal.dpy, wins[i], &tfor))
			continue;
		manage_clientwin(wins[i], MANAGE_INITIAL);
		wins[i]=None;
	}

	for(i=0; i<nwins; i++){
		if(wins[i]==None)
			continue;
		manage_clientwin(wins[i], MANAGE_INITIAL);
	}
	
	XFree((void*)wins);
}


static WRootWin *preinit_rootwin(int xscr)
{
	Display *dpy=wglobal.dpy;
	WRootWin *rootwin;
	WRectangle geom;
	Window root;
	int i;
	
	/* Try to select input on the root window */
	root=RootWindow(dpy, xscr);
	
	redirect_error=FALSE;

	XSetErrorHandler(my_redirect_error_handler);
	XSelectInput(dpy, root, ROOT_MASK);
	XSync(dpy, 0);
	XSetErrorHandler(my_error_handler);

	if(redirect_error){
		warn("Unable to redirect root window events for screen %d.", xscr);
		return NULL;
	}
	
	rootwin=ALLOC(WRootWin);
	
	if(rootwin==NULL){
		warn_err();
		return NULL;
	}
	
	/* Init the struct */
	WOBJ_INIT(rootwin, WRootWin);
	
	geom.x=0; geom.y=0;
	geom.w=DisplayWidth(dpy, xscr);
	geom.h=DisplayHeight(dpy, xscr);
	
	region_init((WRegion*)rootwin, NULL, geom);
	
	rootwin->reg.flags|=REGION_BINDINGS_ARE_GRABBED;
	rootwin->reg.rootwin=rootwin;
	rootwin->root=root;
	rootwin->xscr=xscr;
	rootwin->default_cmap=DefaultColormap(dpy, xscr);
	rootwin->screen_list=NULL;
	
	MARK_REGION_MAPPED(rootwin);
	
	scan_initial_windows(rootwin);

	region_add_bindmap((WRegion*)rootwin, &ioncore_rootwin_bindmap);
	
	return rootwin;
}


static Atom net_virtual_roots=None;


static WScreen *add_screen(WRootWin *rw, int id, WRectangle geom, 
						   bool useroot)
{
	WScreen *scr;
	CARD32 p[1];
	
#ifdef CF_ALWAYS_VIRTUAL_ROOT
	useroot=FALSE;
#endif

	scr=create_screen(rw, id, geom, useroot);
	
	if(scr==NULL)
		return NULL;
	
	region_set_manager((WRegion*)scr, (WRegion*)rw, &(rw->screen_list));
	
	region_map((WRegion*)scr);

	if(!useroot){
		p[1]=region_x_window((WRegion*)scr);
		XChangeProperty(wglobal.dpy, rw->root, net_virtual_roots,
						XA_WINDOW, 32, PropModeAppend, (uchar*)&(p[1]), 1);
	}

	return scr;
}


WRootWin *manage_rootwin(int xscr, bool noxinerama)
{
	WRootWin *rootwin;
	int nxi=0;
#ifndef CF_NO_XINERAMA
	XineramaScreenInfo *xi=NULL;
	int i;
	int event_base, error_base;
	
	if(!noxinerama){
		if(XineramaQueryExtension(wglobal.dpy, &event_base, &error_base)){
			xi=XineramaQueryScreens(wglobal.dpy, &nxi);
			
			if(xi!=NULL && wglobal.rootwins!=NULL){
				warn("Don't know how to get Xinerama information for "
					 "multiple X root windows.");
				XFree(xi);
				return NULL;
			}
		}
	}
#endif
	
	rootwin=preinit_rootwin(xscr);

	if(rootwin==NULL){
#ifndef CF_NO_XINERAMA
		if(xi!=NULL)
			XFree(xi);
#endif
		return NULL;
	}

	preinit_graphics(rootwin);
	
	net_virtual_roots=XInternAtom(wglobal.dpy, "_NET_VIRTUAL_ROOTS", False);
	XDeleteProperty(wglobal.dpy, rootwin->root, net_virtual_roots);
	
#ifndef CF_NO_XINERAMA
	if(xi!=NULL && nxi!=0){
		bool useroot=FALSE;
		WRectangle geom;

		for(i=0; i<nxi; i++){
			geom.x=xi[i].x_org;
			geom.y=xi[i].y_org;
			geom.w=xi[i].width;
			geom.h=xi[i].height;
			/*if(nxi==1)
				useroot=(geom.x==0 && geom.y==0);*/
			if(!add_screen(rootwin, i, geom, useroot))
				warn("Unable to add viewport for Xinerama screen %d", i);
		}
		XFree(xi);
	}else
#endif
	{
		nxi=1;
		add_screen(rootwin, xscr, REGION_GEOM(rootwin), TRUE);
	}
	
	if(rootwin->screen_list==NULL){
		warn("Unable to add a viewport to X screen %d.", xscr);
		destroy_obj((WObj*)rootwin);
		return NULL;
	}
	
	if(nxi>1){
		/* If a WScreen uses the root window, we'll let win_context point
		 * to it.
		 */
		XSaveContext(wglobal.dpy, rootwin->root, wglobal.win_context,
					 (XPointer)rootwin);
	}

	/* */ {
		/* TODO: typed LINK_ITEM */
		WRegion *tmp=(WRegion*)wglobal.rootwins;
		LINK_ITEM(tmp, (WRegion*)rootwin, p_next, p_prev);
		wglobal.rootwins=(WRootWin*)tmp;
	}
	
	read_draw_config(rootwin);
	postinit_graphics(rootwin);
	set_cursor(rootwin->root, CURSOR_DEFAULT);
	
	return rootwin;
}


void rootwin_deinit(WRootWin *rw)
{
	WRegion *reg, *next;

	while(rw->screen_list!=NULL)
		destroy_obj((WObj*)rw->screen_list);
	
	/* */ {
		WRegion *tmp=(WRegion*)wglobal.rootwins;
		UNLINK_ITEM(tmp, (WRegion*)rw, p_next, p_prev);
		wglobal.rootwins=(WRootWin*)tmp;
	}
	
	XSelectInput(wglobal.dpy, rw->root, 0);
	XDeleteContext(wglobal.dpy, rw->root, wglobal.win_context);

	region_deinit((WRegion*)rw);
}


/*}}}*/


/*{{{ region dynfun implementations */


static void rootwin_set_focus_to(WRootWin *rootwin, bool warp)
{
	WRegion *sub;
	
	sub=REGION_ACTIVE_SUB(rootwin);
	
	if(sub==NULL || !REGION_IS_MAPPED(sub)){
		FOR_ALL_MANAGED_ON_LIST(rootwin->screen_list, sub){
			if(REGION_IS_MAPPED(sub))
				break;
		}
	}

	if(sub!=NULL)
		region_set_focus_to(sub, warp);
	else
		SET_FOCUS(rootwin->root);
}


static void rootwin_fit(WRootWin *rootwin, WRectangle geom)
{
	warn("Don't know how to rootwin_fit");
}


static void rootwin_map(WRootWin *rootwin)
{
	warn("Attempt to map a root window.");
}


static void rootwin_unmap(WRootWin *rootwin)
{
	warn("Attempt to unmap a root window -- impossible");
}


static bool reparent_rootwin(WRootWin *rootwin, WWindow *par, WRectangle geom)
{
	warn("Attempt to reparent a root window -- impossible");
	return FALSE;
}


static void rootwin_remove_managed(WRootWin *rootwin, WRegion *reg)
{
	region_unset_manager(reg, (WRegion*)rootwin, &(rootwin->screen_list));
}


static Window rootwin_x_window(WRootWin *rootwin)
{
	return rootwin->root;
}


/*}}}*/


/*{{{ Misc */


/*EXTL_DOC
 * Returns the root window \var{reg} is on.
 */
EXTL_EXPORT_MEMBER
WRootWin *region_rootwin_of(const WRegion *reg)
{
	WRootWin *scr;
	assert(reg!=NULL);
	scr=(WRootWin*)(reg->rootwin);
	assert(scr!=NULL);
	return scr;
}


Window region_root_of(const WRegion *reg)
{
	return region_rootwin_of(reg)->root;
}


WGRData *region_grdata_of(const WRegion *reg)
{
	return &(region_rootwin_of(reg)->grdata);
}


bool same_rootwin(const WRegion *reg1, const WRegion *reg2)
{
	return (reg1->rootwin==reg2->rootwin);
}


static bool scr_ok(WRegion *r)
{
	return (WOBJ_IS(r, WScreen) && REGION_IS_MAPPED(r));
}


/*EXTL_DOC
 * Returns previously active screen on root window \var{rootwin}.
 */
EXTL_EXPORT_MEMBER
WScreen *rootwin_current_scr(WRootWin *rootwin)
{
	WRegion *r=REGION_ACTIVE_SUB(rootwin);
	
	/* There should be no non-WScreen as children or managed by us, but... */
	
	if(r!=NULL && scr_ok(r))
		return (WScreen*)r;
	
	FOR_ALL_MANAGED_ON_LIST(rootwin->screen_list, r){
		if(scr_ok(r))
			break;
	}
	
	return (WScreen*)r;
}


/*}}}*/


/*{{{ Workspace and client window management setup */


bool setup_rootwins()
{
	WRootWin *rootwin;
	WRegion *reg;
	int n=0;
	
	FOR_ALL_ROOTWINS(rootwin){
		FOR_ALL_MANAGED_ON_LIST(rootwin->screen_list, reg){
			if(WOBJ_IS(reg, WScreen) &&
			   screen_initialize_workspaces((WScreen*)reg))
				n++;
		}
		rootwin_manage_initial_windows(rootwin);
	}
	
	return (n!=0);
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab rootwin_dynfuntab[]={
	{region_fit, rootwin_fit},
	{region_map, rootwin_map},
	{region_unmap, rootwin_unmap},
	{region_set_focus_to, rootwin_set_focus_to},
	{(DynFun*)region_x_window, (DynFun*)rootwin_x_window},
	{(DynFun*)reparent_region, (DynFun*)reparent_rootwin},
	/*{region_request_managed_geom, region_request_managed_geom_unallow},*/
	{region_remove_managed, rootwin_remove_managed},
	END_DYNFUNTAB
};


IMPLOBJ(WRootWin, WRegion, rootwin_deinit, rootwin_dynfuntab);

	
/*}}}*/
