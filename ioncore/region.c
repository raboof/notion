/*
 * wmcore/region.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <limits.h>

#include "common.h"
#include "global.h"
#include "thingp.h"
#include "region.h"
#include "colormap.h"
#include "font.h"
#include "focus.h"
#include "attach.h"
#include "close.h"
#include "regbind.h"


#if 0
#define D(X) X
#else
#define D(X)
#endif

#define FOR_ALL_SUBREGIONS(REG, SUB) \
	FOR_ALL_TYPED(REG, SUB, WRegion)

#define FOR_ALL_SUBREGIONS_REVERSE(REG, SUB) \
	for((SUB)=LAST_THING(REG, WRegion);      \
		(SUB)!=NULL;                         \
		(SUB)=PREV_THING(SUB, WRegion))


static void region_unuse_name(WRegion *reg);


static void default_rect_params(const WRegion *reg, WRectangle geom,
								WWinGeomParams *ret);
static bool default_reparent(WRegion *reg, WWinGeomParams params);
static void default_notify_rootpos(WRegion *reg, int x, int y);
static Window default_restack(WRegion *reg, Window other, int mode);
static void default_request_sub_geom(WRegion *reg, WRegion *sub,
									WRectangle geom, WRectangle *geomret,
									bool tryonly);
static WRegion *default_selected_sub(WRegion *reg);


static DynFunTab region_dynfuntab[]={
	{region_rect_params, default_rect_params},
	{(DynFun*)reparent_region, (DynFun*)default_reparent},
	{region_notify_rootpos, default_notify_rootpos},
	{(DynFun*)region_restack, (DynFun*)default_restack},
	{region_request_sub_geom, default_request_sub_geom},
	{(DynFun*)region_selected_sub, (DynFun*)default_selected_sub},
	END_DYNFUNTAB
};


IMPLOBJ(WRegion, WThing, deinit_region, region_dynfuntab)

	
/* List of named regions */
static WRegion *region_list=NULL;


/*{{{ Init & deinit */


void init_region(WRegion *reg, void *scr, WRectangle geom)
{
	reg->screen=scr;
	reg->geom=geom;
	reg->flags=0;
	reg->uldata=NULL;
	reg->bindings=NULL;
	
	reg->active_sub=NULL;
	
	reg->ni.name=NULL;
	reg->ni.instance=0;
	reg->ni.n_next=reg;
	reg->ni.n_prev=reg;
	reg->ni.g_next=NULL;
	reg->ni.g_prev=NULL;
}


void deinit_region(WRegion *reg)
{
	detach_region(reg);
	region_unuse_name(reg);
	region_remove_bindings(reg);
}


/*}}}*/


/*{{{ Dynfuns */


void fit_region(WRegion *reg, WRectangle geom)
{
	CALL_DYN(fit_region, reg, (reg, geom));
}


void map_region(WRegion *reg)
{
	CALL_DYN(map_region, reg, (reg));
}


void unmap_region(WRegion *reg)
{
	CALL_DYN(unmap_region, reg, (reg));
}


bool switch_subregion(WRegion *reg, WRegion *sub)
{
	bool ret=TRUE;
	CALL_DYN_RET(ret, bool, switch_subregion, reg, (reg, sub));
	return ret;
}


void region_rect_params(const WRegion *reg, WRectangle geom,
						WWinGeomParams *ret)
{
	CALL_DYN(region_rect_params, reg, (reg, geom, ret));
}


bool reparent_region(WRegion *reg, WWinGeomParams params)
{
	bool ret=FALSE;
	CALL_DYN_RET(ret, bool, reparent_region, reg, (reg, params));
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


void region_notify_subname(WRegion *reg, WRegion *sub)
{
	CALL_DYN(region_notify_subname, reg, (reg, sub));
}


void region_activated(WRegion *reg)
{
	CALL_DYN(region_activated, reg, (reg));
}


void region_inactivated(WRegion *reg)
{
	CALL_DYN(region_inactivated, reg, (reg));
}


void region_sub_activated(WRegion *reg, WRegion *sub)
{
	CALL_DYN(region_sub_activated, reg, (reg, sub));
}


void focus_region(WRegion *reg, bool warp)
{
	CALL_DYN(focus_region, reg, (reg, warp));
}

void region_request_sub_geom(WRegion *reg, WRegion *sub,
							 WRectangle geom, WRectangle *geomret,
							 bool tryonly)
{
	CALL_DYN(region_request_sub_geom, reg, (reg, sub, geom, geomret, tryonly));
}


void region_remove_sub(WRegion *reg, WRegion *sub)
{
	CALL_DYN(region_remove_sub, reg, (reg, sub));
}


WRegion *region_selected_sub(WRegion *reg)
{
	WRegion *ret=NULL;
	CALL_DYN_RET(ret, WRegion*, region_selected_sub, reg, (reg));
	return ret;
}


/*}}}*/


/*{{{ Dynfun defaults */


static void default_rect_params(const WRegion *reg, WRectangle geom,
								WWinGeomParams *ret)
{
	WRegion *par;
	
	par=FIND_PARENT1(reg, WRegion);
	
	assert(par!=NULL);

	region_rect_params(par, REGION_GEOM(reg), ret);
	
	ret->win_x+=geom.x;
	ret->win_y+=geom.y;
	ret->geom=geom;
}

		
/* Dummy subregion reparent for non-window regions */
static bool default_reparent(WRegion *reg, WWinGeomParams params)
{
	WRegion *sub;
	WWinGeomParams params2;
	bool rs;
	
	FOR_ALL_SUBREGIONS(reg, sub){
		params2=params;
		params2.win_x+=REGION_GEOM(sub).x;
		params2.win_y+=REGION_GEOM(sub).y;
		params2.geom=REGION_GEOM(sub);
		rs=reparent_region(sub, params);
		assert(rs==TRUE);
	}
	
	REGION_GEOM(reg)=params.geom;
	
	return TRUE;
}


static void default_notify_rootpos(WRegion *reg, int x, int y)
{
	notify_subregions_rootpos(reg, x, y);
}


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


static void default_request_sub_geom(WRegion *reg, WRegion *sub,
									 WRectangle geom, WRectangle *geomret,
									 bool tryonly)
{
	if(geomret!=NULL)
		*geomret=geom;
	
	if(!tryonly)
		fit_region(sub, geom);
}


void region_request_sub_geom_unallow(WRegion *reg, WRegion *sub,
									 WRectangle geom, WRectangle *geomret,
									 bool tryonly)
{
	if(geomret!=NULL)
		*geomret=REGION_GEOM(sub);
}

void region_request_sub_geom_constrain(WRegion *reg, WRegion *sub,
									   WRectangle geom, WRectangle *geomret,
									   bool tryonly)
{
	if(geom.w>REGION_GEOM(reg).w){
		geom.x=0;
		geom.w=REGION_GEOM(reg).w;
	}else if(geom.x<0){
		geom.x=0;
	}else if(geom.x+geom.w>REGION_GEOM(reg).w){
		geom.x=REGION_GEOM(reg).w-geom.w;
	}

	if(geom.h>REGION_GEOM(reg).h){
		geom.y=0;
		geom.h=REGION_GEOM(reg).h;
	}else if(geom.y<0){
		geom.h=0;
	}else if(geom.y+geom.h>REGION_GEOM(reg).h){
		geom.y=REGION_GEOM(reg).h-geom.h;
	}
	
	if(geomret!=NULL)
		*geomret=geom;
	
	if(!tryonly)
		fit_region(sub, geom);
}


static WRegion *default_selected_sub(WRegion *reg)
{
	return reg->active_sub;
}


/*}}}*/


/*{{{ Detach */


static void region_check_active_sub(WRegion *reg, WRegion *sub)
{
	if(reg->active_sub!=sub)
		return;
	
	reg->active_sub=NULL;
	
	if(REGION_IS_ACTIVE(reg) && wglobal.focus_next==NULL)
		set_focus(reg);
}


void detach_region(WRegion *reg)
{
	WThing *p=reg->thing.t_parent;

	/* Links are broken right below so modifying ggrab_top must be done
	 * before releasing the grabs at the end of this function.
	 */
	if(wglobal.ggrab_top==reg)
		wglobal.ggrab_top=FIND_PARENT1(reg, WRegion);
	
	if(p!=NULL){
		if(WTHING_IS(p, WRegion))
			region_check_active_sub((WRegion*)p, reg);
		
		region_remove_sub((WRegion*)p, reg);
	}

	unlink_thing((WThing*)reg);
		
	/* remove_sub might need the active flag to restore focus where
	 * appropriate so we do not zero the flags in check_active_sub.
	 * However, because the parent might have been destroyed at this point
	 * and we thus must reset active_sub earlier as we do not know that,
	 * it is expected not to need that information. Ugly? Yes.
	 */

	while(reg!=NULL && REGION_IS_ACTIVE(reg)){
		D(fprintf(stderr, "detach-deact %s [%p]!\n", WOBJ_TYPESTR(reg), reg);)
		release_ggrabs(reg);
		reg->flags&=~REGION_ACTIVE;
		reg=reg->active_sub;
	}
}


bool detach_reparent_region(WRegion *reg, WWinGeomParams params)
{
	if(!reparent_region(reg, params))
		return FALSE;
	
	detach_region(reg);
	return TRUE;
}


/*}}}*/


/*{{{ Focus */


void region_got_focus(WRegion *reg, WRegion *sub)
{
	WRegion *r;
	
	if(REGION_IS_ACTIVE(reg)){
		D(fprintf(stderr, "got focus (act) %s [%p:%p]\n", WOBJ_TYPESTR(reg), reg, sub);)
		r=reg->active_sub;
		reg->active_sub=sub;
		if(r!=sub && r!=NULL)
			region_lost_focus(r);
	}else{
		D(fprintf(stderr, "got focus (inact) %s [%p:%p]\n", WOBJ_TYPESTR(reg), reg, sub);)
		reg->flags|=REGION_ACTIVE;
		reg->active_sub=sub;
		
		activate_ggrabs(reg);
		
		r=FIND_PARENT1(reg, WRegion);
		if(r!=NULL){
			D(fprintf(stderr, "p--> ");)
			region_sub_activated(r, reg);
			region_got_focus(r, reg);
		}
		region_activated(reg);
		
		wglobal.ggrab_top=reg;
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
	release_ggrabs(reg);
	
	if(wglobal.ggrab_top==reg)
		wglobal.ggrab_top=FIND_PARENT1(reg, WRegion);

	if(!REGION_IS_ACTIVE(reg)){
		D(fprintf(stderr, "lost focus (inact) %s [%p:]\n", WOBJ_TYPESTR(reg), reg);)
		return;
	}
	
	D(fprintf(stderr, "lost focus (act) %s [%p:]\n", WOBJ_TYPESTR(reg), reg);)
	
	reg->flags&=~REGION_ACTIVE;
	
	region_inactivated(reg);
	
	if(reg->active_sub!=NULL){
		D(fprintf(stderr, "l--> ");)
		region_lost_focus(reg->active_sub);
	}
}


/*}}}*/
	
	
/*{{{ Goto */


bool display_region(WRegion *reg)
{
	WRegion *preg;
	
	preg=FIND_PARENT1(reg, WRegion);
	
	if(preg==NULL)
		return TRUE;
	
	if(!display_region(preg))
		return FALSE;
	
	if(REGION_IS_MAPPED(reg))
		return TRUE;
	
	return switch_subregion(preg, reg);
}


bool switch_region(WRegion *reg)
{
	set_previous_of(reg);
	protect_previous();

	if(!display_region(reg))
		return FALSE;

	unprotect_previous();

	return TRUE;
}


bool goto_region(WRegion *reg)
{
	if(switch_region(reg)){
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


void map_all_subregions(WRegion *reg)
{
	WRegion *sub;
	
	FOR_ALL_SUBREGIONS(reg, sub){
		map_region(sub);
	}
}


void unmap_all_subregions(WRegion *reg)
{
	WRegion *sub;
	
	FOR_ALL_SUBREGIONS(reg, sub){
		unmap_region(sub);
	}
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


void region_params2(const WRegion *reg, WRectangle geom, WWinGeomParams *ret)
{
	WRegion *par=FIND_PARENT1(reg, WRegion);
	
	if(par==NULL){
		/* WScreen */
		ret->geom=geom;
		ret->win_x=geom.x;
		ret->win_y=geom.y;
		ret->win=None;
		return;
	}
	
	region_rect_params(par, geom, ret);
}


void region_params(const WRegion *reg, WWinGeomParams *ret)
{
	region_params2(reg, REGION_GEOM(reg), ret);
}


/*}}}*/


/*{{{ Names */

#define NAMEDNUM_TMPL "<%d>"


uint region_name_instance(WRegion *reg)
{
	return reg->ni.instance;
}


const char *region_name(WRegion *reg)
{
	return reg->ni.name;
}


char *region_full_name(WRegion *reg)
{
	const char *str=region_name(reg);
	char tmp[16];
	
	if(str==NULL)
		return NULL;
	
	if(reg->ni.instance!=0){
		sprintf(tmp, NAMEDNUM_TMPL, reg->ni.instance);
		return scat(str, tmp);
	}else{
		return scopy(str);
	}
}


char *region_make_label(WRegion *reg, int maxw, XFontStruct *font)
{
	const char *str=region_name(reg);
	char tmp[16];
	WGRData *grdata;
	
	if(str==NULL)
		return NULL;
	
	if(reg->ni.instance!=0)
		sprintf(tmp, NAMEDNUM_TMPL, reg->ni.instance);
	else
		*tmp='\0';
	
	return make_label(font, str, tmp, maxw, NULL);
}


static bool region_use_name(WRegion *reg, const char *namep)
{
	WRegion *p, *np, *minp=NULL;
	int mininst=INT_MAX;
	const char *str;
	char *name;
	
	name=scopy(namep);
	
	if(name==NULL){
		warn_err();
		return FALSE;
	}
	
	region_unuse_name(reg);
	reg->ni.name=name;
	
	for(p=region_list; p!=NULL; p=p->ni.g_next){
		if(p==reg)
			continue;
		str=region_name(p);
		if(strcmp(str, name)==0)
			break;
	}
	
	if(p!=NULL){
		for(; ; p=np){
			assert(p!=reg);
			np=p->ni.n_next;
			if(p->ni.instance+1==np->ni.instance){
				continue;
			}else if(p->ni.instance>=np->ni.instance && np->ni.instance!=0){
				mininst=0;
				minp=p;
			}else if(p->ni.instance+1<mininst){
				mininst=p->ni.instance+1;
				minp=p;
				continue;
			}
			break;
		}
	
		assert(minp!=NULL);
	
		np=minp->ni.n_next;
		reg->ni.n_next=np;
		np->ni.n_prev=reg;
		minp->ni.n_next=reg;
		reg->ni.n_prev=minp;
		reg->ni.instance=mininst;
	}

	LINK_ITEM(region_list, reg, ni.g_next, ni.g_prev);
	
	return TRUE;
}


static void region_unuse_name(WRegion *reg)
{
	reg->ni.n_next->ni.n_prev=reg->ni.n_prev;
	reg->ni.n_prev->ni.n_next=reg->ni.n_next;
	reg->ni.n_next=reg;
	reg->ni.n_prev=reg;
	reg->ni.instance=0;
	if(reg->ni.name!=NULL){
		free(reg->ni.name);
		reg->ni.name=NULL;
	}
	
	if(reg->ni.g_prev!=NULL){
		UNLINK_ITEM(region_list, reg, ni.g_next, ni.g_prev);
	}
}


bool set_region_name(WRegion *reg, const char *p)
{
	WRegion *par;
	
	if(p==NULL || *p=='\0'){
		region_unuse_name(reg);
	}else{
		if(!region_use_name(reg, p))
			return FALSE;
	}
	
	par=FIND_PARENT1(reg, WRegion);
	
	if(par!=NULL)
		region_notify_subname(par, reg);
	
	return TRUE;
}


/*}}}*/


/*{{{ Lookup */


WRegion *do_lookup_region(const char *cname, WObjDescr *descr)
{
	WRegion *reg;
	char *name;

	for(reg=region_list; reg!=NULL; reg=reg->ni.g_next){
		if(!wobj_is((WObj*)reg, descr))
			continue;
		
		name=(char*)region_full_name(reg);
		
		if(name==NULL)
			continue;
		
		if(strcmp(name, cname)){
			free(name);
			continue;
		}
		
		free(name);
		
		return reg;
	}
	
	return NULL;
}


int do_complete_region(char *nam, char ***cp_ret, char **beg, WObjDescr *descr)
{
	WRegion *reg;
	char *name;
	char **cp;
	int n=0, l=strlen(nam);
	int lnum=0;
	
	*cp_ret=NULL;
	
again:
	
	for(reg=region_list; reg!=NULL; reg=reg->ni.g_next){
		
		if(!wobj_is((WObj*)reg, descr))
			continue;

		name=(char*)region_full_name(reg);
		
		if(name==NULL)
			continue;
		
		if((lnum==0 && l && strncmp(name, nam, l)) ||
		   (strstr(name, nam)==NULL)){
			   free(name);
			   continue;
		}
		
		cp=REALLOC_N(*cp_ret, char*, n, n+1);
		
		if(cp==NULL){
			warn_err();
			free(name);
		}else{
			cp[n]=name;
			n++;
			*cp_ret=cp;
		}
	}
	
	if(n==0 && lnum==0 && l>1){
		lnum=1;
		goto again;
	}
	
	return n;
}


WRegion *lookup_region(const char *name)
{
	return do_lookup_region(name, &OBJDESCR(WObj));
}


int complete_region(char *nam, char ***cp_ret, char **beg)
{
	return do_complete_region(nam, cp_ret, beg, &OBJDESCR(WObj));
}


/*}}}*/


