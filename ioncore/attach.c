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

	return region_add_managed(mgr, reg, &param);
}


bool region_add_managed(WRegion *mgr, WRegion *reg, 
						const WAttachParams *param)
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
					 
	return (region_do_add_managed(mgr, (WRegionAddFn*)add_fn_reparent,
								  (void*)reg, param)!=NULL);
}


/*}}}*/


/*{{{ Attach Lua interface */


static WRegion *add_fn_load(WWindow *par, WRectangle geom, ExtlTab *tab)
{
	return load_create_region(par, geom, *tab);
}


static void get_attach_params(ExtlTab tab, WAttachParams *param)
{
	ExtlTab geomtab;
	bool sel;
	char *typestr;
	
	param->flags=0;
	
	if(extl_table_gets_t(tab, "geom", &geomtab)){
		if(extltab_to_geom(geomtab, &(param->geomrq)))
			param->flags|=(REGION_ATTACH_SIZERQ|REGION_ATTACH_POSRQ);
		extl_unref_table(geomtab);
	}

	if(extl_table_is_bool_set(tab, "selected"))
		param->flags|=REGION_ATTACH_SWITCHTO;
}


WRegion *region_add_managed_load(WRegion *mgr, ExtlTab tab)
{
	WAttachParams param;
	get_attach_params(tab, &param);
	return region_do_add_managed(mgr, (WRegionAddFn*)add_fn_load,
								 (void*)&tab, &param);
}


/*EXTL_DOC
 * Create a new region to be managed by \var{mgr}. The manager
 * must implement \code{region_do_add_managed}. The table \var{desc}
 * should contain the type of region to create in the field \var{type}.
 * The following optional generic fields are also known, but the
 * table may also contain additional parameters that depend on the type
 * of region being created.
 * \begin{tabularx}{\linewidth}{lX}
 *  \hline
 *  Field & Description \\
 *  \hline
 * 	\var{type} & Class name of the object to create (mandatory). \\
 *  \var{geom} & Geometry \emph{request}; a table with fields
 *				 \var{x}, \var{y}, \var{w} and \var{h}. \\
 * 	\var{selected} & Boolean value indicating whether the new region should
 *					 be made the selected one within \var{mgr}. \\
 *  \var{name} & Name of the region. Passed to \fnref{region_set_name}. \\
 * \end{tabularx}
 */
EXTL_EXPORT
WRegion *region_manage_new(WRegion *mgr, ExtlTab desc)
{
	return region_add_managed_load(mgr, desc);
}


/*EXTL_DOC
 * Requests that \var{mgr} start managing \var{reg}. The optional
 * table argument \var{tab} may contain the fields \var{geom}
 * and \var{selected}; see \fnref{region_manage_new} for details.
 */
EXTL_EXPORT
bool region_manage(WRegion *mgr, WRegion *reg, ExtlTab tab)
{
	WAttachParams param;
	get_attach_params(tab, &param);
	return region_add_managed(mgr, reg, &param);
}


/*}}}*/


/*{{{ Rescue */


bool region_can_manage_clientwins(WRegion *reg)
{
	return (HAS_DYN(reg, genws_add_clientwin) ||
			HAS_DYN(reg, region_do_add_managed));
}


WRegion *default_find_rescue_manager_for(WRegion *reg, WRegion *todst)
{
	WRegion *p;
	
	if(reg!=todst && !WOBJ_IS_BEING_DESTROYED(reg)){
		if(region_can_manage_clientwins(reg))
			return reg;
	}
	
	p=region_manager_or_parent(reg);
	if(p==NULL)
		return NULL;
	
	if(!WOBJ_IS_BEING_DESTROYED(p)){
		WRegion *nm=region_find_rescue_manager_for(p, reg);
		if(nm!=NULL)
			return nm;
	}
	
	return default_find_rescue_manager_for(p, reg);
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
	return default_find_rescue_manager_for(reg, reg);
}


typedef struct{
	WRegion *dest;
	bool hascw;
	int xroot, yroot;
} RescueState;


static void init_rescue(RescueState *st, WRegion *reg)
{
	st->dest=region_find_rescue_manager(reg);

	if(st->dest==NULL)
		return;
	
	st->hascw=FALSE;
	
	if(HAS_DYN(st->dest, genws_add_clientwin)){
		st->hascw=TRUE;
	}else if(!HAS_DYN(st->dest, region_do_add_managed)){
		return;
	}

	region_rootpos(st->dest, &(st->xroot), &(st->yroot));
}


static bool do_rescue(RescueState *st, WClientWin *cwin)
{
	WAttachParams param;
	XSizeHints szhnt;

	/* We do not descend into other objects, their destroy functions
	 * can handle the rescue.
	 */
	param.geomrq.x-=st->xroot;
	param.geomrq.y-=st->yroot;
	param.geomrq.w=REGION_GEOM(cwin).w;
	param.geomrq.h=REGION_GEOM(cwin).h;
	param.flags=(REGION_ATTACH_POSRQ|REGION_ATTACH_SIZERQ);
	/* TODO: StaticGravity */
		
	if(st->hascw){
		return genws_add_clientwin((WGenWS*)(st->dest), cwin, &param);
	}else{
		return region_add_managed(st->dest, (WRegion*)cwin, &param);
	}
}


bool rescue_managed_clientwins(WRegion *reg, WRegion *list)
{
	WRegion *r, *next;
	RescueState st;
	
	if(list==NULL || wglobal.opmode==OPMODE_DEINIT)
		return TRUE;
	
	init_rescue(&st, reg);
		
	FOR_ALL_MANAGED_ON_LIST_W_NEXT(list, r, next){
		if(!WOBJ_IS(r, WClientWin))
			continue;

		if(!do_rescue(&st, (WClientWin*)r)){
			warn("Unable to rescue a client window managed by a %s.",
				 WOBJ_TYPESTR(reg));
			return FALSE;
		}
	}
	
	return TRUE;
}


bool rescue_child_clientwins(WRegion *reg)
{
	WClientWin *r, *next;
	RescueState st;
	
	if(wglobal.opmode==OPMODE_DEINIT)
		return TRUE;
	
	init_rescue(&st, reg);
		
	for(r=FIRST_CHILD(reg, WClientWin); r!=NULL; r=next){
		next=NEXT_CHILD(r, WClientWin);
		if(!do_rescue(&st, r)){
			warn("Unable to rescue a client window contained in a %s.",
				 WOBJ_TYPESTR(reg));
			return FALSE;
		}
	}
	
	return TRUE;
}


/*}}}*/

