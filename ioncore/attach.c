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


WRegion *region_do_add_managed(WRegion *reg, WRegionAddFn *fn, void *fnparam,
							   const WAttachParams *param)
{
	WRegion *ret=NULL;
	CALL_DYN_RET(ret, WRegion*, region_do_add_managed, reg,
				 (reg, fn, fnparam, param));
	return ret;
}


bool region_supports_add_managed(WRegion *reg)
{
	return HAS_DYN(reg, region_do_add_managed);
}


static WRegion *add_fn_new(WWindow *par, WRectangle geom,
						   WRegionSimpleCreateFn *fn)
{
	return fn(par, geom);
}


WRegion *region_add_managed_new_simple(WRegion *mgr, WRegionSimpleCreateFn *fn,
									   int flags)
{
	WAttachParams param;
	param.flags=flags&(REGION_ATTACH_SWITCHTO|REGION_ATTACH_DOCKAPP);
	
	return region_do_add_managed(mgr, (WRegionAddFn*)add_fn_new,
								 (void*)fn, &param);
}


static WRegion *add_fn_reparent(WWindow *par, WRectangle geom, WRegion *reg)
{
	if(!reparent_region(reg, par, geom)){
		warn("Unable to reparent");
		return NULL;
	}
	region_detach_manager(reg);
	return reg;
}


bool region_add_managed_simple(WRegion *mgr, WRegion *reg, int flags)
{
	WAttachParams param;
	param.flags=flags&(REGION_ATTACH_SWITCHTO|REGION_ATTACH_DOCKAPP);
	/*param.flags|=REGION_ATTACH_GEOMRQ;
	param.geomrq=REGION_GEOM(reg);*/

	return region_add_managed(mgr, reg, &param);
}


bool region_add_managed(WRegion *mgr, WRegion *reg, 
						const WAttachParams *param)
{
	if(REGION_MANAGER(reg)==mgr)
		return TRUE;
	
	return (region_do_add_managed(mgr, (WRegionAddFn*)add_fn_reparent,
								  (void*)reg, param)!=NULL);
}


/*}}}*/


/*{{{ find_new_manager */


WRegion *default_do_find_new_manager(WRegion *reg, WRegion *todst)
{
	WRegion *p;
	
	if(region_supports_add_managed(reg))
		return reg;
	
	p=REGION_MANAGER(reg);

	if(p==NULL){
		p=REGION_PARENT_CHK(reg, WRegion);
		if(p==NULL)
			return NULL;
	}
	
	reg=region_do_find_new_manager(p, todst);
	if(reg!=NULL)
		return reg;
	return default_do_find_new_manager(p, todst);
}


WRegion *region_do_find_new_manager(WRegion *r2, WRegion *reg)
{
	WRegion *ret=NULL;
	CALL_DYN_RET(ret, WRegion*, region_do_find_new_manager, r2, (r2, reg));
	return ret;
}



WRegion *region_find_new_manager(WRegion *reg)
{
	WRegion *p=REGION_MANAGER(reg), *nm;

	if(p==NULL){
		p=REGION_PARENT_CHK(reg, WRegion);
		if(p==NULL)
			return NULL;
	}
	
	nm=region_do_find_new_manager(p, reg);
	if(nm!=NULL)
		return nm;
	return default_do_find_new_manager(p, reg);
}


bool region_move_managed_on_list(WRegion *dest, WRegion *src,
								 WRegion *list)
{
	WRegion *r, *next;
	bool success=TRUE;
	
	assert(region_supports_add_managed(dest));
	
	FOR_ALL_MANAGED_ON_LIST_W_NEXT(list, r, next){
		if(!region_add_managed_simple(dest, r, 0))
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
	
	warn("Unable to move managed regions of a %s somewhere else.",
		 WOBJ_TYPESTR(reg));
	
	return FALSE;
}


/*}}}*/

