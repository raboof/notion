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

#include "common.h"
#include "region.h"
#include "readconfig.h"
#include "viewport.h"
#include "saveload.h"
#include "names.h"
#include "objp.h"
#include "attach.h"
#include "reginfo.h"
#include "extl.h"


/*{{{ Load support functions */


WRegion *load_create_region(WWindow *par, WRectangle geom, ExtlTab tab)
{
	char *objclass, *name;
	WRegionLoadCreateFn* fn;
	WRegion *reg;
	
	if(!extl_table_gets_s(tab, "type", &objclass))
		return NULL;

	fn=lookup_region_load_create_fn(objclass);
	
	if(fn==NULL){
		warn("Unknown class \"%s\", cannot create region", objclass);
		free(objclass);
		return NULL;
	}

	free(objclass);
	
	reg=fn(par, geom, tab);
	
	if(reg==NULL)
		return NULL;
	
	if(extl_table_gets_s(tab, "name", &name)){
		region_set_name(reg, name);
		free(name);
	}
	
	return reg;
}


static WRegion *add_fn_load(WWindow *par, WRectangle geom, ExtlTab *tab)
{
	return load_create_region(par, geom, *tab);
}


WRegion *region_add_managed_load(WRegion *mgr, ExtlTab tab)
{
	return region_do_add_managed(mgr, (WRegionAddFn*)&add_fn_load,
								 (void*)&tab, 0, NULL);
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
		if(((*str)&0x7f)<32){
			/* Lua uses decimal in escapes */
			fprintf(file, "\\%3d", (int)(uchar)(*str));
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
	
	if(name!=NULL){
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


/*}}}*/


/*{{{ save_workspaces, load_workspaces */


static WViewport *current_vp=NULL;


EXTL_EXPORT
bool add_to_viewport(ExtlTab tab)
{
	if(current_vp==NULL)
		return FALSE;
	
	return (region_add_managed_load((WRegion*)current_vp, tab)!=NULL);
}



bool load_workspaces(WViewport *vp)
{
	bool successp;
	char *filename;
	
	filename=get_cfgfile_for_scr("saves/workspaces", vp->id);
	if(filename==NULL)
		return FALSE;
	
	current_vp=vp;
	successp=read_config(filename);
	current_vp=NULL;
	
	free(filename);
	
	return successp;
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
	
	fprintf(file, "-- This file was created by and is modified by Ion.\n");
	FOR_ALL_MANAGED_ON_LIST(vp->ws_list, reg){
		if(region_supports_save(reg)){
			fprintf(file, "add_to_viewport {\n");
			region_save_to_file(reg, file, 1);
			fprintf(file, "}\n");
		}
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

	if(!ensuredir(wsconf))
		return FALSE;

	current_vp=vp;
	successp=do_save_workspaces(vp, wsconf);
	current_vp=NULL;
	
	free(wsconf);
	
	return successp;
}


/*}}}*/

