/*
 * ion/ioncore/saveload.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

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

WRegion *load_create_region(WWindow *par, WRectangle geom, ExtlTab tab)
{
	char *objclass, *name;
	WRegionLoadCreateFn* fn;
	WRegion *reg;
	
	if(!extl_table_gets_s(tab, "type", &objclass))
		return NULL;

	fn=lookup_region_load_create_fn(objclass);
	
	if(fn==NULL){
		warn("Unknown class \"%s\", cannot create region.", objclass);
		if(loading_workspaces && wglobal.ws_save_enabled){
			wglobal.ws_save_enabled=FALSE;
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
	
	if(!WOBJ_IS(reg, WClientWin)){
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


void save_indent_line(FILE *file, int lvl)
{
	while(lvl-->0)
		fprintf(file, "    ");
}


void write_escaped_string(FILE *file, const char *str)
{
	fputc('"', file);

	while(str && *str){
		if(((*str)&0x7f)<32 || *str=='"'){
			/* Lua uses decimal in escapes */
			fprintf(file, "\\%03d", (int)(uchar)(*str));
		}else{
			fputc(*str, file);
		}
		str++;
	}
	
	fputc('"', file);
}


void begin_saved_region(WRegion *reg, FILE *file, int lvl)
{
	const char *name;
	
	/*save_indent_line(file, lvl-1);*/
	/*fprintf(file, "{\n");*/
	save_indent_line(file, lvl);
	fprintf(file, "type = \"%s\",\n", WOBJ_TYPESTR(reg));
	
	name=region_name(reg);
	
	if(name!=NULL && !WOBJ_IS(reg, WClientWin)){
		save_indent_line(file, lvl);
		fprintf(file, "name = ");
		write_escaped_string(file, name);
		fprintf(file, ",\n");
	}
}


/*void end_saved_region(WRegion *reg, FILE *file, int lvl)
{
	save_indent_line(file, lvl-1);
	fprintf(file, "},\n");
}*/


void save_geom(WRectangle geom, FILE *file, int lvl)
{
	save_indent_line(file, lvl);
	fprintf(file, "geom = { x = %d, y = %d, w = %d, h = %d},\n",
			geom.x, geom.y, geom.w, geom.h);
}


/*}}}*/


/*{{{ save_workspaces, load_workspaces */


bool load_workspaces()
{
	bool ret;
	loading_workspaces=TRUE;
	ret=read_config_for_args("workspaces", FALSE, NULL, NULL);
	loading_workspaces=FALSE;
	return ret;
}


static bool ensuredir(char *f)
{
	char *p=strrchr(f, '/');
	int tryno=0;
	bool ret=TRUE;
	
	if(p==NULL)
		return TRUE;
	
	*p='\0';
	
	if(access(f, F_OK)!=0){
		ret=FALSE;
		do{
			if(mkdir(f, 0700)==0){
				ret=TRUE;
				break;
			}
			if(!ensuredir(f))
				break;
			tryno++;
		}while(tryno<2);
		
		if(!ret)
			warn_obj(f, "Unable to create directory");
	}

	*p='/';
	return ret;
}


bool save_workspaces()
{
	bool successp=TRUE;
	char *wsconf;
	FILE *file;
	WScreen *scr;
	
	wsconf=get_savefile_for("workspaces");

	if(wsconf==NULL)
		return FALSE;

	if(!ensuredir(wsconf))
		return FALSE;

	file=fopen(wsconf, "w");

	if(file==NULL){
		warn_err_obj(wsconf);
		free(wsconf);
		return FALSE;
	}

	fprintf(file, "-- This file was created by and is modified by Ion.\n\n");

	FOR_ALL_SCREENS(scr){
		fprintf(file, "initialise_screen_id(%d, {\n", scr->id);
		if(!region_save_to_file((WRegion*)scr, file, 1))
			successp=FALSE;
		fprintf(file, "})\n\n");
	}
	
	fclose(file);
	
	free(wsconf);
	
	return successp;
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_EXPROT
 * Enable or disable workspaces saving on exit.
 */
EXTL_EXPORT
void enable_workspace_saves(bool enable)
{
	wglobal.ws_save_enabled=enable;
}


/*}}}*/

