/*
 * ion/floatws/floatws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/parser.h>
#include <ioncore/common.h>
#include <ioncore/screen.h>
#include <ioncore/focus.h>
#include <ioncore/global.h>
#include <ioncore/objp.h>
#include <ioncore/region.h>
#include <ioncore/wsreg.h>
#include <ioncore/funtabs.h>
#include <ioncore/viewport.h>
#include <ioncore/names.h>
#include <ioncore/saveload.h>
#include <ioncore/attach.h>
#include <ioncore/regbind.h>
#include <ioncore/genframe-pointer.h>

#include "floatws.h"
#include "floatframe.h"
#include "funtabs.h"


/*{{{ Static function declarations */


static void deinit_floatws(WFloatWS *ws);
static void fit_floatws(WFloatWS *ws, WRectangle geom);
static bool reparent_floatws(WFloatWS *ws, WWindow *parent, WRectangle geom);
static void map_floatws(WFloatWS *ws);
static void unmap_floatws(WFloatWS *ws);
static void focus_floatws(WFloatWS *ws, bool warp);
static bool floatws_save_to_file(WFloatWS *ws, FILE *file, int lvl);
static bool floatws_display_managed(WFloatWS *ws, WRegion *reg);
static void floatws_remove_managed(WFloatWS *ws, WRegion *reg);
static void floatws_managed_activated(WFloatWS *ws, WRegion *reg);
static bool floatws_add_clientwin(WFloatWS *ws, WClientWin *cwin,
								  const XWindowAttributes *attr,
								  int init_state, WWinProp *props);
static bool floatws_add_transient(WFloatWS *ws, WClientWin *tfor,
								  WClientWin *cwin,
								  const XWindowAttributes *attr,
								  int init_state, WWinProp *props);
static Window floatws_x_window(const WFloatWS *ws);
static bool floatws_handle_drop(WFloatWS *floatws, int x, int y,
								WRegion *dropped);


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab floatws_dynfuntab[]={
	{fit_region, fit_floatws},
	{map_region, map_floatws},
	{unmap_region, unmap_floatws},
	{focus_region, focus_floatws},
	{(DynFun*)reparent_region, (DynFun*)reparent_floatws},
	
	{(DynFun*)region_ws_add_clientwin, (DynFun*)floatws_add_clientwin},
	{(DynFun*)region_ws_add_transient, (DynFun*)floatws_add_transient},

	{region_managed_activated, floatws_managed_activated},
	{region_remove_managed, floatws_remove_managed},
	{(DynFun*)region_display_managed, (DynFun*)floatws_display_managed},
	
/*	{(DynFun*)region_do_find_new_manager, (DynFun*)floatws_do_find_new_manager},*/
	
	{(DynFun*)region_save_to_file, (DynFun*)floatws_save_to_file},

	{(DynFun*)region_x_window, (DynFun*)floatws_x_window},

	{(DynFun*)region_handle_drop, (DynFun*)floatws_handle_drop},
	
	END_DYNFUNTAB
};


IMPLOBJ(WFloatWS, WGenWS, deinit_floatws, floatws_dynfuntab, &floatws_funclist)


/*}}}*/


/*{{{ region dynfun implementations */


static void fit_floatws(WFloatWS *ws, WRectangle geom)
{
	REGION_GEOM(ws)=geom;
}


static bool reparent_floatws(WFloatWS *ws, WWindow *parent, WRectangle geom)
{
	WRegion *sub, *next;
	bool rs;
	int xdiff, ydiff;
	
	if(!same_screen((WRegion*)ws, (WRegion*)parent))
		return FALSE;
	
	XReparentWindow(wglobal.dpy, ws->dummywin, parent->win, geom.x, geom.h);
					
	region_detach_parent((WRegion*)ws);
	region_set_parent((WRegion*)ws, (WRegion*)parent);
	
	xdiff=geom.x-REGION_GEOM(ws).x;
	ydiff=geom.y-REGION_GEOM(ws).y;
	
	FOR_ALL_MANAGED_ON_LIST_W_NEXT(ws->managed_list, sub, next){
		WRectangle g=REGION_GEOM(sub);
		g.x+=xdiff;
		g.y+=ydiff;
		if(!reparent_region(sub, parent, g)){
			warn("Problem: can't reparent a %s managed by a WFloatWS"
				 "being reparented. Detaching from this object.",
				 WOBJ_TYPESTR(sub));
			region_detach_manager(sub);
		}
	}
	
	return TRUE;
}


static void map_floatws(WFloatWS *ws)
{
	WRegion *reg;

	MARK_REGION_MAPPED(ws);
	XMapWindow(wglobal.dpy, ws->dummywin);
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
		map_region(reg);
	}
}


static void unmap_floatws(WFloatWS *ws)
{
	WRegion *reg;
	
	MARK_REGION_UNMAPPED(ws);
	XUnmapWindow(wglobal.dpy, ws->dummywin);
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
		unmap_region(reg);
	}
}


static void focus_floatws(WFloatWS *ws, bool warp)
{
	if(ws->current_managed==NULL){
		SET_FOCUS(ws->dummywin);
		if(warp)
			do_move_pointer_to((WRegion*)ws);
		return;
	}

	focus_region(ws->current_managed, warp);
}


static bool floatws_display_managed(WFloatWS *ws, WRegion *reg)
{
	if(!region_is_fully_mapped((WRegion*)ws))
	   return FALSE;
	map_region(reg);
	return TRUE;
}


static void floatws_remove_managed(WFloatWS *ws, WRegion *reg)
{
	WRegion *next=NULL;
	
	region_unset_manager(reg, (WRegion*)ws, &(ws->managed_list));
	if(ws->current_managed==reg){
		next=NEXT_MANAGED_WRAP(ws->managed_list, reg);
		ws->current_managed=NULL;
	}else{
		/* REGION_IS_ACTIVE(reg) and ws->current_managed!=reg should
		 * not happen.
		 */
		next=ws->current_managed;
	}
	
	region_remove_bindmap_owned(reg, &floatws_bindmap, (WRegion*)ws);
	
	if(REGION_IS_ACTIVE(reg))
		do_set_focus(next!=NULL ? next : (WRegion*)ws, FALSE);
}


static void floatws_managed_activated(WFloatWS *ws, WRegion *reg)
{
	ws->current_managed=reg;
}


static Window floatws_x_window(const WFloatWS *ws)
{
	return ws->dummywin;
}


/*}}}*/


/*{{{ Create/destroy */


static bool init_floatws(WFloatWS *ws, WWindow *parent, WRectangle bounds)
{
	if(!WOBJ_IS(parent, WWindow))
		return FALSE;

	ws->dummywin=XCreateWindow(wglobal.dpy, parent->win,
								bounds.x, bounds.y, 1, 1, 0,
								CopyFromParent, InputOnly,
								CopyFromParent, 0, NULL);
	if(ws->dummywin==None)
		return FALSE;

	XSelectInput(wglobal.dpy, ws->dummywin,
				 FocusChangeMask|KeyPressMask|KeyReleaseMask|
				 ButtonPressMask|ButtonReleaseMask);
	XSaveContext(wglobal.dpy, ws->dummywin, wglobal.win_context,
				 (XPointer)ws);
	
	init_genws(&(ws->genws), parent, bounds);

	ws->managed_list=NULL;
	ws->current_managed=NULL;
	
	region_add_bindmap((WRegion*)ws, &floatws_bindmap);
	
	return TRUE;
}


WFloatWS *create_floatws(WWindow *parent, WRectangle bounds)
{
	CREATEOBJ_IMPL(WFloatWS, floatws, (p, parent, bounds));
}


void deinit_floatws(WFloatWS *ws)
{
	while(ws->managed_list!=NULL)
		floatws_remove_managed(ws, ws->managed_list);

	deinit_genws(&(ws->genws));

	XDeleteContext(wglobal.dpy, ws->dummywin, wglobal.win_context);
	XDestroyWindow(wglobal.dpy, ws->dummywin);
}


/*}}}*/


/*{{{ add_clientwin/transient */


static void floatws_add_managed(WFloatWS *ws, WRegion *reg)
{
	region_set_manager(reg, (WRegion*)ws, &(ws->managed_list));
	region_add_bindmap_owned(reg, &floatws_bindmap, (WRegion*)ws);

	if(region_is_fully_mapped((WRegion*)ws))
		map_region(reg);
}


WRegion *find_existing(WFloatWS *ws)
{
	WRegion *r=ws->current_managed;
	if(r!=NULL && region_supports_add_managed(r))
		return r;
			
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, r){
		if(region_supports_add_managed(r))
			return r;
	}
	return NULL;
}


static bool floatws_add_clientwin(WFloatWS *ws, WClientWin *cwin,
								  const XWindowAttributes *attr,
								  int init_state, WWinProp *props)
{
	WRegion *target=NULL;
	WWindow *par;
	bool geomset=FALSE;
	bool newreg=FALSE;

	par=FIND_PARENT1(ws, WWindow);
	assert(par!=NULL);

#if 0 /*def CF_PLACEMENT_GEOM*/
	geomset=(cwin->size_hints.win_gravity!=ForgetGravity &&
			 (attr->x>CF_STUBBORN_TRESH &&
			  attr->y>CF_STUBBORN_TRESH));
	if(geomset && (!props || !props->stubborn))
		target=(WRegion*)find_frame_at(ws, attr->x, attr->y);
#endif
	
#ifdef CF_FLOATWS_ATTACH_TO_CURRENT
	if(target==NULL)
		target=find_existing(ws);
#endif
		
	if(target==NULL){
		WRectangle fgeom=initial_to_floatframe_geom(GRDATA_OF(ws),
													REGION_GEOM(cwin),
													attr->win_gravity);
		
		target=(WRegion*)create_floatframe(par, fgeom, 0);
	
		if(target==NULL){
			warn("Failed to create a new WFloatFrame for client window");
			return FALSE;
		}

		floatws_add_managed(ws, target);
		newreg=TRUE;
	}

	assert(SCREEN_OF(target)==SCREEN_OF(cwin));
	
	if(!finish_add_clientwin(target, cwin, init_state, props)){
		if(newreg)
			destroy_obj((WObj*)target);
		return FALSE;
	}
	
#ifdef CF_SWITCH_NEW_CLIENTS
	/* TODO: dummy InputOnly window */
	if(region_manages_active_reg((WRegion*)ws)){
		display_region_sp((WRegion*)cwin);
		set_focus((WRegion*)cwin);
	}
#endif
	
	return TRUE;
}


static bool floatws_add_transient(WFloatWS *ws, WClientWin *tfor,
								  WClientWin *cwin,
								  const XWindowAttributes *attr,
								  int init_state, WWinProp *props)
{
	if(floatws_add_clientwin(ws, cwin, attr, init_state, props)){
		/* TODO: set up stacking */
		return TRUE;
	}
	
	return FALSE;
}


static bool floatws_handle_drop(WFloatWS *ws, int x, int y,
								WRegion *dropped)
{
	WRectangle fgeom;
	WRegion *target;
	WWindow *par;
	
	par=FIND_PARENT1(ws, WWindow);
	
	if(par==NULL)
		return FALSE;
	
	fgeom=initial_to_floatframe_geom(GRDATA_OF(ws), REGION_GEOM(dropped),
									 ForgetGravity);
	fgeom.x=x;
	fgeom.y=y;

	target=(WRegion*)create_floatframe(par, fgeom, 0);
	
	if(target==NULL){
		warn("Failed to create a new WFloatFrame.");
		return FALSE;
	}
	
	if(!region_add_managed(target, dropped, REGION_ATTACH_SWITCHTO)){
		destroy_obj((WObj*)target);
		warn("Failed to attach dropped region to created WFloatFrame");
		return FALSE;
	}
	
	floatws_add_managed(ws, target);

	return TRUE;
}


/*}}}*/


/*{{{ Circulate */


void floatws_circulate(WFloatWS *ws)
{
	WRegion *r=NEXT_MANAGED_WRAP(ws->managed_list, ws->current_managed);
	if(r!=NULL)
		goto_region(r);
}

void floatws_backcirculate(WFloatWS *ws)
{
	WRegion *r=PREV_MANAGED_WRAP(ws->managed_list, ws->current_managed);
	if(r!=NULL)
		goto_region(r);
}


/*}}}*/


/*{{{ Save/load */


static bool floatws_save_to_file(WFloatWS *ws, FILE *file, int lvl)
{
	begin_saved_region((WRegion*)ws, file, lvl);
	end_saved_region((WRegion*)ws, file, lvl);
	return TRUE;
}


WRegion *floatws_load(WWindow *par, WRectangle geom, Tokenizer *tokz)
{
	WFloatWS *ws;
	
	ws=create_floatws(par, geom);
	
	parse_config_tokz_skip_section(tokz);
	
	return (WRegion*)ws;
}


/*}}}*/


