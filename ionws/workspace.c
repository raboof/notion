/*
 * ion/workspace.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <wmcore/common.h>
#include <wmcore/screen.h>
#include <wmcore/property.h>
#include <wmcore/focus.h>
#include <wmcore/global.h>
#include <wmcore/thingp.h>
#include <wmcore/region.h>
#include <wmcore/wsreg.h>
#include <wmcore/funtabs.h>
#include "bindmaps.h"
#include "placement.h"
#include "workspace.h"
#include "split.h"
#include "frame.h"
#include "confws.h"
#include "funtabs.h"


static void deinit_workspace(WWorkspace *ws);
static void fit_workspace(WWorkspace *ws, WRectangle geom);
static bool reparent_workspace(WWorkspace *ws, WWinGeomParams params);
static void map_workspace(WWorkspace *ws);
static void unmap_workspace(WWorkspace *ws);
static bool workspace_switch_subregion(WWorkspace *ws, WRegion *sub);
static void focus_workspace(WWorkspace *ws, bool warp);
extern void workspace_sub_activated(WWorkspace *ws, WRegion *reg);


/* split.c */
extern void workspace_request_sub_geom(WWorkspace *ws, WRegion *sub,
									   WRectangle geom, WRectangle *geomret,
									   bool tryonly);
extern WRegion *workspace_do_find_new_home(WRegion *reg);


static DynFunTab workspace_dynfuntab[]={
	{fit_region, fit_workspace},
	{(DynFun*)reparent_region, (DynFun*)reparent_workspace},
	{map_region, map_workspace},
	{unmap_region, unmap_workspace},
	{(DynFun*)switch_subregion, (DynFun*)workspace_switch_subregion},
	{focus_region, focus_workspace},
	{region_request_sub_geom, workspace_request_sub_geom},
	{region_sub_activated, workspace_sub_activated},
	{region_remove_sub, workspace_remove_sub},
	{(DynFun*)region_do_find_new_home, (DynFun*)workspace_do_find_new_home},
	
	{(DynFun*)region_ws_add_clientwin, (DynFun*)workspace_add_clientwin},
	{(DynFun*)region_ws_add_transient, (DynFun*)workspace_add_transient},
	
	END_DYNFUNTAB
};


IMPLOBJ(WWorkspace, WRegion, deinit_workspace, workspace_dynfuntab,
		&ion_workspace_funclist)


/*{{{ region dynfun implementations */


static void fit_workspace(WWorkspace *ws, WRectangle geom)
{
	if(ws->splitree!=NULL){
		tree_do_resize(ws->splitree, HORIZONTAL, geom.x, geom.w);
		tree_do_resize(ws->splitree, VERTICAL, geom.y, geom.h);
	}
}


static bool reparent_workspace(WWorkspace *ws, WWinGeomParams params)
{
	WRegion *sub;
	WWinGeomParams params2;
	bool rs;
	
	FOR_ALL_TYPED(ws, sub, WRegion){
		params2=params;
		params2.win_x+=REGION_GEOM(sub).x;
		params2.win_y+=REGION_GEOM(sub).y;
		params2.geom=REGION_GEOM(sub);
		rs=reparent_region(sub, params);
		assert(rs==TRUE);
	}
	
	REGION_GEOM(ws)=params.geom;
	
	fit_workspace(ws, params.geom);
	
	return TRUE;
}


static void map_workspace(WWorkspace *ws)
{
	map_all_subregions((WRegion*)ws);
	MARK_REGION_MAPPED(ws);
}


static void unmap_workspace(WWorkspace *ws)
{
	unmap_all_subregions((WRegion*)ws);
	MARK_REGION_UNMAPPED(ws);
}


static bool workspace_switch_subregion(WWorkspace *ws, WRegion *sub)
{
	/* All subregions are always visible --- that's the idea
	 * behind Ion's workspaces :-P.
	 */
	return TRUE;
}


static void focus_workspace(WWorkspace *ws, bool warp)
{
	WRegion *sub=workspace_find_current(ws);
	
	if(sub==NULL){
		warn("Trying to focus an empty workspace.");
		return;
	}

	focus_region(sub, warp);
}


/*}}}*/


/*{{{ Create/destroy */


static WFrame *create_initial_frame(WWorkspace *ws, WWinGeomParams params)
{
	WFrame *frame;
	
	frame=create_frame(SCREEN_OF(ws), params, 0, 0);
	
	if(frame==NULL)
		return NULL;
	
	ws->splitree=(WObj*)frame;
	workspace_add_sub(ws, (WRegion*)frame);

	return frame;
}


static bool init_workspace(WWorkspace *ws, WScreen *scr,
						   WWinGeomParams params, const char *name, bool ci)
{
	init_region(&(ws->region), scr, params.geom);
	
	if(name!=NULL){
		if(!set_region_name(&(ws->region), name)){
			deinit_region(&ws->region);
			return FALSE;
		}
	}
	
	ws->region.thing.flags|=WTHING_DESTREMPTY;
	ws->splitree=NULL;
	
	if(ci){
		if(create_initial_frame(ws, params)==NULL){
			deinit_region(&(ws->region));
			return FALSE;
		}
	}

	region_add_bindmap((WRegion*)ws, &ion_workspace_bindmap, TRUE);
	
	return TRUE;
}


WWorkspace *create_workspace(WScreen *scr, WWinGeomParams params,
							 const char *name, bool ci)
{
	CREATETHING_IMPL(WWorkspace, workspace, (p, scr, params, name, ci));
}

static WRegion *create_new_ws(WScreen *scr, WWinGeomParams params, void *fnp)
{
	return (WRegion*)create_workspace(scr, params, (char*)fnp, TRUE);
}


WWorkspace *create_new_workspace_on_scr(WScreen *scr, const char *name)
{
	return (WWorkspace*)region_attach_new((WRegion*)scr, create_new_ws,
										  (char *)name, 0);
}


void deinit_workspace(WWorkspace *ws)
{
	deinit_region(&(ws->region));
}


/*}}}*/


/*{{{ init_workspaces */


void init_workspaces(WScreen *scr)
{
	WWorkspace *ws=NULL;
	char *wsname=NULL;

	wsname=get_string_property(scr->root.win, wglobal.atom_workspace, NULL);

	read_workspaces(scr);
	
	if(scr->sub_count==0){
		ws=create_new_workspace_on_scr(scr, "main");
	}else{
		if(wsname!=NULL)
			ws=lookup_workspace(wsname);
		if(ws==NULL)
			ws=FIRST_THING(scr, WWorkspace);
	}
	
	if(wsname!=NULL)
		free(wsname);
	
	assert(ws!=NULL);
	
	goto_region((WRegion*)ws);
}


void setup_screens()
{
	WScreen *scr;
	
	FOR_ALL_SCREENS(scr){
		/* TODO: Should be moved somewhere else */
		region_add_bindmap((WRegion*)scr, &wmcore_screen_bindmap, TRUE);
		
		init_workspaces(scr);
		manage_initial_windows(scr);
	}
}


/*}}}*/


/*{{{ Switch */


bool switch_workspace_name(const char *str)
{
	WWorkspace *ws=lookup_workspace(str);
	
	if(ws==NULL)
		return FALSE;

	goto_region((WRegion*)ws);
	
	return TRUE;
}


/*}}}*/


/*{{{ Names */


WWorkspace *lookup_workspace(const char *name)
{
	return (WWorkspace*)do_lookup_region(name, &OBJDESCR(WWorkspace));
}


int complete_workspace(char *nam, char ***cp_ret, char **beg)
{
	return do_complete_region(nam, cp_ret, beg, &OBJDESCR(WWorkspace));
}


/*}}}*/

