/*
 * ion/ioncore/screen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
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
#include "screen.h"
#include "cursor.h"
#include "global.h"
#include "event.h"
#include "drawp.h"
#include "grdata.h"
#include "thingp.h"
#include "conf-draw.h"
#include "clientwin.h"
#include "property.h"
#include "focus.h"
#include "funtabs.h"
#include "regbind.h"
#include "viewport.h"

static void fit_screen(WScreen *scr, WRectangle geom);
static void map_screen(WScreen *scr);
static void unmap_screen(WScreen *scr);
static void focus_screen(WScreen *scr, bool warp);
static bool reparent_screen(WScreen *scr, WWindow *par, WRectangle geom);
static void screen_sub_params(const WScreen *scr, WRectangle **geom);
static void screen_activated(WScreen *scr);
static void screen_remove_managed(WScreen *scr, WRegion *reg);


static DynFunTab screen_dynfuntab[]={
	{fit_region, fit_screen},
	{map_region, map_screen},
	{unmap_region, unmap_screen},
	{focus_region, focus_screen},
	{(DynFun*)reparent_region, (DynFun*)reparent_screen},
	/*{region_request_managed_geom, region_request_managed_geom_unallow},*/
	{region_activated, screen_activated},
	{region_remove_managed, screen_remove_managed},
	END_DYNFUNTAB
};


IMPLOBJ(WScreen, WWindow, deinit_screen, screen_dynfuntab,
		&ioncore_screen_funclist)


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
	sprintf(num, "%d", ev->request_code);
	XGetErrorDatabaseText(dpy, "XRequest", num, "", request, 64);

	if(request[0]=='\0')
		sprintf(request, "<unknown request>");

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


Window create_simple_window_bg(const WScreen *scr, Window par,
							   WRectangle geom, WColor background)
{
	return XCreateSimpleWindow(wglobal.dpy, par,
							   geom.x, geom.y, geom.w, geom.h,
							   0, BlackPixel(wglobal.dpy, scr->xscr),
							   COLOR_PIXEL(background));
}


Window create_simple_window(const WScreen *scr, Window par, WRectangle geom)
{
	return create_simple_window_bg(scr, par, geom, scr->grdata.frame_colors.bg);
}


/*}}}*/


/*{{{ Init/deinit */


static void scan_initial_windows(WScreen *scr)
{
	Window dummy_root, dummy_parent, *wins=NULL;
	uint nwins=0, i, j;
	XWMHints *hints;
	
	XQueryTree(wglobal.dpy, scr->root.win,
			   &dummy_root, &dummy_parent, &wins, &nwins);
	
	for(i=0; i<nwins; i++){
		if(wins[i]==None)
			continue;
		if(FIND_WINDOW(wins[i])!=NULL){
			wins[i]=None;
			continue;
		}
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
	
	scr->tmpwins=wins;
	scr->tmpnwins=nwins;
}


void manage_initial_windows(WScreen *scr)
{
	Window *wins=scr->tmpwins;
	Window tfor=None;
	int i, nwins=scr->tmpnwins;

	scr->tmpwins=NULL;
	scr->tmpnwins=0;
	
	for(i=0; i<nwins; i++){
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


static WScreen *preinit_screen(int xscr)
{
	Display *dpy=wglobal.dpy;
	WScreen *scr;
	WRectangle geom;
	Window rootwin;
	int i;
	
	/* Try to select input on the root window */
	rootwin=RootWindow(dpy, xscr);
	
	redirect_error=FALSE;

	XSetErrorHandler(my_redirect_error_handler);
	XSelectInput(dpy, rootwin, ROOT_MASK);
	XSync(dpy, 0);
	XSetErrorHandler(my_error_handler);

	if(redirect_error){
		warn("Unable to redirect root window events for screen %d.", xscr);
		return NULL;
	}
	
	scr=ALLOC(WScreen);
	
	if(scr==NULL){
		warn_err();
		return NULL;
	}
	
	/* Init the struct */
	WTHING_INIT(scr, WScreen);
	
	geom.x=0; geom.y=0;
	geom.w=DisplayWidth(dpy, xscr);
	geom.h=DisplayHeight(dpy, xscr);
	
	init_window((WWindow*)scr, NULL, rootwin, geom);
	
	scr->root.region.flags|=REGION_BINDINGS_ARE_GRABBED;
	scr->root.region.screen=scr;
	scr->xscr=xscr;
	scr->default_cmap=DefaultColormap(dpy, xscr);
	scr->default_viewport=NULL;
	scr->current_viewport=NULL;
	scr->viewport_list=NULL;
	
	scr->w_unit=7;
	scr->h_unit=13;
	
	scr->bcount=NULL;
	scr->n_bcount=0;
	
	MARK_REGION_MAPPED(scr);
	
	for(i=0; i<SCREEN_MAX_STACK; i++)
		scr->u.stack_lists[i]=NULL;
	
	scan_initial_windows(scr);

	region_add_bindmap((WRegion*)scr, &ioncore_screen_bindmap);
	
	return scr;
}


static void postinit_screen(WScreen *scr)
{
	set_cursor(scr->root.win, CURSOR_DEFAULT);
}


WViewport *add_viewport(WScreen *scr, int id, WRectangle geom)
{
	WViewport *vp=create_viewport(scr, id, geom);
	
	if(vp==NULL)
		return NULL;
	
	region_set_manager((WRegion*)vp, (WRegion*)scr, &(scr->viewport_list));
	
	map_region((WRegion*)vp);
	
	if(scr->default_viewport==NULL)
		scr->default_viewport=vp;
	
	return vp;
}


WScreen *manage_screen(int xscr)
{
	WScreen *scr;
	
#ifndef CF_NO_XINERAMA
	XineramaScreenInfo *xi=NULL;
	int i, nxi=0;
	int event_base, error_base;
	
	if(XineramaQueryExtension(wglobal.dpy, &event_base, &error_base)){
		xi=XineramaQueryScreens(wglobal.dpy, &nxi);
	
		if(xi!=NULL && wglobal.screens!=NULL){
			warn("Unable to support both Xinerama and normal multihead.");
			XFree(xi);
			return NULL;
		}
	}
#endif
	
	scr=preinit_screen(xscr);
		
	if(scr==NULL){
#ifndef CF_NO_XINERAMA
		if(xi!=NULL)
			XFree(xi);
#endif
		return NULL;
	}
	
	preinit_graphics(scr);
	
	read_draw_config(scr);

#ifndef CF_NO_XINERAMA
	if(xi!=NULL && nxi!=0){
		WRectangle geom;

		for(i=0; i<nxi; i++){
			geom.x=xi[i].x_org;
			geom.y=xi[i].y_org;
			geom.w=xi[i].width;
			geom.h=xi[i].height;
			/*pgeom("Detected Xinerama screen", geom);*/
			if(!add_viewport(scr, i, geom))
				warn("Unable to add viewport for Xinerama screen %d", i);
		}
		XFree(xi);
	}else
#endif
	add_viewport(scr, xscr, REGION_GEOM(scr));
	
	if(scr->default_viewport==NULL){
		warn("Unable to add a viewport to X screen %d.", xscr);
		destroy_thing((WThing*)scr);
		return NULL;
	}
	
	/* */ {
		/* TODO: typed LINK_ITEM */
		WThing *tmp=(WThing*)wglobal.screens;
		LINK_ITEM(tmp, (WThing*)scr, t_next, t_prev);
		wglobal.screens=(WScreen*)tmp;
	}
	
	postinit_graphics(scr);
	postinit_screen(scr);
	
	return scr;
}


void deinit_screen(WScreen *scr)
{
	WRegion *reg, *next;

	if(wglobal.active_screen==scr)
		wglobal.active_screen=NULL;
	
	FOR_ALL_MANAGED_ON_LIST_W_NEXT(scr->viewport_list, reg, next){
		region_unset_manager(reg, (WRegion*)scr, &(scr->viewport_list));
	}
	
	/* */ {
		WThing *tmp=(WThing*)wglobal.screens;
		UNLINK_ITEM(tmp, (WThing*)scr, t_next, t_prev);
		wglobal.screens=(WScreen*)tmp;
	}
	
	deinit_region((WRegion*)scr);
}


/*}}}*/


/*{{{ region dynfun implementations */


static void fit_screen(WScreen *scr, WRectangle geom)
{
	warn("Don't know how to fit_screen");
}


static void map_screen(WScreen *scr)
{
	warn("Attempt to map a screen.");
}


static void unmap_screen(WScreen *scr)
{
	warn("Attempt to unmap a screen -- impossible");
}


static void focus_screen(WScreen *scr, bool warp)
{
	WRegion *sub;
	
	sub=REGION_ACTIVE_SUB(scr);
	
	if(sub==NULL){
		sub=FIRST_THING(scr, WRegion);
		while(sub!=NULL && !REGION_IS_MAPPED(sub)){
			sub=NEXT_THING(sub, WRegion);
		}
	}
		
	if(sub!=NULL)
		focus_region(sub, warp);
	else
		focus_window(&(scr->root), warp);
}


static bool reparent_screen(WScreen *scr, WWindow *par, WRectangle geom)
{
	warn("Attempt to reparent a screen -- impossible");
	return FALSE;
}


static void screen_activated(WScreen *scr)
{
	wglobal.active_screen=scr;
}


static void screen_remove_managed(WScreen *scr, WRegion *reg)
{
	if((WRegion*)scr->current_viewport==reg)
		scr->current_viewport=NULL;
	
	region_unset_manager(reg, (WRegion*)scr, NULL);
}


/*}}}*/


/*{{{ Misc */


WScreen *screen_of(const WRegion *reg)
{
	WScreen *scr;
	assert(reg!=NULL);
	scr=(WScreen*)(reg->screen);
	assert(scr!=NULL);
	return scr;
}


Window root_of(const WRegion *reg)
{
	WScreen *scr=screen_of(reg);
	return scr->root.win;
}


WGRData *grdata_of(const WRegion *reg)
{
	WScreen *scr=screen_of(reg);
	return &(scr->grdata);
}


bool same_screen(const WRegion *reg1, const WRegion *reg2)
{
	return (reg1->screen==reg2->screen);
}


/*}}}*/



/*{{{ Workspace and client window management setup */


bool setup_screens()
{
	WScreen *scr;
	WViewport *vp;
	int n=0;
	
	FOR_ALL_SCREENS(scr){
		FOR_ALL_TYPED(scr, vp, WViewport){
			if(init_workspaces_on_vp(vp))
				n++;
		}
		manage_initial_windows(scr);
	}
	
	return (n!=0);
}


/*}}}*/

