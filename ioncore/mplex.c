/*
 * ion/ioncore/mplex.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "objp.h"
#include "window.h"
#include "global.h"
#include "rootwin.h"
#include "focus.h"
#include "event.h"
#include "attach.h"
#include "resize.h"
#include "tags.h"
#include "sizehint.h"
#include "stacking.h"
#include "extlconv.h"
#include "genws.h"
#include "genframe-pointer.h"
#include "bindmaps.h"
#include "regbind.h"


#define WMPLEX_WIN(MPLEX) ((MPLEX)->win.win)
#define WMPLEX_MGD_UNVIEWABLE(MPLEX) \
			((MPLEX)->flags&WMPLEX_MANAGED_UNVIEWABLE)


/*{{{ Destroy/create mplex */


bool mplex_init(WMPlex *mplex, WWindow *parent, Window win,
				WRectangle geom)
{
	mplex->managed_count=0;
	mplex->managed_list=NULL;
	mplex->current_sub=NULL;
	mplex->current_input=NULL;
	mplex->flags=0;
	
	if(!window_init((WWindow*)mplex, parent, win, geom))
		return FALSE;
	
	mplex->win.region.flags|=REGION_BINDINGS_ARE_GRABBED;

	region_add_bindmap((WRegion*)mplex, &ioncore_mplex_bindmap);

	return TRUE;
}


void mplex_deinit(WMPlex *mplex)
{
	window_deinit((WWindow*)mplex);
}


/*}}}*/


/*{{{ Ordering */


/*EXTL_DOC
 * Move currently selected region to the right.
 */
EXTL_EXPORT
void mplex_move_current_right(WMPlex *mplex)
{
	WRegion *reg, *next;
	
	if((reg=mplex->current_sub)==NULL)
		return;
	if((next=NEXT_MANAGED(mplex->managed_list, reg))==NULL)
		return;
	
	UNLINK_ITEM(mplex->managed_list, reg, mgr_next, mgr_prev);
	LINK_ITEM_AFTER(mplex->managed_list, next, reg, mgr_next, mgr_prev);
	
	mplex_managed_changed(mplex, FALSE);
}


/*EXTL_DOC
 * Move currently selected region to the left.
 */
EXTL_EXPORT
void mplex_move_current_left(WMPlex *mplex)
{
	WRegion *reg, *prev;
	
	if((reg=mplex->current_sub)==NULL)
		return;
	if((prev=PREV_MANAGED(mplex->managed_list, reg))==NULL)
		return;

	UNLINK_ITEM(mplex->managed_list, reg, mgr_next, mgr_prev);
	LINK_ITEM_BEFORE(mplex->managed_list, prev, reg, mgr_next, mgr_prev);

	mplex_managed_changed(mplex, FALSE);
}


/*}}}*/


/*{{{ Resize and reparent */


static void reparent_or_fit(WMPlex *mplex, const WRectangle *geom,
							WWindow *parent)
{
	bool wchg=(REGION_GEOM(mplex).w!=geom->w);
	bool hchg=(REGION_GEOM(mplex).h!=geom->h);
	bool move=(REGION_GEOM(mplex).x!=geom->x ||
			   REGION_GEOM(mplex).y!=geom->y);
	
	if(parent!=NULL){
		XReparentWindow(wglobal.dpy, WMPLEX_WIN(mplex), parent->win,
						geom->x, geom->y);
		XResizeWindow(wglobal.dpy, WMPLEX_WIN(mplex), geom->w, geom->h);
	}else{
		XMoveResizeWindow(wglobal.dpy, WMPLEX_WIN(mplex),
						  geom->x, geom->y, geom->w, geom->h);
	}
	
	if(move && !wchg && !hchg)
		region_notify_subregions_move(&(mplex->win.region));
	else if(wchg || hchg)
		mplex_fit_managed(mplex);
	
	if(wchg || hchg)
		mplex_size_changed(mplex, wchg, hchg);
}


bool mplex_reparent(WMPlex *mplex, WWindow *parent, WRectangle geom)
{
	if(!same_rootwin((WRegion*)mplex, (WRegion*)parent))
		return FALSE;
	
	region_detach_parent((WRegion*)mplex);
	region_set_parent((WRegion*)mplex, (WRegion*)parent);
	reparent_or_fit(mplex, &geom, parent);
	
	return TRUE;
}


void mplex_fit(WMPlex *mplex, WRectangle geom)
{
	reparent_or_fit(mplex, &geom, NULL);
}


void mplex_fit_managed(WMPlex *mplex)
{
	WRectangle geom;
	WRegion *sub;
	
	if(WMPLEX_MGD_UNVIEWABLE(mplex))
		return;
	
	mplex_managed_geom(mplex, &geom);
	
	FOR_ALL_MANAGED_ON_LIST(mplex->managed_list, sub){
		region_fit(sub, geom);
	}
	
	if(mplex->current_input!=NULL)
		region_fit(mplex->current_input, geom);
}


static void mplex_request_managed_geom(WMPlex *mplex, WRegion *sub,
									   int flags, WRectangle geom, 
									   WRectangle *geomret)
{
	/* Just try to give it the maximum size */
	mplex_managed_geom(mplex, &geom);
	
	if(geomret!=NULL)
		*geomret=geom;
	
	if(!(flags&REGION_RQGEOM_TRYONLY))
		region_fit(sub, geom);
}


/*}}}*/


/*{{{ Mapping */


void mplex_map(WMPlex *mplex)
{
	window_map((WWindow*)mplex);
	/* A lame requirement of the ICCCM is that client windows should be
	 * unmapped if the parent is unmapped.
	 */
	if(mplex->current_sub!=NULL && !WMPLEX_MGD_UNVIEWABLE(mplex))
		region_map(mplex->current_sub);
}


void mplex_unmap(WMPlex *mplex)
{
	window_unmap((WWindow*)mplex);
	/* A lame requirement of the ICCCM is that client windows should be
	 * unmapped if the parent is unmapped.
	 */
	if(mplex->current_sub!=NULL)
		region_unmap(mplex->current_sub);
}


/*}}}*/


/*{{{ Managed region switching */


static bool mplex_do_display_managed(WMPlex *mplex, WRegion *sub)
{
	bool mapped;
	
	if(sub==mplex->current_sub || sub==mplex->current_input)
		return TRUE;
	
	if(mplex->current_sub!=NULL && REGION_IS_MAPPED(mplex))
		region_unmap(mplex->current_sub);
	
	mplex->current_sub=sub;
	
	if(REGION_IS_MAPPED(mplex) && !WMPLEX_MGD_UNVIEWABLE(mplex))
		region_map(sub);
	else
		region_unmap(sub);
	
	if(mplex->current_input==NULL){
		if(REGION_IS_ACTIVE(mplex))
			set_focus(sub);
	}
	
	/* Many programs will get upset if the visible, although only such,
	 * client window is not the lowest window in the mplex. xprop/xwininfo
	 * will return the information for the lowest window. 'netscape -remote'
	 * will not work at all if there are no visible netscape windows.
	 */
	region_lower(sub);
	
	return TRUE;
}


bool mplex_display_managed(WMPlex *mplex, WRegion *sub)
{
	if(mplex_do_display_managed(mplex, sub)){
		mplex_managed_changed(mplex, TRUE);
		return TRUE;
	}
	
	return FALSE;
}


/*EXTL_DOC
 * Returns the \var{n}:th object managed by \var{mplex}.
 */
EXTL_EXPORT
WRegion *mplex_nth_managed(WMPlex *mplex, uint n)
{
	WRegion *reg=FIRST_MANAGED(mplex->managed_list);
	
	while(n-->0 && reg!=NULL)
		reg=NEXT_MANAGED(mplex->managed_list, reg);
	
	return reg;
}


static void do_switch(WMPlex *mplex, WRegion *sub)
{
	if(sub==NULL)
		return;
	
	/* Allow workspaces to warp on switch */
	if(region_may_control_focus((WRegion*)mplex) && WOBJ_IS(sub, WGenWS))
		region_goto(sub);
	else
		region_display_sp(sub);
}


/*EXTL_DOC
 * Have \var{mplex} display the \var{n}:th object managed by it.
 */
EXTL_EXPORT
void mplex_switch_nth(WMPlex *mplex, uint n)
{
	do_switch(mplex, mplex_nth_managed(mplex, n));
}


/*EXTL_DOC
 * Have \var{mplex} display next (wrt. currently selected) object managed 
 * by it.
 */
EXTL_EXPORT
void mplex_switch_next(WMPlex *mplex)
{
	do_switch(mplex, NEXT_MANAGED_WRAP(mplex->managed_list, 
									   mplex->current_sub));
}


/*EXTL_DOC
 * Have \var{mplex} display previous (wrt. currently selected) object
 * managed by it.
 */
EXTL_EXPORT
void mplex_switch_prev(WMPlex *mplex)
{
	do_switch(mplex, PREV_MANAGED_WRAP(mplex->managed_list, 
									   mplex->current_sub));
}


/*}}}*/


/*{{{ Add/remove managed */


static WRegion *mplex_do_add_managed(WMPlex *mplex, WRegionAddFn *fn,
									 void *fnparams, 
									 const WAttachParams *param)
{
	WRectangle geom;
	WRegion *reg;
	
	mplex_managed_geom(mplex, &geom);
	
	reg=fn((WWindow*)mplex, geom, fnparams);
	
	if(reg==NULL)
		return NULL;
	
	if(mplex->current_sub!=NULL && wglobal.opmode!=OPMODE_INIT){
		region_set_manager(reg, (WRegion*)mplex, NULL);
		if(mplex->flags&WMPLEX_ADD_TO_END){
			LINK_ITEM(mplex->managed_list, reg, mgr_next, mgr_prev);
		}else{
			LINK_ITEM_AFTER(mplex->managed_list, mplex->current_sub,
							reg, mgr_next, mgr_prev);
		}
	}else{
		region_set_manager(reg, (WRegion*)mplex, &(mplex->managed_list));
	}
	
	mplex->managed_count++;
	
	if(mplex->managed_count==1 || param->flags&REGION_ATTACH_SWITCHTO){
		mplex_do_display_managed(mplex, reg);
		mplex_managed_added(mplex, reg, TRUE);
	}else{
		region_unmap(reg);
		mplex_managed_added(mplex, reg, FALSE);
	}
	
	return reg;
}


static void mplex_do_remove(WMPlex *mplex, WRegion *sub)
{
	WRegion *next=NULL;
	
	if(mplex->current_sub==sub){
		next=PREV_MANAGED(mplex->managed_list, sub);
		if(next==NULL)
			next=NEXT_MANAGED(mplex->managed_list, sub);
		mplex->current_sub=NULL;
	}
	
	region_unset_manager(sub, (WRegion*)mplex, &(mplex->managed_list));
	mplex->managed_count--;
	
	if(wglobal.opmode!=OPMODE_DEINIT){
		if(next!=NULL)
			mplex_do_display_managed(mplex, next);
		mplex_managed_removed(mplex, sub, next!=NULL);
	}
}


void mplex_remove_managed(WMPlex *mplex, WRegion *reg)
{
	if(mplex->current_input==reg){
		region_unset_manager(reg, (WRegion*)mplex, NULL);
		mplex->current_input=NULL;
		if(REGION_IS_ACTIVE(mplex))
			set_focus((WRegion*)mplex);
	}else{
		mplex_do_remove(mplex, reg);
	}
}


/*EXTL_DOC
 * Attach all tagged regions to \var{mplex}.
 */
EXTL_EXPORT
void mplex_attach_tagged(WMPlex *mplex)
{
	WRegion *reg;
	
	while((reg=tag_take_first())!=NULL){
		if(region_is_ancestor((WRegion*)mplex, reg)){
			warn("Cannot attach tagged region: ancestor");
			continue;
		}
		region_add_managed_simple((WRegion*)mplex, reg, 0);
	}
}


/*}}}*/


/*{{{ Focus  */


void mplex_focus(WMPlex *mplex, bool warp)
{
	if(warp)
		do_move_pointer_to((WRegion*)mplex);
	
	if(!WMPLEX_MGD_UNVIEWABLE(mplex)){
		if(mplex->current_input!=NULL){
			region_set_focus_to((WRegion*)mplex->current_input, FALSE);
			return;
		}else if(mplex->current_sub!=NULL){
			region_set_focus_to(mplex->current_sub, FALSE);
			return;
		}
	}
	
	SET_FOCUS(WMPLEX_WIN(mplex));
}


static WRegion *mplex_managed_enter_to_focus(WMPlex *mplex, WRegion *reg)
{
	return mplex->current_input;
}


/*}}}*/


/*{{{ Inputs */


/*EXTL_DOC
 * Returns the currently active ''input'' (query, message etc.) in
 * \var{mplex} or nil.
 */
EXTL_EXPORT
WRegion *mplex_current_input(WMPlex *mplex)
{
	return mplex->current_input;
}


WRegion *mplex_add_input(WMPlex *mplex, WRegionAddFn *fn, void *fnp)
{
	WRectangle geom;
	WRegion *sub;
	
	if(mplex->current_input!=NULL || WMPLEX_MGD_UNVIEWABLE(mplex))
		return NULL;
	
	mplex_managed_geom(mplex, &geom);
	sub=fn((WWindow*)mplex, geom, fnp);
	
	if(sub==NULL)
		return NULL;
	
	mplex->current_input=sub;
	region_set_manager(sub, (WRegion*)mplex, NULL);
	region_keep_on_top(sub);
	region_map(sub);
	
	if(region_may_control_focus((WRegion*)mplex))
		set_focus(sub);

	return sub;
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Return the object managed by and currenly displayed in \var{mplex}.
 */
EXTL_EXPORT
WRegion *mplex_current(WMPlex *mplex)
{
	return mplex->current_sub;
}

/*EXTL_DOC
 * Returns a list of regions managed by \var{mplex}.
 */
EXTL_EXPORT
ExtlTab mplex_managed_list(WMPlex *mplex)
{
	return managed_list_to_table(mplex->managed_list, NULL);
}


/*EXTL_DOC
 * Returns the number of regions managed/multiplexed by \var{mplex}.
 */
EXTL_EXPORT
int mplex_managed_count(WMPlex *mplex)
{
	return mplex->managed_count;
}


static bool mplex_handle_drop(WMPlex *mplex, int x, int y,
								 WRegion *dropped)
{
	return region_add_managed_simple((WRegion*)mplex, dropped,
									 REGION_ATTACH_SWITCHTO);
}


/*}}}*/


/*{{{ Dynfuns */


void mplex_managed_geom(const WMPlex *mplex, WRectangle *geom)
{
	CALL_DYN(mplex_managed_geom, mplex, (mplex, geom));
}


void mplex_size_changed(WMPlex *mplex, bool wchg, bool hchg)
{
	CALL_DYN(mplex_size_changed, mplex, (mplex, wchg, hchg));
}


void mplex_managed_changed(WMPlex *mplex, bool sw)
{
	CALL_DYN(mplex_managed_changed, mplex, (mplex, sw));
}


void mplex_managed_added(WMPlex *mplex, WRegion *nreg, bool sw)
{
	CALL_DYN(mplex_managed_added, mplex, (mplex, nreg, sw));
}

static void mplex_managed_added_default(WMPlex *mplex, WRegion *nreg, bool sw)
{
	mplex_managed_changed(mplex, sw);
}

void mplex_managed_removed(WMPlex *mplex, WRegion *nreg, bool sw)
{
	CALL_DYN(mplex_managed_removed, mplex, (mplex, nreg, sw));
}

static void mplex_managed_removed_default(WMPlex *mplex, WRegion *nreg, 
										  bool sw)
{
	mplex_managed_changed(mplex, sw);
}

/*}}}*/


/*{{{ Dynfuntab and class info */


static DynFunTab mplex_dynfuntab[]={
	{region_fit, mplex_fit},
	{(DynFun*)reparent_region, (DynFun*)mplex_reparent},

	{region_set_focus_to, mplex_focus},
	{(DynFun*)region_managed_enter_to_focus,
	 (DynFun*)mplex_managed_enter_to_focus},
	
	{(DynFun*)region_do_add_managed, (DynFun*)mplex_do_add_managed},
	{region_remove_managed, mplex_remove_managed},
	{region_request_managed_geom, mplex_request_managed_geom},
	{(DynFun*)region_display_managed, (DynFun*)mplex_display_managed},

	{(DynFun*)region_handle_drop, (DynFun*)mplex_handle_drop},
	
	{region_map, mplex_map},
	{region_unmap, mplex_unmap},
	
	{mplex_managed_added, mplex_managed_added_default},
	{mplex_managed_removed, mplex_managed_removed_default},
	
	END_DYNFUNTAB
};
									   

IMPLOBJ(WMPlex, WWindow, mplex_deinit, mplex_dynfuntab);


/*}}}*/
