/*
 * wmcore/attach.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
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


void region_do_attach_params(const WRegion *reg, WWinGeomParams *params)
{
	CALL_DYN(region_do_attach_params, reg, (reg, params));
}


void region_do_attach(WRegion *reg, WRegion *sub, int flags)
{
	CALL_DYN(region_do_attach, reg, (reg, sub, flags));
}


bool region_supports_attach(WRegion *reg)
{
	/* Checking for just one should be enough. */
	return (HAS_DYN(reg, region_do_attach) /* &&
			HAS_DYN(reg, region_do_attach_params)*/);
}


WRegion *region_attach_new(WRegion *reg, WRegionCreateFn *fn, void *fnp,
						   int flags)
{
	WWinGeomParams params;
	WRegion *sub;
	
	/*region_do_attach_params(reg, &params);*/
	
	CALL_DYN(region_do_attach_params, reg, (reg, &params));
	if(funnotfound)
		return NULL;
	
	sub=fn(SCREEN_OF(reg), params, fnp);
	
	if(sub!=NULL)
		region_do_attach(reg, sub, flags);
	
	return sub;
}

	
bool region_attach_sub(WRegion *reg, WRegion *sub, int flags)
{
	WWinGeomParams params;
	
	if(thing_is_child((WThing*)reg, (WThing*)sub))
		return TRUE;
	
	/*region_do_attach_params(reg, &params);*/
	{
		CALL_DYN(region_do_attach_params, reg, (reg, &params));
		if(funnotfound)
			return FALSE;
	}
	
	detach_reparent_region(sub, params);
	region_do_attach(reg, sub, flags);
	
	return TRUE;
}


/*}}}*/


/*{{{ find_new_home */


WRegion *default_do_find_new_home(WRegion *reg)
{
	WRegion *p;
	
	if(region_supports_attach(reg))
		return reg;
	
	p=FIND_PARENT1(reg, WRegion);

	if(p!=NULL)
		p=region_do_find_new_home(p);
	
	return p;
}


WRegion *region_do_find_new_home(WRegion *reg)
{
	WRegion *ret=NULL;
	CALL_DYN_RET(reg, WRegion*, region_do_find_new_home, reg, (reg));
	if(funnotfound)
		ret=default_do_find_new_home(reg);
	return ret;
}



WRegion *region_find_new_home(WRegion *reg)
{
	WRegion *p=FIND_PARENT1(reg, WRegion);
	
	if(p!=NULL)
		p=region_do_find_new_home(p);
	
	return p;
}

bool region_move_subregions(WRegion *dest, WRegion *src)
{
	WRegion *r;
	bool success=TRUE;
	
	assert(region_supports_attach(dest));
	
	while(1){
		r=FIRST_THING(src, WRegion);
		if(r==NULL)
			break;
		if(!region_attach_sub(dest, r, 0))
			success=FALSE;
	}
	
	return success;
}


bool region_rescue_subregions(WRegion *reg)
{
	WRegion *p=region_find_new_home(reg);
	WRegion *r;
	
	if(p!=NULL){
		if(region_move_subregions(p, reg))
			return TRUE;
	}
	
	/* This should not happen unless the region is not
	 * properly in a tree with a WScreen root.
	 */
	
	warn("Unable to move subregions of a (to-be-destroyed) frame"
		 "somewhere else.");
	
	return FALSE;
}


bool region_move_clientwins(WRegion *dest, WRegion *src)
{
	WClientWin *r;
	bool success=TRUE;
	
	assert(region_supports_attach(dest));
	
	FOR_ALL_TYPED(src, r, WClientWin){
		if(!region_attach_sub(dest, (WRegion*)r, 0))
			success=FALSE;
	}
	
	return success;
}


bool region_rescue_clientwins(WRegion *reg)
{
	WRegion *p=region_find_new_home(reg);
	WRegion *r;
	
	if(p!=NULL){
		if(region_move_clientwins(p, reg))
			return TRUE;
	}
	
	/* This should not happen unless the region is not
	 * properly in a tree with a WScreen root.
	 */
	
	warn("Unable to move client windows of a (to-be-destroyed) "
		 "somewhere else.");
	
	return FALSE;
}


/*}}}*/

