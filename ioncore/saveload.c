/*
 * ion/ioncore/saveload.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <libtu/parser.h>
#include <libtu/tokenizer.h>

#include "common.h"
#include "region.h"
#include "readconfig.h"
#include "viewport.h"
#include "saveload.h"
#include "names.h"
#include "objp.h"
#include "attach.h"
#include "reginfo.h"


/*{{{ Load support functions */


WRegion *load_create_region(WWindow *par, WRectangle geom,
							Tokenizer *tokz, int n, Token *toks)
{
	char *objclass, *name;
	WRegionLoadCreateFn* fn;
	WRegion *reg;
	
	assert(n>1);
	assert(TOK_IS_STRING(&(toks[1])));
	
	objclass=TOK_STRING_VAL(&(toks[1]));
	fn=lookup_region_load_create_fn(objclass);
	
	if(fn==NULL){
		warn("Unknown class \"%s\", cannot create region", objclass);
		parse_config_tokz_skip_section(tokz);
		return NULL;
	}
	
	reg=fn(par, geom, tokz);
	
	if(reg==NULL)
		return NULL;
	
	if(n>=3){
		assert(TOK_IS_STRING(&(toks[2])));
		region_set_name(reg, TOK_STRING_VAL(&(toks[2])));
	}
	
	return reg;
}


typedef struct{
	Tokenizer *tokz;
	int n;
	Token *toks;
	bool read;
} AddParams;
	

static WRegion *add_fn_load(WWindow *par, WRectangle geom, AddParams *params)
{
	params->read=TRUE;
	return load_create_region(par, geom, params->tokz, params->n, params->toks);
}


WRegion *region_add_managed_load(WRegion *mgr, Tokenizer *tokz,
								 int n, Token *toks)
{
	AddParams params;
	WRegion *reg;
	
	params.tokz=tokz;
	params.n=n;
	params.toks=toks;
	params.read=FALSE;
	
	reg=region_do_add_managed(mgr, (WRegionAddFn*)&add_fn_load,
							  (void*)&params, 0, NULL);
	
	if(!params.read)
		parse_config_tokz_skip_section(tokz);
	
	return reg;
}


/*}}}*/


/*{{{ Save support functions */


bool region_supports_save(WRegion *reg)
{
	return HAS_DYN(reg, region_save_to_file);
}


bool region_save_to_file(WRegion *reg, FILE *file, int lvl)
{
	bool ret=FALSE;
	CALL_DYN_RET(ret, bool, region_save_to_file, reg, (reg, file, lvl));
	return ret;
}


void save_indent_line(FILE *file, int lvl)
{
	while(lvl-->0)
		fprintf(file, "    ");
}


void write_escaped_string(FILE *file, const char *str)
{
	fputc('"', file);

	while(*str){
		if(((*str)&0x7f)<32)
			fprintf(file, "\\%o", (int)(uchar)(*str));
		else
			fputc(*str, file);
		str++;
	}
	
	fputc('"', file);
}


void begin_saved_region(WRegion *reg, FILE *file, int lvl)
{
	const char *name;
	
	save_indent_line(file, lvl-1);
	fprintf(file, "region \"%s\"", WOBJ_TYPESTR(reg));
	
	name=region_name(reg);
	
	if(name!=NULL){
		fprintf(file, ", ");
		write_escaped_string(file, name);
	}
	
	fprintf(file, " {\n");
}


void end_saved_region(WRegion *reg, FILE *file, int lvl)
{
	save_indent_line(file, lvl-1);
	fprintf(file, "}\n");
}


/*}}}*/


/*{{{ save_workspaces, load_workspaces */


static WViewport *current_vp=NULL;


static bool opt_viewport_region(Tokenizer *tokz, int n, Token *toks)
{
	return (region_add_managed_load((WRegion*)current_vp,
									tokz, n, toks)!=NULL);
}


static ConfOpt wsconf_opts[]={
	{"region", "s?s", opt_viewport_region, CONFOPTS_NOT_SET},
	END_CONFOPTS
};


bool load_workspaces(WViewport *vp)
{
	bool successp;

	current_vp=vp;
	successp=read_config_for_scr("saves/workspaces", vp->id, wsconf_opts);
	current_vp=NULL;
	
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


static bool do_save_workspaces(WViewport *vp, char *wsconf)
{
	FILE *file;
	WRegion *reg;
	
	if(!ensuredir(wsconf))
		return FALSE;
	
	file=fopen(wsconf, "w");
	
	if(file==NULL){
		warn_err_obj(wsconf);
		return FALSE;
	}
	
	fprintf(file, "# This file was created by and is modified by Ion.\n");
	
	FOR_ALL_MANAGED_ON_LIST(vp->ws_list, reg){
		if(region_supports_save(reg))
		   region_save_to_file(reg, file, 1);
	}
	
	fclose(file);
	
	return TRUE;
}


bool save_workspaces(WViewport *vp)
{
	bool successp;
	char *wsconf;
	
	wsconf=get_savefile_for_scr("saves/workspaces", vp->id);
	
	if(wsconf==NULL)
		return FALSE;
	
	current_vp=vp;
	successp=do_save_workspaces(vp, wsconf);
	current_vp=NULL;
	
	free(wsconf);
	
	return successp;
}


/*}}}*/

