/*
 * wmcore/screen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
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
#include "attach.h"
#include "focus.h"
#include "funtabs.h"
#include "regbind.h"

static void screen_remove_sub(WScreen *scr, WRegion *sub);
static void fit_screen(WScreen *scr, WRectangle geom);
static void map_screen(WScreen *scr);
static void unmap_screen(WScreen *scr);
static bool screen_switch_subregion(WScreen *scr, WRegion *sub);
static void focus_screen(WScreen *scr, bool warp);
static bool reparent_screen(WScreen *scr, WWinGeomParams params);

static void screen_sub_params(const WScreen *scr, WWinGeomParams *ret);
static void screen_do_attach(WScreen *scr, WRegion *sub, int flags);

static void screen_activated(WScreen *scr);

static DynFunTab screen_dynfuntab[]={
	{fit_region, fit_screen},
	{map_region, map_screen},
	{unmap_region, unmap_screen},
	{(DynFun*)switch_subregion, (DynFun*)screen_switch_subregion},
	{focus_region, focus_screen},
	{(DynFun*)reparent_region, (DynFun*)reparent_screen},
	{region_request_sub_geom, region_request_sub_geom_unallow},
	{region_activated, screen_activated},
	
	{region_do_attach_params, screen_sub_params},
	{region_do_attach, screen_do_attach},
	
	{region_remove_sub, screen_remove_sub},
	
	END_DYNFUNTAB
};


IMPLOBJ(WScreen, WWindow, deinit_screen, screen_dynfuntab)


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


Window create_simple_window_bg(WScreen *scr, WWinGeomParams params,
							   ulong background)
{
	return XCreateSimpleWindow(wglobal.dpy, params.win,
							   params.win_x, params.win_y,
							   params.geom.w, params.geom.h,
							   0, BlackPixel(wglobal.dpy, scr->xscr),
							   background);
}


Window create_simple_window(WScreen *scr, WWinGeomParams params)
{
	return create_simple_window_bg(scr, params, scr->grdata.frame_colors.bg);
}


/*}}}*/


/*{{{ Init/deinit */


static void scan_initial_windows(WScreen *scr)
{
	Window dummy_root, dummy_parent, *wins;
	uint nwins, i, j;
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
	int i, nwins=scr->tmpnwins;

	scr->tmpwins=NULL;
	scr->tmpnwins=0;
	
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
	
	init_window((WWindow*)scr, scr, rootwin, geom);
	
	/*scr->root.bindmap=NULL; &(wglobal.main_bindmap);*/
	
	scr->xscr=xscr;
	scr->default_cmap=DefaultColormap(dpy, xscr);

	scr->sub_count=0;
	scr->current_sub=NULL;
	scr->w_unit=7;
	scr->h_unit=13;
	
	scr->bcount=NULL;
	scr->n_bcount=0;
	
	MARK_REGION_MAPPED(scr);
	
	for(i=0; i<SCREEN_MAX_STACK; i++)
		scr->u.stack_lists[i]=NULL;
	
	scan_initial_windows(scr);
	
	return scr;
}


static void postinit_screen(WScreen *scr)
{
	/*init_workspaces(scr);*/
	/*scan_initial_windows(scr);*/
	set_cursor(scr->root.win, CURSOR_DEFAULT);
	/* TODO: Need to reorder initilisation code */
	/*region_add_bindmap((WRegion*)scr, &wmcore_screen_bindmap, TRUE);*/
}


WScreen *manage_screen(int xscr)
{
	WScreen *scr;
	WThing *tmp;
	
	scr=preinit_screen(xscr);
		
	if(scr==NULL)
		return NULL;
		
	preinit_graphics(scr);
	
	read_draw_config(scr);
	
	if(wglobal.screens==NULL){
		wglobal.screens=scr;
	}else{
		/* TODO: typed LINK_ITEM */
		tmp=(WThing*)wglobal.screens;
		LINK_ITEM(tmp, (WThing*)scr, t_next, t_prev);
		wglobal.screens=(WScreen*)tmp;
	}

	postinit_graphics(scr);
	
	postinit_screen(scr);
	
	return scr;
}


void deinit_screen(WScreen *scr)
{
	if(wglobal.active_screen==scr)
		wglobal.active_screen=NULL;
}


/*}}}*/


/*{{{ Attach/detach */


static void screen_sub_params(const WScreen *scr, WWinGeomParams *ret)
{
	ret->geom=REGION_GEOM(scr);
	ret->win_x=0;
	ret->win_y=0;
	ret->win=scr->root.win;
}


static void screen_do_attach(WScreen *scr, WRegion *sub, int flags)
{
	if(scr->current_sub!=NULL && wglobal.opmode!=OPMODE_INIT)
		link_thing_after((WThing*)(scr->current_sub), (WThing*)sub);
	else
		link_thing((WThing*)scr, (WThing*)sub);
	
	scr->sub_count++;
	
	if(scr->sub_count==1)
		flags|=REGION_ATTACH_SWITCHTO;

	if(flags&REGION_ATTACH_SWITCHTO)
		screen_switch_subregion(scr, sub);
	else
		unmap_region(sub);
}


bool screen_attach_sub(WScreen *scr, WRegion *sub, bool switchto)
{
	WWinGeomParams params;

	screen_sub_params(scr, &params);
	detach_reparent_region(sub, params);
	
	screen_do_attach(scr, sub, switchto ? REGION_ATTACH_SWITCHTO : 0);
	
	return TRUE;
}


static bool screen_do_detach_sub(WScreen *scr, WRegion *sub)
{
	WRegion *next=NULL;

	if(scr->current_sub==sub){
		next=PREV_THING(sub, WRegion);
		if(next==NULL)
			next=NEXT_THING(sub, WRegion);
		scr->current_sub=NULL;
	}
	
	unlink_thing((WThing*)sub);
	scr->sub_count--;
	
	if(wglobal.opmode!=OPMODE_DEINIT){
		if(next!=NULL)
			screen_switch_subregion(scr, next);
	}

	return TRUE;
}


static void screen_remove_sub(WScreen *scr, WRegion *sub)
{
	screen_do_detach_sub(scr, sub);
}


/*}}}*/


/*{{{ region dynfun implementations */


static void fit_screen(WScreen *scr, WRectangle geom)
{
	WRegion *sub;
	
	REGION_GEOM(scr)=geom;
	
	FOR_ALL_TYPED(scr, sub, WRegion){
		fit_region(sub, geom);
	}
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
	WRegion *sub=scr->current_sub;
	if(sub!=NULL)
		focus_region(sub, warp);
	else
		focus_window(&(scr->root), warp);
}


static bool screen_switch_subregion(WScreen *scr, WRegion *sub)
{
	char *n;
	
	if(sub==scr->current_sub)
		return FALSE;
	
	if(scr->current_sub!=NULL)
		unmap_region(scr->current_sub);
	scr->current_sub=sub;
	map_region(sub);

	if(wglobal.opmode!=OPMODE_DEINIT){
		n=region_full_name(sub);

		if(n==NULL){
			set_string_property(scr->root.win, wglobal.atom_workspace, "");
		}else{
			set_string_property(scr->root.win, wglobal.atom_workspace, n);
			free(n);
		}
	}
	
	if(REGION_IS_ACTIVE(scr)){
		if(wglobal.warp_enabled)
			warp(sub);
		else
			do_set_focus(sub, FALSE);
	}
	
	return TRUE;
}


static bool reparent_screen(WScreen *scr, WWinGeomParams params)
{
	warn("Attempt to reparent a screen -- impossible");
	return FALSE;
}


static void screen_activated(WScreen *scr)
{
	wglobal.active_screen=scr;
}


/*}}}*/


/*{{{ Workspace switch */


WScreen *nth_screen(int ndx)
{
	WScreen *scr=wglobal.screens;
	while(ndx-->0 && scr!=NULL)
		scr=NEXT_THING(scr, WScreen);
	return scr;
}


static WRegion *nth_sub(WScreen *scr, uint n)
{
	return NTH_THING(scr, n, WRegion);
}


static WRegion *next_sub(WScreen *scr)
{
	WRegion *reg=NULL;
	if(scr->current_sub!=NULL)
		reg=NEXT_THING(scr->current_sub, WRegion);
	if(reg==NULL)
		reg=FIRST_THING(scr, WRegion);
	return reg;
}


static WRegion *prev_sub(WScreen *scr)
{
	WRegion *reg=NULL;
	if(scr->current_sub!=NULL)
		reg=PREV_THING(scr->current_sub, WRegion);
	if(reg==NULL)
		reg=LAST_THING(scr, WRegion);
	return reg;
}


void screen_switch_nth(WScreen *scr, uint n)
{
	WRegion *sub=nth_sub(scr, n);
	if(sub!=NULL)
		switch_region(sub);
}


void screen_switch_nth2(int scrnum, int n)
{
	WScreen *scr=nth_screen(scrnum);
	
	if(scr==NULL)
		return;
	
	if(n<0)
		switch_region((WRegion*)scr);
	else
		screen_switch_nth(scr, n);
}
	
	   
void screen_switch_next(WScreen *scr)
{
	WRegion *sub=next_sub(scr);
	if(sub!=NULL)
		switch_region(sub);
}


void screen_switch_prev(WScreen *scr)
{
	WRegion *sub=prev_sub(scr);
	if(sub!=NULL)
		switch_region(sub);
}


/*}}}*/


/*{{{ Misc */


WScreen *screen_of(const WRegion *reg)
{
	WScreen *scr=(WScreen*)(reg->screen);
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


