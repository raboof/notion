/*
 * ion/ioncore/rootwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include <stdio.h>
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
#include "gr.h"
#include "clientwin.h"
#include "property.h"
#include "focus.h"
#include "regbind.h"
#include "screen.h"
#include "screen.h"
#include "bindmaps.h"
#include "readconfig.h"
#include "resize.h"
#include "saveload.h"
#include "netwm.h"


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


Window create_simple_window(WRootWin *rw, Window par, const WRectangle *geom)
{
    int w=geom->w;
    int h=geom->h;
    
    if(w<=0)
        w=1;
    if(h<=0)
        h=1;
    
	return XCreateSimpleWindow(wglobal.dpy, par, geom->x, geom->y, w, h,
							   0, 0, BlackPixel(wglobal.dpy, rw->xscr));
}


/*}}}*/


/*{{{ Init/deinit */


static void scan_initial_windows(WRootWin *rootwin)
{
	Window dummy_root, dummy_parent, *wins=NULL;
	uint nwins=0, i, j;
	XWMHints *hints;
	
	XQueryTree(wglobal.dpy, WROOTWIN_ROOT(rootwin), &dummy_root, &dummy_parent,
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


static void create_wm_windows(WRootWin *rootwin)
{
	rootwin->dummy_win=XCreateWindow(wglobal.dpy, WROOTWIN_ROOT(rootwin),
									 0, 0, 1, 1, 0,
									 CopyFromParent, InputOnly,
									 CopyFromParent, 0, NULL);

	XSelectInput(wglobal.dpy, rootwin->dummy_win, PropertyChangeMask);
}


static void preinit_gr(WRootWin *rootwin)
{
	XGCValues gcv;
	ulong gcvmask;

	/* Create XOR gc (for resize) */
	gcv.line_style=LineSolid;
	gcv.join_style=JoinBevel;
	gcv.cap_style=CapButt;
	gcv.fill_style=FillSolid;
	gcv.line_width=2;
	gcv.subwindow_mode=IncludeInferiors;
	gcv.function=GXxor;
	gcv.foreground=~0L;
	
	gcvmask=(GCLineStyle|GCLineWidth|GCFillStyle|
			 GCJoinStyle|GCCapStyle|GCFunction|
			 GCSubwindowMode|GCForeground);

	rootwin->xor_gc=XCreateGC(wglobal.dpy, WROOTWIN_ROOT(rootwin), 
							  gcvmask, &gcv);
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

	rootwin->xscr=xscr;
	rootwin->default_cmap=DefaultColormap(dpy, xscr);
	rootwin->screen_list=NULL;
	rootwin->tmpwins=NULL;
	rootwin->tmpnwins=0;
	rootwin->dummy_win=None;
	rootwin->xor_gc=None;
	
	geom.x=0; geom.y=0;
	geom.w=DisplayWidth(dpy, xscr);
	geom.h=DisplayHeight(dpy, xscr);
	
	if(!window_init((WWindow*)rootwin, NULL, root, &geom)){
		free(rootwin);
		return NULL;
	}

	((WRegion*)rootwin)->flags|=REGION_BINDINGS_ARE_GRABBED;
	((WRegion*)rootwin)->rootwin=rootwin;
	
	MARK_REGION_MAPPED(rootwin);
	
	scan_initial_windows(rootwin);

	create_wm_windows(rootwin);
	preinit_gr(rootwin);
	netwm_init_rootwin(rootwin);
	
	region_add_bindmap((WRegion*)rootwin, &ioncore_rootwin_bindmap);
	
	return rootwin;
}


static Atom net_virtual_roots=None;


static WScreen *add_screen(WRootWin *rw, int id, const WRectangle *geom, 
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
		p[0]=region_x_window((WRegion*)scr);
		XChangeProperty(wglobal.dpy, WROOTWIN_ROOT(rw), net_virtual_roots,
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

	net_virtual_roots=XInternAtom(wglobal.dpy, "_NET_VIRTUAL_ROOTS", False);
	XDeleteProperty(wglobal.dpy, WROOTWIN_ROOT(rootwin), net_virtual_roots);
	
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
			if(!add_screen(rootwin, i, &geom, useroot))
				warn("Unable to add viewport for Xinerama screen %d", i);
		}
		XFree(xi);
	}else
#endif
	{
		nxi=1;
		add_screen(rootwin, xscr, &REGION_GEOM(rootwin), TRUE);
	}
	
	if(rootwin->screen_list==NULL){
		warn("Unable to add a viewport to X screen %d.", xscr);
		destroy_obj((WObj*)rootwin);
		return NULL;
	}
	
	/* */ {
		/* TODO: typed LINK_ITEM */
		WRegion *tmp=(WRegion*)wglobal.rootwins;
		LINK_ITEM(tmp, (WRegion*)rootwin, p_next, p_prev);
		wglobal.rootwins=(WRootWin*)tmp;
	}

	set_cursor(WROOTWIN_ROOT(rootwin), CURSOR_DEFAULT);
	
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
	
	XSelectInput(wglobal.dpy, WROOTWIN_ROOT(rw), 0);
	
	XFreeGC(wglobal.dpy, rw->xor_gc);
	
	window_deinit((WWindow*)rw);
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
		SET_FOCUS(WROOTWIN_ROOT(rootwin));
}


static void rootwin_fit(WRootWin *rootwin, const WRectangle *geom)
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


static bool reparent_rootwin(WRootWin *rootwin, WWindow *par, 
							 const WRectangle *geom)
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
	return WROOTWIN_ROOT(rootwin);
}


/*}}}*/


/*{{{ Misc */


/*EXTL_DOC
 * Returns the root window \var{reg} is on.
 */
EXTL_EXPORT_MEMBER
WRootWin *region_rootwin_of(const WRegion *reg)
{
	WRootWin *rw;
	assert(reg!=NULL); /* Lua interface should not pass NULL reg. */
	rw=(WRootWin*)(reg->rootwin);
	assert(rw!=NULL);
	return rw;
}


Window region_root_of(const WRegion *reg)
{
	return WROOTWIN_ROOT(region_rootwin_of(reg));
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


/*EXTL_DOC
 * Returns a table of root windows indexed by the X screen id.
 */
EXTL_EXPORT
ExtlTab root_windows()
{
	WRootWin *rootwin;
	ExtlTab tab=extl_create_table();
	
	FOR_ALL_ROOTWINS(rootwin){
		extl_table_seti_o(tab, rootwin->xscr, (WObj*)rootwin);
	}

	return tab;
}


WRootWin *find_rootwin_for_root(Window root)
{
	WRootWin *rootwin;
	
	FOR_ALL_ROOTWINS(rootwin){
		if(WROOTWIN_ROOT(rootwin)==root)
			break;
	}
	
	return rootwin;
}


/*}}}*/


/*{{{ Workspace and client window management setup */


bool setup_rootwins()
{
	WRootWin *rootwin;
	WRegion *reg;
	int n=0;
	
	load_workspaces();
	
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
	{(DynFun*)region_reparent, (DynFun*)reparent_rootwin},
	{region_remove_managed, rootwin_remove_managed},
	END_DYNFUNTAB
};


IMPLOBJ(WRootWin, WWindow, rootwin_deinit, rootwin_dynfuntab);

	
/*}}}*/
