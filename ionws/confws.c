/*
 * ion/confws.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <libtu/parser.h>
#include <libtu/tokenizer.h>

#include <wmcore/common.h>
#include <wmcore/global.h>
#include <wmcore/readconfig.h>
#include <wmcore/targetid.h>
#include <wmcore/screen.h>
#include "workspace.h"
#include "split.h"
#include "frame.h"


extern int tree_size(WObj *obj, int dir);


static WWsSplit *current_split=NULL;
static WWorkspace *current_ws=NULL;
static WScreen *current_screen=NULL;


/*{{{ Conf. functions */


static WRectangle get_geom()
{
	WRectangle geom;
	int s, pos;
	
	if(current_split==NULL){
		if(current_ws==NULL)
			geom=REGION_GEOM(current_screen);
		else
			geom=REGION_GEOM(current_ws);
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

static void get_params(WWinGeomParams *params)
{
	/*if(current_ws==NULL)*/
		region_rect_params((WRegion*)current_screen, get_geom(), params);
	/*else
		region_rect_params((WRegion*)current_ws, get_geom(), params);*/
}


static bool check_splits(const Tokenizer *tokz, int l)
{
	if(current_split!=NULL){
		if(current_split->br!=NULL){
			tokz_warn(tokz, l,
					  "A split can only contain two subsplits or frames");
			return FALSE;
		}
	}else{
		if(current_ws->splitree!=NULL){
			tokz_warn(tokz, l, "There can only be one frame or split at "
							   "workspace level");
			return FALSE;
		}
	}
	
	return TRUE;
}


static bool opt_workspace_split(int dir, Tokenizer *tokz, int n, Token *toks)
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
		current_ws->splitree=(WObj*)split;
	else if(current_split->tl==NULL)
		current_split->tl=(WObj*)split;
	else
		current_split->br=(WObj*)split;
	
	split->parent=current_split;
	
	current_split=split;
	
	return TRUE;
}


static bool opt_workspace_vsplit(Tokenizer *tokz, int n, Token *toks)
{
	return opt_workspace_split(VERTICAL, tokz, n, toks);
}


static bool opt_workspace_hsplit(Tokenizer *tokz, int n, Token *toks)
{
	return opt_workspace_split(HORIZONTAL, tokz, n, toks);
}


static bool opt_split_end(Tokenizer *tokz, int n, Token *toks)
{
	WWsSplit *split=current_split;

	current_split=split->parent;
	
	if(split->br!=NULL)
		return TRUE;

	tokz_warn(tokz, tokz->line, "Split not full");
	
	remove_split(current_ws, split);
	
	return TRUE;
}


/*static bool opt_split_cancel(Tokenizer *tokz, int n, Token *toks)
{
	current_split=current_split->parent;
	return FALSE;
}*/


static bool opt_workspace_frame(Tokenizer *tokz, int n, Token *toks)
{
	WWinGeomParams params;
	int id;
	WFrame *frame;
	
	if(!check_splits(tokz, toks[1].line))
		return FALSE;
	
	id=TOK_LONG_VAL(&(toks[1]));
	
	get_params(&params);
	frame=create_frame(current_screen, params, id, 0);

	if(frame==NULL)
		return FALSE;
	
	if(current_split==NULL)
		current_ws->splitree=(WObj*)frame;
	else if(current_split->tl==NULL)
		current_split->tl=(WObj*)frame;
	else
		current_split->br=(WObj*)frame;
	
	SPLIT_OF((WRegion*)frame)=current_split;

	workspace_add_sub(current_ws, (WRegion*)frame);
	
	return TRUE;
}


static bool opt_workspace(Tokenizer *tokz, int n, Token *toks)
{
	WWinGeomParams params;
	char *name=TOK_STRING_VAL(&(toks[1]));
	
	if(*name=='\0'){
		tokz_warn(tokz, toks[1].line, "Empty name");
		return FALSE;
	}
	
	get_params(&params);
	current_ws=create_workspace(current_screen, params, name, FALSE);
	
	if(current_ws==NULL)
		return FALSE;
	
	region_attach_sub((WRegion*)current_screen, (WRegion*)current_ws, 0);

	return TRUE;
}


static bool opt_workspace_end(Tokenizer *tokz, int n, Token *toks)
{
	if(current_ws->splitree==NULL){
		tokz_warn(tokz, tokz->line, "Workspace empty");
		destroy_thing((WThing*)current_ws);
	}
	current_ws=NULL;
	return TRUE;
}


/*static bool opt_workspace_cancel(Tokenizer *tokz, int n, Token *toks)
{
	destroy_thing((WThing*)current_ws);
	current_ws=NULL;
	return FALSE;
}*/



/*}}}*/


/*{{{ Save functions */


static void indent(FILE *file, int lvl)
{
	while(lvl--)
		putc('\t', file);
}


static void write_obj(FILE *file, WObj *obj, int lvl)
{
	WWsSplit *split;
	int tls, brs;
	
	if(WOBJ_IS(obj, WFrame)){
		indent(file, lvl);
		fprintf(file, "frame %d\n", ((WFrame*)obj)->target_id);
		return;
	}
	
	if(!WOBJ_IS(obj, WWsSplit))
		return;
	
	split=(WWsSplit*)obj;
	
	tls=tree_size(split->tl, split->dir);
	brs=tree_size(split->br, split->dir);
	
	indent(file, lvl);
	if(split->dir==HORIZONTAL)
		fprintf(file, "hsplit %d, %d {\n", tls, brs);
	else
		fprintf(file, "vsplit %d, %d {\n", tls, brs);
	
	write_obj(file, split->tl, lvl+1);
	write_obj(file, split->br, lvl+1);
	
	indent(file, lvl);
	fprintf(file, "}\n");
}

	
static void dodo_write_workspaces(FILE *file)
{
	WWorkspace *ws;
	int i=0;
	
	FOR_ALL_TYPED(current_screen, ws, WWorkspace){
		i++;
		if(region_name((WRegion*)ws)==NULL){
			warn("Not saving workspace %d -- no name", i);
			continue;
		}
		
		if(ws->splitree==NULL){
			warn("Empty workspace -- this cannot happen");
			continue;
		}
		
		fprintf(file, "workspace \"%s\" {\n", region_name((WRegion*)ws));
		write_obj(file, ws->splitree, 1);
		fprintf(file, "}\n");
	}
}


/*}}}*/


/*{{{ ConfOpts */


static ConfOpt split_opts[]={
	{"vsplit", "ll",  opt_workspace_vsplit, split_opts},
	{"hsplit", "ll",  opt_workspace_hsplit, split_opts},
	{"frame", "l",  opt_workspace_frame, NULL},
	
	{"#end", NULL, opt_split_end, NULL},
	/*{"#cancel", NULL, opt_split_cancel, NULL},*/
	{NULL, NULL ,NULL, NULL}
};


static ConfOpt workspace_opts[]={
	{"vsplit", "ll",  opt_workspace_vsplit, split_opts},
	{"hsplit", "ll",  opt_workspace_hsplit, split_opts},
	{"frame", "l",  opt_workspace_frame, NULL},
	
	{"#end", NULL, opt_workspace_end, NULL},
	/*{"#cancel", NULL, opt_workspace_cancel, NULL},*/
	{NULL, NULL ,NULL, NULL}
};


static ConfOpt wsconf_opts[]={
	{"workspace", "s", opt_workspace, workspace_opts},
	{NULL, NULL ,NULL, NULL}
};


/*}}}*/


/*{{{ read_workspace, write_workspaces */


bool read_workspaces(WScreen *scr)
{
	bool successp;

	current_screen=scr;
	successp=read_config_for_scr("workspaces", scr, wsconf_opts);
	current_screen=NULL;
	
	return successp;
}


static bool ensuredir(char *f)
{
	char *p=strrchr(f, '/');

	if(p==NULL)
		return TRUE;
	
	*p='\0';
	
	if(access(f, F_OK)){
		if(mkdir(f, 0700)){
			warn_err_obj(f);
			*p='/';
			return FALSE;
		}
	}

	*p='/';
	return TRUE;
}


static bool do_write_workspaces(char *wsconf)
{
	FILE *file;
	
	if(!ensuredir(wsconf))
		return FALSE;
	
	file=fopen(wsconf, "w");
	
	if(file==NULL){
		warn_err_obj(wsconf);
		return FALSE;
	}
	
	fprintf(file, "# This file was created by and is modified by Ion.\n");
	
	dodo_write_workspaces(file);
	
	fclose(file);
	
	return TRUE;
}


bool write_workspaces(WScreen *scr)
{
	bool successp;
	char *wsconf;
	
	wsconf=get_savefile_for_scr("workspaces", scr);
	
	if(wsconf==NULL)
		return FALSE;
	
	current_screen=scr;
	successp=do_write_workspaces(wsconf);
	current_screen=NULL;
	
	free(wsconf);
	
	return successp;
}

/*}}}*/
