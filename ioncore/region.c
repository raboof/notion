/*
 * ion/ioncore/region.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "global.h"
#include "objp.h"
#include "region.h"
#include "colormap.h"
#include "focus.h"
#include "attach.h"
#include "regbind.h"
#include "tags.h"
#include "names.h"
#include "stacking.h"
#include "resize.h"
#include "extl.h"
#include "extlconv.h"
#include "activity.h"


#define FOR_ALL_SUBREGIONS(REG, SUB) \
	FOR_ALL_TYPED_CHILDREN(REG, SUB, WRegion)

#define FOR_ALL_SUBREGIONS_REVERSE(REG, SUB) \
	for((SUB)=LAST_CHILD(REG, WRegion);      \
		(SUB)!=NULL;                         \
		(SUB)=PREV_CHILD(SUB, WRegion))


#define D2(X)


/*{{{ Init & deinit */


void region_init(WRegion *reg, WRegion *parent, const WRectangle *geom)
{
	reg->geom=*geom;
	reg->flags=0;
	reg->bindings=NULL;
	reg->rootwin=NULL;
	
	reg->children=NULL;
	reg->parent=NULL;
	reg->p_next=NULL;
	reg->p_prev=NULL;
	
	reg->active_sub=NULL;
	
	reg->ni.name=NULL;
	reg->ni.namespaceinfo=NULL;
	reg->ni.ns_next=NULL;
	reg->ni.ns_prev=NULL;
	
	reg->manager=NULL;
	reg->mgr_next=NULL;
	reg->mgr_prev=NULL;
	
	reg->stacking.below_list=NULL;
	reg->stacking.next=NULL;
	reg->stacking.prev=NULL;
	reg->stacking.above=NULL;
	
	reg->mgd_activity=FALSE;

	if(!WOBJ_IS(reg, WClientWin))
		region_set_name(reg, WOBJ_TYPESTR(reg));
	
	if(parent!=NULL){
		reg->rootwin=parent->rootwin;
		region_set_parent(reg, parent);
	}else{
		assert(WOBJ_IS(reg, WRootWin));/* || WOBJ_IS(reg, WScreen));*/
	}
}


static void destroy_children(WRegion *reg)
{
	WRegion *sub, *prev=NULL;

	/* destroy children */
	while(1){
		sub=reg->children;
		if(sub==NULL)
			break;
		assert(!WOBJ_IS_BEING_DESTROYED(sub));
		assert(sub!=prev);
		prev=sub;
		destroy_obj((WObj*)sub);
	}
}


void region_deinit(WRegion *reg)
{
	rescue_child_clientwins(reg);
	
	destroy_children(reg);

	if(wglobal.focus_next==reg){
		D(warn("Region to be focused next destroyed[1]."));
		wglobal.focus_next=NULL;
	}
	
	region_detach(reg);
	region_unuse_name(reg);
	region_untag(reg);
	region_remove_bindings(reg);

	if(wglobal.focus_next==reg){
		D(warn("Region to be focused next destroyed[2]."));
		wglobal.focus_next=NULL;
	}
}


/*}}}*/


/*{{{ Dynfuns */


void region_fit(WRegion *reg, const WRectangle *geom)
{
	CALL_DYN(region_fit, reg, (reg, geom));
}


void region_draw_config_updated(WRegion *reg)
{
	CALL_DYN(region_draw_config_updated, reg, (reg));
}


void region_map(WRegion *reg)
{
	CALL_DYN(region_map, reg, (reg));
}


void region_unmap(WRegion *reg)
{
	CALL_DYN(region_unmap, reg, (reg));
}


void region_notify_rootpos(WRegion *reg, int x, int y)
{
	CALL_DYN(region_notify_rootpos, reg, (reg, x, y));
}


Window region_x_window(const WRegion *reg)
{
	Window ret=None;
	CALL_DYN_RET(ret, Window, region_x_window, reg, (reg));
	return ret;
}


void region_activated(WRegion *reg)
{
	CALL_DYN(region_activated, reg, (reg));
}


void region_inactivated(WRegion *reg)
{
	CALL_DYN(region_inactivated, reg, (reg));
}


void region_set_focus_to(WRegion *reg, bool warp)
{
	CALL_DYN(region_set_focus_to, reg, (reg, warp));
}


/*EXTL_DOC
 * Attempt to close/destroy \var{reg}. Whether this operation works
 * depends on whether the particular type of region in question has
 * implemented the feature and, in case of client windows, whether
 * the client supports the \code{WM_DELETE} protocol (see also
 * \fnref{WClientWin.kill}).
 */
EXTL_EXPORT_MEMBER
void region_close(WRegion *reg)
{
	CALL_DYN(region_close, reg, (reg));
}


bool region_may_destroy_managed(WRegion *mgr, WRegion *reg)
{
	bool ret=TRUE;
	CALL_DYN_RET(ret, bool, region_may_destroy_managed, mgr, (mgr, reg));
	return ret;
}

	
/*{{{ Manager region dynfuns */


void region_managed_activated(WRegion *mgr, WRegion *reg)
{
	CALL_DYN(region_managed_activated, mgr, (mgr, reg));
}


void region_managed_inactivated(WRegion *mgr, WRegion *reg)
{
	CALL_DYN(region_managed_inactivated, mgr, (mgr, reg));
}


bool region_display_managed(WRegion *mgr, WRegion *reg)
{
	bool ret=TRUE;
	CALL_DYN_RET(ret, bool, region_display_managed, mgr, (mgr, reg));
	return ret;
}


void region_notify_managed_change(WRegion *mgr, WRegion *reg)
{
	CALL_DYN(region_notify_managed_change, mgr, (mgr, reg));
}


void region_remove_managed(WRegion *mgr, WRegion *reg)
{
	CALL_DYN(region_remove_managed, mgr, (mgr, reg));
}


WRegion *region_managed_enter_to_focus(WRegion *mgr, WRegion *reg)
{
	WRegion *ret=NULL;
	CALL_DYN_RET(ret, WRegion*, region_managed_enter_to_focus, mgr, (mgr, reg));
	return ret;
}


WRegion *region_current(WRegion *mgr)
{
	WRegion *ret=NULL;
	CALL_DYN_RET(ret, WRegion*, region_current, mgr, (mgr));
	return ret;
}


/*}}}*/


/*}}}*/


/*{{{ Dynfun defaults */


static void default_notify_rootpos(WRegion *reg, int x, int y)
{
	region_notify_subregions_rootpos(reg, x, y);
}


#if 0
static Window default_restack(WRegion *reg, Window other, int mode)
{
	
	WRegion *sub;	
	Window w;
	
	if(mode==Above){
		FOR_ALL_SUBREGIONS(reg, sub){
			w=region_restack(sub, other, mode);
			if(w!=None)
				other=w;
		}
	}else{ /* mode==Below */
		FOR_ALL_SUBREGIONS_REVERSE(reg, sub){
			w=region_restack(sub, other, mode);
			if(w!=None)
				other=w;
		}
	}
	
	return other;
}
#endif


void region_default_draw_config_updated(WRegion *reg)
{
	WRegion *sub;
	
	FOR_ALL_TYPED_CHILDREN(reg, sub, WRegion){
		region_draw_config_updated(sub);
	}
}


/*}}}*/


/*{{{ Detach */


void region_detach_parent(WRegion *reg)
{
	WRegion *p=reg->parent;

	region_reset_stacking(reg);

	if(p==NULL || p==reg)
		return;
	
	UNLINK_ITEM(p->children, reg, p_next, p_prev);
	reg->parent=NULL;

	if(p->active_sub==reg){
		p->active_sub=NULL;
		
		/* Removed: seems to confuse floatws:s when frames are
		 * destroyd.
		 */
		/*if(REGION_IS_ACTIVE(reg) && wglobal.focus_next==NULL)
			set_focus(p);*/
	}
}


void region_detach_manager(WRegion *reg)
{
	WRegion *mgr;
	
	mgr=REGION_MANAGER(reg);
	
	if(mgr==NULL)
		return;
	
	D2(fprintf(stderr, "detach %s (mgr:%s)\n", WOBJ_TYPESTR(reg),
			   WOBJ_TYPESTR(mgr)));
	
	/* Restore activity state to non-parent manager */
	if(region_may_control_focus(reg)){
		WRegion *par=region_parent(reg);
		if(par!=NULL && mgr!=par && region_parent(mgr)==par){
			/* REGION_ACTIVE shouldn't be set for windowless regions
			 * but make the parent's active_sub point to it
			 * nevertheless so that region_may_control_focus can
			 * be made to work.
			 */
			D2(fprintf(stderr, "detach mgr %s, %s->active_sub=%s\n",
					   WOBJ_TYPESTR(reg), WOBJ_TYPESTR(par),
					   WOBJ_TYPESTR(mgr)));
			par->active_sub=mgr;
			if(region_x_window(mgr)!=None){
				D2(fprintf(stderr, "\tdo_set_focus\n"));
				do_set_focus(mgr, FALSE);
			}
		}
	}

	region_clear_activity(reg);

	region_remove_managed(mgr, reg);
	
	assert(REGION_MANAGER(reg)==NULL);
}


void region_detach(WRegion *reg)
{
	region_detach_manager(reg);
	region_detach_parent(reg);
}


/*}}}*/


/*{{{ Focus */


bool _region_may_control_focus(WRegion *reg)
{
	WRegion *par, *r2;
	
	if(REGION_IS_ACTIVE(reg))
		return TRUE;
	
	par=region_parent(reg);
	
	if(par==NULL || !REGION_IS_ACTIVE(par))
		return FALSE;
	
	r2=par->active_sub;
	while(r2!=NULL && r2!=par){
		if(r2==reg)
			return TRUE;
		r2=REGION_MANAGER(r2);
	}

	return FALSE;
}


bool region_may_control_focus(WRegion *reg)
{
	bool mcf=_region_may_control_focus(reg);
	D2(fprintf(stderr, "mcf: %s %d\n", WOBJ_TYPESTR(reg), mcf));
	return mcf;
}

void region_got_focus(WRegion *reg)
{
	WRegion *r;
	
	region_clear_activity(reg);
	
	if(!REGION_IS_ACTIVE(reg)){
		D(fprintf(stderr, "got focus (inact) %s [%p]\n", WOBJ_TYPESTR(reg), reg);)
		reg->flags|=REGION_ACTIVE;
		
		r=region_parent(reg);
		if(r!=NULL)
			r->active_sub=reg;
		
		region_activated(reg);
		
		r=REGION_MANAGER(reg);
		if(r!=NULL)
			region_managed_activated(r, reg);
	}else{
		D(fprintf(stderr, "got focus (act) %s [%p]\n", WOBJ_TYPESTR(reg), reg);)
    }

	/* Install default colour map only if there is no active subregion;
	 * their maps should come first. WClientWins will install their maps
	 * in region_activated. Other regions are supposed to use the same
	 * default map.
	 */
	if(reg->active_sub==NULL && !WOBJ_IS(reg, WClientWin))
		install_cmap(ROOTWIN_OF(reg), None); 
}


void region_lost_focus(WRegion *reg)
{
	WRegion *r;
	
	if(!REGION_IS_ACTIVE(reg)){
		D(fprintf(stderr, "lost focus (inact) %s [%p:]\n", WOBJ_TYPESTR(reg), reg);)
		return;
	}
	
	D(fprintf(stderr, "lost focus (act) %s [%p:]\n", WOBJ_TYPESTR(reg), reg);)
	
	reg->flags&=~REGION_ACTIVE;
	region_inactivated(reg);
	r=REGION_MANAGER(reg);
	if(r!=NULL)
		region_managed_inactivated(r, reg);
}


/*EXTL_DOC
 * Is \var{reg} active/does it or one of it's children of focus?
 */
EXTL_EXPORT_MEMBER
bool region_is_active(WRegion *reg)
{
	return REGION_IS_ACTIVE(reg);
}


/*}}}*/

	
/*{{{ Goto */


/*EXTL_DOC
 * Attempt to display \var{reg}.
 */
EXTL_EXPORT_MEMBER
bool region_display(WRegion *reg)
{
	WRegion *mgr, *preg;

	if(region_is_fully_mapped(reg))
		return TRUE;
	
	mgr=REGION_MANAGER(reg);
	
	if(mgr!=NULL){
		if(!region_display(mgr))
			return FALSE;
		return region_display_managed(mgr, reg);
	}
	
	preg=region_parent(reg);

	if(preg!=NULL && !region_display(preg))
		return FALSE;

	region_map(reg);
	return TRUE;
}


/*EXTL_DOC
 * Attempt to display \var{reg} and save the current region
 * activity status for use by \fnref{goto_previous}.
 */
EXTL_EXPORT_MEMBER
bool region_display_sp(WRegion *reg)
{
	bool ret;
	
	set_previous_of(reg);
	protect_previous();
	ret=region_display(reg);
	unprotect_previous();

	return ret;
}


/*EXTL_DOC
 * Attempt to display \var{reg}, save region activity status and then
 * warp to (or simply set focus to if warping is disabled) \var{reg}.
 */
EXTL_EXPORT_MEMBER
bool region_goto(WRegion *reg)
{
	if(region_display_sp(reg)){
		warp(reg);
		return TRUE;
	}
	return FALSE;
}


/*}}}*/


/*{{{ Helpers/misc */


/*EXTL_DOC
 * Is \var{reg} visible/is it and all it's ancestors mapped?
 */
EXTL_EXPORT_MEMBER_AS(WRegion, is_mapped)
bool region_is_fully_mapped(WRegion *reg)
{
	for(; reg!=NULL; reg=region_parent(reg)){
		if(!REGION_IS_MAPPED(reg))
			return FALSE;
	}
	
	return TRUE;
}


void region_rootgeom(WRegion *reg, int *xret, int *yret)
{
	*xret=REGION_GEOM(reg).x;
	*yret=REGION_GEOM(reg).y;
	
	while(1){
		reg=region_parent(reg);
		if(reg==NULL)
			break;
		*xret+=REGION_GEOM(reg).x;
		*yret+=REGION_GEOM(reg).y;
	}
}


void region_notify_subregions_move(WRegion *reg)
{
	int x, y;
	
	region_rootgeom(reg, &x, &y);
	region_notify_subregions_rootpos(reg, x, y);
}


void region_notify_subregions_rootpos(WRegion *reg, int x, int y)
{
	WRegion *sub;
	
	FOR_ALL_SUBREGIONS(reg, sub){
		region_notify_rootpos(sub,
							  x+REGION_GEOM(sub).x,
							  y+REGION_GEOM(sub).y);
	}
}


void region_rootpos(WRegion *reg, int *xret, int *yret)
{
	WRegion *par;

	par=region_parent(reg);
	
	if(par==NULL || par==reg){
		*xret=0;
		*yret=0;
		return;
	}
	
	region_rootpos(par, xret, yret);
	
	*xret+=REGION_GEOM(reg).x;
	*yret+=REGION_GEOM(reg).y;
}


void region_notify_change(WRegion *reg)
{
	WRegion *mgr=REGION_MANAGER(reg);
	
	if(mgr!=NULL)
		region_notify_managed_change(mgr, reg);
}


void region_set_manager(WRegion *reg, WRegion *mgr, WRegion **listptr)
{
	assert(reg->manager==NULL);
	
	reg->manager=mgr;
	if(listptr!=NULL){
		LINK_ITEM(*listptr, reg, mgr_next, mgr_prev);
	}
}


void region_unset_manager(WRegion *reg, WRegion *mgr, WRegion **listptr)
{
	if(reg->manager!=mgr)
		return;
	reg->manager=NULL;
	if(listptr!=NULL){
		UNLINK_ITEM(*listptr, reg, mgr_next, mgr_prev);
	}
}


void region_set_parent(WRegion *reg, WRegion *parent)
{
	assert(reg->parent==NULL && parent!=NULL);
	LINK_ITEM(parent->children, reg, p_next, p_prev);
	reg->parent=parent;
}


void region_attach_parent(WRegion *reg, WRegion *parent)
{
	region_set_parent(reg, parent);
	region_raise(reg);
}


/*EXTL_DOC
 * Returns the region that manages \var{reg}.
 */
EXTL_EXPORT_MEMBER
WRegion *region_manager(WRegion *reg)
{
	return reg->manager;
}


/*EXTL_DOC
 * Returns the parent region of \var{reg}.
 */
EXTL_EXPORT_MEMBER
WRegion *region_parent(WRegion *reg)
{
	return reg->parent;
}


WRegion *region_manager_or_parent(WRegion *reg)
{
	if(reg->manager!=NULL)
		return reg->manager;
	else
		return reg->parent;
}


/*EXTL_DOC
 * Returns the geometry of \var{reg} within its parent; a table with fields
 * \var{x}, \var{y}, \var{w} and \var{h}.
 */
EXTL_EXPORT_MEMBER
ExtlTab region_geom(WRegion *reg)
{
	return geom_to_extltab(&REGION_GEOM(reg));
}


bool region_may_destroy(WRegion *reg)
{
	WRegion *mgr=REGION_MANAGER(reg);
	if(mgr==NULL)
		return TRUE;
	else
		return region_may_destroy_managed(mgr, reg);
}


WRegion *region_get_manager_chk(WRegion *p, const WObjDescr *descr)
{
	WRegion *mgr=NULL;
	
	if(p!=NULL){
		mgr=REGION_MANAGER(p);
		if(wobj_is((WObj*)mgr, descr))
			return mgr;
	}
	
	return NULL;
}


/*}}}*/


/*{{{ Scan */


static WRegion *get_next_child(WRegion *first, const WObjDescr *descr)
{
	while(first!=NULL){
		if(wobj_is((WObj*)first, descr))
			break;
		first=first->p_next;
	}
	
	return first;
}


static WRegion *get_prev_child(WRegion *first, const WObjDescr *descr)
{
	if(first==NULL)
		return NULL;
	
	while(1){
		first=first->p_prev;
		if(first->p_next==NULL)
			return NULL;
		if(wobj_is((WObj*)first, descr))
			break;
	}
	
	return first;
}


WRegion *next_child(WRegion *first, const WObjDescr *descr)
{
	if(first==NULL)
		return NULL;
	
	return get_next_child(first->p_next, descr);
}


WRegion *next_child_fb(WRegion *first, const WObjDescr *descr, WRegion *fb)
{
	WRegion *r=NULL;
	if(first!=NULL)
		r=next_child(first, descr);
	if(r==NULL)
		r=fb;
	return r;
}


WRegion *prev_child(WRegion *first, const WObjDescr *descr)
{
	if(first==NULL)
		return NULL;
	
	return get_prev_child(first, descr);
}


WRegion *prev_child_fb(WRegion *first, const WObjDescr *descr, WRegion *fb)
{
	WRegion *r=NULL;
	if(first!=NULL)
		r=prev_child(first, descr);
	if(r==NULL)
		r=fb;
	return r;
}


WRegion *first_child(WRegion *parent, const WObjDescr *descr)
{
	if(parent==NULL)
		return NULL;
	
	return get_next_child(parent->children, descr);
}


WRegion *last_child(WRegion *parent, const WObjDescr *descr)
{
	WRegion *p;
	
	if(parent==NULL)
		return NULL;
	
	p=parent->children;
	
	if(p==NULL)
		return NULL;
	
	p=p->p_prev;
	
	if(wobj_is((WObj*)p, descr))
		return p;
	
	return get_prev_child(p, descr);
}


WRegion *region_get_parent_chk(WRegion *p, const WObjDescr *descr)
{
	if(p==NULL || p->parent==NULL)
		return NULL;
	if(wobj_is((WObj*)p->parent, descr))
		return p->parent;
	return NULL;
}


WRegion *nth_child(WRegion *parent, int n, const WObjDescr *descr)
{
	WRegion *p;
	
	if(n<0)
		return NULL;
	
	p=first_child(parent, descr);
	   
	while(n-- && p!=NULL)
		p=next_child(p, descr);

	return p;
}


bool region_is_ancestor(WRegion *reg, WRegion *reg2)
{
	while(reg!=NULL){
		if(reg==reg2)
			return TRUE;
		reg=reg->parent;
	}
	
	return FALSE;
}


bool region_is_child(WRegion *reg, WRegion *reg2)
{
	return reg2->parent==reg;
}


/*EXTL_DOC
 * Returns most recently active region that has \var{reg} as its
 * parent.
 */
EXTL_EXPORT_MEMBER
WRegion *region_active_sub(WRegion *reg)
{
	return reg->active_sub;
}

/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab region_dynfuntab[]={
	{region_notify_rootpos, default_notify_rootpos},
	{region_request_managed_geom, region_request_managed_geom_allow},
	{region_draw_config_updated, region_default_draw_config_updated},
	{(DynFun*)region_find_rescue_manager_for, 
	 (DynFun*)default_find_rescue_manager_for},
	END_DYNFUNTAB
};


IMPLOBJ(WRegion, WObj, region_deinit, region_dynfuntab);

	
/*}}}*/

