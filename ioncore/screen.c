/*
 * ion/ioncore/screen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "global.h"
#include "screen.h"
#include "region.h"
#include "attach.h"
#include "objp.h"
#include "focus.h"
#include "regbind.h"
#include "property.h"
#include "names.h"
#include "reginfo.h"
#include "saveload.h"
#include "genws.h"


static bool screen_display_managed(WScreen *scr, WRegion *reg);


/*{{{ Init/deinit */


static bool screen_init(WScreen *scr, WRootWin *rootwin,
						int id, WRectangle geom)
{
	Window dummywin;
#ifdef CF_WINDOWED_SCREENS
	XSetWindowAttributes attr;
	ulong attrflags=0;
#endif
	
	scr->ws_count=0;
	scr->current_ws=NULL;
	scr->ws_list=NULL;
	scr->id=id;
	scr->atom_workspace=None;

#ifdef CF_WINDOWED_SCREENS
	attr.background_pixmap=ParentRelative;
	attrflags=CWBackPixmap;
	
	dummywin=XCreateWindow(wglobal.dpy, rootwin->root.win,
						   geom.x, geom.y, geom.w, geom.h, 0, 
						   DefaultDepth(wglobal.dpy, rootwin->xscr),
						   InputOutput,
						   DefaultVisual(wglobal.dpy, rootwin->xscr),
						   attrflags, &attr);

	if(dummywin==None)
		return FALSE;

	if(!window_init((WWindow*)scr, (WWindow*)rootwin, dummywin, geom)){
		XDestroyWindow(wglobal.dpy, dummywin);
		return FALSE;
	}

	XSelectInput(wglobal.dpy, dummywin, FocusChangeMask|EnterWindowMask|
				 /*KeyPressMask|KeyReleaseMask|
				 ButtonPressMask|ButtonReleaseMask|*/);
#else
	region_init((WRegion*)scr, (WRegion*)rootwin, geom);
#endif
	if(id==0){
		scr->atom_workspace=XInternAtom(wglobal.dpy, "_ION_WORKSPACE", False);
	}else if(id>=0){
		char *str;
		libtu_asprintf(&str, "_ION_WORKSPACE%d", id);
		if(str==NULL){
			warn_err();
		}else{
			scr->atom_workspace=XInternAtom(wglobal.dpy, str, False);
			free(str);
		}
	}

	/* Add rootwin's bindings to screens (ungrabbed) so that bindings
	 * are called with the proper region.
	 */
	region_add_bindmap((WRegion*)scr, &ioncore_rootwin_bindmap);

	return TRUE;
}


WScreen *create_screen(WRootWin *rootwin, int id, WRectangle geom)
{
	CREATEOBJ_IMPL(WScreen, screen, (p, rootwin, id, geom));
}


void screen_deinit(WScreen *scr)
{
	while(scr->ws_list!=NULL)
		region_unset_manager(scr->ws_list, (WRegion*)scr, &(scr->ws_list));

#ifdef CF_WINDOWED_SCREENS
	window_deinit((WWindow*)scr);
#else
	region_deinit((WRegion*)scr);
#endif
}


static bool create_initial_workspace_on_scr(WScreen *scr)
{
	WRegionSimpleCreateFn *fn;
	WRegion *reg;
	
	fn=lookup_region_simple_create_fn_inh("WGenWS");
	
	if(fn==NULL){
		warn("Could not find a complete workspace class. "
			 "Please load some modules.");
		return FALSE;
	}
	
	reg=region_add_managed_new_simple((WRegion*)scr, fn, 0);
	
	if(reg==NULL){
		warn("Unable to create a workspace on screen %d\n", scr->id);
		return FALSE;
	}
	
	region_set_name(reg, "main");
	return TRUE;
}


bool screen_initialize_workspaces(WScreen* scr)
{
	WRegion *ws=NULL;
	char *wsname=NULL;

	if(scr->atom_workspace!=None)
		wsname=get_string_property(ROOT_OF(scr), scr->atom_workspace, NULL);

	load_workspaces(scr);
	
	if(scr->ws_count==0){
		if(!create_initial_workspace_on_scr(scr))
			return FALSE;
	}else{
		if(wsname!=NULL)
			ws=lookup_region(wsname);
		if(ws==NULL || REGION_MANAGER(ws)!=(WRegion*)scr)
			ws=FIRST_MANAGED(scr->ws_list);
		if(ws!=NULL)
			screen_display_managed(scr, ws);
	}
	
	if(wsname!=NULL)
		free(wsname);
	
	return TRUE;
}


/*}}}*/


/*{{{ Attach/detach */


static WRegion *screen_do_add_managed(WScreen *scr, WRegionAddFn *fn,
									  void *fnparams, 
									  const WAttachParams *param)
{
	WRegion *reg;
	WRectangle geom;
	
#ifdef CF_WINDOWED_SCREENS
	geom.x=0;
	geom.y=0;
	geom.w=REGION_GEOM(scr).w;
	geom.h=REGION_GEOM(scr).h;
	
	reg=fn((WWindow*)scr, geom, fnparams);
#else
	{
		WWindow *par=REGION_PARENT_CHK(scr, WWindow);
		if(par==NULL)
			return NULL;
		geom=REGION_GEOM(scr);
		reg=fn((WWindow*)par, geom, fnparams);
	}
#endif
	if(reg==NULL)
		return NULL;

	region_set_manager(reg, (WRegion*)scr, &(scr->ws_list));
	scr->ws_count++;
	
	if(scr->current_ws==NULL || param->flags&REGION_ATTACH_SWITCHTO)
		screen_display_managed(scr, reg);
	else
		region_unmap(reg);
	
	return reg;
}


static void screen_remove_managed(WScreen *scr, WRegion *reg)
{
	WRegion *next=NULL;

	if(scr->current_ws==reg){
		next=PREV_MANAGED(scr->ws_list, reg);
		if(next==NULL)
			next=NEXT_MANAGED(scr->ws_list, reg);
		scr->current_ws=NULL;
	}
	
	region_unset_manager(reg, (WRegion*)scr, &(scr->ws_list));
	scr->ws_count--;
	
	if(wglobal.opmode!=OPMODE_DEINIT){
		if(next!=NULL)
			screen_display_managed(scr, next);
	}
}


/*}}}*/


/*{{{ region dynfun implementations */


static void screen_fit(WScreen *scr, WRectangle geom)
{
	WRegion *sub;
	
	REGION_GEOM(scr)=geom;
	
#ifdef CF_WINDOWED_SCREENS
	XMoveResizeWindow(wglobal.dpy, scr->wwin.win,
					  geom.x, geom.y, geom.w, geom.h);
	
	geom.x=0;
	geom.y=0;
#endif

	FOR_ALL_MANAGED_ON_LIST(scr->ws_list, sub){
		region_fit(sub, geom);
	}
}


static void screen_set_focus_to(WScreen *scr, bool warp)
{
	if(scr->current_ws!=NULL){
		region_set_focus_to(scr->current_ws, warp);
	}else{
		if(warp)
			do_move_pointer_to((WRegion*)scr);
#ifdef CF_WINDOWED_SCREENS
		SET_FOCUS(scr->wwin.win);
#endif
	}
}


static bool screen_display_managed(WScreen *scr, WRegion *reg)
{
	char *n;
	
	if(reg==scr->current_ws)
		return FALSE;

	if(region_is_fully_mapped((WRegion*)scr))
		region_map(reg);
	
	if(scr->current_ws!=NULL)
		region_unmap(scr->current_ws);
	
	scr->current_ws=reg;
	
	if(scr->atom_workspace!=None && wglobal.opmode!=OPMODE_DEINIT){
		n=region_full_name(reg);
		
		if(n==NULL){
			set_string_property(ROOT_OF(scr), scr->atom_workspace, "");
		}else{
			set_string_property(ROOT_OF(scr), scr->atom_workspace, n);
			free(n);
		}
	}
	
	if(wglobal.opmode!=OPMODE_DEINIT && region_may_control_focus((WRegion*)scr))
		warp(reg);
	
	extl_call_named("call_hook", "soo", NULL,
					"screen_workspace_switched", scr, reg);
	
	return TRUE;
}


static WRegion *screen_find_rescue_manager_for(WScreen *scr, WRegion *reg)
{
	WRegion *other;
	
	if(REGION_MANAGER(reg)!=(WRegion*)scr)
		return NULL;

	other=reg;
	while(1){
		other=PREV_MANAGED(scr->ws_list, other);
		if(other==reg)
			break;
		if(WOBJ_IS(other, WGenWS) && region_can_manage_clientwins(other))
			return other;
	}
	return NULL;
}


static void screen_map(WScreen *scr)
{
#ifdef CF_WINDOWED_SCREENS
	window_map((WWindow*)scr);
#else
	MARK_REGION_MAPPED(scr);
#endif
	if(scr->current_ws!=NULL)
		region_map(scr->current_ws);
}


static void screen_unmap(WScreen *scr)
{
#ifdef CF_WINDOWED_SCREENS
	window_unmap((WWindow*)scr);
#else
	MARK_REGION_UNMAPPED(scr);
#endif
	if(scr->current_ws!=NULL)
		region_unmap(scr->current_ws);
}


/*}}}*/


/*{{{ Workspace switch */


static WRegion *nth_ws(WScreen *scr, uint n)
{
	WRegion *r=scr->ws_list;
	
	/*return NTH_MANAGED(scr->ws_list, n);*/
	while(n-->0){
		r=NEXT_MANAGED(scr->ws_list, r);
		if(r==NULL)
			break;
	}
	
	return r;
}


/*EXTL_DOC
 * Display the \var{n}th region (workspace, fullscreen client window)
 * managed by the screen \var{scr}.
 */
EXTL_EXPORT
void screen_switch_nth(WScreen *scr, uint n)
{
	WRegion *sub=nth_ws(scr, n);
	if(sub!=NULL)
		region_display_sp(sub);
}


/*EXTL_DOC
 * Display next region (workspace, fullscreen client window) managed by the
 * screen \var{scr}.
 */
EXTL_EXPORT
void screen_switch_next(WScreen *scr)
{
	WRegion *reg=NEXT_MANAGED_WRAP(scr->ws_list, scr->current_ws);
	if(reg!=NULL)
		region_display_sp(reg);
}


/*EXTL_DOC
 * Display previous region (workspace, fullscreen client window) managed by
 * the screen \var{scr}.
 */
EXTL_EXPORT
void screen_switch_prev(WScreen *scr)
{
	WRegion *reg=PREV_MANAGED_WRAP(scr->ws_list, scr->current_ws);
	if(reg!=NULL)
		region_display_sp(reg);
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Returns the screen \var{reg} is on.
 */
EXTL_EXPORT
WScreen *region_screen_of(WRegion *reg)
{
	while(reg!=NULL){
		if(WOBJ_IS(reg, WScreen))
			return (WScreen*)reg;
		reg=REGION_MANAGER(reg);
	}
	return NULL;
}


/*EXTL_DOC
 * Find the screen with numerical id \var{id}. If Xinerama is
 * not present, \var{id} corresponds to X screen numbers. Otherwise
 * the ids are some arbitrary ordering of Xinerama rootwins.
 */
EXTL_EXPORT
WScreen *find_screen_id(int id)
{
	WRootWin *rootwin;
	WScreen *scr, *maxscr=NULL;
	
	FOR_ALL_ROOTWINS(rootwin){
		FOR_ALL_TYPED_CHILDREN(rootwin, scr, WScreen){
			if(id==-1){
				if(maxscr==NULL || scr->id>maxscr->id)
					maxscr=scr;
			}
			if(scr->id==id)
				return scr;
		}
	}
	
	return maxscr;
}


/*EXTL_DOC
 * Switch focus to the screen with id \var{id}.
 */
EXTL_EXPORT
void goto_nth_screen(int id)
{
	WScreen *scr=find_screen_id(id);
	if(scr!=NULL)
		region_goto((WRegion*)scr);
}


/*EXTL_DOC
 * Switch focus to the next screen.
 */
EXTL_EXPORT
void goto_next_screen()
{
	WScreen *scr=rootwin_current_scr(wglobal.active_rootwin);
	
	if(scr!=NULL)
		scr=find_screen_id(scr->id+1);
	if(scr==NULL)
		scr=find_screen_id(0);
	if(scr!=NULL)
		region_goto((WRegion*)scr);
}


/*EXTL_DOC
 * Switch focus to the previous screen.
 */
EXTL_EXPORT
void goto_prev_screen()
{
	WScreen *scr=rootwin_current_scr(wglobal.active_rootwin);
	
	if(scr==NULL)
		scr=find_screen_id(0);
	else if(scr->id==0)
		scr=find_screen_id(-1);
	else
		scr=find_screen_id(scr->id-1);
	if(scr!=NULL)
		region_goto((WRegion*)scr);
}


/*EXTL_DOC
 * Return the region managed and currently being displayed by the screen
 * \var{scr} (usually a workspace or a fullscreen client window).
 */
EXTL_EXPORT
WRegion *screen_current(WScreen *scr)
{
	return scr->current_ws;
}


/*EXTL_DOC
 * Return the numerical id for screen \var{scr}.
 */
EXTL_EXPORT
int screen_id(WScreen *scr)
{
	return scr->id;
}


static bool screen_may_destroy_managed(WScreen *scr, WRegion *reg)
{
	WRegion *r2;
	
	FOR_ALL_MANAGED_ON_LIST(scr->ws_list, r2){
		if(WOBJ_IS(r2, WGenWS) && r2!=reg)
			return TRUE;
	}
	
	warn("Cannot destroy only workspace.");
	return FALSE;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab screen_dynfuntab[]={
	{region_fit, screen_fit},
	{region_set_focus_to, screen_set_focus_to},
    {region_map, screen_map},
	{region_unmap, screen_unmap},
	
	{(DynFun*)region_display_managed, (DynFun*)screen_display_managed},
	{region_request_managed_geom, region_request_managed_geom_unallow},
	{(DynFun*)region_do_add_managed, (DynFun*)screen_do_add_managed},
	{region_remove_managed, screen_remove_managed},
	
	{(DynFun*)region_may_destroy_managed,
	 (DynFun*)screen_may_destroy_managed},

	{(DynFun*)region_find_rescue_manager_for,
	 (DynFun*)screen_find_rescue_manager_for},
	
	END_DYNFUNTAB
};


#ifdef CF_WINDOWED_SCREENS
IMPLOBJ(WScreen, WWindow, screen_deinit, screen_dynfuntab);
#else
IMPLOBJ(WScreen, WRegion, screen_deinit, screen_dynfuntab);
#endif

/*}}}*/
