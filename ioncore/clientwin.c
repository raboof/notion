/*
 * ion/ioncore/clientwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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
#include "names.h"
#include "stacking.h"
#include "saveload.h"
#include "manage.h"
#include "extl.h"
#include "extlconv.h"
#include "fullscreen.h"
#include "event.h"
#include "rootwin.h"
#include "activity.h"
#include "netwm.h"


static void set_clientwin_state(WClientWin *cwin, int state);
static bool send_clientmsg(Window win, Atom a, Time stmp);


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


static void clientwin_get_winprops(WClientWin *cwin)
{
	ExtlTab tab, tab2;
	int i1, i2;

	if(!extl_call_named("get_winprop", "o", "t", cwin, &tab))
		return;
	
	cwin->proptab=tab;
	
	if(tab==extl_table_none())
		return;

	if(extl_table_is_bool_set(tab, "transparent"))
		cwin->flags|=CWIN_PROP_TRANSPARENT;

	if(extl_table_is_bool_set(tab, "acrobatic"))
		cwin->flags|=CWIN_PROP_ACROBATIC;
	
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

	if(extl_table_gets_t(tab, "min_size", &tab2)){
		if(extl_table_gets_i(tab2, "w", &i1) &&
		   extl_table_gets_i(tab2, "h", &i2)){
			cwin->size_hints.min_width=i1;
			cwin->size_hints.min_height=i2;
			cwin->size_hints.flags|=PMinSize;
			cwin->flags|=CWIN_PROP_MINSIZE;
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
	
	if(extl_table_is_bool_set(tab, "ignore_resizeinc"))
		cwin->flags|=CWIN_PROP_IGNORE_RSZINC;

	if(extl_table_is_bool_set(tab, "ignore_cfgrq"))
		cwin->flags|=CWIN_PROP_IGNORE_CFGRQ;
}


void clientwin_get_size_hints(WClientWin *cwin)
{
	XSizeHints tmp=cwin->size_hints;
	
	get_sizehints(cwin->win, &(cwin->size_hints));
	
	if(cwin->flags&CWIN_PROP_MAXSIZE){
		cwin->size_hints.max_width=tmp.max_width;
		cwin->size_hints.max_height=tmp.max_height;
		cwin->size_hints.flags|=PMaxSize;
	}

	if(cwin->flags&CWIN_PROP_MINSIZE){
		cwin->size_hints.min_width=tmp.min_width;
		cwin->size_hints.min_height=tmp.min_height;
		cwin->size_hints.flags|=PMinSize;
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
	int n=0;
	
	if(wglobal.use_mb && wglobal.atom_net_wm_name!=0)
		list=get_text_property(cwin->win, wglobal.atom_net_wm_name, NULL);

	if(list==NULL){
		list=get_text_property(cwin->win, XA_WM_NAME, &n);
	}else{
		cwin->flags|=CWIN_USE_NET_WM_NAME;
	}

	if(list==NULL){
		if(n==-1){
			/* Special condition kludge: property exists, but couldn't
			 * be converted to a string list.
			 */
			clientwin_set_name(cwin, "???");
		}else{
			region_unuse_name((WRegion*)cwin);
		}
	}else{
		clientwin_set_name(cwin, *list);
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
	bool b;
	
	if(wglobal.opmode==OPMODE_INIT)
		return FALSE;
	
	if(extl_table_gets_b(cwin->proptab, "switchto", &b))
		return b;
	
	return switch_to_new_clients;
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


static bool clientwin_init(WClientWin *cwin, WRegion *parent,
						   Window win, XWindowAttributes *attr)
{
	WRectangle geom;
	char *name;

	cwin->flags=0;
	cwin->win=win;
	cwin->state=WithdrawnState;
	
	geom.x=attr->x;
	geom.y=attr->y;
	geom.w=attr->width;
	geom.h=attr->height;
	
	/* The idiot who invented special server-supported window borders that
	 * are not accounted for in the window size should be "taken behind a
	 * sauna".
	 */
	cwin->orig_bw=attr->border_width;
	configure_cwin_bw(cwin->win, 0);
	if(cwin->orig_bw!=0 && cwin->size_hints.flags&PWinGravity){
		geom.x+=gravity_deltax(cwin->size_hints.win_gravity, 
							   -cwin->orig_bw, -cwin->orig_bw);
		geom.y+=gravity_deltay(cwin->size_hints.win_gravity, 
							   -cwin->orig_bw, -cwin->orig_bw);
	}

	cwin->max_geom=geom;
	cwin->last_h_rq=geom.h;

	cwin->transient_for=None;
	cwin->transient_list=NULL;
	
	cwin->n_cmapwins=0;
	cwin->cmap=attr->colormap;
	cwin->cmaps=NULL;
	cwin->cmapwins=NULL;
	cwin->n_cmapwins=0;
	cwin->event_mask=CLIENT_MASK;

	init_watch(&(cwin->fsinfo.last_mgr_watch));
	
	region_init(&(cwin->region), parent, &geom);
	
	clientwin_get_colormaps(cwin);
	clientwin_get_protocols(cwin);
	clientwin_get_set_name(cwin);
	clientwin_get_winprops(cwin);
	clientwin_get_size_hints(cwin);
	
	XSelectInput(wglobal.dpy, win, cwin->event_mask);

	XSaveContext(wglobal.dpy, win, wglobal.win_context, (XPointer)cwin);
	XAddToSaveSet(wglobal.dpy, win);

	LINK_ITEM(wglobal.cwin_list, cwin, g_cwin_next, g_cwin_prev);
	
	return TRUE;
}


static WClientWin *create_clientwin(WRegion *parent, Window win,
									XWindowAttributes *attr)
{
	CREATEOBJ_IMPL(WClientWin, clientwin, (p, parent, win, attr));
}


static bool handle_target_props(WClientWin *cwin, const WManageParams *param)
{
	WRegion *r=NULL;
	char *target_name=NULL;
	
	if(extl_table_gets_s(cwin->proptab, "target", &target_name)){
		r=lookup_region(target_name, NULL);
		
		free(target_name);
	
		if(r!=NULL){
			if(region_has_manage_clientwin(r))
				if(region_manage_clientwin(r, cwin, param))
					return TRUE;
		}
	}
	
	if(!extl_table_is_bool_set(cwin->proptab, "fullscreen"))
			return FALSE;
	
	return clientwin_enter_fullscreen(cwin, param->switchto);
}


WClientWin *clientwin_get_transient_for(WClientWin *cwin)
{
	Window tforwin;
	WClientWin *tfor=NULL;
	
	if(clientwin_get_transient_mode(cwin)!=TRANSIENT_MODE_NORMAL)
		return NULL;

	if(!XGetTransientForHint(wglobal.dpy, cwin->win, &tforwin))
		return NULL;
	
	if(tforwin==None)
		return NULL;
	
	tfor=find_clientwin(tforwin);
	
	if(tfor==cwin){
		warn("The transient_for hint for \"%s\" points to itself.",
			 region_name((WRegion*)cwin));
	}else if(tfor==NULL){
		if(find_window(tforwin)!=NULL){
			warn("Client window \"%s\" has broken transient_for hint. "
				 "(\"Extended WM hints\" multi-parent brain damage?)",
				 region_name((WRegion*)cwin));
		}
	}else if(!same_rootwin((WRegion*)cwin, (WRegion*)tfor)){
		warn("The transient_for window for \"%s\" is not on the same "
			 "screen.", region_name((WRegion*)cwin));
	}else{
		return tfor;
	}
	
	return NULL;
}


static WClientWin *postmanage_check(WClientWin *cwin,
									XWindowAttributes *attr)
{
	/* Check that the window exists. The previous check and selectinput
	 * do not seem to catch all cases of window destroyal.
	 */
	XSync(wglobal.dpy, False);
	
	if(XGetWindowAttributes(wglobal.dpy, cwin->win, attr))
		return cwin;
	
	warn("Window %#x disappeared", cwin->win);
	
	clientwin_destroyed(cwin);
	return NULL;
}


/* This is called when a window is mapped on the root window.
 * We want to check if we should manage the window and how and
 * act appropriately.
 */
WClientWin* manage_clientwin(Window win, int mflags)
{
	WRootWin *rootwin;
	WClientWin *cwin=NULL;
	XWindowAttributes attr;
	XWMHints *hints;
	int init_state=NormalState;
	WManageParams param=INIT_WMANAGEPARAMS;

	param.dockapp=FALSE;
	
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
		init_state=hints->initial_state;
	
	if(!param.dockapp && init_state==WithdrawnState && 
	   hints->flags&IconWindowHint && hints->icon_window!=None){
		/* The dockapp might be displaying its "main" window if no
		 * wm that understands dockapps has been managing it.
		 */
		if(mflags&MANAGE_INITIAL)
			XUnmapWindow(wglobal.dpy, win);
		
		XSelectInput(wglobal.dpy, win, 0);
		
		win=hints->icon_window;
		
		/* It is a dockapp, do everything again from the beginning, now
		 * with the icon window.
		 */
		param.dockapp=TRUE;
		goto again;
	}
	
	if(hints!=NULL)
		XFree((void*)hints);

	if(!XGetWindowAttributes(wglobal.dpy, win, &attr)){
		if(!(mflags&MANAGE_INITIAL))
			warn("Window %#x disappeared", win);
		goto fail2;
	}
	
	/* Do we really want to manage it? */
	if(!param.dockapp && (attr.override_redirect || 
		(mflags&MANAGE_INITIAL && attr.map_state!=IsViewable))){
		goto fail2;
	}

	rootwin=find_rootwin_for_root(attr.root);

	if(rootwin==NULL){
		warn("Unable to find a matching root window!");
		goto fail2;
	}

	/* Allocate and initialize */
	cwin=create_clientwin((WRegion*)rootwin, win, &attr);
	
	if(cwin==NULL)
		goto fail2;

	param.geom=REGION_GEOM(cwin);
	param.maprq=TRUE;
	param.userpos=(cwin->size_hints.flags&USPosition);
	param.switchto=(init_state!=IconicState && clientwin_get_switchto(cwin));
	param.jumpto=extl_table_is_bool_set(cwin->proptab, "jumpto");
	param.gravity=(cwin->size_hints.flags&PWinGravity
				   ? cwin->size_hints.win_gravity
				   : ForgetGravity);
	
	param.tfor=clientwin_get_transient_for(cwin);

	if(!handle_target_props(cwin, &param)){
		bool managed=FALSE;
		
		CALL_ALT_B(managed, add_clientwin_alt, (cwin, &param));

		if(!managed){
			warn("Unable to manage client window %d\n", win);
			goto failure;
		}
	}
	
	if(!region_is_fully_mapped((WRegion*)cwin) && 
	   wglobal.opmode==OPMODE_NORMAL){
		region_notify_activity((WRegion*)cwin);
	}
	
	
	if(postmanage_check(cwin, &attr)){
		if(param.jumpto && wglobal.focus_next==NULL)
			region_goto((WRegion*)cwin);
		extl_call_named("call_hook", "so", NULL, "clientwin_added", cwin);
		return cwin;
	}

failure:
	clientwin_unmapped(cwin);

fail2:
	XSelectInput(wglobal.dpy, win, 0);
	return NULL;
}


/*}}}*/


/*{{{ Add/remove managed */


static WRegion *clientwin_do_attach_transient(WClientWin *cwin, 
											  WRegionAttachHandler *fn,
											  void *fnparams, 
											  int *h)
{
	WRectangle geom=cwin->max_geom;
	WWindow *par=REGION_PARENT_CHK(cwin, WWindow);
	WRegion *reg;

	if(par==NULL)
		return NULL;
	
	if(*h>0){
		/* Don't increase the height of the transient */
		int diff=geom.h-*h;
		if(diff>0){
			geom.y+=diff;
			geom.h-=diff;
		}
	}else{
		/* Put something less than the full height for the size */
		geom.h/=3;
		geom.y+=geom.h*2;
	}
	
	reg=fn(par, &geom, fnparams);
	
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


bool clientwin_attach_transient(WClientWin *cwin, WRegion *transient)
{
	int h=REGION_GEOM(transient).h;
	
	return attach_reparent_helper((WRegion*)cwin, transient,
								  ((WRegionDoAttachFn*)
								   clientwin_do_attach_transient), 
								  &h);
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


static bool clientwin_do_rescue_clientwins(WClientWin *cwin, WRegion *dst)
{
	bool ret1, ret2;
	
	ret1=region_do_rescue_managed_clientwins((WRegion*)cwin, dst,
											 cwin->transient_list);
	ret2=region_do_rescue_child_clientwins((WRegion*)cwin, dst);
	
	return (ret1 && ret2);
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
	
	if(par==NULL){
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
	
	while(cwin->transient_list!=NULL)
		destroy_obj((WObj*)(cwin->transient_list));
	
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
		}else{
			set_clientwin_state(cwin, WithdrawnState);
			XDeleteProperty(wglobal.dpy, cwin->win, 
							wglobal.atom_net_wm_state);
		}
	}
	clientwin_clear_colormaps(cwin);
	
	reset_watch(&(cwin->fsinfo.last_mgr_watch));
	region_deinit((WRegion*)cwin);
}


/* Used when the window was unmapped */
void clientwin_unmapped(WClientWin *cwin)
{
	bool cf=region_may_control_focus((WRegion*)cwin);
	region_rescue_clientwins((WRegion*)cwin);
	if(cf && cwin->fsinfo.last_mgr_watch.obj!=NULL)
		region_goto((WRegion*)(cwin->fsinfo.last_mgr_watch.obj));
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


static bool send_clientmsg(Window win, Atom a, Time stmp)
{
	XClientMessageEvent ev;
	
	ev.type=ClientMessage;
	ev.window=win;
	ev.message_type=wglobal.atom_wm_protocols;
	ev.format=32;
	ev.data.l[0]=a;
	ev.data.l[1]=stmp;
	
	return (XSendEvent(wglobal.dpy, win, False, 0L, (XEvent*)&ev)!=0);
}


/*EXTL_DOC
 * Attempt to kill (with XKillWindow) the client that owns the X
 * window correspoding to \var{cwin}.
 */
EXTL_EXPORT_MEMBER
void clientwin_kill(WClientWin *cwin)
{
	XKillClient(wglobal.dpy, cwin->win);
}


/*EXTL_DOC
 * Request the application that owns the X window corresponding to
 * \var{cwin} to close the window. This function will fail if the
 * application is not responding or does not support the WM\_DELETE
 * protocol. In that case \fnref{WClientWin.kill} should be used.
 */
EXTL_EXPORT_MEMBER
void clientwin_close(WClientWin *cwin)
{
	if(cwin->flags&CWIN_P_WM_DELETE)
		send_clientmsg(cwin->win, wglobal.atom_wm_delete, get_timestamp());
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
	ce.xconfigure.x=rootx-cwin->orig_bw;
	ce.xconfigure.y=rooty-cwin->orig_bw;
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


static void convert_geom(WClientWin *cwin, const WRectangle *max_geom,
						 WRectangle *geom)
{
	WRectangle r;
	bool bottom=FALSE;
	int htry=max_geom->h;
	
	/* Align transients managed by another client window at bottom. */
	/*if(cwin->transient_for!=None){*/
	if(REGION_MANAGER(cwin)!=NULL &&
	   WOBJ_IS(REGION_MANAGER(cwin), WClientWin)){
		bottom=TRUE;
		if(max_geom->h>cwin->last_h_rq)
			htry=cwin->last_h_rq;
	}
	
	geom->w=max_geom->w;
	geom->h=htry;
	
	/* Don't ignore minimum size at first try. */
	if(bottom)
		correct_size(&(geom->w), &(geom->h), &(cwin->size_hints), TRUE);
	if(!bottom || geom->w>max_geom->w || geom->h>max_geom->h){
		geom->w=max_geom->w;
		geom->h=htry;
		correct_size(&(geom->w), &(geom->h), &(cwin->size_hints), FALSE);
	}

	geom->x=max_geom->x+max_geom->w/2-geom->w/2;
	
	if(bottom)
		geom->y=max_geom->y+max_geom->h-geom->h;
	else
		geom->y=max_geom->y+max_geom->h/2-geom->h/2;
	
	if(geom->h<=1)
		geom->h=1;
	if(geom->w<=1)
		geom->w=1;
}


/*}}}*/


/*{{{ Region dynfuns */


static void clientwin_request_managed_geom(WClientWin *cwin, WRegion *sub,
										   int flags, const WRectangle *geom, 
										   WRectangle *geomret)
{
	WRectangle rgeom=cwin->max_geom;

	if(geomret!=NULL)
		*geomret=rgeom;
	
	if(!(flags&REGION_RQGEOM_TRYONLY))
		region_fit(sub, &rgeom);
}



static void do_fit_clientwin(WClientWin *cwin, const WRectangle *max_geom, 
							 WWindow *np)
{
	WRegion *transient, *next;
	WRectangle geom;
	int diff;
	bool changes;
	
	convert_geom(cwin, max_geom, &geom);

	changes=(REGION_GEOM(cwin).x!=geom.x ||
			 REGION_GEOM(cwin).y!=geom.y ||
			 REGION_GEOM(cwin).w!=geom.w ||
			 REGION_GEOM(cwin).h!=geom.h);
	
	cwin->max_geom=*max_geom;
	REGION_GEOM(cwin)=geom;
	
	if(np==NULL && !changes)
		return;
	
	if(np!=NULL){
		region_detach_parent((WRegion*)cwin);
		do_reparent_clientwin(cwin, np->win, geom.x, geom.y);
		region_attach_parent((WRegion*)cwin, (WRegion*)np);
	}
	
	if(cwin->flags&CWIN_PROP_ACROBATIC && !REGION_IS_MAPPED(cwin)){
		XMoveResizeWindow(wglobal.dpy, cwin->win,
						  -2*max_geom->w, -2*max_geom->h, geom.w, geom.h);
	}else{
		XMoveResizeWindow(wglobal.dpy, cwin->win,
						  geom.x, geom.y, geom.w, geom.h);
	}
	
	cwin->flags&=~CWIN_NEED_CFGNTFY;
	
	FOR_ALL_MANAGED_ON_LIST_W_NEXT(cwin->transient_list, transient, next){
		if(np==NULL){
			region_fit(transient, max_geom);
		}else{
			if(!reparent_region(transient, np, max_geom)){
				warn("Problem: can't reparent a %s managed by a WClientWin"
					 "being reparented. Detaching from this object.",
					 WOBJ_TYPESTR(transient));
				region_detach_manager(transient);
			}
			region_stack_above(transient, (WRegion*)cwin);
		}
	}
}


static void clientwin_fit(WClientWin *cwin, const WRectangle *geom)
{
	do_fit_clientwin(cwin, geom, NULL);
}


static bool reparent_clientwin(WClientWin *cwin, WWindow *par, 
							   const WRectangle *geom)
{
	int rootx, rooty;
	
	if(!same_rootwin((WRegion*)cwin, (WRegion*)par))
		return FALSE;
	
	do_fit_clientwin(cwin, geom, par);
	sendconfig_clientwin(cwin);

	if(!CLIENTWIN_IS_FULLSCREEN(cwin) && 
	   cwin->fsinfo.last_mgr_watch.obj!=NULL){
		reset_watch(&(cwin->fsinfo.last_mgr_watch));
	}
	
	netwm_update_state(cwin);

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
	Time stmp;
	
	if(warp)
		do_move_pointer_to((WRegion*)cwin);
	
	if(reg!=NULL){
		region_set_focus_to(reg, FALSE);
		return;
	}

	if(cwin->flags&CWIN_P_WM_TAKE_FOCUS){
		stmp=get_timestamp();
		send_clientmsg(cwin->win, wglobal.atom_wm_take_focus, stmp);
	}
	XSetInputFocus(wglobal.dpy, cwin->win, RevertToParent, CurrentTime);
	XSync(wglobal.dpy, 0);
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
	if(relw_ret!=NULL)
		*relw_ret=REGION_GEOM(cwin).w;
	if(relh_ret!=NULL)
		*relh_ret=REGION_GEOM(cwin).h;
	*hints_ret=cwin->size_hints;
	
	adjust_size_hints_for_managed(hints_ret, cwin->transient_list);
}


static WRegion *clientwin_managed_enter_to_focus(WClientWin *cwin, WRegion *reg)
{
	return region_topmost_stacked_above((WRegion*)cwin);
}


/*}}}*/


/*{{{ Identity & lookup */


/*EXTL_DOC
 * Returns a table containing the class, instance and role properties
 * for \var{cwin}.
 */
EXTL_EXPORT_MEMBER
ExtlTab clientwin_get_ident(WClientWin *cwin)
{
	char **p=NULL, *wrole=NULL;
	int n=0, n2=0, tmp=0;
	ExtlTab tab;
	
	p=get_text_property(cwin->win, XA_WM_CLASS, &n);
	wrole=get_string_property(cwin->win, wglobal.atom_wm_window_role, &n2);
	
	tab=extl_create_table();
	if(n>=2 && p[1]!=NULL)
		extl_table_sets_s(tab, "class", p[1]);
	if(n>=1 && p[0]!=NULL)
		extl_table_sets_s(tab, "instance", p[0]);
	if(wrole!=NULL)
		extl_table_sets_s(tab, "role", wrole);
	
	if(p!=NULL)
		XFreeStringList(p);
	
	return tab;
}


WClientWin *find_clientwin(Window win)
{
	return FIND_WINDOW_T(win, WClientWin);
}


/*}}}*/


/*{{{ ConfigureRequest */


#undef MAX
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))


void clientwin_handle_configure_request(WClientWin *cwin,
										XConfigureRequestEvent *ev)
{
	if(ev->value_mask&CWBorderWidth)
		cwin->orig_bw=ev->border_width;
	
	if(cwin->flags&CWIN_PROP_IGNORE_CFGRQ){
		sendconfig_clientwin(cwin);
		return;
	}

	/* check full screen request */
	if((ev->value_mask&(CWWidth|CWHeight))==(CWWidth|CWHeight)){
		bool sw=region_may_control_focus((WRegion*)cwin);
		if(clientwin_check_fullscreen_request(cwin, ev->width, ev->height, sw))
			return;
	}

	cwin->flags|=CWIN_NEED_CFGNTFY;

	if(ev->value_mask&(CWX|CWY|CWWidth|CWHeight)){
		WRectangle geom;
		WRegion *mgr;
		int rqflags=REGION_RQGEOM_WEAK_ALL;
		int gdx=0, gdy=0;

		/* Do I need to insert another disparaging comment on the person who
		 * invented special server-supported window borders that are not 
		 * accounted for in the window size? Keep it simple, stupid!
		 */
		if(cwin->size_hints.flags&PWinGravity){
			gdx=gravity_deltax(cwin->size_hints.win_gravity, 
							   -cwin->orig_bw, -cwin->orig_bw);
			gdy=gravity_deltay(cwin->size_hints.win_gravity, 
							   -cwin->orig_bw, -cwin->orig_bw);
		}
		
		/* Rootpos is usually wrong here, but managers (frames) that respect
		 * position at all, should define region_request_clientwin_geom to
		 * handle this how they see fit.
		 */
		region_rootpos((WRegion*)cwin, &(geom.x), &(geom.y));
		geom.w=REGION_GEOM(cwin).w;
		geom.h=REGION_GEOM(cwin).h;
		
		if(ev->value_mask&CWWidth){
			/* If x was not changed, keep reference point where it was */
			if(cwin->size_hints.flags&PWinGravity){
				geom.x+=gravity_deltax(cwin->size_hints.win_gravity, 0,
									   ev->width-geom.w);
			}
			geom.w=MAX(ev->width, 1);
			rqflags&=~REGION_RQGEOM_WEAK_W;
		}
		if(ev->value_mask&CWHeight){
			/* If y was not changed, keep reference point where it was */
			if(cwin->size_hints.flags&PWinGravity){
				geom.y+=gravity_deltay(cwin->size_hints.win_gravity, 0,
									   ev->height-geom.h);
			}
			geom.h=MAX(ev->height, 1);
			cwin->last_h_rq=geom.h;
			rqflags&=~REGION_RQGEOM_WEAK_H;
		}
		if(ev->value_mask&CWX){
			geom.x=ev->x+gdx;
			rqflags&=~REGION_RQGEOM_WEAK_X;
		}
		if(ev->value_mask&CWY){
			geom.y=ev->y+gdy;
			rqflags&=~REGION_RQGEOM_WEAK_Y;
		}
		
		mgr=region_manager((WRegion*)cwin);
		if(mgr!=NULL && HAS_DYN(mgr, region_request_clientwin_geom)){
			/* Manager gets to decide how to handle position request. */
			region_request_clientwin_geom(mgr, cwin, rqflags, &geom);
		}else{
			region_convert_root_geom(region_parent((WRegion*)cwin),
									 &geom);
			region_request_geom((WRegion*)cwin, rqflags, &geom, NULL);
		}
	}

	if(cwin->flags&CWIN_NEED_CFGNTFY){
		sendconfig_clientwin(cwin);
		cwin->flags&=~CWIN_NEED_CFGNTFY;
	}
}


/*}}}*/


/*{{{ Kludges */


/*EXTL_DOC
 * Attempts to fix window size problems with non-ICCCM compliant
 * programs.
 */
EXTL_EXPORT_MEMBER
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
	fprintf(file, "windowid = %lu,\n", (unsigned long)(cwin->win));
	save_indent_line(file, lvl);
	
	if(last_checkcode!=0){
		chkc=last_checkcode++;
		set_integer_property(cwin->win, wglobal.atom_checkcode, chkc);
		fprintf(file, "checkcode = %d,\n", chkc);
	}
	
	return TRUE;
}


WRegion *clientwin_load(WWindow *par, const WRectangle *geom, ExtlTab tab)
{
	double wind=0;
	Window win=None;
	int chkc=0, real_chkc=0;
	WClientWin *cwin=NULL;
	XWindowAttributes attr;
	WRectangle rg;

	if(!extl_table_gets_d(tab, "windowid", &wind) ||
	   !extl_table_gets_i(tab, "checkcode", &chkc)){
		return NULL;
	}
	
	win=(Window)wind;

	if(!get_integer_property(win, wglobal.atom_checkcode, &real_chkc))
		return NULL;

	if(real_chkc!=chkc){
		warn("Client window check code mismatch.");
		return NULL;
	}

	/* Found it! */
	
	if(!XGetWindowAttributes(wglobal.dpy, win, &attr)){
		warn("Window %#x disappeared", win);
		return NULL;
	}
	
	if(attr.override_redirect || 
	   (wglobal.opmode==OPMODE_INIT && attr.map_state!=IsViewable)){
		warn("Saved client window does not want to be managed");
		return NULL;
	}

	attr.x=geom->x;
	attr.y=geom->y;
	attr.width=geom->w;
	attr.height=geom->h;

	cwin=create_clientwin((WRegion*)par, win, &attr);
	
	if(cwin==NULL)
		return FALSE;
	
	/* Reparent and resize taking limits set by size hints into account */
	convert_geom(cwin, geom, &rg);
	REGION_GEOM(cwin)=rg;
	do_reparent_clientwin(cwin, par->win, rg.x, rg.y);
	XResizeWindow(wglobal.dpy, win, rg.w, rg.h);
	
	return (WRegion*)postmanage_check(cwin, &attr);
}


/*}}}*/


/*{{{ Dynfuntab and class info */


static DynFunTab clientwin_dynfuntab[]={
	{region_fit,
	 clientwin_fit},
	
	{(DynFun*)reparent_region,
	 (DynFun*)reparent_clientwin},
	
	{region_map,
	 clientwin_map},
	
	{region_unmap,
	 clientwin_unmap},
	
	{region_set_focus_to, 
	 clientwin_set_focus_to},
	
	{(DynFun*)region_display_managed,
	 (DynFun*)clientwin_display_managed},
	
	{region_notify_rootpos, 
	 clientwin_notify_rootpos},
	
	{(DynFun*)region_restack, 
	 (DynFun*)clientwin_restack},
	
	{(DynFun*)region_x_window, 
	 (DynFun*)clientwin_x_window},
	
	{region_activated, 
	 clientwin_activated},
	
	{region_resize_hints, 
	 clientwin_resize_hints},
	
	{(DynFun*)region_managed_enter_to_focus,
	 (DynFun*)clientwin_managed_enter_to_focus},
	
	{region_remove_managed, 
	 clientwin_remove_managed},
	
	{region_request_managed_geom, 
	 clientwin_request_managed_geom},
	
	{region_close, 
	 clientwin_close},
	
	{(DynFun*)region_save_to_file, 
	 (DynFun*)clientwin_save_to_file},
	
	{(DynFun*)region_do_rescue_clientwins,
	 (DynFun*)clientwin_do_rescue_clientwins},
	
	END_DYNFUNTAB
};


IMPLOBJ(WClientWin, WRegion, clientwin_deinit, clientwin_dynfuntab);


/*}}}*/
