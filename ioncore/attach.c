/*
 * ion/ioncore/attach.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <limits.h>

#include "common.h"
#include "region.h"
#include "attach.h"
#include "objp.h"
#include "clientwin.h"


/*{{{ Attach */


void region_add_managed_params(const WRegion *reg, WWindow **par,
							   WRectangle *geom)
{
	CALL_DYN(region_add_managed_params, reg, (reg, par, geom));
}


void region_add_managed_doit(WRegion *reg, WRegion *sub, int flags)
{
	CALL_DYN(region_add_managed_doit, reg, (reg, sub, flags));
}


bool region_supports_add_managed(WRegion *reg)
{
	return HAS_DYN(reg, region_add_managed_doit);
}


WRegion *region_add_managed_new(WRegion *mgr, WRegionCreateFn *fn,
								void *fnp, int flags)
{
	WRectangle geom;
	WRegion *reg;
	WWindow *par=NULL;

	geom.x=0;
	geom.y=0;
	geom.w=0;
	geom.h=0;
	
	{ /* CALL_DYN defines variables */
		CALL_DYN(region_add_managed_params, mgr, (mgr, &par, &geom));
		if(funnotfound)
			return NULL;
	}
	
	reg=fn(par, geom, fnp);
	
	if(reg!=NULL)
		region_add_managed_doit(mgr, reg, flags);
	
	return reg;
}


WRegion *region_add_managed_new_simple(WRegion *mgr, WRegionSimpleCreateFn *fn,
									   int flags)
{
	WRectangle geom;
	WRegion *reg;
	WWindow *par=NULL;

	geom.x=0;
	geom.y=0;
	geom.w=0;
	geom.h=0;
	
	{ /* CALL_DYN defines variables */
		CALL_DYN(region_add_managed_params, mgr, (mgr, &par, &geom));
		if(funnotfound)
			return NULL;
	}
	
	reg=fn(par, geom);
	
	if(reg!=NULL)
		region_add_managed_doit(mgr, reg, flags);
	
	return reg;
}

	
bool region_add_managed(WRegion *mgr, WRegion *reg, int flags)
{
	WRectangle geom;
	WWindow *par=NULL;
	
	if(REGION_MANAGER(reg)==mgr)
		return TRUE;
	
	/* clientwin_add_managed_params wants the old geometry */
	geom=REGION_GEOM(reg);
	
	{ /* CALL_DYN defines variables */
		CALL_DYN(region_add_managed_params, mgr, (mgr, &par, &geom));
		if(funnotfound)
			return FALSE;
	}
	
	if(FIND_PARENT1(reg, WWindow)!=par){
		if(!reparent_region(reg, par, geom)){
			warn("Unable to reparent.");
			return FALSE;
		}
	}else{
		fit_region(reg, geom);
	}
	
	region_detach_manager(reg);
	region_add_managed_doit(mgr, reg, flags);
	
	return TRUE;
}


/*}}}*/


/*{{{ find_new_manager */


WRegion *default_do_find_new_manager(WRegion *reg)
{
	WRegion *p;
	
	if(region_supports_add_managed(reg))
		return reg;
	
	p=REGION_MANAGER(reg);

	if(p==NULL){
		p=FIND_PARENT1(reg, WRegion);
		if(p==NULL)
			return NULL;
	}
	
	return region_do_find_new_manager(p);
}


WRegion *region_do_find_new_manager(WRegion *reg)
{
	WRegion *ret=NULL;
	CALL_DYN_RET(ret, WRegion*, region_do_find_new_manager, reg, (reg));
	if(funnotfound)
		ret=default_do_find_new_manager(reg);
	return ret;
}



WRegion *region_find_new_manager(WRegion *reg)
{
	WRegion *p=FIND_PARENT1(reg, WRegion);
	
	if(p!=NULL)
		p=region_do_find_new_manager(p);
	
	return p;
}


bool region_move_managed_on_list(WRegion *dest, WRegion *src,
								 WRegion *list)
{
	WRegion *r, *next;
	bool success=TRUE;
	
	assert(region_supports_add_managed(dest));
	
	FOR_ALL_MANAGED_ON_LIST_W_NEXT(list, r, next){
		if(!region_add_managed(dest, r, 0))
			success=FALSE;
	}
	
	return success;
}


bool region_rescue_managed_on_list(WRegion *reg, WRegion *list)
{
	WRegion *p;
	
	if(list==NULL)
		return TRUE;
	
	p=region_find_new_manager(reg);
	
	if(p!=NULL){
		if(region_move_managed_on_list(p, reg, list))
			return TRUE;
	}
	
	/* This should not happen unless the region is not
	 * properly in a tree with a WScreen root.
	 */
	
	warn("Unable to move subregions of a (to-be-destroyed) %s "
		 "somewhere else.", WOBJ_TYPESTR(reg));
	
	return FALSE;
}


/*}}}*/

