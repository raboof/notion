/*
 * ion/ionws/ionws.c
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
#include "placement.h"
#include "ionws.h"
#include "split.h"
#include "ionframe.h"
#include "funtabs.h"


static void deinit_ionws(WIonWS *ws);
static void fit_ionws(WIonWS *ws, WRectangle geom);
static bool reparent_ionws(WIonWS *ws, WWindow *parent, WRectangle geom);
static void map_ionws(WIonWS *ws);
static void unmap_ionws(WIonWS *ws);
static void focus_ionws(WIonWS *ws, bool warp);
static bool ionws_save_to_file(WIonWS *ws, FILE *file, int lvl);
static bool ionws_display_managed(WIonWS *ws, WRegion *reg);


static DynFunTab ionws_dynfuntab[]={
	{fit_region, fit_ionws},
	{map_region, map_ionws},
	{unmap_region, unmap_ionws},
	{focus_region, focus_ionws},
	{(DynFun*)reparent_region, (DynFun*)reparent_ionws},
	
	{(DynFun*)region_ws_add_clientwin, (DynFun*)ionws_add_clientwin},
	{(DynFun*)region_ws_add_transient, (DynFun*)ionws_add_transient},

	{region_request_managed_geom, ionws_request_managed_geom},
	{region_managed_activated, ionws_managed_activated},
	{region_remove_managed, ionws_remove_managed},
	{(DynFun*)region_display_managed, (DynFun*)ionws_display_managed},
	
	{(DynFun*)region_do_find_new_manager, (DynFun*)ionws_do_find_new_manager},
	
	{(DynFun*)region_save_to_file, (DynFun*)ionws_save_to_file},

	END_DYNFUNTAB
};


IMPLOBJ(WIonWS, WGenWS, deinit_ionws, ionws_dynfuntab,
		&ionws_funclist)


/*{{{ region dynfun implementations */


static void fit_ionws(WIonWS *ws, WRectangle geom)
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
	
	if(!same_screen((WRegion*)ws, (WRegion*)parent))
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
	
	fit_ionws(ws, geom);
	
	return TRUE;
}


static void map_ionws(WIonWS *ws)
{
	WRegion *reg;

	MARK_REGION_MAPPED(ws);
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
		map_region(reg);
	}
}


static void unmap_ionws(WIonWS *ws)
{
	WRegion *reg;
	
	MARK_REGION_UNMAPPED(ws);
	
	FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
		unmap_region(reg);
	}
}


static void focus_ionws(WIonWS *ws, bool warp)
{
	WRegion *sub=ionws_find_current(ws);
	
	if(sub==NULL){
		warn("Trying to focus an empty ionws.");
		return;
	}

	focus_region(sub, warp);
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
	
	frame=create_ionframe(parent, geom, 0,0);

	if(frame==NULL)
		return NULL;
	
	ws->split_tree=(WObj*)frame;
	ionws_add_managed(ws, (WRegion*)frame);

	return frame;
}


static bool init_ionws(WIonWS *ws, WWindow *parent, WRectangle bounds, bool ci)
{
	init_genws(&(ws->genws), parent, bounds);
	ws->split_tree=NULL;
	
	if(ci){
		if(create_initial_frame(ws, parent, bounds)==NULL){
			deinit_genws(&(ws->genws));
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


void deinit_ionws(WIonWS *ws)
{
	WRegion *reg;
	
	while(ws->managed_list!=NULL)
		ionws_remove_managed(ws, ws->managed_list);

	deinit_genws(&(ws->genws));
}


/*}}}*/


/*{{{ Save */


static void write_obj(WObj *obj, FILE *file, int lvl)
{
	WWsSplit *split;
	int tls, brs;
	const char *name;
	
	if(WOBJ_IS(obj, WRegion)){
		if(!region_supports_save((WRegion*)obj))
			warn("Unable to save a %s\n", WOBJ_TYPESTR(obj));
		else
			region_save_to_file((WRegion*)obj, file, lvl+1);
		return;
	}
	
	if(!WOBJ_IS(obj, WWsSplit))
		return;
	
	split=(WWsSplit*)obj;
	
	tls=split_tree_size(split->tl, split->dir);
	brs=split_tree_size(split->br, split->dir);
	
	save_indent_line(file, lvl);
	if(split->dir==HORIZONTAL)
		fprintf(file, "hsplit %d, %d {\n", tls, brs);
	else
		fprintf(file, "vsplit %d, %d {\n", tls, brs);
	
	write_obj(split->tl, file, lvl+1);
	write_obj(split->br, file, lvl+1);
	
	save_indent_line(file, lvl);
	fprintf(file, "}\n");
}


static bool ionws_save_to_file(WIonWS *ws, FILE *file, int lvl)
{
	begin_saved_region((WRegion*)ws, file, lvl);
	write_obj(ws->split_tree, file, lvl);
	end_saved_region((WRegion*)ws, file, lvl);
	return TRUE;
}


/*}}}*/


/*{{{ Load */


static WWindow *tmp_par=NULL;
static WRectangle tmp_geom;
static WIonWS *current_ws=NULL;
static WWsSplit *current_split=NULL;


static WRectangle get_geom()
{
	WRectangle geom;
	int s, pos;
	
	if(current_split==NULL){
		geom=tmp_geom;
	}else{
		geom=current_split->geom;

		if(current_split->dir==VERTICAL){
			pos=geom.y;
			s=geom.h;
		}else{
			pos=geom.x;
			s=geom.w;
		}
		
		if(current_split->tl==NULL){
			s=current_split->tmpsize;
		}else{
			s-=current_split->tmpsize;
			pos+=current_split->tmpsize;
		}
			
		if(current_split->dir==VERTICAL){
			geom.y=pos;
			geom.h=s;
		}else{
			geom.x=pos;
			geom.w=s;
		}
	}
	
	return geom;
}


static bool check_splits(const Tokenizer *tokz, int l)
{
	if(current_split!=NULL){
		if(current_split->br!=NULL){
			tokz_warn(tokz, l,
					  "A split can only contain two subsplits or regions");
			return FALSE;
		}
	}else{
		if(current_ws->split_tree!=NULL){
			tokz_warn(tokz, l, "There can only be one frame or split at "
					           "workspace level");
			return FALSE;
		}
	}
	
	return TRUE;
}


static bool opt_ionws_split(int dir, Tokenizer *tokz, int n, Token *toks)
{
	WRectangle geom;
	WWsSplit *split;
	int brs, tls;
	int w, h;
	
	if(!check_splits(tokz, toks[1].line))
		return FALSE;
	
	tls=TOK_LONG_VAL(&(toks[1]));
	brs=TOK_LONG_VAL(&(toks[2]));
	
	geom=get_geom();
	
	if(dir==HORIZONTAL)
		tls=geom.w*tls/(tls+brs);
	else
		tls=geom.h*tls/(tls+brs);
	
	split=create_split(dir, NULL, NULL, geom);
	
	if(split==NULL)
		return FALSE;
		
	split->tmpsize=tls;
	
	if(current_split==NULL)
		current_ws->split_tree=(WObj*)split;
	else if(current_split->tl==NULL)
		current_split->tl=(WObj*)split;
	else
		current_split->br=(WObj*)split;
	
	split->parent=current_split;
	current_split=split;
	
	return TRUE;
}


static bool opt_ionws_vsplit(Tokenizer *tokz, int n, Token *toks)
{
	return opt_ionws_split(VERTICAL, tokz, n, toks);
}


static bool opt_ionws_hsplit(Tokenizer *tokz, int n, Token *toks)
{
	return opt_ionws_split(HORIZONTAL, tokz, n, toks);
}


static bool opt_split_end(Tokenizer *tokz, int n, Token *toks)
{
	WWsSplit *split=current_split;

	current_split=split->parent;
	
	if(split->br!=NULL)
		return TRUE;
	
	tokz_warn(tokz, tokz->line, "Split not full");
	
	if(current_split==NULL)
		current_ws->split_tree=split->tl;
	else if(current_split->tl==(WObj*)split)
		current_split->tl=split->tl;
	else
		current_split->br=split->tl;
	
	free(split);
	
	return TRUE;
}


static bool opt_ionws_region(Tokenizer *tokz, int n, Token *toks)
{
	WRegion *reg;
	WRectangle geom;
	
	if(!check_splits(tokz, toks[1].line))
		return FALSE;
	
	geom=get_geom();
	
	reg=load_create_region(tmp_par, geom, tokz, n, toks);

	if(reg==NULL)
		return FALSE;
	
	if(current_split==NULL)
		current_ws->split_tree=(WObj*)reg;
	else if(current_split->tl==NULL)
		current_split->tl=(WObj*)reg;
	else
		current_split->br=(WObj*)reg;
	
	SPLIT_OF((WRegion*)reg)=current_split;
	ionws_add_managed(current_ws, reg);
	
	return TRUE;
}


static ConfOpt split_opts[]={
	{"vsplit", "ll",  opt_ionws_vsplit, split_opts},
	{"hsplit", "ll",  opt_ionws_hsplit, split_opts},
	{"region", "s?s", opt_ionws_region, CONFOPTS_NOT_SET},
	{"#end", NULL, opt_split_end, NULL},
	END_CONFOPTS
};


static ConfOpt ionws_opts[]={
	{"vsplit", "ll",  opt_ionws_vsplit, split_opts},
	{"hsplit", "ll",  opt_ionws_hsplit, split_opts},
	{"region", "s?s", opt_ionws_region, CONFOPTS_NOT_SET},
	END_CONFOPTS
};


WRegion *ionws_load(WWindow *par, WRectangle geom, Tokenizer *tokz)
{
	WIonWS *ws;
	
	ws=create_ionws(par, geom, FALSE);
	
	if(ws==NULL)
		return NULL;

	tmp_par=par;
	tmp_geom=geom;
	current_split=NULL;
	current_ws=ws;
	
	parse_config_tokz(tokz, ionws_opts);
	
	tmp_par=NULL;
	current_split=NULL;
	current_ws=NULL;
	
	if(ws->split_tree==NULL){
		warn("Workspace empty");
		destroy_obj((WObj*)ws);
		return NULL;
	}
	
	return (WRegion*)ws;
}


/*}}}*/

