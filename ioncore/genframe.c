/*
 * ion/ioncore/genframe.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include "names.h"
#include "saveload.h"
#include "genframep.h"
#include "genframe-pointer.h"
#include "sizehint.h"
#include "stacking.h"
#include "extlconv.h"
#include "mplex.h"
#include "bindmaps.h"
#include "regbind.h"
#include "gr.h"
#include "genws.h"
#include "activity.h"


#define genframe_draw(F, C) window_draw((WWindow*)F, C)
#define genframe_managed_geom(F, G) mplex_managed_geom((WMPlex*)(F), G)

#define SET_SHADE_FLAG(F) ((F)->flags|=WGENFRAME_SHADED, \
						   (F)->mplex.flags|=WMPLEX_MANAGED_UNVIEWABLE)
#define UNSET_SHADE_FLAG(F) ((F)->flags&=~WGENFRAME_SHADED, \
							 (F)->mplex.flags&=~WMPLEX_MANAGED_UNVIEWABLE)


static bool set_genframe_background(WGenFrame *genframe, bool set_always);
static void genframe_initialise_gr(WGenFrame *genframe);
static bool genframe_initialise_titles(WGenFrame *genframe);
static void genframe_free_titles(WGenFrame *genframe);


/*{{{ Destroy/create genframe */


bool genframe_init(WGenFrame *genframe, WWindow *parent, 
				   const WRectangle *geom)
{
	WRectangle mg;
	
	genframe->flags=0;
	genframe->saved_w=0;
	genframe->saved_h=0;
	genframe->saved_x=0;
	genframe->saved_y=0;
	genframe->tab_dragged_idx=-1;
	genframe->titles=NULL;
	genframe->titles_n=0;
	genframe->bar_h=0;
	genframe->tr_mode=GR_TRANSPARENCY_DEFAULT;
	genframe->brush=NULL;
	genframe->bar_brush=NULL;

	if(!mplex_init_new((WMPlex*)genframe, parent, geom))
		return FALSE;
	
	genframe_initialise_gr(genframe);
	genframe_initialise_titles(genframe);
	
	XSelectInput(wglobal.dpy, WGENFRAME_WIN(genframe), FRAME_MASK);

	region_add_bindmap((WRegion*)genframe, &ioncore_genframe_bindmap);

	genframe_managed_geom(genframe, &mg);
	
	if(mg.h<=1)
		SET_SHADE_FLAG(genframe);
	
	return TRUE;
}


WGenFrame *create_genframe(WWindow *parent, const WRectangle *geom)
{
	CREATEOBJ_IMPL(WGenFrame, genframe, (p, parent, geom));
}


static void release_brushes(WGenFrame *genframe)
{
	if(genframe->bar_brush!=NULL){
		grbrush_release(genframe->bar_brush, WGENFRAME_WIN(genframe));
		genframe->bar_brush=NULL;
	}
	
	if(genframe->brush!=NULL){
		grbrush_release(genframe->brush, WGENFRAME_WIN(genframe));
		genframe->brush=NULL;
	}
}


void genframe_deinit(WGenFrame *genframe)
{
	genframe_free_titles(genframe);
	release_brushes(genframe);
	mplex_deinit((WMPlex*)genframe);
}


/*}}}*/


/*{{{ Tabs */


int genframe_tab_at_x(const WGenFrame *genframe, int x)
{
	WRectangle bg;
	int tab, tx;
	
	genframe_bar_geom(genframe, &bg);
	
	if(x>=bg.x+bg.w || x<bg.x)
		return -1;
	
	tx=bg.x;

	for(tab=0; tab<WGENFRAME_MCOUNT(genframe); tab++){
		tx+=genframe_nth_tab_w(genframe, tab);
		if(x<tx)
			break;
	}
	
	return tab;
}


int genframe_nth_tab_x(const WGenFrame *genframe, int n)
{
	uint x=0;
	int i;
	
	for(i=0; i<n; i++)
		x+=genframe_nth_tab_w(genframe, i);
	
	return x;
}


static int genframe_nth_tab_w_iw(const WGenFrame *genframe, int n, bool inner)
{
	WRectangle bg;
	GrBorderWidths bdw=GR_BORDER_WIDTHS_INIT;
	int m=WGENFRAME_MCOUNT(genframe);
	uint w;
	
	genframe_bar_geom(genframe, &bg);

	if(m==0)
		m=1;

	if(genframe->bar_brush!=NULL)
		grbrush_get_border_widths(genframe->bar_brush, &bdw);
	
	/* Remove borders */
	w=bg.w-bdw.left-bdw.right-(bdw.tb_ileft+bdw.tb_iright+bdw.spacing)*(m-1);
	
	if(w<=0)
		return 0;
	
	/* Get n:th tab's portion of free area */
	w=(((n+1)*w)/m-(n*w)/m);
	
	/* Add n:th tab's borders back */
	if(!inner){
		w+=(n==0 ? bdw.left : bdw.tb_ileft);
		w+=(n==m-1 ? bdw.right : bdw.tb_iright+bdw.spacing);
	}
			
	return w;
}


int genframe_nth_tab_w(const WGenFrame *genframe, int n)
{
	return genframe_nth_tab_w_iw(genframe, n, FALSE);
}


int genframe_nth_tab_iw(const WGenFrame *genframe, int n)
{
	return genframe_nth_tab_w_iw(genframe, n, TRUE);
}



static void update_attr(WGenFrame *genframe, int i, WRegion *reg)
{
	int flags=0;
	static const char *attrs[]={
		"unselected-not_tagged-not_dragged-no_activity",
		"selected-not_tagged-not_dragged-no_activity",
		"unselected-tagged-not_dragged-no_activity",
		"selected-tagged-not_dragged-no_activity",
		"unselected-not_tagged-dragged-no_activity",
		"selected-not_tagged-dragged-no_activity",
		"unselected-tagged-dragged-no_activity",
		"selected-tagged-dragged-no_activity",
		"unselected-not_tagged-not_dragged-activity",
		"selected-not_tagged-not_dragged-activity",
		"unselected-tagged-not_dragged-activity",
		"selected-tagged-not_dragged-activity",
		"unselected-not_tagged-dragged-activity",
		"selected-not_tagged-dragged-activity",
		"unselected-tagged-dragged-activity",
		"selected-tagged-dragged-activity"
	};

	if(i>=genframe->titles_n){
		/* Might happen when deinitialising */
		return;
	}
	
	if(reg==WGENFRAME_CURRENT(genframe))
		flags|=0x01;
	if(reg!=NULL && reg->flags&REGION_TAGGED)
		flags|=0x02;
	if(i==genframe->tab_dragged_idx)
		flags|=0x04;
	if(reg!=NULL && region_activity(reg))
		flags|=0x08;
	
	genframe->titles[i].attr=attrs[flags];
}


void genframe_update_attr_nth(WGenFrame *genframe, int i)
{
	WRegion *reg;
	
	if(i<0 || i>=genframe->titles_n)
		return;

	update_attr(genframe, i, mplex_nth_managed((WMPlex*)genframe, i));
}


static void update_attrs(WGenFrame *genframe)
{
	int i=0;
	WRegion *sub;
	
	FOR_ALL_MANAGED_ON_LIST(WGENFRAME_MLIST(genframe), sub){
		update_attr(genframe, i, sub);
		i++;
	}
}


static void genframe_free_titles(WGenFrame *genframe)
{
	int i;
	
	if(genframe->titles!=NULL){
		for(i=0; i<genframe->titles_n; i++){
			if(genframe->titles[i].text)
				free(genframe->titles[i].text);
		}
		free(genframe->titles);
		genframe->titles=NULL;
	}
	genframe->titles_n=0;
}


static void do_init_title(WGenFrame *genframe, int i, WRegion *sub)
{
	genframe->titles[i].text=NULL;
	genframe->titles[i].iw=genframe_nth_tab_iw(genframe, i);
	update_attr(genframe, i, sub);
}

	
static bool genframe_initialise_titles(WGenFrame *genframe)
{
	int i, n=WGENFRAME_MCOUNT(genframe);
	WRegion *sub;
	
	genframe_free_titles(genframe);

	if(n==0)
		n=1;
	
	genframe->titles=ALLOC_N(GrTextElem, n);
	if(genframe->titles==NULL)
		return FALSE;
	genframe->titles_n=n;
	
	if(WGENFRAME_MCOUNT(genframe)==0){
		do_init_title(genframe, 0, NULL);
	}else{
		i=0;
		FOR_ALL_MANAGED_ON_LIST(WGENFRAME_MLIST(genframe), sub){
			do_init_title(genframe, i, sub);
			i++;
		}
	}
	
	genframe_recalc_bar(genframe);

	return TRUE;
}


/*}}}*/


/*{{{ Resize and reparent */


static void reparent_or_fit(WGenFrame *genframe, const WRectangle *geom,
							WWindow *parent)
{
	WRectangle old_geom, mg;
	bool wchg=(REGION_GEOM(genframe).w!=geom->w);
	bool hchg=(REGION_GEOM(genframe).h!=geom->h);
	bool move=(REGION_GEOM(genframe).x!=geom->x ||
			   REGION_GEOM(genframe).y!=geom->y);
	
	if(parent!=NULL){
		region_detach_parent((WRegion*)genframe);
		XReparentWindow(wglobal.dpy, WGENFRAME_WIN(genframe), parent->win,
						geom->x, geom->y);
		XResizeWindow(wglobal.dpy, WGENFRAME_WIN(genframe), geom->w, geom->h);
		region_attach_parent((WRegion*)genframe, (WRegion*)parent);
	}else{
		XMoveResizeWindow(wglobal.dpy, WGENFRAME_WIN(genframe),
						  geom->x, geom->y, geom->w, geom->h);
	}
	
	old_geom=REGION_GEOM(genframe);
	REGION_GEOM(genframe)=*geom;

	if(hchg){
		genframe->flags|=WGENFRAME_SAVED_VERT;
		genframe->saved_y=old_geom.y;
		genframe->saved_h=old_geom.h;
	}

	if(wchg){
		genframe->flags|=WGENFRAME_SAVED_HORIZ;
		genframe->saved_x=old_geom.x;
		genframe->saved_w=old_geom.w;
	}

	genframe_managed_geom(genframe, &mg);
	if(hchg && mg.h<=1){
		if(!(genframe->flags&(WGENFRAME_SHADED|WGENFRAME_TAB_HIDE))){
			SET_SHADE_FLAG(genframe);
			if(WGENFRAME_CURRENT(genframe)!=NULL)
				region_unmap(WGENFRAME_CURRENT(genframe));
		}
	}else if(hchg){
		if(genframe->flags&WGENFRAME_SHADED && REGION_IS_MAPPED(genframe)){
			if(WGENFRAME_CURRENT(genframe)!=NULL)
				region_map(WGENFRAME_CURRENT(genframe));
		}
		UNSET_SHADE_FLAG(genframe);
	}

	if(move && !wchg && !hchg)
		region_notify_subregions_move((WRegion*)genframe);
	else if(wchg || hchg)
		mplex_fit_managed((WMPlex*)genframe);

	if(wchg || hchg)
		mplex_size_changed((WMPlex*)genframe, wchg, hchg);
}


bool genframe_reparent(WGenFrame *genframe, WWindow *parent, 
					   const WRectangle *geom)
{
	if(!same_rootwin((WRegion*)genframe, (WRegion*)parent))
		return FALSE;
	
	reparent_or_fit(genframe, geom, parent);
	return TRUE;
}


void genframe_fit(WGenFrame *genframe, const WRectangle *geom)
{
	reparent_or_fit(genframe, geom, NULL);
}


void genframe_resize_hints(WGenFrame *genframe, XSizeHints *hints_ret,
						   uint *relw_ret, uint *relh_ret)
{
	WRectangle subgeom;
	uint wdummy, hdummy;
	
	/*if(WGENFRAME_CURRENT(genframe)==NULL){*/
		genframe_managed_geom(genframe, &subgeom);
		if(relw_ret!=NULL)
			*relw_ret=subgeom.w;
		if(relh_ret!=NULL)
			*relh_ret=subgeom.h;
	/*}else{
		if(relw_ret!=NULL)
			*relw_ret=REGION_GEOM(WGENFRAME_CURRENT(genframe)).w;
		if(relh_ret!=NULL)
			*relh_ret=REGION_GEOM(WGENFRAME_CURRENT(genframe)).h;
	}*/
	
	if(WGENFRAME_CURRENT(genframe)!=NULL){
		region_resize_hints(WGENFRAME_CURRENT(genframe), hints_ret,
							&wdummy, &hdummy);
	}else{
		hints_ret->flags=0;
	}
	
	adjust_size_hints_for_managed(hints_ret, WGENFRAME_MLIST(genframe));
}


/*}}}*/


/*{{{ Focus  */


void genframe_inactivated(WGenFrame *genframe)
{
	genframe_draw(genframe, FALSE);
	extl_call_named("call_hook", "soo", NULL,
					"genframe_inactivated",
					genframe, WGENFRAME_CURRENT(genframe));
}


void genframe_activated(WGenFrame *genframe)
{
	genframe_draw(genframe, FALSE);
	extl_call_named("call_hook", "soo", NULL,
					"genframe_activated",
					genframe, WGENFRAME_CURRENT(genframe));
}


/*}}}*/


/*{{{ Misc. */


static bool set_genframe_background(WGenFrame *genframe, bool set_always)
{
	GrTransparency mode=GR_TRANSPARENCY_DEFAULT;
	
	if(WGENFRAME_CURRENT(genframe)!=NULL){
		if(WOBJ_IS(WGENFRAME_CURRENT(genframe), WClientWin)){
			WClientWin *cwin=(WClientWin*)WGENFRAME_CURRENT(genframe);
			mode=(cwin->flags&CWIN_PROP_TRANSPARENT
				  ? GR_TRANSPARENCY_YES : GR_TRANSPARENCY_NO);
		}else if(!WOBJ_IS(WGENFRAME_CURRENT(genframe), WGenWS)){
			mode=GR_TRANSPARENCY_NO;
		}
	}
	
	if(mode!=genframe->tr_mode || set_always){
		genframe->tr_mode=mode;
		if(genframe->brush!=NULL){
			grbrush_enable_transparency(genframe->brush, 
										WGENFRAME_WIN(genframe), mode);
			genframe_draw(genframe, TRUE);
		}
		return TRUE;
	}
	
	return FALSE;
}


/*EXTL_DOC
 * Toggle tab visibility.
 */
EXTL_EXPORT_MEMBER
void genframe_toggle_tab(WGenFrame *genframe)
{
	if(genframe->flags&WGENFRAME_SHADED)
		return;
	
	genframe->flags^=WGENFRAME_TAB_HIDE;
	mplex_fit_managed(&(genframe->mplex));
	XClearWindow(wglobal.dpy, WGENFRAME_WIN(genframe));
	genframe_draw(genframe, TRUE);
}


void genframe_notify_managed_change(WGenFrame *genframe, WRegion *sub)
{
	/* TODO: Should only draw/update the affected tab.*/
	update_attrs(genframe);
	genframe_recalc_bar(genframe);
	genframe_draw_bar(genframe, FALSE);
}


static void genframe_size_changed_default(WGenFrame *genframe,
										  bool wchg, bool hchg)
{
	if(wchg)
		genframe_recalc_bar(genframe);
	/* We should get a request from X to draw the frame... */
}



static void genframe_managed_changed(WGenFrame *genframe, int mode, bool sw,
									 WRegion *reg)
{
	if(mode!=MPLEX_CHANGE_SWITCHONLY)
		genframe_initialise_titles(genframe);
	else
		update_attrs(genframe);

	if(sw){
		extl_call_named("call_hook", "soo", NULL,
						"genframe_managed_switched",
						genframe, WGENFRAME_CURRENT(genframe));
		if(set_genframe_background(genframe, FALSE))
			return;
	}

	genframe_draw_bar(genframe, mode!=MPLEX_CHANGE_SWITCHONLY);
}


void genframe_load_saved_geom(WGenFrame* genframe, ExtlTab tab)
{
	int p=0, s=0;
	
	if(extl_table_gets_i(tab, "saved_x", &p) &&
	   extl_table_gets_i(tab, "saved_w", &s)){
		genframe->saved_x=p;
		genframe->saved_w=s;
		genframe->flags|=WGENFRAME_SAVED_HORIZ;
	}

	if(extl_table_gets_i(tab, "saved_y", &p) &&
	   extl_table_gets_i(tab, "saved_h", &s)){
		genframe->saved_y=p;
		genframe->saved_h=s;
		genframe->flags|=WGENFRAME_SAVED_VERT;
	}
}


/*}}}*/


/*{{{ Dynfuns */


const char *genframe_style(WGenFrame *genframe)
{
	const char *ret=NULL;
	CALL_DYN_RET(ret, const char *, genframe_style, genframe, (genframe));
	return ret;
}


const char *genframe_tab_style(WGenFrame *genframe)
{
	const char *ret=NULL;
	CALL_DYN_RET(ret, const char *, genframe_tab_style, genframe, (genframe));
	return ret;
}


void genframe_recalc_bar(WGenFrame *genframe)
{
	CALL_DYN(genframe_recalc_bar, genframe, (genframe));
	
}


void genframe_draw_bar(const WGenFrame *genframe, bool complete)
{
	CALL_DYN(genframe_draw_bar, genframe, (genframe, complete));
}


void genframe_bar_geom(const WGenFrame *genframe, WRectangle *geom)
{
	CALL_DYN(genframe_bar_geom, genframe, (genframe, geom));
}


void genframe_border_geom(const WGenFrame *genframe, WRectangle *geom)
{
	CALL_DYN(genframe_border_geom, genframe, (genframe, geom));
}


void genframe_border_inner_geom(const WGenFrame *genframe, WRectangle *geom)
{
	CALL_DYN(genframe_border_inner_geom, genframe, (genframe, geom));
}


void genframe_brushes_updated(WGenFrame *genframe)
{
	CALL_DYN(genframe_brushes_updated, genframe, (genframe));
}


/*}}}*/


/*{{{ Drawing routines and such */


static void genframe_initialise_gr(WGenFrame *genframe)
{
	Window win=WGENFRAME_WIN(genframe);
	GrBorderWidths bdw;
	GrFontExtents fnte;

	genframe->bar_h=0;
	
	genframe->brush=gr_get_brush(ROOTWIN_OF(genframe), win,
								 genframe_style(genframe));
	if(genframe->brush==NULL)
		return;
	
	genframe->bar_brush=grbrush_get_slave(genframe->brush, 
										  ROOTWIN_OF(genframe), win,
										  genframe_tab_style(genframe));
	
	if(genframe->bar_brush==NULL)
		return;
	
	grbrush_get_border_widths(genframe->bar_brush, &bdw);
	grbrush_get_font_extents(genframe->bar_brush, &fnte);
	
	genframe->bar_h=bdw.top+bdw.bottom+fnte.max_height;
	
	genframe_brushes_updated(genframe);
}


void genframe_draw_config_updated(WGenFrame *genframe)
{
	Window win=WGENFRAME_WIN(genframe);
	WRectangle geom;
	WRegion *sub;
	
	release_brushes(genframe);
	
	genframe_initialise_gr(genframe);

	genframe_managed_geom(genframe, &geom);
	
	FOR_ALL_TYPED_CHILDREN(genframe, sub, WRegion){
		region_draw_config_updated(sub);
		if(REGION_MANAGER(sub)==(WRegion*)genframe)
			region_fit(sub, &geom);
	}
	
	genframe_recalc_bar(genframe);
	set_genframe_background(genframe, TRUE);
}


void genframe_draw_bar_default(const WGenFrame *genframe, bool complete)
{
	WRectangle geom;
	const char *cattr=(REGION_IS_ACTIVE(genframe) 
					   ? "active" : "inactive");
	
	if(genframe->bar_brush==NULL ||
	   genframe->flags&WGENFRAME_TAB_HIDE ||
	   genframe->titles==NULL){
		return;
	}
	
	genframe_bar_geom(genframe, &geom);
	
	grbrush_draw_textboxes(genframe->bar_brush, WGENFRAME_WIN(genframe), 
						   &geom, genframe->titles_n, genframe->titles,
						   complete, cattr);
}


void genframe_draw_default(const WGenFrame *genframe, bool complete)
{
	WRectangle geom;
	const char *attr=(REGION_IS_ACTIVE(genframe) 
					  ? "active" : "inactive");
	
	if(genframe->brush==NULL)
		return;
	
	genframe_border_geom(genframe, &geom);
	
	grbrush_draw_border(genframe->brush, WGENFRAME_WIN(genframe), &geom,
						attr);

	genframe_draw_bar(genframe, FALSE);
}


static const char *genframe_style_default(WGenFrame *genframe)
{
	return "frame";
}


static const char *genframe_tab_style_default(WGenFrame *genframe)
{
	return "frame-tab";
}


/*}}}*/


/*{{{ Dynfuntab and class info */


static DynFunTab genframe_dynfuntab[]={
	{region_fit, genframe_fit},
	{(DynFun*)region_reparent, (DynFun*)genframe_reparent},
	{region_resize_hints, genframe_resize_hints},

	{genframe_draw_bar, genframe_draw_bar_default},
	{window_draw, genframe_draw_default},

	{mplex_managed_changed, genframe_managed_changed},
	{mplex_size_changed, genframe_size_changed_default},
	{region_notify_managed_change, genframe_notify_managed_change},
	
	{region_activated, genframe_activated},
	{region_inactivated, genframe_inactivated},

	{(DynFun*)window_press, (DynFun*)genframe_press},
	
	{region_draw_config_updated, genframe_draw_config_updated},
	
	{(DynFun*)genframe_style, (DynFun*)genframe_style_default},
	{(DynFun*)genframe_tab_style, (DynFun*)genframe_tab_style_default},
	END_DYNFUNTAB
};
									   

IMPLOBJ(WGenFrame, WMPlex, genframe_deinit, genframe_dynfuntab);


/*}}}*/
