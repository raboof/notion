/*
 * ion/ioncore/attach.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <limits.h>

#include "common.h"
#include "global.h"
#include "region.h"
#include "attach.h"
#include "objp.h"
#include "clientwin.h"
#include "saveload.h"
#include "manage.h"
#include "extlconv.h"
#include "names.h"


/*{{{ Attach */


/* new */


static WRegion *add_fn_new(WWindow *par, const WRectangle *geom,
						   WRegionSimpleCreateFn *fn)
{
	return fn(par, geom);
}


WRegion *attach_new_helper(WRegion *mgr, WRegionSimpleCreateFn *cfn,
						   WRegionDoAttachFn *fn, void *param)
{
	return fn(mgr, (WRegionAttachHandler*)add_fn_new, (void*)cfn, param);
}


/* load */


static WRegion *add_fn_load(WWindow *par, const WRectangle *geom, 
							ExtlTab *tab)
{
	return load_create_region(par, geom, *tab);
}


WRegion *attach_load_helper(WRegion *mgr, ExtlTab tab,
							WRegionDoAttachFn *fn, void *param)
{
	return fn(mgr, (WRegionAttachHandler*)add_fn_load, (void*)&tab, param);
}


/* reparent */


static WRegion *add_fn_reparent(WWindow *par, const WRectangle *geom, 
								WRegion *reg)
{
	if(!reparent_region(reg, par, geom)){
		warn("Unable to reparent");
		return NULL;
	}
	region_detach_manager(reg);
	return reg;
}


bool attach_reparent_helper(WRegion *mgr, WRegion *reg, 
							WRegionDoAttachFn *fn, void *param)
{
	WRegion *reg2;
	
	if(REGION_MANAGER(reg)==mgr)
		return TRUE;
	
	/* Check that reg is not a parent or manager of mgr */
	reg2=mgr;
	for(reg2=mgr; reg2!=NULL; reg2=REGION_MANAGER(reg2)){
		if(reg2==reg){
			warn("Trying to make a %s manage a %s above it in management "
				 "hierarchy", WOBJ_TYPESTR(mgr), WOBJ_TYPESTR(reg));
			return FALSE;
		}
	}
	
	for(reg2=region_parent(mgr); reg2!=NULL; reg2=region_parent(reg2)){
		if(reg2==reg){
			warn("Trying to make a %s manage its ancestor (a %s)",
				 WOBJ_TYPESTR(mgr), WOBJ_TYPESTR(reg));
			return FALSE;
		}
	}
	
	reg2=fn(mgr, (WRegionAttachHandler*)add_fn_reparent, (void*)reg, param);
	
	return (reg2!=NULL);
}



/*}}}*/


/*{{{ Rescue */


WRegion *default_find_rescue_manager_for(WRegion *reg, WRegion *chld)
{
	if(region_has_manage_clientwin(reg))
		return reg;
	return NULL;
}


WRegion *region_find_rescue_manager_for(WRegion *r2, WRegion *reg)
{
	WRegion *ret=NULL;
	CALL_DYN_RET(ret, WRegion*, region_find_rescue_manager_for, r2, (r2, reg));
	return ret;
}


/* Find new manager for the WClientWins in reg */
WRegion *region_find_rescue_manager(WRegion *reg)
{
	WRegion *p;
	
	while(1){
		p=region_manager_or_parent(reg);
		if(p==NULL)
			break;
		if(!WOBJ_IS_BEING_DESTROYED(p)){
			WRegion *nm=region_find_rescue_manager_for(p, reg);
			if(nm!=NULL)
				return nm;
		}
		reg=p;
	}
	
	return NULL;
}


static bool do_rescue(WRegion *dest, WRegion *r)
{
	WManageParams param=INIT_WMANAGEPARAMS;
	bool res=FALSE;
	
	if(!WOBJ_IS(r, WClientWin)){
		res=region_do_rescue_clientwins(r, dest);
	}else if(dest!=NULL){
		region_rootpos(dest, &(param.geom.x), &(param.geom.y));
		param.geom.w=REGION_GEOM(r).w;
		param.geom.h=REGION_GEOM(r).h;
		res=region_manage_clientwin(dest, (WClientWin*)r, &param);
	}
	
	if(!res)
		warn("Unable to rescue \"%s\".", region_name(r));
	
	return res;
}


bool region_do_rescue_managed_clientwins(WRegion *reg, WRegion *dest, 
										 WRegion *list)
{
	WRegion *r, *next;
	bool res=TRUE;
	
	FOR_ALL_MANAGED_ON_LIST_W_NEXT(list, r, next){
		if(!do_rescue(dest, r))
			res=FALSE;
	}
	
	return res;
}


bool region_do_rescue_child_clientwins(WRegion *reg, WRegion *dest)
{
	WRegion *r, *next;
	bool res=TRUE;
	
	for(r=reg->children; r!=NULL; r=next){
		next=r->p_next;
		
		if(!do_rescue(dest, r))
			res=FALSE;
	}
	
	return res;
}


bool region_do_rescue_clientwins(WRegion *reg, WRegion *dest)
{
	bool ret=FALSE;
	CALL_DYN_RET(ret, bool, region_do_rescue_clientwins, reg, (reg, dest));
	return ret;
}


bool region_rescue_clientwins(WRegion *reg)
{
	WRegion *dest=region_find_rescue_manager(reg);
	return region_do_rescue_clientwins(reg, dest);
}


/*}}}*/

