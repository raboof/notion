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
#include "geom.h"


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
	
	if(region_name(reg)==NULL){
		if(extl_table_gets_s(tab, "name", &name)){
			int instrq=-1;
			extl_table_gets_i(tab, "name_instance", &instrq);
			region_set_name_instrq(reg, name, instrq);
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
	
	if(name!=NULL){
		save_indent_line(file, lvl);
		fprintf(file, "name = ");
		write_escaped_string(file, name);
		fprintf(file, ",\n");
		save_indent_line(file, lvl);
		fprintf(file, "name_instance = %d,\n", region_name_instance(reg));
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


static WViewport *current_vp=NULL;


/* 2003-10-05, TODO: keep for savefile backwards compatibility 
 * for now, but remove eventually.
 */
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
	successp=extl_dofile(filename, "o", NULL, vp);
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
	const char *name;
	
	if(!ensuredir(wsconf))
		return FALSE;
	
	file=fopen(wsconf, "w");
	
	if(file==NULL){
		warn_err_obj(wsconf);
		return FALSE;
	}
	
	fprintf(file, "-- This file was created by and is modified by Ion.\n");
	
	name=region_name((WRegion*)vp);
	if(name!=NULL){
		fprintf(file, "region_set_name(arg[1], ");
		write_escaped_string(file, name);
		fprintf(file, ")\n");
	}
	
	FOR_ALL_MANAGED_ON_LIST(vp->ws_list, reg){
		if(region_supports_save(reg)){
			fprintf(file, "region_manage_new(arg[1], {\n");
			region_save_to_file(reg, file, 1);
			fprintf(file, "})\n");
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

