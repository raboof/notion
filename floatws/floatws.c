/*
 * ion/floatws/floatws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <ioncore/common.h>
#include <ioncore/rootwin.h>
#include <ioncore/focus.h>
#include <ioncore/global.h>
#include <ioncore/objp.h>
#include <ioncore/region.h>
#include <ioncore/manage.h>
#include <ioncore/screen.h>
#include <ioncore/names.h>
#include <ioncore/saveload.h>
#include <ioncore/attach.h>
#include <ioncore/regbind.h>
#include <ioncore/genframe-pointer.h>
#include <ioncore/stacking.h>
#include <ioncore/extlconv.h>
#include <ioncore/defer.h>

#include "floatws.h"
#include "floatframe.h"
#include "placement.h"
#include "main.h"


/*{{{ region dynfun implementations */


static void floatws_fit(WFloatWS *ws, WRectangle geom)
{
	REGION_GEOM(ws)=geom;
}


static bool reparent_floatws(WFloatWS *ws, WWindow *parent, WRectangle geom)
{
	WRegion *sub, *next;
	bool rs;
	int xdiff, ydiff;
	
	if(!same_rootwin((WRegion*)ws, (WRegion*)parent))
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


static void floatws_map(WFloatWS *ws)
{
	WRegion *reg;

	MARK_REGION_MAPPED(ws);
	XMapWindow(wglobal.dpy, ws->dummywin);
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
		region_map(reg);
	}
}


static void floatws_unmap(WFloatWS *ws)
{
	WRegion *reg;
	
	MARK_REGION_UNMAPPED(ws);
	XUnmapWindow(wglobal.dpy, ws->dummywin);
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
		region_unmap(reg);
	}
}


static void floatws_set_focus_to(WFloatWS *ws, bool warp)
{
	WRegion *r=ws->current_managed;
		
	if(r==NULL)
		r=ws->managed_list;

	if(r==NULL){
		SET_FOCUS(ws->dummywin);
		if(warp)
			do_move_pointer_to((WRegion*)ws);
		return;
	}
	
	region_set_focus_to(r, warp);
}


static bool floatws_display_managed(WFloatWS *ws, WRegion *reg)
{
	if(!region_is_fully_mapped((WRegion*)ws))
	   return FALSE;
	region_map(reg);
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


static bool floatws_init(WFloatWS *ws, WWindow *parent, WRectangle bounds)
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
	
	genws_init(&(ws->genws), parent, bounds);

	ws->managed_list=NULL;
	ws->current_managed=NULL;
	
	region_add_bindmap((WRegion*)ws, &floatws_bindmap);
	
	return TRUE;
}


WFloatWS *create_floatws(WWindow *parent, WRectangle bounds)
{
	CREATEOBJ_IMPL(WFloatWS, floatws, (p, parent, bounds));
}


void floatws_deinit(WFloatWS *ws)
{
	while(ws->managed_list!=NULL)
		destroy_obj((WObj*)(ws->managed_list));

	genws_deinit(&(ws->genws));

	XDeleteContext(wglobal.dpy, ws->dummywin, wglobal.win_context);
	XDestroyWindow(wglobal.dpy, ws->dummywin);
}


/*EXTL_DOC
 * Destroys \var{ws} unless this would put the WM in a possibly unusable
 * state.
 */
EXTL_EXPORT
bool floatws_destroy(WFloatWS *ws)
{
	if(!region_may_destroy((WRegion*)ws))
		return FALSE;
	
	/* TODO: move frames to other workspaces */
	
	if(!rescue_managed_clientwins((WRegion*)ws, ws->managed_list))
		return FALSE;
	
	defer_destroy((WObj*)ws);
	return TRUE;
}


/*}}}*/


/*{{{ add_clientwin/transient */


static void floatws_add_managed(WFloatWS *ws, WRegion *reg)
{
	region_set_manager(reg, (WRegion*)ws, &(ws->managed_list));
	region_add_bindmap_owned(reg, &floatws_bindmap, (WRegion*)ws);

	if(region_is_fully_mapped((WRegion*)ws))
		region_map(reg);
}


static WRegion *floatws_do_add_managed(WFloatWS *ws, WRegionAddFn *fn,
									   void *fnparams, 
									   const WAttachParams *param)
{
	WRectangle geom;
	WWindow *par;
	WRegion *reg;

	if(REGION_ATTACH_IS_GEOMRQ(param->flags)){
		geom=param->geomrq;
	}else{
		warn("Attempt to add region that does not request geometry on a "
			 "WFloatWS");
		return NULL;
	}
	
	par=REGION_PARENT_CHK(ws, WWindow);
	assert(par!=NULL);
	
	reg=fn(par, geom, fnparams);

	if(reg!=NULL)
		floatws_add_managed(ws, reg);
	
	return reg;
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


static bool floatws_add_clientwin(WFloatWS *ws,
								  WClientWin *cwin,
								  const WAttachParams *params)
{
	WRegion *target=NULL;
	WWindow *par;
	bool newreg=FALSE;
	bool respectpos=FALSE;

	par=REGION_PARENT_CHK(ws, WWindow);
	assert(par!=NULL);
	
#ifdef CF_FLOATWS_ATTACH_TO_CURRENT
	if(target==NULL)
		target=find_existing(ws);
#endif
	
	if(target==NULL){
		WRectangle fgeom;
		int gravity=NorthWestGravity;
		
		if(params->flags&REGION_ATTACH_SIZERQ){
			fgeom.w=params->geomrq.w;
			fgeom.h=params->geomrq.h;
		}else{
			fgeom.w=REGION_GEOM(cwin).w;
			fgeom.h=REGION_GEOM(cwin).h;
		}

		if(params->flags&REGION_ATTACH_POSRQ){
			fgeom.x=params->geomrq.x-REGION_GEOM(ws).x;
			fgeom.y=params->geomrq.y-REGION_GEOM(ws).x;
			respectpos=TRUE;
		}else{
			fgeom.x=0;
			fgeom.y=0;
		}
		
		if(params->flags&REGION_ATTACH_SIZE_HINTS &&
		   params->size_hints->flags&PWinGravity){
			gravity=params->size_hints->win_gravity;
		}
		
		fgeom=initial_to_floatframe_geom(GRDATA_OF(ws), fgeom, gravity);

		if(params->flags&REGION_ATTACH_MAPRQ && wglobal.opmode!=OPMODE_INIT){
			/* When the window is mapped by application request, position
			 * request is only honoured if the position was given by the user
			 * and in case of a transient (the app may know better where to 
			 * place them).
			 */
			respectpos=(cwin->transient_for!=None ||
						cwin->size_hints.flags&USPosition);
		}

		/* However, if the requested geometry does not overlap the
		 * workspaces's geometry, position request is never honoured.
		 */
		if((fgeom.x+fgeom.w<=REGION_GEOM(ws).x) ||
		   (fgeom.y+fgeom.h<=REGION_GEOM(ws).y) ||
		   (fgeom.x>=REGION_GEOM(ws).x+REGION_GEOM(ws).w) ||
		   (fgeom.y>=REGION_GEOM(ws).y+REGION_GEOM(ws).h)){
			respectpos=FALSE;
		}

		if(!respectpos)
			floatws_calc_placement(ws, &fgeom);
		
		target=(WRegion*)create_floatframe(par, fgeom, 0);
	
		if(target==NULL){
			warn("Failed to create a new WFloatFrame for client window");
			return FALSE;
		}

		floatws_add_managed(ws, target);
		newreg=TRUE;
	}

	assert(same_rootwin(target, (WRegion*)cwin));
	
	if(!finish_add_clientwin(target, cwin, params)){
		if(newreg)
			destroy_obj((WObj*)target);
		return FALSE;
	}
	
	if(clientwin_get_switchto(cwin)){
		if(region_may_control_focus((WRegion*)ws)){
			region_display_sp((WRegion*)cwin);
			set_focus((WRegion*)cwin);
		}
	}
	
	return TRUE;
}


static bool floatws_handle_drop(WFloatWS *ws, int x, int y,
								WRegion *dropped)
{
	WRectangle fgeom;
	WRegion *target;
	WWindow *par;
	
	par=REGION_PARENT_CHK(ws, WWindow);
	
	if(par==NULL)
		return FALSE;
	
	fgeom=initial_to_floatframe_geom(GRDATA_OF(ws), REGION_GEOM(dropped),
									 ForgetGravity);
	
	/* The x and y arguments are in root coordinate space */
	region_rootpos((WRegion*)par, &fgeom.x, &fgeom.y);
	fgeom.x=x-fgeom.x;
	fgeom.y=y-fgeom.y;

	target=(WRegion*)create_floatframe(par, fgeom, 0);
	
	if(target==NULL){
		warn("Failed to create a new WFloatFrame.");
		return FALSE;
	}
	
	if(!region_add_managed_simple(target, dropped, REGION_ATTACH_SWITCHTO)){
		destroy_obj((WObj*)target);
		warn("Failed to attach dropped region to created WFloatFrame");
		return FALSE;
	}
	
	floatws_add_managed(ws, target);

	return TRUE;
}


/* A handler for add_clientwin_alt hook to handle transients for windows
 * on WFloatWS:s differently from the normal behaviour.
 */
bool add_clientwin_floatws_transient(WClientWin *cwin, WAttachParams *param)
{
	WRegion *stack_above;
	WFloatWS *ws;
	
	if(!(param->flags&REGION_ATTACH_TFOR))
		return FALSE;

	stack_above=(WRegion*)REGION_PARENT_CHK(param->tfor, WFloatFrame);
	if(stack_above==NULL)
		return FALSE;
	
	ws=REGION_MANAGER_CHK(stack_above, WFloatWS);
	if(ws==NULL)
		return FALSE;
	
	if(!floatws_add_clientwin(ws, cwin, param))
		return FALSE;

	region_stack_above(REGION_MANAGER(cwin), stack_above);

	return TRUE;
}


/*}}}*/


/*{{{ Circulate */


/*EXTL_DOC
 * Activate next object on \var{ws}.
 */
EXTL_EXPORT
WRegion *floatws_circulate(WFloatWS *ws)
{
	WRegion *r=NEXT_MANAGED_WRAP(ws->managed_list, ws->current_managed);
	if(r!=NULL)
		region_goto(r);
	return r;
}


/*EXTL_DOC
 * Activate previous object on \var{ws}.
 */
EXTL_EXPORT
WRegion *floatws_backcirculate(WFloatWS *ws)
{
	WRegion *r=PREV_MANAGED_WRAP(ws->managed_list, ws->current_managed);
	if(r!=NULL)
		region_goto(r);
	return r;
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Returns a list of regions managed by the workspace (frames, mostly).
 */
EXTL_EXPORT
ExtlTab floatws_managed_list(WFloatWS *ws)
{
	return managed_list_to_table(ws->managed_list, NULL);
}


/*}}}*/


/*{{{ Save/load */


static bool floatws_save_to_file(WFloatWS *ws, FILE *file, int lvl)
{
	WRegion *mgd;
	
	begin_saved_region((WRegion*)ws, file, lvl);
	save_indent_line(file, lvl);
	fprintf(file, "managed = {\n");
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, mgd){
		save_indent_line(file, lvl+1);
		fprintf(file, "{\n");
		if(region_save_to_file((WRegion*)mgd, file, lvl+2))
			save_geom(REGION_GEOM(mgd), file, lvl+2);
		save_indent_line(file, lvl+1);
		fprintf(file, "},\n");
	}
	save_indent_line(file, lvl);
	fprintf(file, "},\n");
	
	return TRUE;
}


WRegion *floatws_load(WWindow *par, WRectangle geom, ExtlTab tab)
{
	WFloatWS *ws;
	ExtlTab substab, subtab;
	int i, n;
	
	ws=create_floatws(par, geom);
	
	if(ws==NULL)
		return NULL;
		
	if(!extl_table_gets_t(tab, "managed", &substab))
		return (WRegion*)ws;

	n=extl_table_get_n(substab);
	for(i=1; i<=n; i++){
		if(extl_table_geti_t(substab, i, &subtab)){
			region_add_managed_load((WRegion*)ws, subtab);
			extl_unref_table(subtab);
		}
	}
	
	extl_unref_table(substab);

	return (WRegion*)ws;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab floatws_dynfuntab[]={
	{region_fit, floatws_fit},
	{region_map, floatws_map},
	{region_unmap, floatws_unmap},
	{region_set_focus_to, floatws_set_focus_to},
	{(DynFun*)reparent_region, (DynFun*)reparent_floatws},
	
	{(DynFun*)genws_add_clientwin, (DynFun*)floatws_add_clientwin},

	{region_managed_activated, floatws_managed_activated},
	{region_remove_managed, floatws_remove_managed},
	{(DynFun*)region_display_managed, (DynFun*)floatws_display_managed},
	
	{(DynFun*)region_save_to_file, (DynFun*)floatws_save_to_file},

	{(DynFun*)region_x_window, (DynFun*)floatws_x_window},

	{(DynFun*)region_handle_drop, (DynFun*)floatws_handle_drop},

	{(DynFun*)region_do_add_managed, (DynFun*)floatws_do_add_managed},
	
	END_DYNFUNTAB
};


IMPLOBJ(WFloatWS, WGenWS, floatws_deinit, floatws_dynfuntab);


/*}}}*/

