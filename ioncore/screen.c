/*
 * ion/ioncore/screen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "global.h"
#include "screen.h"
#include "region.h"
#include "attach.h"
#include "objp.h"
#include "focus.h"
#include "property.h"
#include "names.h"
#include "reginfo.h"
#include "saveload.h"
#include "resize.h"
#include "genws.h"
#include "event.h"
#include "bindmaps.h"
#include "regbind.h"


#define SCR_MLIST(SCR) ((SCR)->mplex.managed_list)
#define SCR_MCOUNT(SCR) ((SCR)->mplex.managed_count)
#define SCR_CURRENT(SCR) ((SCR)->mplex.current_sub)
#define SCR_WIN(SCR) ((SCR)->mplex.win.win)


/*{{{ Init/deinit */


static bool screen_init(WScreen *scr, WRootWin *rootwin,
						int id, WRectangle geom, bool useroot)
{
	Window win;
	XSetWindowAttributes attr;
	ulong attrflags=0;
	
	scr->id=id;
	scr->atom_workspace=None;
	scr->uses_root=useroot;
	scr->managed_off.x=0;
	scr->managed_off.y=0;
	scr->managed_off.w=0;
	scr->managed_off.h=0;
	
	if(useroot){
		win=rootwin->root;
	}else{
		attr.background_pixmap=ParentRelative;
		attrflags=CWBackPixmap;
		
		win=XCreateWindow(wglobal.dpy, rootwin->root,
						  geom.x, geom.y, geom.w, geom.h, 0, 
						  DefaultDepth(wglobal.dpy, rootwin->xscr),
						  InputOutput,
						  DefaultVisual(wglobal.dpy, rootwin->xscr),
						  attrflags, &attr);
		
		if(win==None)
			return FALSE;
	}

	if(!mplex_init((WMPlex*)scr, NULL, win, geom)){
		XDestroyWindow(wglobal.dpy, win);
		return FALSE;
	}

	scr->mplex.win.region.rootwin=rootwin;
	region_set_parent((WRegion*)scr, (WRegion*)rootwin);
	scr->mplex.flags|=WMPLEX_ADD_TO_END;
	scr->mplex.win.region.flags|=REGION_BINDINGS_ARE_GRABBED;
	if(useroot)
		scr->mplex.win.region.flags|=REGION_MAPPED;
	
	XSelectInput(wglobal.dpy, win, 
				 FocusChangeMask|EnterWindowMask|
				 KeyPressMask|KeyReleaseMask|
				 ButtonPressMask|ButtonReleaseMask|
				 (useroot ? ROOT_MASK : 0));

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

	LINK_ITEM(wglobal.screens, scr, next_scr, prev_scr);
	
	if(wglobal.active_screen==NULL)
		wglobal.active_screen=scr;

	return TRUE;
}


WScreen *create_screen(WRootWin *rootwin, int id, WRectangle geom,
					   bool useroot)
{
	CREATEOBJ_IMPL(WScreen, screen, (p, rootwin, id, geom, useroot));
}


void screen_deinit(WScreen *scr)
{
	UNLINK_ITEM(wglobal.screens, scr, next_scr, prev_scr);
	
	while(SCR_MLIST(scr)!=NULL)
		region_unset_manager(SCR_MLIST(scr), (WRegion*)scr, &(SCR_MLIST(scr)));

	if(scr->uses_root)
		SCR_WIN(scr)=None;
	
	mplex_deinit((WMPlex*)scr);
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
	
	if(SCR_MCOUNT(scr)==0){
		if(!create_initial_workspace_on_scr(scr))
			return FALSE;
	}else{
		if(wsname!=NULL)
			ws=lookup_region(wsname, NULL);
		if(ws==NULL || REGION_MANAGER(ws)!=(WRegion*)scr)
			ws=FIRST_MANAGED(SCR_MLIST(scr));
		if(ws!=NULL)
			region_display_managed((WRegion*)scr, ws);
	}
	
	if(wsname!=NULL)
		free(wsname);
	
	return TRUE;
}


/*}}}*/


/*{{{ Attach/detach */


void screen_managed_geom(WScreen *scr, WRectangle *geom)
{
	geom->x=scr->managed_off.x;
	geom->y=scr->managed_off.y;
	geom->w=REGION_GEOM(scr).w+scr->managed_off.w;
	geom->h=REGION_GEOM(scr).h+scr->managed_off.h;
}


/*}}}*/


/*{{{ region dynfun implementations */


static void screen_fit(WScreen *scr, WRectangle geom)
{
	WRegion *sub;
	
	if(scr->uses_root){
		screen_managed_geom(scr, &geom);
	
		FOR_ALL_MANAGED_ON_LIST(SCR_MLIST(scr), sub){
			region_fit(sub, geom);
		}
	}else{
		mplex_fit((WMPlex*)scr, geom);
	}
}


static void screen_managed_changed(WScreen *scr, bool sw)
{
	WRegion *reg;
	const char *n=NULL;
	
	if(!sw)
		return;
	
	reg=SCR_CURRENT(scr);

	if(scr->atom_workspace!=None && wglobal.opmode!=OPMODE_DEINIT){
		if(reg!=NULL)
			n=region_name(reg);
		
		set_string_property(ROOT_OF(scr), scr->atom_workspace, 
							n==NULL ? "" : n);
	}
	
	extl_call_named("call_hook", "soo", NULL,
					"screen_workspace_switched", scr, reg);
}


static WRegion *screen_find_rescue_manager_for(WScreen *scr, WRegion *reg)
{
	WRegion *other;
	
	/* TODO: checking that reg is on the managed list should be done
	 * more cleanly.
	 */
	if(REGION_MANAGER(reg)!=(WRegion*)scr || reg==scr->mplex.current_input)
		return NULL;

	other=reg;
	while(1){
		other=PREV_MANAGED(SCR_MLIST(scr), other);
		if(other==NULL)
			break;
		if(WOBJ_IS(other, WGenWS) && region_can_manage_clientwins(other))
			return other;
	}

	other=reg;
	while(1){
		other=NEXT_MANAGED(SCR_MLIST(scr), other);
		if(other==NULL)
			break;
		if(WOBJ_IS(other, WGenWS) && region_can_manage_clientwins(other))
			return other;
	}
	return NULL;
}


static void screen_map(WScreen *scr)
{
	if(scr->uses_root)
		return;
	mplex_map((WMPlex*)scr);
}


static void screen_unmap(WScreen *scr)
{
	if(scr->uses_root)
		return;
	mplex_unmap((WMPlex*)scr);
}


static void screen_activated(WScreen *scr)
{
	wglobal.active_screen=scr;
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
		reg=region_parent(reg);
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
	WScreen *scr, *maxscr=NULL;
	
	FOR_ALL_SCREENS(scr){
		if(id==-1){
			if(maxscr==NULL || scr->id>maxscr->id)
				maxscr=scr;
		}
		if(scr->id==id)
			return scr;
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
	WScreen *scr=wglobal.active_screen;
	
	if(scr!=NULL)
		scr=scr->next_scr;
	if(scr==NULL)
		scr=wglobal.screens;
	if(scr!=NULL)
		region_goto((WRegion*)scr);
}


/*EXTL_DOC
 * Switch focus to the previous screen.
 */
EXTL_EXPORT
void goto_prev_screen()
{
	WScreen *scr=wglobal.active_screen;

	if(scr!=NULL)
		scr=scr->prev_scr;
	else
		scr=wglobal.screens;
	if(scr!=NULL)
		region_goto((WRegion*)scr);
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
	
	FOR_ALL_MANAGED_ON_LIST(SCR_MLIST(scr), r2){
		if(WOBJ_IS(r2, WGenWS) && r2!=reg)
			return TRUE;
	}
	
	warn("Cannot destroy only workspace.");
	return FALSE;
}


void screen_set_managed_offset(WScreen *scr, const WRectangle *off)
{
	scr->managed_off=*off;
	mplex_fit_managed((WMPlex*)scr);
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab screen_dynfuntab[]={
	{region_fit, screen_fit},
    {region_map, screen_map},
	{region_unmap, screen_unmap},
	{region_activated, screen_activated},
	
	{(DynFun*)region_may_destroy_managed,
	 (DynFun*)screen_may_destroy_managed},

	{(DynFun*)region_find_rescue_manager_for,
	 (DynFun*)screen_find_rescue_manager_for},
	
	{mplex_managed_changed, screen_managed_changed},
	
	{mplex_managed_geom, screen_managed_geom},
	
	END_DYNFUNTAB
};


IMPLOBJ(WScreen, WMPlex, screen_deinit, screen_dynfuntab);


/*}}}*/
