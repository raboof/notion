/*
 * ion/ionws/ionws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
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
#include <ioncore/extl.h>
#include "placement.h"
#include "ionws.h"
#include "split.h"
#include "ionframe.h"


/*{{{ region dynfun implementations */


static void ionws_fit(WIonWS *ws, WRectangle geom)
{
	REGION_GEOM(ws)=geom;
	
	if(ws->split_tree!=NULL){
		split_tree_do_resize(ws->split_tree, HORIZONTAL, geom.x, geom.w);
		split_tree_do_resize(ws->split_tree, VERTICAL, geom.y, geom.h);
	}
}


static bool reparent_ionws(WIonWS *ws, WWindow *parent, WRectangle geom)
{
	WRegion *sub, *next;
	bool rs;
	int xdiff, ydiff;
	
	if(!same_rootwin((WRegion*)ws, (WRegion*)parent))
		return FALSE;
	
	region_detach_parent((WRegion*)ws);
	region_set_parent((WRegion*)ws, (WRegion*)parent);
	
	xdiff=geom.x-REGION_GEOM(ws).x;
	ydiff=geom.y-REGION_GEOM(ws).y;
	
	FOR_ALL_MANAGED_ON_LIST_W_NEXT(ws->managed_list, sub, next){
		WRectangle g=REGION_GEOM(sub);
		g.x+=xdiff;
		g.y+=ydiff;
		if(!reparent_region(sub, parent, g)){
			warn("Problem: can't reparent a %s managed by a WIonWS"
				 "being reparented. Detaching from this object.",
				 WOBJ_TYPESTR(sub));
			region_detach_manager(sub);
		}
	}
	
	ionws_fit(ws, geom);
	
	return TRUE;
}


static void ionws_map(WIonWS *ws)
{
	WRegion *reg;

	MARK_REGION_MAPPED(ws);
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
		region_map(reg);
	}
}


static void ionws_unmap(WIonWS *ws)
{
	WRegion *reg;
	
	MARK_REGION_UNMAPPED(ws);
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
		region_unmap(reg);
	}
}


static void ionws_set_focus_to(WIonWS *ws, bool warp)
{
	WRegion *sub=ionws_current(ws);
	
	if(sub==NULL){
		warn("Trying to focus an empty ionws.");
		return;
	}

	region_set_focus_to(sub, warp);
}


static bool ionws_display_managed(WIonWS *ws, WRegion *reg)
{
	return TRUE;
}


/*}}}*/


/*{{{ Create/destroy */


static WIonFrame *create_initial_frame(WIonWS *ws, WWindow *parent,
									   WRectangle geom)
{
	WIonFrame *frame;
	
	frame=create_ionframe(parent, geom, 0);

	if(frame==NULL)
		return NULL;
	
	ws->split_tree=(WObj*)frame;
	ionws_add_managed(ws, (WRegion*)frame);

	return frame;
}


static bool ionws_init(WIonWS *ws, WWindow *parent, WRectangle bounds, bool ci)
{
	genws_init(&(ws->genws), parent, bounds);
	ws->split_tree=NULL;
	
	if(ci){
		if(create_initial_frame(ws, parent, bounds)==NULL){
			genws_deinit(&(ws->genws));
			return FALSE;
		}
	}
	
	return TRUE;
}


WIonWS *create_ionws(WWindow *parent, WRectangle bounds, bool ci)
{
	CREATEOBJ_IMPL(WIonWS, ionws, (p, parent, bounds, ci));
}


WIonWS *create_ionws_simple(WWindow *parent, WRectangle bounds)
{
	return create_ionws(parent, bounds, TRUE);
}


void ionws_deinit(WIonWS *ws)
{
	WRegion *reg;
	
	while(ws->managed_list!=NULL)
		ionws_remove_managed(ws, ws->managed_list);

	genws_deinit(&(ws->genws));
}


static bool ionws_may_destroy_managed(WIonWS *ws, WRegion *reg)
{
	if(ws->split_tree==(WObj*)reg)
		return region_may_destroy((WRegion*)ws);
	else
		return TRUE;
}


/*}}}*/


/*{{{ Save */


static void write_obj(WObj *obj, FILE *file, int lvl)
{
	WWsSplit *split;
	int tls, brs;
	const char *name;
	
	if(obj==NULL)
		goto fail2;
	
	if(WOBJ_IS(obj, WRegion)){
		if(region_supports_save((WRegion*)obj)){
			if(region_save_to_file((WRegion*)obj, file, lvl))
				return;
		}
		goto fail;
	}
	
	if(!WOBJ_IS(obj, WWsSplit))
		goto fail;
	
	split=(WWsSplit*)obj;
	
	tls=split_tree_size(split->tl, split->dir);
	brs=split_tree_size(split->br, split->dir);
	
	save_indent_line(file, lvl);
	fprintf(file, "split_dir = \"%s\",\n", (split->dir==VERTICAL
										   ? "vertical" : "horizontal"));
	
	save_indent_line(file, lvl);
	fprintf(file, "split_tls = %d, split_brs = %d,\n", tls, brs);
	
	save_indent_line(file, lvl);
	fprintf(file, "tl = {\n");
	write_obj(split->tl, file, lvl+1);
	save_indent_line(file, lvl);
	fprintf(file, "},\n");
			
	save_indent_line(file, lvl);
	fprintf(file, "br = {\n");
	write_obj(split->br, file, lvl+1);
	save_indent_line(file, lvl);
	fprintf(file, "},\n");

	return;
	
fail:		
	warn("Unable to save a %s\n", WOBJ_TYPESTR(obj));
fail2:
	fprintf(file, "{},\n");
}


static bool ionws_save_to_file(WIonWS *ws, FILE *file, int lvl)
{
	begin_saved_region((WRegion*)ws, file, lvl);
	save_indent_line(file, lvl);
	fprintf(file, "split_tree = {\n");
	write_obj(ws->split_tree, file, lvl+1);
	save_indent_line(file, lvl);
	fprintf(file, "},\n");
	return TRUE;
}


/*}}}*/


/*{{{ Load */


extern void set_split_of(WObj *obj, WWsSplit *split);
static WObj *load_obj(WIonWS *ws, WWindow *par, WRectangle geom, ExtlTab tab);


static WObj *load_split(WIonWS *ws, WWindow *par, WRectangle geom,
						ExtlTab tab)
{
	WWsSplit *split;
	char *dir_str;
	int dir, brs, tls;
	ExtlTab subtab;
	WObj *tl=NULL, *br=NULL;
	WRectangle geom2;

	if(!extl_table_gets_i(tab, "split_tls", &tls))
		return FALSE;
	if(!extl_table_gets_i(tab, "split_brs", &brs))
		return FALSE;
	if(!extl_table_gets_s(tab, "split_dir", &dir_str))
		return FALSE;
	if(strcmp(dir_str, "vertical")==0){
		dir=VERTICAL;
	}else if(strcmp(dir_str, "horizontal")==0){
		dir=HORIZONTAL;
	}else{
		free(dir_str);
		return NULL;
	}
	free(dir_str);

	split=create_split(dir, NULL, NULL, geom);
	if(split==NULL){
		warn("Unable to create a split.\n");
		return NULL;
	}
		
	geom2=geom;
	if(dir==HORIZONTAL){
		tls=geom.w*tls/(tls+brs);
		geom2.w=tls;
	}else{
		tls=geom.h*tls/(tls+brs);
		geom2.h=tls;
	}
	
	if(extl_table_gets_t(tab, "tl", &subtab)){
		tl=load_obj(ws, par, geom2, subtab);
		extl_unref_table(subtab);
	}

	geom2=geom;
	if(tl!=NULL){
		if(dir==HORIZONTAL){
			geom2.w-=tls;
			geom2.x+=tls;
		}else{
			geom2.h-=tls;
			geom2.y+=tls;
		}
	}
			
	if(extl_table_gets_t(tab, "br", &subtab)){
		br=load_obj(ws, par, geom2, subtab);
		extl_unref_table(subtab);
	}
	
	if(tl==NULL || br==NULL){
		free(split);
		return (tl==NULL ? br : tl);
	}
	
	set_split_of(tl, split);
	set_split_of(br, split);

	split->tmpsize=tls;
	split->tl=tl;
	split->br=br;
	
	return (WObj*)split;
}


static WObj *load_obj(WIonWS *ws, WWindow *par, WRectangle geom,
					  ExtlTab tab)
{
	char *typestr;
	WRegion *reg;
	
	if(extl_table_gets_s(tab, "type", &typestr)){
		free(typestr);
		reg=load_create_region(par, geom, tab);
		if(reg!=NULL)
			ionws_add_managed(ws, reg);
		return (WObj*)reg;
	}
	
	return load_split(ws, par, geom, tab);
}


WRegion *ionws_load(WWindow *par, WRectangle geom, ExtlTab tab)
{
	WIonWS *ws;
	ExtlTab treetab;
	bool ci=TRUE;

	if(extl_table_gets_t(tab, "split_tree", &treetab))
		ci=FALSE;
	
	ws=create_ionws(par, geom, ci);
	
	if(ws==NULL){
		if(!ci)
			extl_unref_table(treetab);
		return NULL;
	}

	if(!ci){
		ws->split_tree=load_obj(ws, par, geom, treetab);
		extl_unref_table(treetab);
	}
	
	if(ws->split_tree==NULL){
		warn("Workspace empty");
		destroy_obj((WObj*)ws);
		return NULL;
	}
	
	return (WRegion*)ws;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab ionws_dynfuntab[]={
	{region_fit, ionws_fit},
	{region_map, ionws_map},
	{region_unmap, ionws_unmap},
	{region_set_focus_to, ionws_set_focus_to},
	{(DynFun*)reparent_region, (DynFun*)reparent_ionws},
	
	{(DynFun*)genws_add_clientwin, (DynFun*)ionws_add_clientwin},

	{region_request_managed_geom, ionws_request_managed_geom},
	{region_managed_activated, ionws_managed_activated},
	{region_remove_managed, ionws_remove_managed},
	{(DynFun*)region_display_managed, (DynFun*)ionws_display_managed},
	
	{(DynFun*)region_find_rescue_manager_for, 
	 (DynFun*)ionws_find_rescue_manager_for},
	
	{(DynFun*)region_save_to_file, (DynFun*)ionws_save_to_file},

	{(DynFun*)region_may_destroy_managed,
	 (DynFun*)ionws_may_destroy_managed},

	END_DYNFUNTAB
};


IMPLOBJ(WIonWS, WGenWS, ionws_deinit, ionws_dynfuntab);

	
/*}}}*/

