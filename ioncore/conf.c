/*
 * ion/ioncore/conf.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>

#include <libtu/map.h>

#include "common.h"
#include "global.h"
#include "readconfig.h"
#include "modules.h"
#include "rootwin.h"
#include "bindmaps.h"


/*EXTL_DOC
 * Enable/disable opaque move/resize mode.
 */
EXTL_EXPORT
void enable_opaque_resize(bool opaque)
{
	wglobal.opaque_resize=opaque;
}


/*EXTL_DOC
 * Set double click delay in milliseconds.
 */
EXTL_EXPORT
void set_dblclick_delay(int dd)
{
	wglobal.dblclick_delay=(dd<0 ? 0 : dd);
}


/*EXTL_DOC
 * Set keyboard resize mode auto-finish delay in milliseconds.
 */
EXTL_EXPORT
void set_resize_delay(int rd)
{
	wglobal.resize_delay=(rd<0 ? 0 : rd);
}


/*EXTL_DOC
 * Enable/disable warping pointer to be contained in activated region.
 */
EXTL_EXPORT
void enable_warp(bool warp)
{
	wglobal.warp_enabled=warp;
}


bool ioncore_read_config(const char *cfgfile)
{
	bool ret;
	char unset[]="???";
	int n=0;

	if(cfgfile==NULL)
		cfgfile="ioncore";
	
	ret=do_include(cfgfile, ".");
	
	if(ioncore_rootwin_bindmap.nbindings==0)
		unset[n++]='g';
	if(ioncore_mplex_bindmap.nbindings==0)
		unset[n++]='m';
	if(ioncore_genframe_bindmap.nbindings==0)
		unset[n++]='f';
	
	if(n>0){
		warn("Some bindmaps were empty, loading ioncore-efbb");
		read_config_args("ioncore-efbb", TRUE, "S", NULL, unset);
	}
	
	return ret;	
}
