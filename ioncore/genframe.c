/*
 * ion/ioncore/genframe.c
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
#include "drawp.h"
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


#define genframe_draw(F, C) draw_window((WWindow*)F, C)
#define genframe_managed_geom(F, G) mplex_managed_geom((WMPlex*)(F), G)

#define SET_SHADE_FLAG(F) ((F)->flags|=WGENFRAME_SHADED, \
						   (F)->mplex.flags|=WMPLEX_MANAGED_UNVIEWABLE)
#define UNSET_SHADE_FLAG(F) ((F)->flags&=~WGENFRAME_SHADED, \
							 (F)->mplex.flags&=~WMPLEX_MANAGED_UNVIEWABLE)

static bool set_genframe_background(WGenFrame *genframe, bool set_always);



/*{{{ Destroy/create genframe */


bool genframe_init(WGenFrame *genframe, WWindow *parent, WRectangle geom)
{
	Window win;
	XSetWindowAttributes attr;
	WGRData *grdata=GRDATA_OF(parent);
	ulong attrflags=0;
	WRootWin *rootwin=ROOTWIN_OF(parent);
	
	genframe->flags=0;
	genframe->saved_w=0;
	genframe->saved_h=0;
	genframe->saved_x=0;
	genframe->saved_y=0;
	genframe->tab_pressed_sub=NULL;
	genframe->tab_spacing=0;
	
	if(grdata->transparent_background){
		attr.background_pixmap=ParentRelative;
		attrflags=CWBackPixmap;
		genframe->flags|=WGENFRAME_TRANSPARENT;
	}else{
		attr.background_pixel=COLOR_PIXEL(grdata->frame_bgcolor);
		attrflags=CWBackPixel;
	}
	
	win=XCreateWindow(wglobal.dpy, parent->win, geom.x, geom.y,
					  geom.w, geom.h, 0, 
					  DefaultDepth(wglobal.dpy, rootwin->xscr),
					  InputOutput,
					  DefaultVisual(wglobal.dpy, rootwin->xscr),
					  attrflags, &attr);
	
	if(!mplex_init((WMPlex*)genframe, parent, win, geom)){
		XDestroyWindow(wglobal.dpy, win);
		return FALSE;
	}
	
	/* in mplex */
	/*genframe->win.region.flags|=REGION_BINDINGS_ARE_GRABBED;*/
	
	XSelectInput(wglobal.dpy, win, FRAME_MASK);

	region_add_bindmap((WRegion*)genframe, &ioncore_genframe_bindmap);
	
	return TRUE;
}


WGenFrame *create_genframe(WWindow *parent, WRectangle geom)
{
	CREATEOBJ_IMPL(WGenFrame, genframe, (p, parent, geom));
}


void genframe_deinit(WGenFrame *genframe)
{
	mplex_deinit((WMPlex*)genframe);
}


/*}}}*/


/*{{{ Tabs */


int genframe_tab_at_x(const WGenFrame *genframe, int x)
{
	WRectangle bg;
	
	genframe_bar_geom(genframe, &bg);
	x-=bg.x;
	
	if(x<0 || bg.w==0)
		return -1;
	
	return (x*WGENFRAME_MCOUNT(genframe))/bg.w;
}


int genframe_nth_tab_x(const WGenFrame *genframe, int n)
{
	WRectangle bg;
	
	genframe_bar_geom(genframe, &bg);
	
	if(WGENFRAME_MCOUNT(genframe)==0)
		return bg.x;
	
	return (n*bg.w)/WGENFRAME_MCOUNT(genframe)+bg.x;
}


int genframe_nth_tab_w(const WGenFrame *genframe, int n)
{
	WGRData *grdata=GRDATA_OF(genframe);
	int start=genframe_nth_tab_x(genframe, n);
	WRectangle bg;

	genframe_bar_geom(genframe, &bg);
	
	if(n==WGENFRAME_MCOUNT(genframe)-1)
		return bg.w+bg.x-start;
	else
		return genframe_nth_tab_x(genframe, n+1)-genframe->tab_spacing-start;
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
		XReparentWindow(wglobal.dpy, WGENFRAME_WIN(genframe), parent->win,
						geom->x, geom->y);
		XResizeWindow(wglobal.dpy, WGENFRAME_WIN(genframe), geom->w, geom->h);
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


bool genframe_reparent(WGenFrame *genframe, WWindow *parent, WRectangle geom)
{
	if(!same_rootwin((WRegion*)genframe, (WRegion*)parent))
		return FALSE;
	
	region_detach_parent((WRegion*)genframe);
	region_set_parent((WRegion*)genframe, (WRegion*)parent);
	reparent_or_fit(genframe, &geom, parent);
	
	return TRUE;
}


void genframe_fit(WGenFrame *genframe, WRectangle geom)
{
	reparent_or_fit(genframe, &geom, NULL);
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
}


void genframe_activated(WGenFrame *genframe)
{
	genframe_draw(genframe, FALSE);
}


/*}}}*/


/*{{{ Misc. */


static bool set_genframe_background(WGenFrame *genframe, bool set_always)
{
	XSetWindowAttributes attr;
	ulong attrflags=0;
	bool tr=FALSE, chg=FALSE;
	WGRData *grdata=GRDATA_OF(genframe);
	
	if(WGENFRAME_MCOUNT(genframe)==0){
		tr=grdata->transparent_background;
	}else if(WGENFRAME_CURRENT(genframe)!=NULL &&
			 WOBJ_IS(WGENFRAME_CURRENT(genframe), WClientWin)){
		tr=((WClientWin*)WGENFRAME_CURRENT(genframe))->flags&CWIN_PROP_TRANSPARENT;
	}
	
	if(tr){
		attr.background_pixmap=ParentRelative;
		attrflags=CWBackPixmap;
		chg=!(genframe->flags&WGENFRAME_TRANSPARENT);
		genframe->flags|=WGENFRAME_TRANSPARENT;
	}else{
		attr.background_pixel=COLOR_PIXEL(grdata->frame_bgcolor);
		attrflags=CWBackPixel;
		chg=(genframe->flags&WGENFRAME_TRANSPARENT);
		genframe->flags&=~WGENFRAME_TRANSPARENT;
	}
	
	if(chg || set_always){
		XChangeWindowAttributes(wglobal.dpy, WGENFRAME_WIN(genframe),
								attrflags, &attr);
		genframe_draw(genframe, TRUE);
		return TRUE;
	}
	
	return FALSE;
}


/*EXTL_DOC
 * Toggle tab visibility.
 */
EXTL_EXPORT
void genframe_toggle_tab(WGenFrame *genframe)
{
	if(genframe->flags&WGENFRAME_SHADED)
		return;
	
	if(genframe->flags&WGENFRAME_TAB_HIDE)
		genframe->flags&=~WGENFRAME_TAB_HIDE;
	else
		genframe->flags|=WGENFRAME_TAB_HIDE;
	mplex_fit_managed(&(genframe->mplex));
	genframe_draw(genframe,TRUE);
}


void genframe_draw_config_updated(WGenFrame *genframe)
{
	WGRData *grdata=GRDATA_OF(genframe);
	XSetWindowAttributes attr;
	ulong attrflags;
	WRegion *sub;
	WRectangle geom;
	
	genframe_managed_geom(genframe, &geom);
	
	FOR_ALL_TYPED_CHILDREN(genframe, sub, WRegion){
		region_draw_config_updated(sub);
		if(REGION_MANAGER(sub)==(WRegion*)genframe)
			region_fit(sub, geom);
	}
	
	genframe_recalc_bar(genframe);
	set_genframe_background(genframe, TRUE);
}


void genframe_notify_managed_change(WGenFrame *genframe, WRegion *sub)
{
	genframe_recalc_bar(genframe);
	/* TODO: Should only draw only the affected tab if there were no
	 * changes in sizes */
	genframe_draw_bar(genframe, FALSE);
}


static void genframe_size_changed_default(WGenFrame *genframe,
										  bool wchg, bool hchg)
{
	if(wchg)
		genframe_recalc_bar(genframe);
	/* We should get a request from X to draw the frame... */
}


static bool genframe_do_managed_changed(WGenFrame *genframe, bool sw)
{
	if(sw){
		extl_call_named("call_hook", "soo", NULL,
						"genframe_managed_switched",
						genframe, WGENFRAME_CURRENT(genframe));
		return set_genframe_background(genframe, FALSE);
	}
	return FALSE;
}

static void genframe_managed_changed(WGenFrame *genframe, bool sw)
{
	if(!genframe_do_managed_changed(genframe, sw))
		genframe_draw_bar(genframe, FALSE);
}


static void genframe_managed_added(WGenFrame *genframe, WRegion *reg, 
								   bool sw)
{
	genframe_recalc_bar(genframe);
	if(!genframe_do_managed_changed(genframe, sw))
		genframe_draw_bar(genframe, TRUE);
}


static void genframe_managed_removed(WGenFrame *genframe, WRegion *reg, 
									 bool sw)
{
	if(genframe->tab_pressed_sub==reg)
		genframe->tab_pressed_sub=NULL;
	
	if(REGION_LABEL(reg)!=NULL){
		free(REGION_LABEL(reg));
		REGION_LABEL(reg)=NULL;
	}
			
	genframe_recalc_bar(genframe);
	if(!genframe_do_managed_changed(genframe, sw))
		genframe_draw_bar(genframe, TRUE);
}


/*}}}*/


/*{{{ Dynfuns */


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


void genframe_border_inner_geom(const WGenFrame *genframe, WRectangle *geom)
{
	CALL_DYN(genframe_border_inner_geom, genframe, (genframe, geom));
}


void genframe_size_changed(WGenFrame *genframe, bool wchg, bool hchg)
{
	CALL_DYN(mplex_size_changed, genframe, (genframe, wchg, hchg));
}


/*}}}*/


/*{{{ genframe_draw_bar_default */


void genframe_draw_bar_default(const WGenFrame *genframe, bool complete)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WRegion *sub, *next;
	WGRData *grdata=GRDATA_OF(genframe);
	WRectangle bg;
	int n;
	
	genframe_bar_geom(genframe, &bg);
	
	dinfo->win=WGENFRAME_WIN(genframe);
	dinfo->draw=WGENFRAME_DRAW(genframe);
	
	dinfo->grdata=grdata;
	dinfo->gc=grdata->tab_gc;
	dinfo->geom=bg;
	dinfo->border=&(grdata->tab_border);
	dinfo->font=grdata->tab_font;
	
	/*if(complete)
		XClearArea(wglobal.dpy, WIN, X, Y, W, H, False);*/
	
	if(WGENFRAME_MCOUNT(genframe)==0){
		if(REGION_IS_ACTIVE(genframe))
			COLORS=&(grdata->act_tab_sel_colors);
		else
			COLORS=&(grdata->tab_sel_colors);
		draw_textbox(dinfo, "<empty frame>", CF_TAB_TEXT_ALIGN, TRUE);
		return;
	}
	
	n=0;
	FOR_ALL_MANAGED_ON_LIST(WGENFRAME_MLIST(genframe), sub){
		dinfo->geom.w=genframe_nth_tab_w(genframe, n);
		dinfo->geom.x=genframe_nth_tab_x(genframe, n);
		n++;
		
		if(REGION_IS_ACTIVE(genframe)){
			if(sub==WGENFRAME_CURRENT(genframe))
				COLORS=&(grdata->act_tab_sel_colors);
			else
				COLORS=&(grdata->act_tab_colors);
		}else{
			if(sub==WGENFRAME_CURRENT(genframe))
				COLORS=&(grdata->tab_sel_colors);
			else
				COLORS=&(grdata->tab_colors);
		}
		
		if(REGION_LABEL(sub)!=NULL)
			draw_textbox(dinfo, REGION_LABEL(sub), CF_TAB_TEXT_ALIGN, TRUE);
		else
			draw_textbox(dinfo, "?", CF_TAB_TEXT_ALIGN, TRUE);
		
		if(REGION_IS_TAGGED(sub)){
			set_foreground(wglobal.dpy, grdata->copy_gc, COLORS->fg);
			copy_masked(grdata, grdata->stick_pixmap, WIN, 0, 0,
						grdata->stick_pixmap_w, grdata->stick_pixmap_h,
						I_X+I_W-grdata->stick_pixmap_w, I_Y);
		}
		
		if(genframe->flags&WGENFRAME_TAB_DRAGGED &&
		   sub==genframe->tab_pressed_sub){
			/* drag */
			if(grdata->bar_inside_frame){
				if(REGION_IS_ACTIVE(genframe)){
					set_foreground(wglobal.dpy, grdata->stipple_gc,
								   grdata->act_frame_colors.bg);
				}else{
					set_foreground(wglobal.dpy, grdata->stipple_gc,
								   grdata->frame_colors.bg);
				}
			}
			XFillRectangle(wglobal.dpy, WIN, grdata->stipple_gc, X, Y, W, H);
		}
	}
}


/*}}}*/


/*{{{ Dynfuntab and class info */


static DynFunTab genframe_dynfuntab[]={
	{region_fit, genframe_fit},
	{(DynFun*)reparent_region, (DynFun*)genframe_reparent},
	{region_resize_hints, genframe_resize_hints},

	{genframe_draw_bar, genframe_draw_bar_default},

	{mplex_managed_changed, genframe_managed_changed},
	{mplex_managed_added, genframe_managed_added},
	{mplex_managed_removed, genframe_managed_removed},
	{mplex_size_changed, genframe_size_changed_default},
	{region_notify_managed_change, genframe_notify_managed_change},
	
	{region_activated, genframe_activated},
	{region_inactivated, genframe_inactivated},

	{(DynFun*)window_press, (DynFun*)genframe_press},
	{(DynFun*)window_release, (DynFun*)genframe_release},
	
	{region_draw_config_updated, genframe_draw_config_updated},
	
	END_DYNFUNTAB
};
									   

IMPLOBJ(WGenFrame, WMPlex, genframe_deinit, genframe_dynfuntab);


/*}}}*/
