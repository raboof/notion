/*
 * ion/ioncore/reginfo.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <ctype.h>

#include "common.h"
#include "region.h"
#include "objp.h"
#include "attach.h"
#include "reginfo.h"


/*{{{ Region class registration */


INTRSTRUCT(RegClassInfo);
	
DECLSTRUCT(RegClassInfo){
	WObjDescr *descr;
	WRegionSimpleCreateFn *sc_fn;
	WRegionLoadCreateFn *lc_fn;
	RegClassInfo *next, *prev;
};


static RegClassInfo *reg_class_infos;


static RegClassInfo *lookup_reg_class_info_by_name(const char *name)
{
	RegClassInfo *info;
	
	if(name==NULL)
		return NULL;
	
	for(info=reg_class_infos; info!=NULL; info=info->next){
		if(strcmp(info->descr->name, name)==0)
			return info;
	}
	return NULL;
}


static RegClassInfo *lookup_reg_class_info_by_name_inh(const char *name)
{
	RegClassInfo *info;
	WObjDescr *descr;

	if(name==NULL)
		return NULL;
	
	for(info=reg_class_infos; info!=NULL; info=info->next){
		descr=info->descr;
		while(descr!=NULL){
			if(strcmp(descr->name, name)==0){
				return info;
			}
			descr=descr->ancestor;
		}
	}
	return NULL;
}


static RegClassInfo *lookup_reg_class_info(WObjDescr *descr)
{
	RegClassInfo *info;
	
	for(info=reg_class_infos; info!=NULL; info=info->next){
		if(info->descr==descr)
			return info;
	}
	return NULL;
}


bool register_region_class(WObjDescr *descr, WRegionSimpleCreateFn *sc_fn,
						   WRegionLoadCreateFn *lc_fn)
{
	RegClassInfo *info;
	
	if(descr==NULL)
		return FALSE;
	
	info=ALLOC(RegClassInfo);
	if(info==NULL){
		warn_err();
		return FALSE;
	}
	
	info->descr=descr;
	info->sc_fn=sc_fn;
	info->lc_fn=lc_fn;
	LINK_ITEM(reg_class_infos, info, next, prev);
	
	return TRUE;
}


void unregister_region_class(WObjDescr *descr)
{
	RegClassInfo *info=lookup_reg_class_info(descr);
	
	if(info!=NULL){
		UNLINK_ITEM(reg_class_infos, info, next, prev);
		free(info);
	}
}


WRegionLoadCreateFn *lookup_region_load_create_fn(const char *name)
{
	RegClassInfo *info=lookup_reg_class_info_by_name(name);
	return (info==NULL ? NULL : info->lc_fn);
}


WRegionSimpleCreateFn *lookup_region_simple_create_fn(const char *name)
{
	RegClassInfo *info=lookup_reg_class_info_by_name(name);
	return (info==NULL ? NULL : info->sc_fn);
}


WRegionSimpleCreateFn *lookup_region_simple_create_fn_inh(const char *name)
{
	RegClassInfo *info=lookup_reg_class_info_by_name_inh(name);
	return (info==NULL ? NULL : info->sc_fn);
}


/*}}}*/

