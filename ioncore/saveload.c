/*
 * ion/ioncore/saveload.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include "common.h"
#include "global.h"
#include "region.h"
#include "readconfig.h"
#include "screen.h"
#include "saveload.h"
#include "names.h"
#include "objp.h"
#include "attach.h"
#include "reginfo.h"
#include "extl.h"
#include "extlconv.h"


/*{{{ Load support functions */

static bool loading_workspaces=FALSE;

WRegion *create_region_load(WWindow *par, const WRectangle *geom, 
							ExtlTab tab)
{
	char *objclass=NULL, *name=NULL;
	WRegionLoadCreateFn* fn=NULL;
    WRegClassInfo *info=NULL;
	WRegion *reg=NULL;
	
	if(!extl_table_gets_s(tab, "type", &objclass))
		return NULL;

    info=ioncore_lookup_regclass(objclass, FALSE, FALSE, TRUE);
    if(info!=NULL)
        fn=info->lc_fn;
	
	if(fn==NULL){
		warn("Unknown class \"%s\", cannot create region.", objclass);
		if(loading_workspaces && ioncore_g.ws_save_enabled){
			ioncore_g.ws_save_enabled=FALSE;
			warn("Disabling workspace saving on exit to prevent savefile "
				 "corruption.\n"
				 "Call enable_workspace_saves(TRUE) to re-enable saves or\n"
				 "fix your configuration files and restart.");
		}
		
		free(objclass);
		return NULL;
	}

	free(objclass);
	
	reg=fn(par, geom, tab);
	
	if(reg==NULL)
		return NULL;
	
	if(!OBJ_IS(reg, WClientWin)){
		if(extl_table_gets_s(tab, "name", &name)){
			region_set_name(reg, name);
			free(name);
		}
	}
	
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


void file_indent(FILE *file, int lvl)
{
	while(lvl-->0)
		fprintf(file, "    ");
}


void file_write_escaped_string(FILE *file, const char *str)
{
	fputc('"', file);

	while(str && *str){
		if(((*str)&0x7f)<32 || *str=='"' || *str=='\\'){
			/* Lua uses decimal in escapes */
			fprintf(file, "\\%03d", (int)(uchar)(*str));
		}else{
			fputc(*str, file);
		}
		str++;
	}
	
	fputc('"', file);
}


void region_save_identity(WRegion *reg, FILE *file, int lvl)
{
	const char *name;
	
	/*file_indent(file, lvl-1);*/
	/*fprintf(file, "{\n");*/
	file_indent(file, lvl);
	fprintf(file, "type = \"%s\",\n", OBJ_TYPESTR(reg));
	
	name=region_name(reg);
	
	if(name!=NULL && !OBJ_IS(reg, WClientWin)){
		file_indent(file, lvl);
		fprintf(file, "name = ");
		file_write_escaped_string(file, name);
		fprintf(file, ",\n");
	}
}


/*}}}*/


/*{{{ save_workspaces, load_workspaces */


bool ioncore_load_workspaces()
{
	bool ret;
	loading_workspaces=TRUE;
	ret=ioncore_read_config("workspaces", NULL, FALSE);
	loading_workspaces=FALSE;
	return ret;
}


bool ioncore_save_workspaces()
{
	bool successp=TRUE;
	char *wsconf;
	FILE *file;
	WScreen *scr;
	
	wsconf=ioncore_get_savefile("workspaces");

	if(wsconf==NULL)
		return FALSE;

	file=fopen(wsconf, "w");

	if(file==NULL){
		warn_err_obj(wsconf);
		free(wsconf);
		return FALSE;
	}

	fprintf(file, "-- This file was created by and is modified by Ion.\n\n");

	FOR_ALL_SCREENS(scr){
		fprintf(file, "ioncore.initialise_screen_id(%d, {\n", scr->id);
		if(!region_save_to_file((WRegion*)scr, file, 1))
			successp=FALSE;
		fprintf(file, "})\n\n");
	}
	
	fclose(file);
	
	free(wsconf);
	
	return successp;
}


/*}}}*/


