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
#include "objp.h"
#include "global.h"
#include "property.h"
#include "focus.h"
#include "sizehint.h"
#include "hooks.h"
#include "event.h"
#include "clientwin.h"
#include "colormap.h"
#include "resize.h"
#include "attach.h"
#include "regbind.h"
#include "mwmhints.h"
#include "viewport.h"
#include "names.h"
#include "stacking.h"
#include "saveload.h"


static void set_clientwin_state(WClientWin *cwin, int state);
static void send_clientmsg(Window win, Atom a);


WHooklist *add_clientwin_alt=NULL;


/*{{{ Get properties */


void clientwin_get_protocols(WClientWin *cwin)
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


static void get_winprops(WClientWin *cwin)
{
	ExtlTab tab, tab2;
	int i1, i2;
	bool b;

	if(!extl_call_named("get_winprop", "o", "t", cwin, &tab))
		return;
	
	if(tab==extl_table_none())
		return;
	
	cwin->proptab=tab;

	if(extl_table_gets_b(tab, "transparent", &b)){
		if(b)
			cwin->flags|=CWIN_PROP_TRANSPARENT;
	}

	if(extl_table_gets_b(tab, "acrobatic", &b)){
		if(b)
			cwin->flags|=CWIN_PROP_ACROBATIC;
	}
	
	if(extl_table_gets_t(tab, "max_size", &tab2)){
		if(extl_table_gets_i(tab2, "w", &i1) &&
		   extl_table_gets_i(tab2, "h", &i2)){
			cwin->size_hints.max_width=i1;
			cwin->size_hints.max_height=i2;
			cwin->size_hints.flags|=PMaxSize;
			cwin->flags|=CWIN_PROP_MAXSIZE;
		}
		extl_unref_table(tab2);
	}

	if(extl_table_gets_t(tab, "aspect", &tab2)){
		if(extl_table_gets_i(tab2, "w", &i1) &&
		   extl_table_gets_i(tab2, "h", &i2)){
			cwin->size_hints.min_aspect.x=i1;
			cwin->size_hints.max_aspect.x=i1;
			cwin->size_hints.min_aspect.y=i2;
			cwin->size_hints.max_aspect.y=i2;
			cwin->size_hints.flags|=PAspect;
			cwin->flags|=CWIN_PROP_ASPECT;
		}
		extl_unref_table(tab2);
	}
	
	if(extl_table_gets_b(tab, "ignore_resizeinc", &b)){
		if(b)
			cwin->flags|=CWIN_PROP_IGNORE_RSZINC;
	}
}


void clientwin_get_size_hints(WClientWin *cwin)
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
	
	if(cwin->flags&CWIN_PROP_IGNORE_RSZINC)
		cwin->size_hints.flags&=~PResizeInc;
}


void clientwin_get_set_name(WClientWin *cwin)
{
	char **list=NULL;
#ifdef CF_UTF8
	list=get_text_property(cwin->win, wglobal.atom_net_wm_name);
	if(list==NULL)
		list=get_text_property(cwin->win, XA_WM_NAME);
	else
		cwin->flags|=CWIN_USE_NET_WM_NAME;
#else
	list=get_text_property(cwin->win, XA_WM_NAME);
#endif

	if(list==NULL){
		region_unuse_name((WRegion*)cwin);
	}else{
		stripws(*list);
		region_set_name((WRegion*)cwin, *list);
		XFreeStringList(list);
	}
}


/* Some standard winprops */

static int switch_to_new_clients=TRUE;

/*EXTL_DOC
 * Should newly created client window be switched to immediately or
 * should the active window retain focus by default?
 */
EXTL_EXPORT
void set_switch_to_new_clients(bool sw)
{
	switch_to_new_clients=sw;
}

bool clientwin_get_switchto(WClientWin *cwin)
{
	bool switchto=switch_to_new_clients;
	
	if(wglobal.opmode==OPMODE_INIT)
		return FALSE;
	
	extl_table_gets_b(cwin->proptab, "switchto", &switchto);
	
	return switchto;
}


int clientwin_get_transient_mode(WClientWin *cwin)
{
	char *s;
	int mode=TRANSIENT_MODE_NORMAL;
	
	if(extl_table_gets_s(cwin->proptab, "transient_mode", &s)){
		if(strcmp(s, "current")==0)
			mode=TRANSIENT_MODE_CURRENT;
		else if(strcmp(s, "off")==0)
			mode=TRANSIENT_MODE_OFF;
		free(s);
	}
	return mode;
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


static bool clientwin_init(WClientWin *cwin, WWindow *parent,
						   Window win, const XWindowAttributes *attr)
{
	WRectangle geom;
	char *name;
	
	geom.x=attr->x;
	geom.y=attr->y;
	geom.h=attr->height;
	geom.w=attr->width;
	
	region_init(&(cwin->region), &(parent->region), geom);
	
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

	get_winprops(cwin);

	init_watch(&(cwin->last_mgr_watch));
	
	get_colormaps(cwin);
	
	clientwin_get_size_hints(cwin);
	clientwin_get_protocols(cwin);

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
	CREATEOBJ_IMPL(WClientWin, clientwin, (p, parent, win, attr));
}


/* This is called when a window is mapped on the root window.
 * We want to check if we should manage the window and how and
 * act appropriately.
 */
WClientWin* manage_clientwin(Window win, int mflags)
{
	WScreen *scr;
	WClientWin *cwin=NULL;
	XWindowAttributes attr;
	XWMHints *hints;
	int state=NormalState;
	bool managed=FALSE;
	WAttachParams param;
	
	param.flags=0;

again:
	/* Is the window already being managed? */
	cwin=find_clientwin(win);
	if(cwin!=NULL)
		return cwin;
	
	/* Select for UnmapNotify and DestroyNotify as the
	 * window might get destroyed or unmapped in the meanwhile.
	 */
	XSelectInput(wglobal.dpy, win, StructureNotifyMask);

	
	/* Is it a dockapp?
	 */
	hints=XGetWMHints(wglobal.dpy, win);

	if(hints!=NULL && hints->flags&StateHint)
		state=hints->initial_state;
	
	if(!(param.flags&REGION_ATTACH_DOCKAPP) && state==WithdrawnState &&
	   hints->flags&IconWindowHint && hints->icon_window!=None){
		/* The dockapp might be displaying its "main" window if no
		 * wm that understands dockapps has been managing it.
		 */
		if(mflags&MANAGE_INITIAL)
			XUnmapWindow(wglobal.dpy, win);
		
		XSelectInput(wglobal.dpy, win, 0);
		
		win=hints->icon_window;
		
		/* It is a dock, do everything again from the beginning, now
		 * with the icon window.
		 */
		param.flags|=REGION_ATTACH_DOCKAPP;
		goto again;
	}
	
	if(hints!=NULL){
		if(hints->flags&StateHint){
			param.init_state=hints->initial_state;
			param.flags|=REGION_ATTACH_INITSTATE;
		}
		
		XFree((void*)hints);
	}

	if(!XGetWindowAttributes(wglobal.dpy, win, &attr)){
		warn("Window disappeared");
		goto fail2;
	}
	
	/* Get the actual state if any */
	/*get_win_state(win, &state);*/
	
	/* Do we really want to manage it? */
	if(!(param.flags&REGION_ATTACH_DOCKAPP) && (attr.override_redirect || 
		(mflags&MANAGE_INITIAL && attr.map_state!=IsViewable))){
		goto fail2;
	}

	scr=FIND_WINDOW_T(attr.root, WScreen);

	if(scr==NULL)
		goto fail2;

	/*if(state!=NormalState && state!=IconicState)
		state=NormalState;*/

	/* Set up some additional parameters */

	/*if(clientwin_get_switchto(cwin))
		param.flags|=REGION_ATTACH_SWITCHTO;
	
	if(param.flags&REGION_ATTACH_INITSTATE && param.init_state==IconicState)
		param.flags&=~REGION_ATTACH_SWITCHTO;*/

	/* Allocate and initialize */
	cwin=create_clientwin((WWindow*)scr, win, &attr);
	
	if(cwin==NULL)
		goto fail2;

	param.flags|=(REGION_ATTACH_SIZERQ|REGION_ATTACH_POSRQ|REGION_ATTACH_MAPRQ);
	param.geomrq=REGION_GEOM(cwin);

	CALL_ALT_B(managed, add_clientwin_alt, (cwin, &param));

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


static WRegion *clientwin_do_add_managed(WClientWin *cwin, 
										 WRegionAddFn *fn, void *fnparams, 
										 const WAttachParams *param)
{
	WRectangle geom=cwin->max_geom;
	WWindow *par=REGION_PARENT_CHK(cwin, WWindow);
	WRegion *reg;

	if(par==NULL)
		return NULL;
	
	if(param->flags&REGION_ATTACH_SIZERQ && param->geomrq.h>0){
		/* Don't increase the height of the transient */
		int diff=geom.h-param->geomrq.h;
		if(diff>0){
			geom.y+=diff;
			geom.h-=diff;
		}
	}else{
		/* Put something less than the full height for the size */
		geom.h/=3;
		geom.y+=geom.h*2;
	}
	
	reg=fn(par, geom, fnparams);
	
	if(reg==NULL)
		return NULL;
	
	region_set_manager(reg, (WRegion*)cwin, &(cwin->transient_list));
	region_stack_above(reg, (WRegion*)cwin);
	
	if(REGION_IS_MAPPED((WRegion*)cwin))
		region_map(reg);
	else
		region_unmap(reg);
	
	if(REGION_IS_ACTIVE(cwin))
		set_focus(reg);
	
	return reg;
}


static void clientwin_remove_managed(WClientWin *cwin, WRegion *transient)
{
	WRegion *reg;
	
	region_unset_manager(transient, (WRegion*)cwin, &(cwin->transient_list));
	region_reset_stacking(transient);
	
	if(REGION_IS_ACTIVE(transient)){
		reg=region_topmost_stacked_above((WRegion*)cwin);
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
	WWindow *par;
	Window dummy;
	int x=0, y=0;
	
	XGetWindowAttributes(wglobal.dpy, cwin->win, &attr);
	
	par=REGION_PARENT_CHK(cwin, WWindow);
	
	if(par==NULL || WOBJ_IS(par, WScreen)){
		x=REGION_GEOM(cwin).x;
		y=REGION_GEOM(cwin).y;
	}else{
		XTranslateCoordinates(wglobal.dpy, cwin->win, attr.root, 0, 0,
							  &x, &y, &dummy);
		x-=REGION_GEOM(cwin).x;
		y-=REGION_GEOM(cwin).y;
	}
	
	XReparentWindow(wglobal.dpy, cwin->win, attr.root, x, y);
}


void clientwin_deinit(WClientWin *cwin)
{
	WRegion *reg;
	
	UNLINK_ITEM(wglobal.cwin_list, cwin, g_cwin_next, g_cwin_prev);
	
	if(cwin->win!=None){
		XSelectInput(wglobal.dpy, cwin->win, 0);
		
		reparent_root(cwin);
		
		if(cwin->orig_bw!=0)
			configure_cwin_bw(cwin->win, cwin->orig_bw);
		
		XRemoveFromSaveSet(wglobal.dpy, cwin->win);
		XDeleteContext(wglobal.dpy, cwin->win, wglobal.win_context);
		
		if(wglobal.opmode==OPMODE_DEINIT){
			XMapWindow(wglobal.dpy, cwin->win);
			/* Make sure the topmost window has focus; it doesn't really
			 * matter which one has as long as some has.
			 */
			SET_FOCUS(cwin->win);
		}
	}
	clear_colormaps(cwin);
	
	reset_watch(&(cwin->last_mgr_watch));
	region_deinit((WRegion*)cwin);
}


/* Used when the window was unmapped */
void clientwin_unmapped(WClientWin *cwin)
{
	region_rescue_managed_on_list((WRegion*)cwin, cwin->transient_list);
	if(cwin->last_mgr_watch.obj!=NULL)
		region_goto((WRegion*)(cwin->last_mgr_watch.obj));
	destroy_obj((WObj*)cwin);
}


/* Used when the window was deastroyed */
void clientwin_destroyed(WClientWin *cwin)
{
	XDeleteContext(wglobal.dpy, cwin->win, wglobal.win_context);
	cwin->win=None;
	clientwin_unmapped(cwin);
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


/*EXTL_DOC
 * Attempt to kill (with XKillWindow) the client that owns the X
 * window correspoding to \var{cwin}.
 */
EXTL_EXPORT
void clientwin_kill(WClientWin *cwin)
{
	XKillClient(wglobal.dpy, cwin->win);
}


/*EXTL_DOC
 * Request the application that owns the X window corresponding to
 * \var{cwin} to close the window. This function will fail if the
 * application is not responding or does not support the WM\_DELETE
 * protocol. In that case \fnref{clientwin_kill} should be used.
 */
EXTL_EXPORT
void clientwin_close(WClientWin *cwin)
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
	   WOBJ_IS(REGION_MANAGER(cwin), WClientWin)){
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
		region_fit(sub, rgeom);
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
			region_fit(transient, geom2);
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


static void clientwin_fit(WClientWin *cwin, WRectangle geom)
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
	
	return TRUE;
}


static void clientwin_map(WClientWin *cwin)
{
	WRegion *sub;
	
	show_clientwin(cwin);
	MARK_REGION_MAPPED(cwin);
	
	FOR_ALL_MANAGED_ON_LIST(cwin->transient_list, sub){
		region_map(sub);
	}
}


static void clientwin_unmap(WClientWin *cwin)
{
	WRegion *sub;
	
	hide_clientwin(cwin);
	MARK_REGION_UNMAPPED(cwin);
	
	FOR_ALL_MANAGED_ON_LIST(cwin->transient_list, sub){
		region_unmap(sub);
	}
}


static void clientwin_set_focus_to(WClientWin *cwin, bool warp)
{
	WRegion *reg=region_topmost_stacked_above((WRegion*)cwin);
	
	if(warp)
		do_move_pointer_to((WRegion*)cwin);
	
	if(reg!=NULL){
		region_set_focus_to(reg, FALSE);
		return;
	}
	
	SET_FOCUS(cwin->win);
	
	if(cwin->flags&CWIN_P_WM_TAKE_FOCUS)
		send_clientmsg(cwin->win, wglobal.atom_wm_take_focus);
}


static bool clientwin_display_managed(WClientWin *cwin, WRegion *sub)
{
	if(!REGION_IS_MAPPED(cwin))
		return FALSE;
	region_map(sub);
	region_raise(sub);
	return TRUE;
}


static Window clientwin_restack(WClientWin *cwin, Window other, int mode)
{
	do_restack_window(cwin->win, other, mode);
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
	return region_topmost_stacked_above((WRegion*)cwin);
}


/*}}}*/


/*{{{ Names & lookup */


/*EXTL_DOC
 * Similar to \fnref{lookup_region} but only looks for objects of type
 * \type{WClientWin}.
 */
EXTL_EXPORT
WClientWin *lookup_clientwin(const char *name)
{
	return (WClientWin*)do_lookup_region(name, &OBJDESCR(WClientWin));
}


/*EXTL_DOC
 * Similar to \fnref{complete_region} but only includes objects of type
 * \type{WClientWin}.
 */
EXTL_EXPORT
ExtlTab complete_clientwin(const char *nam)
{
	return do_complete_region(nam, &OBJDESCR(WClientWin));
}


/*EXTL_DOC
 * Returns a table containing the class, instance and role properties
 * for \var{cwin}.
 */
EXTL_EXPORT
ExtlTab clientwin_get_ident(WClientWin *cwin)
{
	char *winstance=NULL, *wclass=NULL, *wrole=NULL;
	int n=0, n2=0, tmp=0;
	ExtlTab tab;
	
	winstance=get_string_property(cwin->win, XA_WM_CLASS, &n);
	wrole=get_string_property(cwin->win, wglobal.atom_wm_window_role, &n2);
	
	if(winstance!=NULL){
		tmp=strlen(winstance);
		if(tmp+1<n)
			wclass=winstance+tmp+1;
	}
	
	tab=extl_create_table();
	if(wclass!=NULL)
		extl_table_sets_s(tab, "class", wclass);
	if(winstance!=NULL)
		extl_table_sets_s(tab, "instance", winstance);
	if(wrole!=NULL)
		extl_table_sets_s(tab, "role", wrole);
	
	if(winstance!=NULL)
		free(winstance);
	if(wrole!=NULL)
		free(wrole);
	
	return tab;
}


WClientWin *find_clientwin(Window win)
{
	return FIND_WINDOW_T(win, WClientWin);
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
	if((ev->value_mask&(CWWidth|CWHeight))==(CWWidth|CWHeight)){
		if(clientwin_check_fullscreen_request(cwin, ev->width, ev->height))
			return;
	}
	
	geom=REGION_GEOM(cwin);
	
	/* ConfigureRequest coordinates are coordinates of the window manager
	 * frame if such exists.
	 */
	par=REGION_PARENT_CHK(cwin, WWindow);
	if(par==NULL || WOBJ_IS(par, WScreen))
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


bool clientwin_check_fullscreen_request(WClientWin *cwin, int w, int h)
{
	WRegion *reg;
	WMwmHints *mwm;
	
	mwm=get_mwm_hints(cwin->win);
	if(mwm==NULL || !(mwm->flags&MWM_HINTS_DECORATIONS) ||
	   mwm->decorations!=0)
		return FALSE;
	
	FOR_ALL_MANAGED_ON_LIST(SCREEN_OF(cwin)->viewport_list, reg){
		if(!WOBJ_IS(reg, WViewport))
			continue;
		if(REGION_GEOM(reg).w==w && REGION_GEOM(reg).h==h){
			return clientwin_fullscreen_vp(cwin, (WViewport*)reg,
										   clientwin_get_switchto(cwin));
		}
	}
	
	/* Catch Xinerama-unaware apps here */
	if(REGION_GEOM(SCREEN_OF(cwin)).w==w && REGION_GEOM(SCREEN_OF(cwin)).h==h){
		return clientwin_enter_fullscreen(cwin, 
										  clientwin_get_switchto(cwin));
	}
	return FALSE;
}


bool clientwin_fullscreen_vp(WClientWin *cwin, WViewport *vp, bool switchto)
{
	if(REGION_MANAGER(cwin)!=NULL){
		setup_watch(&(cwin->last_mgr_watch), (WObj*)REGION_MANAGER(cwin), NULL);
	}else if(vp->current_ws!=NULL){
		setup_watch(&(cwin->last_mgr_watch), (WObj*)vp->current_ws, NULL);
	}
	
	if(!region_add_managed_simple((WRegion*)vp, (WRegion*)cwin, switchto ? 
								  REGION_ATTACH_SWITCHTO : 0)){
		warn("Failed to enter full screen mode");
		return FALSE;
	}
	
	return TRUE;
}


bool clientwin_enter_fullscreen(WClientWin *cwin, bool switchto)
{
	WViewport *vp=region_viewport_of((WRegion*)cwin);
	
	if(vp==NULL){
		vp=SCREEN_OF(cwin)->current_viewport;
		if(vp==NULL)
			return FALSE;
	}
	
	return clientwin_fullscreen_vp(cwin, vp, switchto);
}


bool clientwin_leave_fullscreen(WClientWin *cwin, bool switchto)
{	
	WRegion *reg;
	
	if(cwin->last_mgr_watch.obj==NULL)
		return FALSE;

	reg=(WRegion*)cwin->last_mgr_watch.obj;
	reset_watch(&(cwin->last_mgr_watch));
	if(!WOBJ_IS(reg, WRegion))
		return FALSE;
	if(!region_supports_add_managed(reg))
		return FALSE;
	/* TODO: geomrq for correct placement on floatws:s */
	if(!region_add_managed_simple(reg, (WRegion*)cwin, REGION_ATTACH_SWITCHTO))
		return FALSE;
	return region_goto(reg);
}


/*EXTL_DOC
 * Toggle between full screen and normal (framed) mode.
 */
EXTL_EXPORT
bool clientwin_toggle_fullscreen(WClientWin *cwin)
{
	if(cwin->last_mgr_watch.obj!=NULL)
		return clientwin_leave_fullscreen(cwin, TRUE);
	
	return clientwin_enter_fullscreen(cwin, TRUE);
}


/*}}}*/


/*{{{ Kludges */


/*EXTL_DOC
 * Attempts to fix window size problems with non-ICCCM compliant
 * programs.
 */
EXTL_EXPORT
void clientwin_broken_app_resize_kludge(WClientWin *cwin)
{
	XResizeWindow(wglobal.dpy, cwin->win, 2*cwin->max_geom.w,
				  2*cwin->max_geom.h);
	XFlush(wglobal.dpy);
	XResizeWindow(wglobal.dpy, cwin->win, REGION_GEOM(cwin).w,
				  REGION_GEOM(cwin).h);
}


/*}}}*/


/*{{{ Save/load */


static int last_checkcode=1;

static bool clientwin_save_to_file(WClientWin *cwin, FILE *file, int lvl)
{
	WRegion *sub;
	int chkc=0;

	begin_saved_region((WRegion*)cwin, file, lvl);
	save_indent_line(file, lvl);
	fprintf(file, "windowid = %.32f,\n", (double)(cwin->win));
	save_indent_line(file, lvl);
	
	if(last_checkcode!=0){
		chkc=last_checkcode++;
		set_integer_property(cwin->win, wglobal.atom_checkcode, chkc);
		fprintf(file, "checkcode = %d,\n", chkc);
	}
	/*save_indent_line(file, lvl);
	fprintf(file, "subs = {\n");
	FOR_ALL_MANAGED_ON_LIST(cwin->transient_list, sub){
		region_save_to_file((WRegion*)sub, file, lvl+1);
	}*/
	
	return TRUE;
}


WRegion *clientwin_load(WWindow *par, WRectangle geom, ExtlTab tab)
{
	double wind=0;
	Window win=None;
	int chkc=0, real_chkc=0;
	WClientWin *cwin=NULL;
	XWindowAttributes attr;
	/*ExtlTab substab, subtab;
	int n, i;*/

	if(!extl_table_gets_d(tab, "windowid", &wind) ||
	   !extl_table_gets_i(tab, "checkcode", &chkc)){
		return NULL;
	}
	
	win=(Window)wind;

	if(!get_integer_property(win, wglobal.atom_checkcode, &real_chkc))
		return NULL;
	
	set_integer_property(win, wglobal.atom_checkcode, 0);

	if(real_chkc!=chkc || chkc==0){
		warn("Client window check code mismatch.");
		return NULL;
	}

	/* Found it! */
	
	if(!XGetWindowAttributes(wglobal.dpy, win, &attr)){
		warn("Window disappeared");
		return NULL;
	}
	
	if(attr.override_redirect || 
	   (wglobal.opmode==OPMODE_INIT && attr.map_state!=IsViewable)){
		warn("Saved client window does not want to be managed");
		return NULL;
	}

	attr.x=geom.x;
	attr.y=geom.y;
	attr.width=geom.w;
	attr.height=geom.h;

	cwin=create_clientwin(par, win, &attr);
	
	if(cwin!=NULL){
		/* Reparent and resize taking limits set by size hints into account */
		convert_geom(cwin, geom, &geom);
		REGION_GEOM(cwin)=geom;
		do_reparent_clientwin(cwin, par->win, geom.x, geom.y);
		XResizeWindow(wglobal.dpy, win, geom.w, geom.h);
	}

	/* TODO: checks that the window still is there */
	
	/*if(extl_table_gets_t(tab, "subs", &substab)){
		n=extl_table_get_n(substab);
		for(i=1; i<=n; i++){
			if(extl_table_geti_t(substab, i, &subtab)){
				region_add_managed_load((WRegion*)frame, subtab);
				extl_unref_table(subtab);
			}
		}
		extl_unref_table(substab);
	}*/
	
	return (WRegion*)cwin;
}


/*}}}*/


/*{{{ Dynfuntab and class info */


static DynFunTab clientwin_dynfuntab[]={
	{region_fit, clientwin_fit},
	{(DynFun*)reparent_region, (DynFun*)reparent_clientwin},
	{region_map, clientwin_map},
	{region_unmap, clientwin_unmap},
	{region_set_focus_to, clientwin_set_focus_to},
	{(DynFun*)region_display_managed, (DynFun*)clientwin_display_managed},
	{region_notify_rootpos, clientwin_notify_rootpos},
	{(DynFun*)region_restack, (DynFun*)clientwin_restack},
	{(DynFun*)region_x_window, (DynFun*)clientwin_x_window},
	{region_activated, clientwin_activated},
	{region_resize_hints, clientwin_resize_hints},
	{(DynFun*)region_managed_enter_to_focus,
	 (DynFun*)clientwin_managed_enter_to_focus},
	{region_remove_managed, clientwin_remove_managed},
	{(DynFun*)region_do_add_managed, (DynFun*)clientwin_do_add_managed},
	{region_request_managed_geom, clientwin_request_managed_geom},
	{region_close, clientwin_close},
	{(DynFun*)region_save_to_file, (DynFun*)clientwin_save_to_file},
	END_DYNFUNTAB
};


IMPLOBJ(WClientWin, WRegion, clientwin_deinit, clientwin_dynfuntab);


/*}}}*/
