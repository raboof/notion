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
#include "saveload.h"


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

	if(extl_table_gets_b(tab, "selected", &sel)){
		if(sel)
			param->flags|=REGION_ATTACH_SWITCHTO;
	}
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
 					 be made the selected one within \var{mgr}. \\
 *  \var{name} & The (short) name of the region. Note that this name
 *               should not be used to reference the object but the full
 * 				 name with instance number (\fnref{region_full_name}). \\
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

