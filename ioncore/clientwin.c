/*
 * wmcore/clientwin.c
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
#include "close.h"
#include "attach.h"
#include "funtabs.h"
#include "regbind.h"
#include "mwmhints.h"
#include "viewport.h"


#define TOPMOST_TRANSIENT(CWIN) LAST_THING(CWIN, WRegion)
#define LOWEST_TRANSIENT(CWIN) FIRST_THING(CWIN, WRegion)
#define HIGHER_TRANSIENT(REG) NEXT_THING(REG, WRegion)
#define LOWER_TRANSIENT(REG) PREV_THING(REG, WRegion)

static void deinit_clientwin(WClientWin *cwin);
static void clientwin_remove_sub(WClientWin *cwin, WRegion *sub);
static void fit_clientwin(WClientWin *cwin, WRectangle geom);
static bool reparent_clientwin(WClientWin *cwin, WWinGeomParams params);
static void map_clientwin(WClientWin *cwin);
static void unmap_clientwin(WClientWin *cwin);
static void focus_clientwin(WClientWin *cwin, bool warp);
static bool clientwin_switch_subregion(WClientWin *cwin, WRegion *sub);
static void clientwin_notify_rootpos(WClientWin *cwin, int x, int y);
static void clientwin_do_notify_rootpos(WClientWin *cwin, int x, int y);
static Window clientwin_restack(WClientWin *cwin, Window other, int mode);
static Window clientwin_lowest_win(WClientWin *cwin);
static void clientwin_activated(WClientWin *cwin);
static void clientwin_resize_hints(WClientWin *cwin, XSizeHints *hints_ret,
								   uint *relw_ret, uint *relh_ret);


static DynFunTab clientwin_dynfuntab[]={
	{fit_region, fit_clientwin},
	{(DynFun*)reparent_region, (DynFun*)reparent_clientwin},
	{map_region, map_clientwin},
	{unmap_region, unmap_clientwin},
	{focus_region, focus_clientwin},
	{(DynFun*)switch_subregion, (DynFun*)clientwin_switch_subregion},
	{region_notify_rootpos, clientwin_notify_rootpos},
	{(DynFun*)region_restack, (DynFun*)clientwin_restack},
	{(DynFun*)region_lowest_win, (DynFun*)clientwin_lowest_win},
	{region_activated, clientwin_activated},
	/* Perhaps resize accordingly instead? */
	{region_request_sub_geom, region_request_sub_geom_constrain},
	
	{region_resize_hints, clientwin_resize_hints},
	
	{region_remove_sub, clientwin_remove_sub},
	
	{region_request_close, close_clientwin},
	
	END_DYNFUNTAB
};


IMPLOBJ(WClientWin, WRegion, deinit_clientwin, clientwin_dynfuntab,
		&wmcore_clientwin_funclist)


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


/*}}}*/


/*{{{ Manage/create */


static void configure_cwin_bw(Window win, int bw)
{
	XWindowChanges wc;
	ulong wcmask=CWBorderWidth;
	
	wc.border_width=bw;
	XConfigureWindow(wglobal.dpy, win, wcmask, &wc);
}


static bool init_clientwin(WClientWin *cwin, WScreen *scr,
						   Window win, const XWindowAttributes *attr)
{
	WRectangle geom;
	char *name;
	
	geom.x=attr->x;
	geom.y=attr->y;
	geom.h=attr->height;
	geom.w=attr->width;
	
	cwin->x_off_cache=attr->x;
	cwin->y_off_cache=attr->y;
	cwin->win_geom.x=0;
	cwin->win_geom.y=0;	
	cwin->win_geom.h=attr->height;
	cwin->win_geom.w=attr->width;
	
	init_region(&cwin->region, scr, geom);

	cwin->flags=0;
	cwin->win=win;
	cwin->orig_bw=attr->border_width;
	
	name=get_string_property(win, XA_WM_NAME, NULL);
	
	if(name!=NULL){
		stripws(name);
		set_region_name(&cwin->region, name);
		free(name);
	}

	cwin->event_mask=CLIENT_MASK;
	cwin->transient_for=None;
	cwin->state=WithdrawnState;
	
	cwin->n_cmapwins=0;
	cwin->cmap=attr->colormap;
	cwin->cmaps=NULL;
	cwin->cmapwins=NULL;
	cwin->n_cmapwins=0;
	
	get_colormaps(cwin);
	
	get_clientwin_size_hints(cwin);
	get_protocols(cwin);

	XSelectInput(wglobal.dpy, win, cwin->event_mask);

	XSaveContext(wglobal.dpy, win, wglobal.win_context, (XPointer)cwin);
	XAddToSaveSet(wglobal.dpy, win);

	if(cwin->orig_bw!=0)
		configure_cwin_bw(win, 0);
	
	LINK_ITEM(wglobal.cwin_list, cwin, g_cwin_next, g_cwin_prev);
	
	region_add_bindmap((WRegion*)cwin, &wmcore_clientwin_bindmap, TRUE);
	
	return TRUE;
}


static WClientWin *create_clientwin(WScreen *scr, Window win,
									const XWindowAttributes *attr)
{
	CREATETHING_IMPL(WClientWin, clientwin, (p, scr, win, attr));
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
	cwin=create_clientwin(scr, win, /*state, */ &attr);
	
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


/*{{{ Attach sub */


bool clientwin_attach_sub(WClientWin *cwin, WRegion *sub, int flags)
{
	WWinGeomParams params;
	
	region_params(&cwin->region, &params);
	params.geom.x=0; params.geom.y=0;
	detach_reparent_region(sub, params);
	
	/* TODO: restack; reparent should raise but this region
	 * might not be the only visible region within the parent
	 * (need that region_topmost_win, lazy me).
	 */
	
	link_thing((WThing*)cwin, (WThing*)sub);
	
	if(REGION_IS_MAPPED((WRegion*)cwin))
	   map_region(sub);
	   
	if(REGION_IS_ACTIVE(cwin))
		set_focus(sub);
	
	return TRUE;
}


/*}}}*/


/*{{{ Unmanage/destroy */


static void reparent_root(WClientWin *cwin)
{
	XWindowAttributes attr;
	Window dummy, root=SCREEN_OF(cwin)->root.win;
	int x, y;
	
	if(XTranslateCoordinates(wglobal.dpy, cwin->win, root,
							  0, 0, &x, &y, &dummy)){
		XReparentWindow(wglobal.dpy, cwin->win, root, x, y);
	}
}


void deinit_clientwin(WClientWin *cwin)
{
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
	/* Rescue transients */
	region_rescue_clientwins((WRegion*)cwin);
	
	destroy_thing((WThing*)cwin);
}


/* Used when the window was deastroyed */
void clientwin_destroyed(WClientWin *cwin)
{
	XDeleteContext(wglobal.dpy, cwin->win, wglobal.win_context);
	cwin->win=None;
	region_rescue_clientwins((WRegion*)cwin);
	destroy_thing((WThing*)cwin);
}


void clientwin_remove_sub(WClientWin *cwin, WRegion *sub)
{
	WRegion *reg;
	
	unlink_thing((WThing*)sub);
	if(REGION_IS_ACTIVE(sub)){
		reg=TOPMOST_TRANSIENT(cwin);
		/*if(reg==sub)
			reg=LOWER_TRANSIENT(sub);*/
		if(reg==NULL)
			reg=&cwin->region;
		set_focus(reg);
	}
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
					-2*cwin->win_geom.w, -2*cwin->win_geom.h);
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
					cwin->win_geom.x+cwin->x_off_cache,
					cwin->win_geom.y+cwin->y_off_cache);
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


static void clientwin_reconf_at(WClientWin *cwin, int rootx, int rooty)
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
	ce.xconfigure.width=cwin->win_geom.w;
	ce.xconfigure.height=cwin->win_geom.h;
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
	clientwin_reconf_at(cwin, rootx, rooty);
}


static void do_reparent_clientwin(WClientWin *cwin, Window win, int x, int y)
{
	XSelectInput(wglobal.dpy, cwin->win,
				 cwin->event_mask&~StructureNotifyMask);
	XReparentWindow(wglobal.dpy, cwin->win, win, x, y);
	XSelectInput(wglobal.dpy, cwin->win, cwin->event_mask);
}


static WRectangle cwin_geom(WClientWin *cwin, WRectangle geom, bool rq)
{
	WRectangle r;
	bool bottom=(cwin->transient_for!=None);

	geom.x=0;
	geom.y=0;
	
	r.w=geom.w;
	r.h=geom.h;
	
	correct_size(&(r.w), &(r.h), &(cwin->size_hints), FALSE);
	
	if(rq){
		if(r.h>geom.h)
			r.h=geom.h;
	}else if(bottom && r.h>cwin->win_geom.h){
		r.h=cwin->win_geom.h;
	}
	
	r.x=geom.x+geom.w/2-r.w/2;
	
	if(bottom)
		r.y=geom.y+geom.h-r.h;
	else
		r.y=geom.y+geom.h/2-r.h/2;

	return r;
}


static void convert_geom(WClientWin *cwin, WRectangle geom,
						 WRectangle *wingeom, bool rq)
{
#if 0
	/* TODO -- Ion doesn't need this stuff at the moment, but PWM will
	 * so that the frames will shrink appropriately.
	 */
	WRegion *sub;
	XSizeHints hints;
	uint dummy_w, dummy_h;
	int maxw, maxh, w, h;
	bool subs;
	
	w=geom->w; h=geom->h;
	correct_size(&w, &h, &(cwin->size_hints), FALSE);
	maxw=w; maxh=h;
	
	FOR_ALL_TYPED(cwin, sub, WRegion){
		region_resize_hints(sub, &hints, &dummy_w, &dummy_h);
		w=geom->w; h=geom->h;
		correct_size(&w, &h, &hints, FALSE);
		if(w>maxw)
			maxw=w;
		if(h>maxh)
			maxh=h;
		subs=TRUE;
	}
	
	geom->x+=geom->w/2-maxw/2;
	geom->w=maxw;
	
	TODO: vertical shrink 
#endif
	
	*wingeom=cwin_geom(cwin, geom, rq);
}


static int min(int a, int b)
{
	return (a<b ? a : b);
}


static int max(int a, int b)
{
	return (a>b ? a : b);
}


/*}}}*/


/*{{{ Region dynfuns */


static void do_fit_clientwin(WClientWin *cwin, WRectangle geom, bool rq)
{
	WRegion *sub;
	WRectangle wingeom;
	WWinGeomParams params;

	region_params2(&cwin->region, geom, &params);
	convert_geom(cwin, geom, &wingeom, rq);
	
	cwin->x_off_cache=params.win_x;
	cwin->y_off_cache=params.win_y;
	cwin->win_geom=wingeom;
	
	/* XMoveResizeWindow won't send a ConfigureNotify event if the
	 * geometry has not changed and some programs expect that.
	 */
	{
		bool changes=(REGION_GEOM(cwin).x!=geom.x ||
					  REGION_GEOM(cwin).y!=geom.y ||
					  REGION_GEOM(cwin).w!=geom.w ||
					  REGION_GEOM(cwin).h!=geom.h);
		if(!changes){
			if(rq)
				sendconfig_clientwin(cwin);
			return;
		}
	}
	
	REGION_GEOM(cwin)=geom;
			
	if(cwin->flags&CWIN_PROP_ACROBATIC && !REGION_IS_MAPPED(cwin)){
		XMoveResizeWindow(wglobal.dpy, cwin->win,
						  -2*wingeom.w, -2*wingeom.h,
						  wingeom.w, wingeom.h);
	}else{
		XMoveResizeWindow(wglobal.dpy, cwin->win,
						  params.win_x+wingeom.x, params.win_y+wingeom.y,
						  wingeom.w, wingeom.h);
	}
	
	geom.x=0;
	geom.y=0;
	
	FOR_ALL_TYPED(cwin, sub, WRegion){
		fit_region(sub, geom);
	}
	
}


static void fit_clientwin(WClientWin *cwin, WRectangle geom)
{
	do_fit_clientwin(cwin, geom, FALSE);
}


static bool reparent_clientwin(WClientWin *cwin, WWinGeomParams params)
{
	WRectangle wingeom;
	WRegion *sub;
	int rootx, rooty;
	WRectangle geom;

	convert_geom(cwin, params.geom, &wingeom, FALSE);

	cwin->x_off_cache=params.win_x;
	cwin->y_off_cache=params.win_y;
	cwin->win_geom=wingeom;
	REGION_GEOM(cwin)=params.geom;

	do_reparent_clientwin(cwin, params.win,
						  params.win_x+wingeom.x,
						  params.win_y+wingeom.y);
	XResizeWindow(wglobal.dpy, cwin->win, wingeom.w, wingeom.h);
	sendconfig_clientwin(cwin);

	params.geom.x=0;
	params.geom.y=0;

	FOR_ALL_TYPED(cwin, sub, WRegion){
		reparent_region(sub, params);
	}
	
	region_rootpos((WRegion*)cwin, &rootx, &rooty);
	clientwin_do_notify_rootpos(cwin, rootx, rooty);

	clientwin_clear_target_id(cwin);
	
	return TRUE;
}


#define CALL_TRANSIENTS(CWIN, FN) {WRegion *sub; \
	FOR_ALL_TYPED(CWIN, sub, WRegion){FN(sub);} }


static void map_clientwin(WClientWin *cwin)
{
	show_clientwin(cwin);
	MARK_REGION_MAPPED(cwin);
	CALL_TRANSIENTS(cwin, map_region);
}


static void unmap_clientwin(WClientWin *cwin)
{
	hide_clientwin(cwin);
	MARK_REGION_UNMAPPED(cwin);
	CALL_TRANSIENTS(cwin, unmap_region);
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

	
static bool clientwin_switch_subregion(WClientWin *cwin, WRegion *sub)
{
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
}

static void clientwin_do_notify_rootpos(WClientWin *cwin, int x, int y)
{
	clientwin_reconf_at(cwin, x+cwin->win_geom.x, y+cwin->win_geom.y);
}


static void clientwin_notify_rootpos(WClientWin *cwin, int x, int y)
{
	clientwin_do_notify_rootpos(cwin, x, y);
	notify_subregions_rootpos(&cwin->region, x, y);
}


static Window clientwin_restack(WClientWin *cwin, Window other, int mode)
{
	WRegion *reg;
	
	do_restack_window(cwin->win, other, mode);
	other=cwin->win;
	
	for(reg=LOWEST_TRANSIENT(cwin); reg!=NULL; reg=HIGHER_TRANSIENT(reg))
		other=region_restack(reg, other, Above);
													   
	return cwin->win;
}


static Window clientwin_lowest_win(WClientWin *cwin)
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
}


/*}}}*/



/*{{{ Names */


void set_clientwin_name(WClientWin *cwin, char *p)
{
	set_region_name(&cwin->region, p);
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

	/* check full screen request */
	if((ev->value_mask&(CWX|CWY))==(CWX|CWY) &&
	   REGION_GEOM(SCREEN_OF(cwin)).w==ev->width &&
	   REGION_GEOM(SCREEN_OF(cwin)).h==ev->height){
		WMwmHints *mwm;
		mwm=get_mwm_hints(cwin->win);
		if(mwm!=NULL && mwm->flags&MWM_HINTS_DECORATIONS &&
		   mwm->decorations==0){
#ifdef CF_SWITCH_NEW_CLIENTS
			if(clientwin_enter_fullscreen(cwin, TRUE))
#else				
			if(clientwin_enter_fullscreen(cwin, TRUE))
#endif
				return;
		}
	}
	
	geom=cwin->win_geom;
	region_rootpos(&(cwin->region), &rx, &ry);
	rx+=geom.x;
	ry+=geom.y;
	
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
	
	if(!sz && !pos){
		sendconfig_clientwin(cwin);
		return;
	}
	
	/* TODO */
#if 0
	region_request_geom(&(cwin->region), geom, NULL, FALSE);
#else
	do_fit_clientwin(cwin, REGION_GEOM(cwin), TRUE);
#endif
}


/*}}}*/


/*{{{ Fullscreen */


bool clientwin_fullscreen_vp(WClientWin *cwin, WViewport *vp, bool switchto)
{
	/* TODO: Remember last parent */

	return region_attach_sub((WRegion*)vp, (WRegion*)cwin,
							 switchto ?  REGION_ATTACH_SWITCHTO : 0);
}

bool clientwin_enter_fullscreen(WClientWin *cwin, bool switchto)
{
	WViewport *vp=FIND_PARENT1(cwin, WViewport);
	
	if(vp==NULL){
		WScreen *scr=SCREEN_OF(cwin);
		if(scr!=NULL)
			vp=viewport_of((WRegion*)cwin);
	}
	
	if(vp==NULL)
		return FALSE;
	
	return clientwin_fullscreen_vp(cwin, vp, switchto);
}


/*}}}*/


/*{{{ Kludges */


void clientwin_broken_app_resize_kludge(WClientWin *cwin)
{
	XResizeWindow(wglobal.dpy, cwin->win, 2*cwin->win_geom.w, 2*cwin->win_geom.h);
	XFlush(wglobal.dpy);
	XResizeWindow(wglobal.dpy, cwin->win, cwin->win_geom.w, cwin->win_geom.h);
}


/*}}}*/

