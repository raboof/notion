/*
 * ion/ioncore/conf.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include <libtu/map.h>

#include "common.h"
#include "global.h"
#include "readconfig.h"
#include "binding.h"
#include "conf-bindings.h"
#include "modules.h"
#include "font.h"


EXTL_EXPORT
void enable_opaque_resize(bool opaque)
{
	wglobal.opaque_resize=opaque;
}


EXTL_EXPORT
void set_dblclick_delay(int dd)
{
	wglobal.dblclick_delay=(dd<0 ? 0 : dd);
}


EXTL_EXPORT
void set_resize_delay(int rd)
{
	wglobal.resize_delay=(rd<0 ? 0 : rd);
}


EXTL_EXPORT
void enable_warp(bool warp)
{
	wglobal.warp_enabled=warp;
}


EXTL_EXPORT
void global_bindings(ExtlTab tab)
{
	process_bindings(&ioncore_screen_bindmap, NULL, tab);
}


bool ioncore_read_config(const char *cfgfile)
{
	if(cfgfile==NULL){
		cfgfile="ioncore";
	}else if(strpbrk(cfgfile, "./")!=NULL){
		return read_config(cfgfile);
	}
	
	return read_config_for(cfgfile);
}
