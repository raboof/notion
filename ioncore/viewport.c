/*
 * ion/ioncore/viewport.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "global.h"
#include "viewport.h"
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


static bool viewport_display_managed(WViewport *vp, WRegion *reg);


/*{{{ Init/deinit */


static bool viewport_init(WViewport *vp, WScreen *scr,
						  int id, WRectangle geom)
{
	vp->ws_count=0;
	vp->current_ws=NULL;
	vp->ws_list=NULL;
	vp->id=id;
	vp->atom_workspace=None;
	
	region_init((WRegion*)vp, (WRegion*)scr, geom);
	
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
	CREATEOBJ_IMPL(WViewport, viewport, (p, scr, id, geom));
}


void viewport_deinit(WViewport *vp)
{
	while(vp->ws_list!=NULL)
		region_unset_manager(vp->ws_list, (WRegion*)vp, &(vp->ws_list));
		
	region_deinit((WRegion*)vp);
}


static bool create_initial_workspace_on_vp(WViewport *vp)
{
	WRegionSimpleCreateFn *fn;
	WRegion *reg;
	
	fn=lookup_region_simple_create_fn_inh("WGenWS");
	
	if(fn==NULL){
		warn("Could not find a complete workspace class. "
			 "Please load some modules.");
		return FALSE;
	}
	
	reg=region_add_managed_new_simple((WRegion*)vp, fn, 0);
	
	if(reg==NULL){
		warn("Unable to create a workspace on viewport %d\n", vp->id);
		return FALSE;
	}
	
	region_set_name(reg, "main");
	return TRUE;
}


bool viewport_initialize_workspaces(WViewport* vp)
{
	WRegion *ws=NULL;
	char *wsname=NULL;

	if(vp->atom_workspace!=None)
		wsname=get_string_property(ROOT_OF(vp), vp->atom_workspace, NULL);

	load_workspaces(vp);
	
	if(vp->ws_count==0){
		if(!create_initial_workspace_on_vp(vp))
			return FALSE;
	}else{
		if(wsname!=NULL)
			ws=lookup_region(wsname);
		if(ws==NULL || REGION_MANAGER(ws)!=(WRegion*)vp)
			ws=FIRST_MANAGED(vp->ws_list);
		if(ws!=NULL)
			viewport_display_managed(vp, ws);
	}
	
	if(wsname!=NULL)
		free(wsname);
	
	return TRUE;
}


/*}}}*/


/*{{{ Attach/detach */


static WRegion *viewport_do_add_managed(WViewport *vp, WRegionAddFn *fn,
										void *fnparams, 
										const WAttachParams *param)
{
	WWindow *par=FIND_PARENT1(vp, WWindow);
	WRegion *reg;
	
	if(par==NULL)
		return NULL;
	
	reg=fn(par, REGION_GEOM(vp), fnparams);
	
	if(reg==NULL)
		return NULL;

	region_set_manager(reg, (WRegion*)vp, &(vp->ws_list));
	vp->ws_count++;
	
	if(vp->current_ws==NULL || param->flags&REGION_ATTACH_SWITCHTO)
		viewport_display_managed(vp, reg);
	else
		region_unmap(reg);
	
	return reg;
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


static void viewport_fit(WViewport *vp, WRectangle geom)
{
	WRegion *sub;
	
	REGION_GEOM(vp)=geom;
	
	geom.x=0;
	geom.y=0;
	
	FOR_ALL_MANAGED_ON_LIST(vp->ws_list, sub){
		region_fit(sub, geom);
	}
}


static void viewport_map(WViewport *vp)
{
	MARK_REGION_MAPPED(vp);
	if(vp->current_ws!=NULL)
		region_map(vp->current_ws);
}


static void viewport_unmap(WViewport *vp)
{
	MARK_REGION_UNMAPPED(vp);
	if(vp->current_ws!=NULL)
		region_unmap(vp->current_ws);
}


static void viewport_set_focus_to(WViewport *vp, bool warp)
{
	if(vp->current_ws!=NULL)
		region_set_focus_to(vp->current_ws, warp);
}


static bool viewport_display_managed(WViewport *vp, WRegion *reg)
{
	char *n;
	
	if(reg==vp->current_ws)
		return FALSE;

	if(region_is_fully_mapped((WRegion*)vp))
		region_map(reg);
	
	if(vp->current_ws!=NULL)
		region_unmap(vp->current_ws);
	
	vp->current_ws=reg;
	
	if(vp->atom_workspace!=None && wglobal.opmode!=OPMODE_DEINIT){
		n=region_full_name(reg);
		
		if(n==NULL){
			set_string_property(ROOT_OF(vp), vp->atom_workspace, "");
		}else{
			set_string_property(ROOT_OF(vp), vp->atom_workspace, n);
			free(n);
		}
	}
	
	if(wglobal.opmode!=OPMODE_DEINIT && region_may_control_focus((WRegion*)vp))
		set_focus(reg);
	
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


EXTL_EXPORT
void viewport_switch_nth(WViewport *vp, uint n)
{
	WRegion *sub=nth_ws(vp, n);
	if(sub!=NULL)
		region_display_sp(sub);
}


EXTL_EXPORT
void viewport_switch_next(WViewport *vp)
{
	WRegion *reg=NEXT_MANAGED_WRAP(vp->ws_list, vp->current_ws);
	if(reg!=NULL)
		region_display_sp(reg);
}


EXTL_EXPORT
void viewport_switch_prev(WViewport *vp)
{
	WRegion *reg=PREV_MANAGED_WRAP(vp->ws_list, vp->current_ws);
	if(reg!=NULL)
		region_display_sp(reg);
}


/*}}}*/


/*{{{ Misc. */


EXTL_EXPORT
WViewport *region_viewport_of(WRegion *reg)
{
	while(reg!=NULL){
		if(WOBJ_IS(reg, WViewport))
			return (WViewport*)reg;
		reg=REGION_MANAGER(reg);
	}
	return NULL;
}


EXTL_EXPORT
WViewport *find_viewport_id(int id)
{
	WScreen *scr;
	WViewport *vp, *maxvp=NULL;
	
	for(scr=wglobal.screens; scr!=NULL; scr=NEXT_CHILD(scr, WScreen)){
		FOR_ALL_TYPED_CHILDREN(scr, vp, WViewport){
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


EXTL_EXPORT
void goto_nth_viewport(int id)
{
	WViewport *vp=find_viewport_id(id);
	if(vp!=NULL)
		region_goto((WRegion*)vp);
}


EXTL_EXPORT
void goto_next_viewport()
{
	WViewport *vp=screen_current_vp(wglobal.active_screen);
	
	if(vp!=NULL)
		vp=find_viewport_id(vp->id+1);
	if(vp==NULL)
		vp=find_viewport_id(0);
	if(vp!=NULL)
		region_goto((WRegion*)vp);
}


EXTL_EXPORT
void goto_prev_viewport()
{
	WViewport *vp=screen_current_vp(wglobal.active_screen);
	
	if(vp==NULL)
		vp=find_viewport_id(0);
	else if(vp->id==0)
		vp=find_viewport_id(-1);
	else
		vp=find_viewport_id(vp->id-1);
	if(vp!=NULL)
		region_goto((WRegion*)vp);
}


EXTL_EXPORT
WRegion *viewport_current(WViewport *vp)
{
	return vp->current_ws;
}


EXTL_EXPORT
int viewport_id(WViewport *vp)
{
	return vp->id;
}


static bool viewport_may_destroy_managed(WViewport *vp, WRegion *reg)
{
	WRegion *r2;
	
	FOR_ALL_MANAGED_ON_LIST(vp->ws_list, r2){
		if(WOBJ_IS(r2, WGenWS) && r2!=reg)
			return TRUE;
	}
	
	warn("Cannot destroy only workspace.");
	return FALSE;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab viewport_dynfuntab[]={
	{region_fit, viewport_fit},
	{region_map, viewport_map},
	{region_unmap, viewport_unmap},
	{region_set_focus_to, viewport_set_focus_to},
	
	{(DynFun*)region_display_managed, (DynFun*)viewport_display_managed},
	{region_request_managed_geom, region_request_managed_geom_unallow},
	{(DynFun*)region_do_add_managed, (DynFun*)viewport_do_add_managed},
	{region_remove_managed, viewport_remove_managed},
	
	{(DynFun*)region_may_destroy_managed,
	 (DynFun*)viewport_may_destroy_managed},
	
	END_DYNFUNTAB
};


IMPLOBJ(WViewport, WRegion, viewport_deinit, viewport_dynfuntab);


/*}}}*/
