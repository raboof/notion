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
#include "manage.h"
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
						int id, const WRectangle *geom, bool useroot)
{
	Window win;
	XSetWindowAttributes attr;
	ulong attrflags=0;
	
	scr->id=id;
	scr->atom_workspace=None;
	scr->uses_root=useroot;
	scr->configured=FALSE;
	scr->managed_off.x=0;
	scr->managed_off.y=0;
	scr->managed_off.w=0;
	scr->managed_off.h=0;
	
	if(useroot){
		win=WROOTWIN_ROOT(rootwin);
	}else{
		attr.background_pixmap=ParentRelative;
		attrflags=CWBackPixmap;
		
		win=XCreateWindow(wglobal.dpy, WROOTWIN_ROOT(rootwin),
						  geom->x, geom->y, geom->w, geom->h, 0, 
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


WScreen *create_screen(WRootWin *rootwin, int id, const WRectangle *geom,
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
	
	reg=mplex_attach_new_simple((WMPlex*)scr, fn, TRUE);
	
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

	if(scr->configured || SCR_MCOUNT(scr)>0)
		return TRUE;
	
	return create_initial_workspace_on_scr(scr);
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


static void screen_fit(WScreen *scr, const WRectangle *geom)
{
	WRegion *sub;
	
	if(scr->uses_root){
		WRectangle mg;
		
		screen_managed_geom(scr, &mg);
	
		FOR_ALL_MANAGED_ON_LIST(SCR_MLIST(scr), sub){
			region_fit(sub, &mg);
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


static WRegion *screen_find_rescue_manager_for(WScreen *scr, WRegion *todst)
{
	WRegion *other;
	
	/* TODO: checking that todst is on the managed list should be done
	 * more cleanly.
	 */
	if(REGION_MANAGER(todst)!=(WRegion*)scr || todst==scr->mplex.current_input)
		return FALSE;

	other=todst;
	while(1){
		other=PREV_MANAGED(SCR_MLIST(scr), other);
		if(other==NULL)
			break;
		if(region_has_manage_clientwin(other) && 
		   !WOBJ_IS_BEING_DESTROYED(other)){
			return other;
		}
	}

	other=todst;
	while(1){
		other=NEXT_MANAGED(SCR_MLIST(scr), other);
		if(other==NULL)
			break;
		if(region_has_manage_clientwin(other) && 
		   !WOBJ_IS_BEING_DESTROYED(other)){
			return other;
		}
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
EXTL_EXPORT_MEMBER
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
EXTL_EXPORT_MEMBER
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

/*{{{ Save/load */


static bool screen_save_to_file(WScreen *scr, FILE *file, int lvl)
{
	WRegion *sub;
	
	begin_saved_region((WRegion*)scr, file, lvl);
	save_indent_line(file, lvl);
	fprintf(file, "subs = {\n");
	FOR_ALL_MANAGED_ON_LIST(SCR_MLIST(scr), sub){
		save_indent_line(file, lvl+1);
		fprintf(file, "{\n");
		region_save_to_file((WRegion*)sub, file, lvl+2);
		if(sub==SCR_CURRENT(scr)){
			save_indent_line(file, lvl+2);
			fprintf(file, "switchto = true,\n");
		}
		save_indent_line(file, lvl+1);
		fprintf(file, "},\n");
	}
	save_indent_line(file, lvl);
	fprintf(file, "},\n");
	
	return TRUE;
}


/*EXTL_DOC
 * (Intended to be called from workspace savefiles.)
 * Set screen name and initial workspaces etc.
 */
EXTL_EXPORT
bool initialise_screen_id(int id, ExtlTab tab)
{
	char *name;
	WScreen *scr=find_screen_id(id);
	ExtlTab substab, subtab;
	int n, i;

	if(scr==NULL){
		warn("No screen #%d\n", id);
		return FALSE;
	}
	
	scr->configured=TRUE;
	
	if(extl_table_gets_s(tab, "name", &name)){
		region_set_name((WRegion*)scr, name);
		free(name);
	}

	if(!extl_table_gets_t(tab, "subs", &substab))
		return TRUE;
	
	n=extl_table_get_n(substab);
	for(i=1; i<=n; i++){
		if(extl_table_geti_t(substab, i, &subtab)){
			mplex_attach_new((WMPlex*)scr, subtab);
			extl_unref_table(subtab);
		}
	}
	extl_unref_table(substab);
	
	return TRUE;
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

	{(DynFun*)region_save_to_file,
	 (DynFun*)screen_save_to_file},
	
	END_DYNFUNTAB
};


IMPLOBJ(WScreen, WMPlex, screen_deinit, screen_dynfuntab);


/*}}}*/
