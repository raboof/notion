/*
 * wmcore/viewport.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "global.h"
#include "viewport.h"
#include "region.h"
#include "attach.h"
#include "funtabs.h"
#include "thingp.h"
#include "focus.h"
#include "regbind.h"
#include "property.h"
#include "names.h"


static void fit_viewport(WViewport *vp, WRectangle geom);
static void map_viewport(WViewport *vp);
static void unmap_viewport(WViewport *vp);
static void focus_viewport(WViewport *vp, bool warp);
static void deinit_viewport(WViewport *vp);

static bool viewport_display_managed(WViewport *vp, WRegion *reg);
static void viewport_remove_managed(WViewport *vp, WRegion *reg);
static void viewport_add_managed_params(const WViewport *vp, WRegion **par,
										WRectangle *ret);
/*static void viewport_add_managed_doit(WViewport *vp, WRegion *sub, int flags);*/


static DynFunTab viewport_dynfuntab[]={
	{fit_region, fit_viewport},
	{map_region, map_viewport},
	{unmap_region, unmap_viewport},
	{focus_region, focus_viewport},
	
	{(DynFun*)region_display_managed, (DynFun*)viewport_display_managed},
	{region_request_managed_geom, region_request_managed_geom_unallow},
	{region_add_managed_params, viewport_add_managed_params},
	{region_add_managed_doit, viewport_add_managed_doit},
	{region_remove_managed, viewport_remove_managed},
	
	END_DYNFUNTAB
};


/*{{{ Init/deinit */


IMPLOBJ(WViewport, WRegion, deinit_viewport, viewport_dynfuntab, NULL)


static bool init_viewport(WViewport *vp, WScreen *scr,
						  int id, WRectangle geom)
{
	vp->ws_count=0;
	vp->current_ws=NULL;
	vp->ws_list=NULL;
	vp->id=id;
	vp->atom_workspace=None;
	
	init_region((WRegion*)vp, (WRegion*)scr, geom);
	
	if(id==0){
		vp->atom_workspace=XInternAtom(wglobal.dpy, "_ION_WORKSPACE", False);
	}else if(id>=0){
		char *str;
		libtu_asprintf(&str, "_ION_WORKSPACE%d", id);
		if(str==NULL){
			warn_err();
		}else{
			vp->atom_workspace=XInternAtom(wglobal.dpy, str, False);
			free(str);
		}
	}
	
	return TRUE;
}


WViewport *create_viewport(WScreen *scr, int id, WRectangle geom)
{
	CREATETHING_IMPL(WViewport, viewport, (p, scr, id, geom));
}


void deinit_viewport(WViewport *vp)
{
	return;
}


/*}}}*/


/*{{{ Attach/detach */


static void viewport_add_managed_params(const WViewport *vp, WRegion **par,
										WRectangle *ret)
{
	*par=FIND_PARENT1(vp, WRegion);
	*ret=REGION_GEOM(vp);
}


void viewport_add_managed_doit(WViewport *vp, WRegion *reg, int flags)
{
	region_set_manager(reg, (WRegion*)vp, &(vp->ws_list));
	vp->ws_count++;
	
	if(vp->current_ws==NULL)
		flags|=REGION_ATTACH_SWITCHTO;

	if(flags&REGION_ATTACH_SWITCHTO)
		viewport_display_managed(vp, reg);
	else
		unmap_region(reg);
}


static void viewport_remove_managed(WViewport *vp, WRegion *reg)
{
	WRegion *next=NULL;

	if(vp->current_ws==reg){
		next=PREV_MANAGED(vp->ws_list, reg);
		if(next==NULL)
			next=NEXT_MANAGED(vp->ws_list, reg);
		vp->current_ws=NULL;
	}
	
	region_unset_manager(reg, (WRegion*)vp, &(vp->ws_list));
	vp->ws_count--;
	
	if(wglobal.opmode!=OPMODE_DEINIT){
		if(next!=NULL)
			viewport_display_managed(vp, next);
	}
}


/*}}}*/


/*{{{ region dynfun implementations */


static void fit_viewport(WViewport *vp, WRectangle geom)
{
	WRegion *sub;
	
	REGION_GEOM(vp)=geom;
	
	geom.x=0;
	geom.y=0;
	
	FOR_ALL_MANAGED_ON_LIST(vp->ws_list, sub){
		fit_region(sub, geom);
	}
}


static void map_viewport(WViewport *vp)
{
	MARK_REGION_MAPPED(vp);
	if(vp->current_ws!=NULL)
		map_region(vp->current_ws);
}


static void unmap_viewport(WViewport *vp)
{
	MARK_REGION_UNMAPPED(vp);
	if(vp->current_ws!=NULL)
		unmap_region(vp->current_ws);
}


static void focus_viewport(WViewport *vp, bool warp)
{
	if(vp->current_ws!=NULL)
		focus_region(vp->current_ws, warp);
}


static bool viewport_display_managed(WViewport *vp, WRegion *reg)
{
	char *n;
	
	if(reg==vp->current_ws)
		return FALSE;
	
	if(vp->current_ws!=NULL)
		unmap_region(vp->current_ws);
	
	vp->current_ws=reg;
	if(region_is_fully_mapped((WRegion*)vp))
		map_region(reg);
	
	if(vp->atom_workspace!=None && wglobal.opmode!=OPMODE_DEINIT){
		n=region_full_name(reg);
		
		if(n==NULL){
			set_string_property(ROOT_OF(vp), vp->atom_workspace, "");
		}else{
			set_string_property(ROOT_OF(vp), vp->atom_workspace, n);
			free(n);
		}
	}
	
	if(region_manages_active_reg(vp))
		do_set_focus(reg, wglobal.warp_enabled);
	
	return TRUE;
}


/*}}}*/


/*{{{ Workspace switch */


static WRegion *nth_ws(WViewport *vp, uint n)
{
	WRegion *r=vp->ws_list;
	
	/*return NTH_MANAGED(vp->ws_list, n);*/
	while(n-->0){
		r=NEXT_MANAGED(vp->ws_list, r);
		if(r==NULL)
			break;
	}
	
	return r;
}


void viewport_display_nth(WViewport *vp, uint n)
{
	WRegion *sub=nth_ws(vp, n);
	if(sub!=NULL)
		display_region_sp(sub);
}


void viewport_display_next(WViewport *vp)
{
	WRegion *reg=NEXT_MANAGED_WRAP(vp->ws_list, vp->current_ws);
	if(reg!=NULL)
		display_region_sp(reg);
}


void viewport_display_prev(WViewport *vp)
{
	WRegion *reg=PREV_MANAGED_WRAP(vp->ws_list, vp->current_ws);
	if(reg!=NULL)
		display_region_sp(reg);
}


/*}}}*/


/*{{{ Misc. */


WViewport *viewport_of(WRegion *reg)
{
	while(reg!=NULL){
		if(WTHING_IS(reg, WViewport))
			return (WViewport*)reg;
		reg=REGION_MANAGER(reg);
	}
	return NULL;
}


WViewport *current_vp(WScreen *scr)
{
	if(scr==NULL)
		return NULL;
	
	if(REGION_ACTIVE_SUB(scr)==NULL)
		return NULL;
	
	return viewport_of(REGION_ACTIVE_SUB(scr));
}


WViewport *find_viewport_id(int id)
{
	WScreen *scr;
	WViewport *vp, *maxvp=NULL;
	
	for(scr=wglobal.screens; scr!=NULL; scr=NEXT_THING(scr, WScreen)){
		FOR_ALL_TYPED(scr, vp, WViewport){
			if(id==-1){
				if(maxvp==NULL || vp->id>maxvp->id)
					maxvp=vp;
			}
			if(vp->id==id)
				return vp;
		}
	}
	
	return maxvp;
}


void goto_viewport_id(int id)
{
	WViewport *vp=find_viewport_id(id);
	if(vp!=NULL)
		goto_region((WRegion*)vp);
}


void goto_next_viewport()
{
	WViewport *vp;
	
	vp=current_vp(wglobal.active_screen);
	
	if(vp!=NULL)
		vp=find_viewport_id(vp->id+1);
	if(vp==NULL)
		vp=find_viewport_id(0);
	if(vp!=NULL)
		goto_region((WRegion*)vp);
}


void goto_prev_viewport()
{
	WScreen *scr=wglobal.active_screen;
	WViewport *vp;
	
	vp=current_vp(wglobal.active_screen);
	
	if(vp==NULL)
		vp=find_viewport_id(0);
	else if(vp->id==0)
		vp=find_viewport_id(-1);
	else
		vp=find_viewport_id(vp->id-1);
	if(vp!=NULL)
		goto_region((WRegion*)vp);
}


void switch_ws_nth(uint n)
{
	WViewport *vp=current_vp(wglobal.active_screen);
	if(vp!=NULL)
		viewport_display_nth(vp, n);
}


void switch_ws_next()
{
	WViewport *vp=current_vp(wglobal.active_screen);
	if(vp!=NULL)
		viewport_display_next(vp);
}


void switch_ws_prev()
{
	WViewport *vp=current_vp(wglobal.active_screen);
	if(vp!=NULL)
		viewport_display_prev(vp);
}


/*}}}*/


