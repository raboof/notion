/*
 * ion/ionws/ionws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include <libtu/objp.h>
#include <ioncore/region.h>
#include <ioncore/manage.h>
#include <ioncore/screen.h>
#include <ioncore/names.h>
#include <ioncore/saveload.h>
#include <ioncore/attach.h>
#include <ioncore/resize.h>
#include <ioncore/extl.h>
#include <ioncore/region-iter.h>
#include "placement.h"
#include "ionws.h"
#include "split.h"
#include "ionframe.h"


/*{{{ region dynfun implementations */


static void ionws_fit(WIonWS *ws, const WRectangle *geom)
{
	int tmp;
	
	REGION_GEOM(ws)=*geom;
	
	if(ws->split_tree==NULL)
		return;
	
	split_tree_resize((Obj*)ws->split_tree, HORIZONTAL, ANY, 
					  geom->x, geom->w);
	split_tree_resize((Obj*)ws->split_tree, VERTICAL, ANY, 
					  geom->y,  geom->h);
}


static bool reparent_ionws(WIonWS *ws, WWindow *parent, 
						   const WRectangle *geom)
{
	WRegion *sub, *next;
	bool rs;
	
	if(!region_same_rootwin((WRegion*)ws, (WRegion*)parent))
		return FALSE;
	
	region_detach_parent((WRegion*)ws);
	region_attach_parent((WRegion*)ws, (WRegion*)parent);
	
	FOR_ALL_MANAGED_ON_LIST_W_NEXT(ws->managed_list, sub, next){
		if(!region_reparent(sub, parent, &REGION_GEOM(sub))){
			warn("Problem: can't reparent a %s managed by a WIonWS"
				 "being reparented. Detaching from this object.",
				 OBJ_TYPESTR(sub));
			region_detach_manager(sub);
		}
	}
	
	ionws_fit(ws, geom);
	
	return TRUE;
}


static void ionws_map(WIonWS *ws)
{
	WRegion *reg;

	REGION_MARK_MAPPED(ws);
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
		region_map(reg);
	}
}


static void ionws_unmap(WIonWS *ws)
{
	WRegion *reg;
	
	REGION_MARK_UNMAPPED(ws);
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
		region_unmap(reg);
	}
}


static void ionws_do_set_focus(WIonWS *ws, bool warp)
{
	WRegion *sub=ionws_current(ws);
	
	if(sub==NULL){
		warn("Trying to focus an empty ionws.");
		return;
	}

	region_do_set_focus(sub, warp);
}


static bool ionws_display_managed(WIonWS *ws, WRegion *reg)
{
	return TRUE;
}


/*}}}*/


/*{{{ Create/destroy */


static WIonFrame *create_initial_frame(WIonWS *ws, WWindow *parent,
									   const WRectangle *geom)
{
	WIonFrame *frame;
	
	frame=create_ionframe(parent, geom);

	if(frame==NULL)
		return NULL;
	
	ws->split_tree=(Obj*)frame;
	ionws_add_managed(ws, (WRegion*)frame);

	return frame;
}


static bool ionws_init(WIonWS *ws, WWindow *parent, const WRectangle *bounds, 
					   bool ci)
{
	ws->managed_splits=extl_create_table();
	
	if(ws->managed_splits==extl_table_none())
		return FALSE;
	
	ws->split_tree=NULL;

	genws_init(&(ws->genws), parent, bounds);
	
	if(ci){
		if(create_initial_frame(ws, parent, bounds)==NULL){
			genws_deinit(&(ws->genws));
			extl_unref_table(ws->managed_splits);
			return FALSE;
		}
	}
	
	return TRUE;
}


WIonWS *create_ionws(WWindow *parent, const WRectangle *bounds, bool ci)
{
	CREATEOBJ_IMPL(WIonWS, ionws, (p, parent, bounds, ci));
}


WIonWS *create_ionws_simple(WWindow *parent, const WRectangle *bounds)
{
	return create_ionws(parent, bounds, TRUE);
}


void ionws_deinit(WIonWS *ws)
{
	WRegion *reg;
	
	while(ws->managed_list!=NULL)
		ionws_remove_managed(ws, ws->managed_list);

	genws_deinit(&(ws->genws));
	
	extl_unref_table(ws->managed_splits);
}


static bool ionws_may_destroy_managed(WIonWS *ws, WRegion *reg)
{
	if(ws->split_tree==(Obj*)reg)
		return region_may_destroy((WRegion*)ws);
	else
		return TRUE;
}


static bool ionws_do_rescue_clientwins(WIonWS *ws, WRegion *dst)
{
	return region_do_rescue_managed_clientwins((WRegion*)ws, dst,
											   ws->managed_list);
}


/*}}}*/


/*{{{ Save */


static void write_obj(Obj *obj, FILE *file, int lvl)
{
	WWsSplit *split;
	int tls, brs;
	const char *name;
	
	if(obj==NULL)
		goto fail2;
	
	if(OBJ_IS(obj, WRegion)){
		if(region_supports_save((WRegion*)obj)){
			if(region_save_to_file((WRegion*)obj, file, lvl))
				return;
		}
		goto fail;
	}
	
	if(!OBJ_IS(obj, WWsSplit))
		goto fail;
	
	split=(WWsSplit*)obj;
	
	tls=split_tree_size(split->tl, split->dir);
	brs=split_tree_size(split->br, split->dir);
	
	file_indent(file, lvl);
	fprintf(file, "split_dir = \"%s\",\n", (split->dir==VERTICAL
										   ? "vertical" : "horizontal"));
	
	file_indent(file, lvl);
	fprintf(file, "split_tls = %d, split_brs = %d,\n", tls, brs);
	
	file_indent(file, lvl);
	fprintf(file, "tl = {\n");
	write_obj(split->tl, file, lvl+1);
	file_indent(file, lvl);
	fprintf(file, "},\n");
			
	file_indent(file, lvl);
	fprintf(file, "br = {\n");
	write_obj(split->br, file, lvl+1);
	file_indent(file, lvl);
	fprintf(file, "},\n");

	return;
	
fail:		
	warn("Unable to save a %s\n", OBJ_TYPESTR(obj));
fail2:
	fprintf(file, "{},\n");
}


static bool ionws_save_to_file(WIonWS *ws, FILE *file, int lvl)
{
	region_save_identity((WRegion*)ws, file, lvl);
	file_indent(file, lvl);
	fprintf(file, "split_tree = {\n");
	write_obj(ws->split_tree, file, lvl+1);
	file_indent(file, lvl);
	fprintf(file, "},\n");
	return TRUE;
}


/*}}}*/


/*{{{ Load */


extern void set_split_of(Obj *obj, WWsSplit *split);
static Obj *load_obj(WIonWS *ws, WWindow *par, const WRectangle *geom, 
					  ExtlTab tab);


static Obj *load_split(WIonWS *ws, WWindow *par, const WRectangle *geom,
						ExtlTab tab)
{
	WWsSplit *split;
	char *dir_str;
	int dir, brs, tls;
	ExtlTab subtab;
	Obj *tl=NULL, *br=NULL;
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
		
	geom2=*geom;
	if(dir==HORIZONTAL){
		if(tls+brs==0)
			tls=geom->w/2;
		else
			tls=geom->w*tls/(tls+brs);
		geom2.w=tls;
	}else{
		if(tls+brs==0)
			tls=geom->h/2;
		else
		tls=geom->h*tls/(tls+brs);
		geom2.h=tls;
	}
	
	if(extl_table_gets_t(tab, "tl", &subtab)){
		tl=load_obj(ws, par, &geom2, subtab);
		extl_unref_table(subtab);
	}

	geom2=*geom;
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
		br=load_obj(ws, par, &geom2, subtab);
		extl_unref_table(subtab);
	}
	
	if(tl==NULL || br==NULL){
		free(split);
		return (tl==NULL ? br : tl);
	}
	
	set_split_of(tl, split);
	set_split_of(br, split);

	/*split->tmpsize=tls;*/
	split->tl=tl;
	split->br=br;
	
	return (Obj*)split;
}


static Obj *load_obj(WIonWS *ws, WWindow *par, const WRectangle *geom,
					  ExtlTab tab)
{
	char *typestr;
	WRegion *reg;
	
	if(extl_table_gets_s(tab, "type", &typestr)){
		free(typestr);
		reg=create_region_load(par, geom, tab);
		if(reg!=NULL)
			ionws_add_managed(ws, reg);
		return (Obj*)reg;
	}
	
	return load_split(ws, par, geom, tab);
}


WRegion *ionws_load(WWindow *par, const WRectangle *geom, ExtlTab tab)
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
		destroy_obj((Obj*)ws);
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
	{region_do_set_focus, ionws_do_set_focus},
	{(DynFun*)region_reparent,
     (DynFun*)reparent_ionws},
	
	{(DynFun*)region_manage_clientwin, 
     (DynFun*)ionws_manage_clientwin},

	{region_request_managed_geom, ionws_request_managed_geom},
	{region_managed_activated, ionws_managed_activated},
	{region_remove_managed, ionws_remove_managed},
	
	{(DynFun*)region_display_managed,
     (DynFun*)ionws_display_managed},
	
	{(DynFun*)region_find_rescue_manager_for, 
	 (DynFun*)ionws_find_rescue_manager_for},
	
	{(DynFun*)region_do_rescue_clientwins,
	 (DynFun*)ionws_do_rescue_clientwins},
	
	{(DynFun*)region_save_to_file,
     (DynFun*)ionws_save_to_file},

	{(DynFun*)region_may_destroy_managed,
	 (DynFun*)ionws_may_destroy_managed},

	{(DynFun*)region_current,
	 (DynFun*)ionws_current},

	END_DYNFUNTAB
};


IMPLCLASS(WIonWS, WGenWS, ionws_deinit, ionws_dynfuntab);

	
/*}}}*/

