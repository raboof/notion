/*
 * wmcore/viewport.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
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


static void viewport_remove_sub(WViewport *vp, WRegion *sub);
static void fit_viewport(WViewport *vp, WRectangle geom);
static void map_viewport(WViewport *vp);
static void unmap_viewport(WViewport *vp);
static bool viewport_switch_subregion(WViewport *vp, WRegion *sub);
static void focus_viewport(WViewport *vp, bool warp);

static void viewport_sub_params(const WViewport *vp, WWinGeomParams *ret);
static void viewport_do_attach(WViewport *vp, WRegion *sub, int flags);

static void deinit_viewport(WViewport *vp);


static DynFunTab viewport_dynfuntab[]={
	{fit_region, fit_viewport},
	{map_region, map_viewport},
	{unmap_region, unmap_viewport},
	{(DynFun*)switch_subregion, (DynFun*)viewport_switch_subregion},
	{focus_region, focus_viewport},
	{region_request_sub_geom, region_request_sub_geom_unallow},
	
	{region_do_attach_params, viewport_sub_params},
	{region_do_attach, viewport_do_attach},
	
	{region_remove_sub, viewport_remove_sub},
	
	END_DYNFUNTAB
};


/*{{{ Init/deinit */


IMPLOBJ(WViewport, WRegion, deinit_viewport, viewport_dynfuntab,
		&wmcore_viewport_funclist)


static bool init_viewport(WViewport *vp, WScreen *scr,
						  int id, WRectangle geom)
{
	vp->sub_count=0;
	vp->current_sub=NULL;
	vp->id=id;
	vp->atom_workspace=None;
	
	WTHING_INIT(vp, WViewport);
	
	init_region((WRegion*)vp, scr, geom);
	
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
	
	region_add_bindmap((WRegion*)vp, &wmcore_viewport_bindmap, TRUE);
	
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


static void viewport_sub_params(const WViewport *vp, WWinGeomParams *ret)
{
	WRectangle geom;
	
	geom.x=0;
	geom.y=0;
	geom.w=REGION_GEOM(vp).w;
	geom.h=REGION_GEOM(vp).h;
	
	region_rect_params((WRegion*)vp, geom, ret);
}


static void viewport_do_attach(WViewport *vp, WRegion *sub, int flags)
{
	/*if(vp->current_sub!=NULL && wglobal.opmode!=OPMODE_INIT)
		link_thing_after((WThing*)(vp->current_sub), (WThing*)sub);
	else*/{
		link_thing((WThing*)vp, (WThing*)sub);
	}
	vp->sub_count++;
	
	if(vp->sub_count==1)
		flags|=REGION_ATTACH_SWITCHTO;

	if(flags&REGION_ATTACH_SWITCHTO)
		viewport_switch_subregion(vp, sub);
	else
		unmap_region(sub);
}


/*
bool viewport_attach_sub(WViewport *vp, WRegion *sub, bool switchto)
{
	WWinGeomParams params;

	viewport_sub_params(vp, &params);
	detach_reparent_region(sub, params);
	viewport_do_attach(vp, sub, switchto ? REGION_ATTACH_SWITCHTO : 0);
	
	return TRUE;
}
*/

static bool viewport_do_detach_sub(WViewport *vp, WRegion *sub)
{
	WRegion *next=NULL;

	if(vp->current_sub==sub){
		next=PREV_THING(sub, WRegion);
		if(next==NULL)
			next=NEXT_THING(sub, WRegion);
		vp->current_sub=NULL;
	}
	
	unlink_thing((WThing*)sub);
	vp->sub_count--;
	
	if(wglobal.opmode!=OPMODE_DEINIT){
		if(next!=NULL)
			viewport_switch_subregion(vp, next);
	}

	return TRUE;
}


static void viewport_remove_sub(WViewport *vp, WRegion *sub)
{
	viewport_do_detach_sub(vp, sub);
}


/*}}}*/


/*{{{ region dynfun implementations */


static void fit_viewport(WViewport *vp, WRectangle geom)
{
	WRegion *sub;
	
	REGION_GEOM(vp)=geom;
	
	geom.x=0;
	geom.y=0;
	
	FOR_ALL_TYPED(vp, sub, WRegion){
		fit_region(sub, geom);
	}
}


static void map_viewport(WViewport *vp)
{
	MARK_REGION_MAPPED(vp);
	if(vp->current_sub!=NULL)
		map_region(vp->current_sub);
}


static void unmap_viewport(WViewport *vp)
{
	MARK_REGION_UNMAPPED(vp);
	if(vp->current_sub!=NULL)
		unmap_region(vp->current_sub);
}


static void focus_viewport(WViewport *vp, bool warp)
{
	if(vp->current_sub!=NULL)
		focus_region(vp->current_sub, warp);
}


static bool viewport_switch_subregion(WViewport *vp, WRegion *sub)
{
	char *n;
	WScreen *scr;
	
	if(sub==vp->current_sub)
		return FALSE;
	
	if(vp->current_sub!=NULL)
		unmap_region(vp->current_sub);
	
	vp->current_sub=sub;
	if(region_is_fully_mapped((WRegion*)vp))
		map_region(sub);

	if(vp->atom_workspace!=None && wglobal.opmode!=OPMODE_DEINIT){
		n=region_full_name(sub);

		if(n==NULL){
			set_string_property(ROOT_OF(vp), vp->atom_workspace, "");
		}else{
			set_string_property(ROOT_OF(vp), vp->atom_workspace, n);
			free(n);
		}
	}
	
	if(REGION_IS_ACTIVE(vp))
		do_set_focus(sub, FALSE);
		
	return TRUE;
}


/*}}}*/


/*{{{ Workspace switch */


static WRegion *nth_sub(WViewport *vp, uint n)
{
	return NTH_THING(vp, n, WRegion);
}


static WRegion *next_sub(WViewport *vp)
{
	return NEXT_THING_FB(vp->current_sub, WRegion,
						 FIRST_THING(vp, WRegion));
}


static WRegion *prev_sub(WViewport *vp)
{
	return PREV_THING_FB(vp->current_sub, WRegion,
						 LAST_THING(vp, WRegion));
}


void viewport_switch_nth(WViewport *vp, uint n)
{
	WRegion *sub=nth_sub(vp, n);
	if(sub!=NULL)
		switch_region(sub);
}


void viewport_switch_next(WViewport *vp)
{
	WRegion *sub=next_sub(vp);
	if(sub!=NULL)
		switch_region(sub);
}


void viewport_switch_prev(WViewport *vp)
{
	WRegion *sub=prev_sub(vp);
	if(sub!=NULL)
		switch_region(sub);
}


/*}}}*/


/*{{{ Misc. */


WViewport *viewport_of(WRegion *reg)
{
	return FIND_PARENT(reg, WViewport);
}


WViewport *find_viewport_id(int id)
{
	WScreen *scr;
	WViewport *vp;
	
	for(scr=wglobal.screens; scr!=NULL; scr=NEXT_THING(scr, WScreen)){
		FOR_ALL_TYPED(scr, vp, WViewport){
			if(vp->id==id)
				return vp;
		}
	}
	
	return NULL;
}


void goto_viewport_id(int id)
{
	WViewport *vp=find_viewport_id(id);
	if(vp!=NULL)
		goto_region((WRegion*)vp);
}


void goto_next_viewport()
{
	WScreen *scr=wglobal.active_screen, *tmpscr;
	WRegion *r;
	WViewport *vp;
	
	if(scr==NULL)
		scr=wglobal.screens;
	
	assert(scr!=NULL);
	
	r=REGION_ACTIVE_SUB(scr);
	if(r!=NULL)
		vp=NEXT_THING(r, WViewport);
	else
		vp=FIRST_THING(scr, WViewport);

	if(vp!=NULL){
		goto_region((WRegion*)vp);
		return;
	}

	tmpscr=scr;
	
	do{
		scr=NEXT_THING_FB(scr, WScreen, wglobal.screens);
		vp=FIRST_THING(scr, WViewport);
		if(vp!=NULL){
			goto_region((WRegion*)vp);
			return;
		}
	}while(scr!=tmpscr);
}


void goto_prev_viewport()
{
	WScreen *scr=wglobal.active_screen, *tmpscr;
	WRegion *r;
	WViewport *vp;
	
	if(scr==NULL)
		scr=wglobal.screens;
	
	assert(scr!=NULL);
	
	r=REGION_ACTIVE_SUB(scr);
	if(r!=NULL)
		vp=PREV_THING(r, WViewport);
	else
		vp=LAST_THING(scr, WViewport);

	if(vp!=NULL){
		goto_region((WRegion*)vp);
		return;
	}

	tmpscr=scr;
	
	do{
		scr=PREV_THING_FB(scr, WScreen, wglobal.screens);
		vp=LAST_THING(scr, WViewport);
		if(vp!=NULL){
			goto_region((WRegion*)vp);
			return;
		}
	}while(scr!=tmpscr);
}


/*}}}*/

