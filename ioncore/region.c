/*
 * wmcore/region.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "global.h"
#include "thingp.h"
#include "region.h"
#include "colormap.h"
#include "focus.h"
#include "attach.h"
#include "regbind.h"
#include "tags.h"
#include "names.h"


#define FOR_ALL_SUBREGIONS(REG, SUB) \
	FOR_ALL_TYPED(REG, SUB, WRegion)

#define FOR_ALL_SUBREGIONS_REVERSE(REG, SUB) \
	for((SUB)=LAST_THING(REG, WRegion);      \
		(SUB)!=NULL;                         \
		(SUB)=PREV_THING(SUB, WRegion))


static void default_notify_rootpos(WRegion *reg, int x, int y);
/*static Window default_restack(WRegion *reg, Window other, int mode);*/
static void default_request_managed_geom(WRegion *mgr, WRegion *reg,
										 WRectangle geom, WRectangle *geomret,
										 bool tryonly);
static WRegion *default_selected_sub(WRegion *reg);
void default_draw_config_updated(WRegion *reg);


static DynFunTab region_dynfuntab[]={
	{region_notify_rootpos, default_notify_rootpos},
	/*{(DynFun*)region_restack, (DynFun*)default_restack},*/
	{region_request_managed_geom, region_request_managed_geom_allow},
	{(DynFun*)region_selected_sub, (DynFun*)default_selected_sub},
	{region_draw_config_updated, default_draw_config_updated},
	END_DYNFUNTAB
};


IMPLOBJ(WRegion, WThing, deinit_region, region_dynfuntab, NULL)

	
/*{{{ Init & deinit */


void init_region(WRegion *reg, WRegion *parent, WRectangle geom)
{
	reg->geom=geom;
	reg->flags=0;
	reg->bindings=NULL;
	if(parent!=NULL){
		reg->screen=parent->screen;
		link_thing((WThing*)parent, (WThing*)reg);
	}else{
		assert(WTHING_IS(reg, WScreen));
		reg->screen=reg;
	}
	
	reg->active_sub=NULL;
	
	reg->ni.name=NULL;
	reg->ni.instance=0;
	reg->ni.n_next=reg;
	reg->ni.n_prev=reg;
	reg->ni.g_next=NULL;
	reg->ni.g_prev=NULL;
	
	reg->funclist=NULL;
	
	reg->manager=NULL;
	reg->mgr_next=NULL;
	reg->mgr_prev=NULL;
	reg->mgr_data=NULL;
}


void deinit_region(WRegion *reg)
{
	region_detach(reg);
	region_unuse_name(reg);
	untag_region(reg);
	region_remove_bindings(reg);
}


/*}}}*/


/*{{{ Dynfuns */


void fit_region(WRegion *reg, WRectangle geom)
{
	CALL_DYN(fit_region, reg, (reg, geom));
}


void region_draw_config_updated(WRegion *reg)
{
	CALL_DYN(region_draw_config_updated, reg, (reg));
}


void map_region(WRegion *reg)
{
	CALL_DYN(map_region, reg, (reg));
}


void unmap_region(WRegion *reg)
{
	CALL_DYN(unmap_region, reg, (reg));
}


bool reparent_region(WRegion *reg, WRegion *par, WRectangle geom)
{
	bool ret=FALSE;
	CALL_DYN_RET(ret, bool, reparent_region, reg, (reg, par, geom));
	return ret;
}


void region_notify_rootpos(WRegion *reg, int x, int y)
{
	CALL_DYN(region_notify_rootpos, reg, (reg, x, y));
}


Window region_restack(WRegion *reg, Window other, int mode)
{
	Window ret=None;
	CALL_DYN_RET(ret, Window, region_restack, reg, (reg, other, mode));
	return ret;
}


Window region_lowest_win(WRegion *reg)
{
	Window ret=None;
	CALL_DYN_RET(ret, Window, region_lowest_win, reg, (reg));
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


void focus_region(WRegion *reg, bool warp)
{
	CALL_DYN(focus_region, reg, (reg, warp));
}


WRegion *region_selected_sub(WRegion *reg)
{
	WRegion *ret=NULL;
	CALL_DYN_RET(ret, WRegion*, region_selected_sub, reg, (reg));
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


void region_request_managed_geom(WRegion *mgr, WRegion *reg,
								 WRectangle geom, WRectangle *geomret,
								 bool tryonly)
{
	CALL_DYN(region_request_managed_geom, mgr,
			 (mgr, reg, geom, geomret, tryonly));
}


void region_remove_managed(WRegion *mgr, WRegion *reg)
{
	CALL_DYN(region_remove_managed, mgr, (mgr, reg));
}

/*}}}*/


/*}}}*/


/*{{{ Dynfun defaults */


static void default_notify_rootpos(WRegion *reg, int x, int y)
{
	notify_subregions_rootpos(reg, x, y);
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


static WRegion *default_selected_sub(WRegion *reg)
{
	return reg->active_sub;
}


/*{{{ Manager region default dynfuns */


void region_request_managed_geom_allow(WRegion *mgr, WRegion *reg,
									   WRectangle geom, WRectangle *geomret,
									   bool tryonly)
{
	if(geomret!=NULL)
		*geomret=geom;
	
	if(!tryonly)
		fit_region(reg, geom);
}


void region_request_managed_geom_unallow(WRegion *mgr, WRegion *reg,
										 WRectangle geom, WRectangle *geomret,
										 bool tryonly)
{
	if(geomret!=NULL)
		*geomret=REGION_GEOM(reg);
}


void region_request_managed_geom_constrain(WRegion *mgr, WRegion *reg,
										   WRectangle geom, WRectangle *geomret,
										   bool tryonly)
{
	WRectangle g;
	int diff;
	WRegion *par;
	
	par=FIND_PARENT1(reg, WRegion);
	
	if(par==mgr){
		/* mgr is also the parent of reg */
		g.x=0;
		g.y=0;
		g.w=REGION_GEOM(mgr).w;
		g.h=REGION_GEOM(mgr).h;
	}else if(FIND_PARENT1(mgr, WRegion)==par){
		/* mgr and reg have the same parent */
		g=REGION_GEOM(mgr);
	}else{
		/* Don't know how to contrain */
		goto doit;
	}
	
	diff=g.x-geom.x;
	if(diff>0){
		geom.x+=diff;
		geom.w-=diff;
	}
	diff=(geom.w+geom.x)-(g.w+g.x);
	if(diff>0)
		geom.w-=diff;
		
	diff=g.y-geom.y;
	if(diff>0){
		geom.y+=diff;
		geom.h-=diff;
	}
	diff=(geom.h+geom.y)-(g.h+g.y);
	if(diff>0)
		geom.h-=diff;
	
doit:
	if(geomret!=NULL)
		*geomret=geom;
	
	if(!tryonly)
		fit_region(reg, geom);
}


/*}}}*/


void default_draw_config_updated(WRegion *reg)
{
	WRegion *sub;
	
	FOR_ALL_TYPED(reg, sub, WRegion){
		region_draw_config_updated(sub);
	}
}


/*}}}*/


/*{{{ Detach */


void region_detach_parent(WRegion *reg)
{
	WThing *p=reg->thing.t_parent;

	if(p==NULL)
		return;
	
	unlink_thing((WThing*)reg);
	
	if(p!=NULL && WTHING_IS(p, WRegion)){
		if(((WRegion*)p)->active_sub==reg){
			((WRegion*)p)->active_sub=NULL;
			
			/*region_remove_sub((WRegion*)p, reg);*/
			
			/* ??? Maybe manager should handle this all? */
			if(REGION_IS_ACTIVE(reg) && wglobal.focus_next==NULL)
				set_focus(((WRegion*)p));
		}else{
			/*region_remove_sub((WRegion*)p, reg);*/
		}
	}

	/* remove_sub might need the active flag to restore focus where
	 * appropriate so we do not zero the flags in check_active_sub.
	 * However, because the parent might have been destroyed at this point
	 * and we thus must reset active_sub earlier as we do not know that,
	 * it is expected not to need that information. Ugly? Yes.
	 */

	while(reg!=NULL && REGION_IS_ACTIVE(reg)){
		D(fprintf(stderr, "detach-deact %s [%p]!\n", WOBJ_TYPESTR(reg), reg);)
		/*release_ggrabs(reg);*/
		reg->flags&=~REGION_ACTIVE;
		reg=reg->active_sub;
	}
}


void region_detach_manager(WRegion *reg)
{
	WRegion *mgr;
	
	mgr=REGION_MANAGER(reg);
	
	if(mgr==NULL)
		return;
	
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


void region_got_focus(WRegion *reg)
{
	WRegion *r;
	
	if(!REGION_IS_ACTIVE(reg)){
		D(fprintf(stderr, "got focus (inact) %s [%p]\n", WOBJ_TYPESTR(reg), reg);)
		reg->flags|=REGION_ACTIVE;
		
		r=FIND_PARENT1(reg, WRegion);
		if(r!=NULL)
			r->active_sub=reg;
		
		r=REGION_MANAGER(reg);
		if(r!=NULL)
			region_managed_activated(r, reg);

		region_activated(reg);
	}else{
		D(fprintf(stderr, "got focus (act) %s [%p]\n", WOBJ_TYPESTR(reg), reg);)
    }
	
	/* Install default colour map only if there is no active subregion;
	 * their maps should come first. WClientWins will install their maps
	 * in region_activated. Other regions are supposed to use the same
	 * default map.
	 */
	if(reg->active_sub==NULL && !WTHING_IS(reg, WClientWin))
		install_cmap(SCREEN_OF(reg), None); 
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


/*}}}*/

	
/*{{{ Goto */


bool display_region(WRegion *reg)
{
	WRegion *mgr, *preg;

	if(region_is_fully_mapped(reg))
		return TRUE;
	
	mgr=REGION_MANAGER(reg);
	
	if(mgr!=NULL){
		if(!display_region(mgr))
			return FALSE;
		return region_display_managed(mgr, reg);
	}
	
	preg=FIND_PARENT1(reg, WRegion);

	if(preg!=NULL && !display_region(preg))
		return FALSE;

	map_region(reg);
	return TRUE;
}


bool display_region_sp(WRegion *reg)
{
	bool ret;
	
	set_previous_of(reg);
	protect_previous();
	ret=display_region(reg);
	unprotect_previous();

	return ret;
}


bool goto_region(WRegion *reg)
{
	if(display_region_sp(reg)){
		warp(reg);
		return TRUE;
	}
	return FALSE;
}


/*}}}*/


/*{{{ Helpers/misc */


bool region_is_fully_mapped(WRegion *reg)
{
	for(; reg!=NULL; reg=FIND_PARENT1(reg, WRegion)){
		if(!REGION_IS_MAPPED(reg))
			return FALSE;
	}
	
	return TRUE;
}


void region_rootgeom(WRegion *reg, int *xret, int *yret)
{
	*xret=REGION_GEOM(reg).x;
	*yret=REGION_GEOM(reg).x;
	
	while(1){
		reg=FIND_PARENT1(reg, WRegion);
		if(reg==NULL)
			break;
		*xret+=REGION_GEOM(reg).x;
		*yret+=REGION_GEOM(reg).y;
	}
}


void notify_subregions_move(WRegion *reg)
{
	int x, y;
	
	region_rootgeom(reg, &x, &y);
	notify_subregions_rootpos(reg, x, y);
}


void notify_subregions_rootpos(WRegion *reg, int x, int y)
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

	par=FIND_PARENT1(reg, WRegion);
	
	if(par==NULL || WTHING_IS(reg, WScreen)){
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


void region_set_parent(WRegion *reg, WRegion *par)
{
	assert(reg->thing.t_parent==NULL && par!=NULL);
	link_thing((WThing*)par, (WThing*)reg);
}



bool region_manages_active_reg(WRegion *reg)
{
	WRegion *p=FIND_PARENT1(reg, WRegion);
	WRegion *reg2;
	
	if(p==NULL || p->active_sub==NULL)
		return FALSE;
	
	reg2=p->active_sub;
	
	while(1){
		if(REGION_MANAGER(reg2)==reg)
			return TRUE;
		reg2=REGION_MANAGER(reg2);
		if(reg2==NULL)
			break;
		if(FIND_PARENT1(reg2, WRegion)!=p)
			break;
	}
	
	return FALSE;
}


/*}}}*/
