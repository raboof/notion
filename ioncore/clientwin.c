/*
 * ion/ioncore/clientwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <limits.h>

#include "common.h"
#include "global.h"
#include "property.h"
#include "focus.h"
#include "sizehint.h"
#include "thingp.h"
#include "hooks.h"
#include "event.h"
#include "clientwin.h"
#include "colormap.h"
#include "resize.h"
#include "attach.h"
#include "funtabs.h"
#include "regbind.h"
#include "mwmhints.h"
#include "viewport.h"
#include "names.h"


#define TOPMOST_TRANSIENT(CWIN) LAST_MANAGED((CWIN)->transient_list)
#define LOWEST_TRANSIENT(CWIN) FIRST_MANAGED((CWIN)->transient_list)
#define HIGHER_TRANSIENT(CWIN, REG) NEXT_MANAGED((CWIN)->transient_list, REG)
#define LOWER_TRANSIENT(CWIN, REG) PREV_MANAGED((CWIN)->transient_list, REG)
#define FOR_ALL_TRANSIENTS(CWIN, R) FOR_ALL_MANAGED_ON_LIST((CWIN)->transient_list, R)

static void deinit_clientwin(WClientWin *cwin);
static void fit_clientwin(WClientWin *cwin, WRectangle geom);
static bool reparent_clientwin(WClientWin *cwin, WWindow *par, WRectangle geom);
static void map_clientwin(WClientWin *cwin);
static void unmap_clientwin(WClientWin *cwin);
static void focus_clientwin(WClientWin *cwin, bool warp);
static bool clientwin_display_managed(WClientWin *cwin, WRegion *reg);
static void clientwin_notify_rootpos(WClientWin *cwin, int x, int y);
static Window clientwin_restack(WClientWin *cwin, Window other, int mode);
static Window clientwin_x_window(WClientWin *cwin);
static void clientwin_activated(WClientWin *cwin);
static void clientwin_resize_hints(WClientWin *cwin, XSizeHints *hints_ret,
								   uint *relw_ret, uint *relh_ret);
static WRegion *clientwin_managed_enter_to_focus(WClientWin *cwin, WRegion *reg);

static void clientwin_remove_managed(WClientWin *cwin, WRegion *reg);
static void clientwin_add_managed_params(const WClientWin *cwin, WRegion **par,
										WRectangle *ret);
static void clientwin_add_managed_doit(WClientWin *cwin, WRegion *reg, int flags);
static void clientwin_request_managed_geom(WClientWin *cwin, WRegion *sub,
										   WRectangle geom, WRectangle *geomret,
										   bool tryonly);


static DynFunTab clientwin_dynfuntab[]={
	{fit_region, fit_clientwin},
	{(DynFun*)reparent_region, (DynFun*)reparent_clientwin},
	{map_region, map_clientwin},
	{unmap_region, unmap_clientwin},
	{focus_region, focus_clientwin},
	{(DynFun*)region_display_managed, (DynFun*)clientwin_display_managed},
	{region_notify_rootpos, clientwin_notify_rootpos},
	{(DynFun*)region_restack, (DynFun*)clientwin_restack},
	{(DynFun*)region_x_window, (DynFun*)clientwin_x_window},
	{region_activated, clientwin_activated},
	{region_resize_hints, clientwin_resize_hints},
	{(DynFun*)region_managed_enter_to_focus,
	 (DynFun*)clientwin_managed_enter_to_focus},
	
	{region_remove_managed, clientwin_remove_managed},
	{region_add_managed_params, clientwin_add_managed_params},
	{region_add_managed_doit, clientwin_add_managed_doit},
	/* Perhaps resize accordingly instead? */
	{region_request_managed_geom, clientwin_request_managed_geom},
	
	END_DYNFUNTAB
};


IMPLOBJ(WClientWin, WRegion, deinit_clientwin, clientwin_dynfuntab,
		&ioncore_clientwin_funclist)


static void set_clientwin_state(WClientWin *cwin, int state);
static void send_clientmsg(Window win, Atom a);


WHooklist *add_clientwin_alt=NULL;


/*{{{ Get properties */


void get_protocols(WClientWin *cwin)
{
	Atom *protocols=NULL, *p;
	int n;
	
	cwin->flags&=~(CWIN_P_WM_DELETE|CWIN_P_WM_TAKE_FOCUS);
	
	if(!XGetWMProtocols(wglobal.dpy, cwin->win, &protocols, &n))
		return;
	
	for(p=protocols; n; n--, p++){
		if(*p==wglobal.atom_wm_delete)
			cwin->flags|=CWIN_P_WM_DELETE;
		else if(*p==wglobal.atom_wm_take_focus)
			cwin->flags|=CWIN_P_WM_TAKE_FOCUS;
	}
	
	if(protocols!=NULL)
		XFree((char*)protocols);
}


void get_clientwin_size_hints(WClientWin *cwin)
{
	XSizeHints tmp=cwin->size_hints;
	
	get_sizehints(SCREEN_OF(cwin), cwin->win, &(cwin->size_hints));
	
	if(cwin->flags&CWIN_PROP_MAXSIZE){
		cwin->size_hints.max_width=tmp.max_width;
		cwin->size_hints.max_height=tmp.max_height;
		cwin->size_hints.flags|=PMaxSize;
	}
	
	if(cwin->flags&CWIN_PROP_ASPECT){
		cwin->size_hints.min_aspect=tmp.min_aspect;
		cwin->size_hints.max_aspect=tmp.max_aspect;
		cwin->size_hints.flags|=PAspect;
	}
}


char **get_name_list(Window win)
{
	XTextProperty prop;
	char **list=NULL;
	int n,i;
	Status st;
	
	st=XGetWMName(wglobal.dpy, win, &prop);
	
	if(!st)
		return NULL;

#ifndef CF_UTF8
	st=XTextPropertyToStringList(&prop, &list, &n);
#else
	st=Xutf8TextPropertyToTextList(wglobal.dpy, &prop, &list, &n);
#endif
	if(n==0 || list==NULL)
		return NULL;
	return list;
}

	
void clientwin_get_set_name(WClientWin *cwin)
{
	char **list=get_name_list(cwin->win);
	
	if(list==NULL){
		region_unuse_name((WRegion*)cwin);
	}else{
		stripws(*list);
		region_set_name((WRegion*)cwin, *list);
		XFreeStringList(list);
	}
}

/*}}}*/


/*{{{ Manage/create */


static void configure_cwin_bw(Window win, int bw)
{
	XWindowChanges wc;
	ulong wcmask=CWBorderWidth;
	
	wc.border_width=bw;
	XConfigureWindow(wglobal.dpy, win, wcmask, &wc);
}


static bool init_clientwin(WClientWin *cwin, WWindow *parent,
						   Window win, const XWindowAttributes *attr)
{
	WRectangle geom;
	char *name;
	
	geom.x=attr->x;
	geom.y=attr->y;
	geom.h=attr->height;
	geom.w=attr->width;
	
	init_region(&(cwin->region), &(parent->region), geom);
	
	cwin->max_geom=geom;
	cwin->flags=0;
	cwin->win=win;
	cwin->orig_bw=attr->border_width;
	
	clientwin_get_set_name(cwin);

	cwin->event_mask=CLIENT_MASK;
	cwin->transient_for=None;
	cwin->transient_list=NULL;
	cwin->state=WithdrawnState;
	
	cwin->n_cmapwins=0;
	cwin->cmap=attr->colormap;
	cwin->cmaps=NULL;
	cwin->cmapwins=NULL;
	cwin->n_cmapwins=0;
	
	init_watch(&(cwin->last_mgr_watch));
	
	get_colormaps(cwin);
	
	get_clientwin_size_hints(cwin);
	get_protocols(cwin);

	XSelectInput(wglobal.dpy, win, cwin->event_mask);

	XSaveContext(wglobal.dpy, win, wglobal.win_context, (XPointer)cwin);
	XAddToSaveSet(wglobal.dpy, win);

	if(cwin->orig_bw!=0)
		configure_cwin_bw(win, 0);
	
	LINK_ITEM(wglobal.cwin_list, cwin, g_cwin_next, g_cwin_prev);
	
	/*region_add_bindmap((WRegion*)cwin, &ioncore_clientwin_bindmap, TRUE);*/
	
	return TRUE;
}


static WClientWin *create_clientwin(WWindow *parent, Window win,
									const XWindowAttributes *attr)
{
	CREATETHING_IMPL(WClientWin, clientwin, (p, parent, win, attr));
}


/* This is called when a window is mapped on the root window.
 * We want to check if we should manage the window and how and
 * act appropriately.
 */
WClientWin* manage_clientwin(Window win, int mflags)
{
	WScreen *scr;
	WClientWin *cwin;
	XWindowAttributes attr;
	XWMHints *hints;
	int state=NormalState;
	bool dock=FALSE, managed=FALSE;
	
again:
	
	/* Select for UnmapNotify and DestroyNotify as the
	 * window might get destroyed or unmapped in the meanwhile.
	 */
	XSelectInput(wglobal.dpy, win, StructureNotifyMask);

	
	/* Is it a dockapp?
	 */
	hints=XGetWMHints(wglobal.dpy, win);

	if(hints!=NULL && hints->flags&StateHint)
		state=hints->initial_state;
	
	if(!dock && state==WithdrawnState &&
	   hints->flags&IconWindowHint && hints->icon_window!=None){
		/* The dockapp might be displaying its "main" window if no
		 * wm that understands dockapps has been managing it.
		 */
		if(mflags&MANAGE_INITIAL)
			XUnmapWindow(wglobal.dpy, win);
		
		XSelectInput(wglobal.dpy, win, 0);
		
		win=hints->icon_window;
		
		/* Is the icon window already being managed? */
		cwin=find_clientwin(win);
		if(cwin!=NULL)
			return cwin;
		
		/* It is a dock, do everything again from the beginning, now
		 * with the icon window.
		 */
		dock=TRUE;
		goto again;
	}
	
	if(hints!=NULL)
		XFree((void*)hints);

	if(!XGetWindowAttributes(wglobal.dpy, win, &attr)){
		warn("Window disappeared");
		goto fail2;
	}
	

	/* Get the actual state if any */
	get_win_state(win, &state);
	
	/* Do we really want to manage it? */
	if(!dock && (attr.override_redirect ||
				 (mflags&MANAGE_INITIAL && attr.map_state!=IsViewable))){
		goto fail2;
	}

	scr=FIND_WINDOW_T(attr.root, WScreen);

	if(scr==NULL)
		goto fail2;

	if(state!=NormalState && state!=IconicState)
		state=NormalState;
	
	/* Allocate and initialize */
	cwin=create_clientwin((WWindow*)scr, win, /*state, */ &attr);
	
	if(cwin==NULL)
		goto fail2;

	CALL_ALT_B(managed, add_clientwin_alt, (cwin, &attr, state, dock));

	if(!managed){
		warn("Unable to manage client window %d\n", win);
		goto failure;
	}
	
	/* Check that the window exists. The previous check and selectinput
	 * do not seem to catch all cases of window destroyal.
	 */
	XSync(wglobal.dpy, False);
	
	if(XGetWindowAttributes(wglobal.dpy, win, &attr))
		return cwin;
	
	warn("Window disappeared");
	
	clientwin_destroyed(cwin);
	return NULL;

failure:
	clientwin_unmapped(cwin);

fail2:

	XSelectInput(wglobal.dpy, win, 0);
	return NULL;
}


/*}}}*/


/*{{{ Add/remove managed */


static void clientwin_add_managed_params(const WClientWin *cwin, WRegion **par,
										 WRectangle *geom)
{
	int htmp=geom->h;
	
	*par=FIND_PARENT1(cwin, WRegion);
	
	*geom=cwin->max_geom;
	
	if(htmp<=0){
		geom->h/=3;
	}else{
		/* Don't increase the height of the transient */
		int diff=geom->h-htmp;
		if(diff>0){
			geom->y+=diff;
			geom->h-=diff;
		}
	}
	
}


static void clientwin_add_managed_doit(WClientWin *cwin, WRegion *transient,
									   int flags)
{
	WRegion *t=NULL;
	Window win=None;
	
	region_set_manager(transient, (WRegion*)cwin, &(cwin->transient_list));
	
	t=TOPMOST_TRANSIENT(cwin);
	if(t!=NULL)
		win=region_x_window(transient);
	else
		win=cwin->win;
	region_restack(transient, win, Above);

	if(REGION_IS_MAPPED((WRegion*)cwin))
	   map_region(transient);
	
	if(REGION_IS_ACTIVE(cwin))
		set_focus(transient);
}


static void clientwin_remove_managed(WClientWin *cwin, WRegion *transient)
{
	WRegion *reg;
	
	region_unset_manager(transient, (WRegion*)cwin, &(cwin->transient_list));
	
	if(REGION_IS_ACTIVE(transient)){
		reg=TOPMOST_TRANSIENT(cwin);
		if(reg==NULL)
			reg=&cwin->region;
		set_focus(reg);
	}
}


/*}}}*/


/*{{{ Unmanage/destroy */


static void reparent_root(WClientWin *cwin)
{
	XWindowAttributes attr;
	Window dummy;
	int x=0, y=0;
	
	XGetWindowAttributes(wglobal.dpy, cwin->win, &attr);
	XTranslateCoordinates(wglobal.dpy, cwin->win, attr.root, 0, 0, &x, &y,
						  &dummy);
	XReparentWindow(wglobal.dpy, cwin->win, attr.root, x, y);
}


void deinit_clientwin(WClientWin *cwin)
{
	reset_watch(&(cwin->last_mgr_watch));
	deinit_region((WRegion*)cwin);

	UNLINK_ITEM(wglobal.cwin_list, cwin, g_cwin_next, g_cwin_prev);
	
	if(cwin->win!=None){
		XSelectInput(wglobal.dpy, cwin->win, 0);

		reparent_root(cwin);
		
		if(cwin->orig_bw!=0)
			configure_cwin_bw(cwin->win, cwin->orig_bw);

		XRemoveFromSaveSet(wglobal.dpy, cwin->win);
		XDeleteContext(wglobal.dpy, cwin->win, wglobal.win_context);
		
		if(wglobal.opmode==OPMODE_DEINIT)
			XMapWindow(wglobal.dpy, cwin->win);
		else
			clientwin_clear_target_id(cwin);
	}
	clear_colormaps(cwin);
}


/* Used when the window was unmapped */
void clientwin_unmapped(WClientWin *cwin)
{
	region_rescue_managed_on_list((WRegion*)cwin, cwin->transient_list);
	destroy_thing((WThing*)cwin);
}


/* Used when the window was deastroyed */
void clientwin_destroyed(WClientWin *cwin)
{
	XDeleteContext(wglobal.dpy, cwin->win, wglobal.win_context);
	cwin->win=None;
	region_rescue_managed_on_list((WRegion*)cwin, cwin->transient_list);
	destroy_thing((WThing*)cwin);
}


/*}}}*/


/*{{{ Kill/close */


static void send_clientmsg(Window win, Atom a)
{
	XClientMessageEvent ev;
	
	ev.type=ClientMessage;
	ev.window=win;
	ev.message_type=wglobal.atom_wm_protocols;
	ev.format=32;
	ev.data.l[0]=a;
	ev.data.l[1]=CurrentTime;
	
	XSendEvent(wglobal.dpy, win, False, 0L, (XEvent*)&ev);
}


void kill_clientwin(WClientWin *cwin)
{
	XKillClient(wglobal.dpy, cwin->win);
}


void close_clientwin(WClientWin *cwin)
{
	if(cwin->flags&CWIN_P_WM_DELETE)
		send_clientmsg(cwin->win, wglobal.atom_wm_delete);
	else
		warn("Client does not support WM_DELETE.");
}


/*}}}*/


/*{{{ State (hide/show) */


static void set_clientwin_state(WClientWin *cwin, int state)
{
	if(cwin->state!=state){
		cwin->state=state;
		set_win_state(cwin->win, state);
	}
}


static void hide_clientwin(WClientWin *cwin)
{
	if(cwin->flags&CWIN_PROP_ACROBATIC){
		XMoveWindow(wglobal.dpy, cwin->win,
					-2*cwin->max_geom.w, -2*cwin->max_geom.h);
		return;
	}
			
	set_clientwin_state(cwin, IconicState);
	XSelectInput(wglobal.dpy, cwin->win,
				 cwin->event_mask&~(StructureNotifyMask|EnterWindowMask));
	XUnmapWindow(wglobal.dpy, cwin->win);
	XSelectInput(wglobal.dpy, cwin->win, cwin->event_mask);
}


static void show_clientwin(WClientWin *cwin)
{
	if(cwin->flags&CWIN_PROP_ACROBATIC){
		XMoveWindow(wglobal.dpy, cwin->win,
					REGION_GEOM(cwin).x, REGION_GEOM(cwin).y);
		if(cwin->state==NormalState)
			return;
	}

	XSelectInput(wglobal.dpy, cwin->win,
				 cwin->event_mask&~(StructureNotifyMask|EnterWindowMask));
	XMapWindow(wglobal.dpy, cwin->win);
	XSelectInput(wglobal.dpy, cwin->win, cwin->event_mask);
	set_clientwin_state(cwin, NormalState);
}


/*}}}*/


/*{{{ Resize/reparent/reconf helpers */


void clientwin_notify_rootpos(WClientWin *cwin, int rootx, int rooty)
{
	XEvent ce;
	Window win;

	if(cwin==NULL)
		return;
	
	win=cwin->win;

	ce.xconfigure.type=ConfigureNotify;
	ce.xconfigure.event=win;
	ce.xconfigure.window=win;
	ce.xconfigure.x=rootx;
	ce.xconfigure.y=rooty;
	ce.xconfigure.width=REGION_GEOM(cwin).w;
	ce.xconfigure.height=REGION_GEOM(cwin).h;
	ce.xconfigure.border_width=cwin->orig_bw;
	ce.xconfigure.above=None;
	ce.xconfigure.override_redirect=False;

	XSelectInput(wglobal.dpy, win, cwin->event_mask&~StructureNotifyMask);
	XSendEvent(wglobal.dpy, win, False, StructureNotifyMask, &ce);
	XSelectInput(wglobal.dpy, win, cwin->event_mask);
}


static void sendconfig_clientwin(WClientWin *cwin)
{
	int rootx, rooty;
	
	region_rootpos(&cwin->region, &rootx, &rooty);
	clientwin_notify_rootpos(cwin, rootx, rooty);
}


static void do_reparent_clientwin(WClientWin *cwin, Window win, int x, int y)
{
	XSelectInput(wglobal.dpy, cwin->win,
				 cwin->event_mask&~StructureNotifyMask);
	XReparentWindow(wglobal.dpy, cwin->win, win, x, y);
	XSelectInput(wglobal.dpy, cwin->win, cwin->event_mask);
}


static void convert_geom(WClientWin *cwin, WRectangle max_geom,
						 WRectangle *geom/*, bool rq*/)
{
	WRectangle r;
	bool bottom=FALSE;
	
	if(REGION_MANAGER(cwin)!=NULL &&
	   WTHING_IS(REGION_MANAGER(cwin), WClientWin)){
			bottom=TRUE;
	}

	geom->w=max_geom.w;
	geom->h=max_geom.h;
	
	correct_size(&(geom->w), &(geom->h), &(cwin->size_hints), FALSE);
	/*
	if(!rq && bottom && geom->h>REGION_GEOM(cwin).h)
	   geom->h=REGION_GEOM(cwin).h;
	*/
	geom->x=max_geom.x+max_geom.w/2-geom->w/2;
	
	if(bottom)
		geom->y=max_geom.y+max_geom.h-geom->h;
	else
		geom->y=max_geom.y+max_geom.h/2-geom->h/2;
}


/*}}}*/


/*{{{ Region dynfuns */


static void clientwin_request_managed_geom(WClientWin *cwin, WRegion *sub,
										   WRectangle geom, WRectangle *geomret,
										   bool tryonly)
{
	WRectangle rgeom=cwin->max_geom;

	if(rgeom.h>geom.h){
		rgeom.h=geom.h;
		rgeom.y+=cwin->max_geom.h-geom.h;
	}
	
	if(geomret!=NULL)
		*geomret=rgeom;
	
	if(!tryonly)
		fit_region(sub, rgeom);
}



static void do_fit_clientwin(WClientWin *cwin, WRectangle max_geom, WWindow *np)
{
	WRegion *transient, *next;
	WRectangle geom, geom2;
	int diff;
	bool changes;
	
	convert_geom(cwin, max_geom, &geom);
	
	changes=(REGION_GEOM(cwin).x!=geom.x ||
			 REGION_GEOM(cwin).y!=geom.y ||
			 REGION_GEOM(cwin).w!=geom.w ||
			 REGION_GEOM(cwin).h!=geom.h);
	
	cwin->max_geom=max_geom;
	REGION_GEOM(cwin)=geom;

	if(np==NULL && !changes)
		return;

	if(np!=NULL)
		do_reparent_clientwin(cwin, np->win, geom.x, geom.y);
	
	if(cwin->flags&CWIN_PROP_ACROBATIC && !REGION_IS_MAPPED(cwin)){
		XMoveResizeWindow(wglobal.dpy, cwin->win,
						  -2*max_geom.w, -2*max_geom.h, geom.w, geom.h);
	}else{
		XMoveResizeWindow(wglobal.dpy, cwin->win,
						  geom.x, geom.y, geom.w, geom.h);
	}

	cwin->flags&=~CWIN_NEED_CFGNTFY;
	
	geom2.x=max_geom.x;
	geom2.w=max_geom.w;
	
	FOR_ALL_MANAGED_ON_LIST_W_NEXT(cwin->transient_list, transient, next){
		geom2.y=max_geom.y;
		geom2.h=max_geom.h;
		diff=geom.h-region_min_h(transient);/*REGION_GEOM(transient).h;*/
		if(diff>0){
			geom2.y+=diff;
			geom2.h-=diff;
		}
		
		if(np==NULL){
		   fit_region(transient, geom2);
		}else{
			if(!reparent_region(transient, np, geom2)){
				warn("Problem: can't reparent a %s managed by a WClientWin"
					 "being reparented. Detaching from this object.",
					 WOBJ_TYPESTR(transient));
				region_detach_manager(transient);
			}
		}
	}
}


static void fit_clientwin(WClientWin *cwin, WRectangle geom)
{
	do_fit_clientwin(cwin, geom, NULL);
}


static bool reparent_clientwin(WClientWin *cwin, WWindow *par, WRectangle geom)
{
	int rootx, rooty;
	
	if(!same_screen((WRegion*)cwin, (WRegion*)par))
		return FALSE;

	region_detach_parent((WRegion*)cwin);
	region_set_parent((WRegion*)cwin, (WRegion*)par);

	do_fit_clientwin(cwin, geom, par);
	
	sendconfig_clientwin(cwin);
	
	clientwin_clear_target_id(cwin);
	
	return TRUE;
}


static void map_clientwin(WClientWin *cwin)
{
	WRegion *sub;
	
	show_clientwin(cwin);
	MARK_REGION_MAPPED(cwin);
	
	FOR_ALL_TRANSIENTS(cwin, sub){
		map_region(sub);
	}
}


static void unmap_clientwin(WClientWin *cwin)
{
	WRegion *sub;
	
	hide_clientwin(cwin);
	MARK_REGION_UNMAPPED(cwin);
	
	FOR_ALL_TRANSIENTS(cwin, sub){
		unmap_region(sub);
	}
}


static void focus_clientwin(WClientWin *cwin, bool warp)
{
	WRegion *reg=TOPMOST_TRANSIENT(cwin);
	
	if(warp)
		do_move_pointer_to((WRegion*)cwin);
	
	if(reg!=NULL){
		focus_region(reg, FALSE);
		return;
	}
	
	SET_FOCUS(cwin->win);
	
	if(cwin->flags&CWIN_P_WM_TAKE_FOCUS)
		send_clientmsg(cwin->win, wglobal.atom_wm_take_focus);
}

	
static bool clientwin_display_managed(WClientWin *cwin, WRegion *sub)
{
	if(REGION_IS_MAPPED(cwin))
		return FALSE;
	map_region(sub);
	return TRUE;
	
#if 0
	WRegion *oldtop=TOPMOST_TRANSIENT(cwin);
	
	assert(oldtop==NULL);
	
	if(oldtop==sub)
		return TRUE;
	
	/* Relink last (topmost) and raise */
	unlink_thing((WThing*)sub);
	link_thing((WThing*)cwin, (WThing*)sub);
	
	/* Should raise above oldtop, but TODO: region_topmost_win */
	region_restack(sub, None, Above);
	return FALSE;
#endif
}


static Window clientwin_restack(WClientWin *cwin, Window other, int mode)
{
	WRegion *reg;
	
	do_restack_window(cwin->win, other, mode);
	other=cwin->win;
	
	for(reg=LOWEST_TRANSIENT(cwin); reg!=NULL; reg=HIGHER_TRANSIENT(cwin, reg))  
		other=region_restack(reg, other, Above);
													   
	return cwin->win;
}


static Window clientwin_x_window(WClientWin *cwin)
{
	return cwin->win;
}


static void clientwin_activated(WClientWin *cwin)
{
	install_cwin_cmap(cwin);
}


static void clientwin_resize_hints(WClientWin *cwin, XSizeHints *hints_ret,
								   uint *relw_ret, uint *relh_ret)
{
	*relw_ret=REGION_GEOM(cwin).w;
	*relh_ret=REGION_GEOM(cwin).h;
	*hints_ret=cwin->size_hints;
	
	adjust_size_hints_for_managed(hints_ret, cwin->transient_list);
}


static WRegion *clientwin_managed_enter_to_focus(WClientWin *cwin, WRegion *reg)
{
	return TOPMOST_TRANSIENT(cwin);
}


/*}}}*/


/*{{{ Names */


void set_clientwin_name(WClientWin *cwin, char *p)
{
	region_set_name(&cwin->region, p);
}


WClientWin *lookup_clientwin(const char *name)
{
	return (WClientWin*)do_lookup_region(name, &OBJDESCR(WClientWin));
}


int complete_clientwin(char *nam, char ***cp_ret, char **beg, void *unused)
{
	return do_complete_region(nam, cp_ret, beg, &OBJDESCR(WClientWin));
}


/*}}}*/


/*{{{ Misc */


WClientWin *find_clientwin(Window win)
{
	return FIND_WINDOW_T(win, WClientWin);
}


void clientwin_set_target_id(WClientWin *cwin, int id)
{
	if(id<=0)
		clientwin_clear_target_id(cwin);
	else
		set_integer_property(cwin->win, wglobal.atom_frame_id, id);
}


void clientwin_clear_target_id(WClientWin *cwin)
{
	XDeleteProperty(wglobal.dpy, cwin->win, wglobal.atom_frame_id);
}

						
/*}}}*/


/*{{{ ConfigureRequest */


void clientwin_handle_configure_request(WClientWin *cwin,
										XConfigureRequestEvent *ev)
{
	WRectangle geom;
	int dx=0, dy=0;
	int rx, ry;
	bool sz=FALSE, pos=FALSE;
	WWindow *par;
	
	/* check full screen request */
	/* TODO: viewport check */
	if((ev->value_mask&(CWX|CWY))==(CWX|CWY) &&
	   REGION_GEOM(SCREEN_OF(cwin)).w==ev->width &&
	   REGION_GEOM(SCREEN_OF(cwin)).h==ev->height){
		WMwmHints *mwm;
		mwm=get_mwm_hints(cwin->win);
		if(mwm!=NULL && mwm->flags&MWM_HINTS_DECORATIONS &&
		   mwm->decorations==0){
			if(clientwin_enter_fullscreen(cwin, REGION_IS_ACTIVE(cwin)))
				return;
		}
	}
	
	geom=REGION_GEOM(cwin);
	
	/* ConfigureRequest coordinates are coordinates of the window manager
	 * frame if such exists.
	 */
	par=FIND_PARENT1(cwin, WWindow);
	if(par==NULL || WTHING_IS(par, WScreen))
		region_rootpos((WRegion*)cwin, &rx, &ry);
	else
		region_rootpos((WRegion*)par, &rx, &ry);
	
	if(ev->value_mask&CWX){
		dx=ev->x-rx;
		geom.x+=dx;
		pos=TRUE;
	}
	
	if(ev->value_mask&CWY){
		dy=ev->y-ry;
		geom.y+=dy;
		pos=TRUE;
	}
	
	if(ev->value_mask&CWWidth){
		geom.w=ev->width;
		if(geom.w<1)
			geom.w=1;
		sz=TRUE;
	}
	
	if(ev->value_mask&CWHeight){
		geom.h=ev->height;
		if(geom.h<1)
			geom.h=1;
		sz=TRUE;
	}
	
	if(ev->value_mask&CWBorderWidth)
		cwin->orig_bw=ev->border_width;

	cwin->flags|=CWIN_NEED_CFGNTFY;
	
	if(sz || pos)
		region_request_geom((WRegion*)cwin, geom, NULL, FALSE);
	
	if(cwin->flags&CWIN_NEED_CFGNTFY){
		sendconfig_clientwin(cwin);
		cwin->flags&=~CWIN_NEED_CFGNTFY;
	}
}


/*}}}*/


/*{{{ Fullscreen */


bool clientwin_fullscreen_vp(WClientWin *cwin, WViewport *vp, bool switchto)
{
	if(REGION_MANAGER(cwin)!=NULL)
		setup_watch(&(cwin->last_mgr_watch), (WThing*)REGION_MANAGER(cwin),
					NULL);
	
	return region_add_managed((WRegion*)vp, (WRegion*)cwin,
							  switchto ?  REGION_ATTACH_SWITCHTO : 0);
}


bool clientwin_enter_fullscreen(WClientWin *cwin, bool switchto)
{
	WViewport *vp=viewport_of((WRegion*)cwin);
	
	if(vp==NULL)
		return FALSE;
	
	return clientwin_fullscreen_vp(cwin, vp, switchto);
}


bool clientwin_leave_fullscreen(WClientWin *cwin, bool switchto)
{	
	WRegion *reg;
	
	if(cwin->last_mgr_watch.thing==NULL)
		return FALSE;

	reg=(WRegion*)cwin->last_mgr_watch.thing;
	reset_watch(&(cwin->last_mgr_watch));
	if(!WTHING_IS(reg, WRegion))
		return FALSE;
	if(!region_supports_add_managed(reg))
		return FALSE;
	if(region_add_managed(reg, (WRegion*)cwin, REGION_ATTACH_SWITCHTO))
		return goto_region(reg);
	return FALSE;
}


bool clientwin_toggle_fullscreen(WClientWin *cwin)
{
	if(cwin->last_mgr_watch.thing!=NULL)
		return clientwin_leave_fullscreen(cwin, TRUE);
	
	return clientwin_enter_fullscreen(cwin, TRUE);
}


/*}}}*/


/*{{{ Kludges */


void clientwin_broken_app_resize_kludge(WClientWin *cwin)
{
	XResizeWindow(wglobal.dpy, cwin->win, 2*cwin->max_geom.w,
				  2*cwin->max_geom.h);
	XFlush(wglobal.dpy);
	XResizeWindow(wglobal.dpy, cwin->win, REGION_GEOM(cwin).w,
				  REGION_GEOM(cwin).h);
}


/*}}}*/

