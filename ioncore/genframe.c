/*
 * ion/ioncore/genframe.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "objp.h"
#include "window.h"
#include "global.h"
#include "screen.h"
#include "focus.h"
#include "drawp.h"
#include "event.h"
#include "targetid.h"
#include "attach.h"
#include "resize.h"
#include "tags.h"
#include "names.h"
#include "saveload.h"
#include "genframep.h"
#include "genframe-pointer.h"
#include "sizehint.h"
#include "stacking.h"


#define genframe_draw(F, C) draw_window((WWindow*)F, C)

static bool set_genframe_background(WGenFrame *genframe, bool set_always);


/*{{{ Destroy/create genframe */


bool genframe_init(WGenFrame *genframe, WWindow *parent, WRectangle geom,
				   int id)
{
	Window win;
	XSetWindowAttributes attr;
	WGRData *grdata=GRDATA_OF(parent);
	ulong attrflags=0;
	
	genframe->flags=0;
	genframe->managed_count=0;
	genframe->managed_list=NULL;
	genframe->current_sub=NULL;
	genframe->current_input=NULL;
	genframe->saved_w=WGENFRAME_NO_SAVED_WH;
	genframe->saved_h=WGENFRAME_NO_SAVED_WH;
	genframe->saved_x=WGENFRAME_NO_SAVED_WH;
	genframe->saved_y=WGENFRAME_NO_SAVED_WH;
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
					  geom.w, geom.h, 0, CopyFromParent, InputOutput,
					  CopyFromParent, attrflags, &attr);
	
	if(!window_init((WWindow*)genframe, parent, win, geom)){
		XDestroyWindow(wglobal.dpy, win);
		return FALSE;
	}
	
	genframe->win.region.flags|=REGION_BINDINGS_ARE_GRABBED;
	
	genframe->target_id=use_target_id((WRegion*)genframe, id);
	
	XSelectInput(wglobal.dpy, win, FRAME_MASK);
	
	return TRUE;
}


WGenFrame *create_genframe(WWindow *parent, WRectangle geom, int id)
{
	CREATEOBJ_IMPL(WGenFrame, genframe, (p, parent, geom, id));
}


void genframe_deinit(WGenFrame *genframe)
{
	while(genframe->managed_list!=NULL)
		destroy_obj((WObj*)genframe->managed_list);
	if(genframe->current_input!=NULL)
		destroy_obj((WObj*)genframe->current_input);
	
	window_deinit((WWindow*)genframe);
	free_target_id(genframe->target_id);
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
	
	return (x*genframe->managed_count)/bg.w;
}


int genframe_nth_tab_x(const WGenFrame *genframe, int n)
{
	WRectangle bg;
	
	genframe_bar_geom(genframe, &bg);
	
	if(genframe->managed_count==0)
		return bg.x;
	
	return (n*bg.w)/genframe->managed_count+bg.x;
}


int genframe_nth_tab_w(const WGenFrame *genframe, int n)
{
	WGRData *grdata=GRDATA_OF(genframe);
	int start=genframe_nth_tab_x(genframe, n);
	WRectangle bg;

	genframe_bar_geom(genframe, &bg);
	
	if(n==genframe->managed_count-1)
		return bg.w+bg.x-start;
	else
		return genframe_nth_tab_x(genframe, n+1)-genframe->tab_spacing-start;
}


EXTL_EXPORT
void genframe_move_current_tab_right(WGenFrame *genframe)
{
	WRegion *reg, *next;
	
	if((reg=genframe->current_sub)==NULL)
		return;
	if((next=NEXT_MANAGED(genframe->managed_list, reg))==NULL)
		return;
	
	UNLINK_ITEM(genframe->managed_list, reg, mgr_next, mgr_prev);
	LINK_ITEM_AFTER(genframe->managed_list, next, reg, mgr_next, mgr_prev);
	
	genframe_draw_bar(genframe, TRUE);
}


EXTL_EXPORT
void genframe_move_current_tab_left(WGenFrame *genframe)
{
	WRegion *reg, *prev;
	
	if((reg=genframe->current_sub)==NULL)
		return;
	if((prev=PREV_MANAGED(genframe->managed_list, reg))==NULL)
		return;

	UNLINK_ITEM(genframe->managed_list, reg, mgr_next, mgr_prev);
	LINK_ITEM_BEFORE(genframe->managed_list, prev, reg, mgr_next, mgr_prev);

	genframe_draw_bar(genframe, TRUE);
}


/*}}}*/


/*{{{ Resize and reparent */


static void reparent_or_fit(WGenFrame *genframe, WRectangle geom,
							WWindow *parent)
{
	bool wchg=(REGION_GEOM(genframe).w!=geom.w);
	bool hchg=(REGION_GEOM(genframe).h!=geom.h);
	bool move=(REGION_GEOM(genframe).x!=geom.x ||
			   REGION_GEOM(genframe).y!=geom.y);
	
	if(parent!=NULL){
		XReparentWindow(wglobal.dpy, WGENFRAME_WIN(genframe), parent->win,
						geom.x, geom.y);
		XResizeWindow(wglobal.dpy, WGENFRAME_WIN(genframe), geom.w, geom.h);
	}else{
		XMoveResizeWindow(wglobal.dpy, WGENFRAME_WIN(genframe),
						  geom.x, geom.y, geom.w, geom.h);
	}
	
	REGION_GEOM(genframe)=geom;

	if(move && !wchg && !hchg){
		region_notify_subregions_move(&(genframe->win.region));
	}else if(wchg || hchg)
		genframe_fit_managed(genframe);

	if(wchg)
		genframe->saved_w=WGENFRAME_NO_SAVED_WH;
	if(hchg)
		genframe->saved_h=WGENFRAME_NO_SAVED_WH;
	
	if(wchg || hchg)
		genframe_size_changed(genframe, wchg, hchg);
}


bool genframe_reparent(WGenFrame *genframe, WWindow *parent, WRectangle geom)
{
	if(!same_screen((WRegion*)genframe, (WRegion*)parent))
		return FALSE;
	
	region_detach_parent((WRegion*)genframe);
	region_set_parent((WRegion*)genframe, (WRegion*)parent);
	reparent_or_fit(genframe, geom, parent);
	
	return TRUE;
}


void genframe_fit(WGenFrame *genframe, WRectangle geom)
{
	reparent_or_fit(genframe, geom, NULL);
}


void genframe_fit_managed(WGenFrame *genframe)
{
	WRectangle geom;
	WRegion *sub;
	
	genframe_managed_geom(genframe, &geom);
	
	FOR_ALL_MANAGED_ON_LIST(genframe->managed_list, sub){
		region_fit(sub, geom);
	}
	
	if(genframe->current_input!=NULL)
		region_fit(genframe->current_input, geom);
}


static void genframe_request_managed_geom(WGenFrame *genframe, WRegion *sub,
										  WRectangle geom, WRectangle *geomret,
										  bool tryonly)
{
	/* Just try to give it the maximum size */
	genframe_managed_geom(genframe, &geom);
	
	if(geomret!=NULL)
		*geomret=geom;
	
	if(!tryonly)
		region_fit(sub, geom);
}


void genframe_resize_hints(WGenFrame *genframe, XSizeHints *hints_ret,
						   uint *relw_ret, uint *relh_ret)
{
	WRectangle subgeom;
	uint wdummy, hdummy;
	
	genframe_managed_geom(genframe, &subgeom);
	
	*relw_ret=subgeom.w;
	*relh_ret=subgeom.h;
	
	if(genframe->current_sub!=NULL){
		region_resize_hints(genframe->current_sub, hints_ret,
							&wdummy, &hdummy);
	}else{
		hints_ret->flags=0;
	}
	
	adjust_size_hints_for_managed(hints_ret, genframe->managed_list);
}


/*}}}*/


/*{{{ Mapping */


void genframe_map(WGenFrame *genframe)
{
	window_map((WWindow*)genframe);
	/* A lame requirement of the ICCCM is that client windows should be
	 * unmapped if the parent is unmapped.
	 */
	if(genframe->current_sub!=NULL)
		region_map(genframe->current_sub);
}


void genframe_unmap(WGenFrame *genframe)
{
	window_unmap((WWindow*)genframe);
	/* A lame requirement of the ICCCM is that client windows should be
	 * unmapped if the parent is unmapped.
	 */
	if(genframe->current_sub!=NULL)
		region_unmap(genframe->current_sub);
}


/*}}}*/


/*{{{ Managed region switching */


bool genframe_display_managed(WGenFrame *genframe, WRegion *sub)
{
	bool mapped;
	
	if(sub==genframe->current_sub || sub==genframe->current_input)
		return TRUE;
	
	if(genframe->current_sub!=NULL && REGION_IS_MAPPED(genframe))
		region_unmap(genframe->current_sub);
	
	genframe->current_sub=sub;
	
	if(REGION_IS_MAPPED(genframe))
		region_map(sub);
	
	if(genframe->current_input==NULL){
		if(REGION_IS_ACTIVE(genframe))
			set_focus(sub);
	}
	
	/* Many programs will get upset if the visible, although only such,
	 * client window is not the lowest window in the genframe. xprop/xwininfo
	 * will return the information for the lowest window. 'netscape -remote'
	 * will not work at all if there are no visible netscape windows.
	 */
	region_lower(sub);
	
	if(!set_genframe_background(genframe, FALSE))
		genframe_draw_bar(genframe, FALSE);
	
	return TRUE;
}


WRegion *genframe_nth_managed(WGenFrame *genframe, uint n)
{
	WRegion *reg=FIRST_MANAGED(genframe->managed_list);
	
	while(n-->0 && reg!=NULL)
		reg=NEXT_MANAGED(genframe->managed_list, reg);
	
	return reg;
}


EXTL_EXPORT
void genframe_switch_nth(WGenFrame *genframe, uint n)
{
	WRegion *sub=genframe_nth_managed(genframe, n);
	if(sub!=NULL)
		region_display_sp(sub);
}


EXTL_EXPORT
void genframe_switch_next(WGenFrame *genframe)
{
	WRegion *sub=NEXT_MANAGED_WRAP(genframe->managed_list, genframe->current_sub);
	if(sub!=NULL)
		region_display_sp(sub);
}


EXTL_EXPORT
void genframe_switch_prev(WGenFrame *genframe)
{
	WRegion *sub=PREV_MANAGED_WRAP(genframe->managed_list, genframe->current_sub);
	if(sub!=NULL)
		region_display_sp(sub);
}


/*}}}*/


/*{{{ Add/remove managed */


static WRegion *genframe_do_add_managed(WGenFrame *genframe, WRegionAddFn *fn,
										void *fnparams, 
										const WAttachParams *param)
{
	WRectangle geom;
	WRegion *reg;

	genframe_managed_geom(genframe, &geom);
	
	reg=fn((WWindow*)genframe, geom, fnparams);

	if(reg==NULL)
		return NULL;
	
	set_target_id(reg, genframe->target_id);
	
	if(genframe->current_sub!=NULL && wglobal.opmode!=OPMODE_INIT){
		region_set_manager(reg, (WRegion*)genframe, NULL);
		LINK_ITEM_AFTER(genframe->managed_list, genframe->current_sub,
						reg, mgr_next, mgr_prev);
	}else{
		region_set_manager(reg, (WRegion*)genframe, &(genframe->managed_list));
	}
	
	genframe->managed_count++;
	
	if(genframe->managed_count==1 || param->flags&REGION_ATTACH_SWITCHTO){
		genframe_recalc_bar(genframe, FALSE);
		genframe_display_managed(genframe, reg);
	}else{
		region_unmap(reg);
		genframe_recalc_bar(genframe, TRUE);
	}
	
	return reg;
}


static void genframe_do_remove(WGenFrame *genframe, WRegion *sub)
{
	WRegion *next=NULL;
	
	if(genframe->tab_pressed_sub==sub)
		genframe->tab_pressed_sub=NULL;
	
	if(genframe->current_sub==sub){
		next=PREV_MANAGED(genframe->managed_list, sub);
		if(next==NULL)
			next=NEXT_MANAGED(genframe->managed_list, sub);
		genframe->current_sub=NULL;
	}
	
	region_unset_manager(sub, (WRegion*)genframe, &(genframe->managed_list));
	genframe->managed_count--;
	
	if(wglobal.opmode!=OPMODE_DEINIT){
		genframe_recalc_bar(genframe, (next==NULL));
		if(next!=NULL)
			genframe_display_managed(genframe, next);
		else
			set_genframe_background(genframe, FALSE);
	}
	
	if(REGION_LABEL(sub)!=NULL){
		free(REGION_LABEL(sub));
		REGION_LABEL(sub)=NULL;
	}
}


void genframe_remove_managed(WGenFrame *genframe, WRegion *reg)
{
	if(genframe->current_input==reg){
		region_unset_manager(reg, (WRegion*)genframe, NULL);
		genframe->current_input=NULL;
		if(REGION_IS_ACTIVE(genframe))
			set_focus((WRegion*)genframe);
	}else{
		genframe_do_remove(genframe, reg);
	}
}


EXTL_EXPORT
void genframe_attach_tagged(WGenFrame *genframe)
{
	WRegion *reg;
	
	while((reg=tag_take_first())!=NULL){
		if(region_is_ancestor((WRegion*)genframe, reg)){
			warn("Cannot attach tagged region: ancestor");
			continue;
		}
		region_add_managed_simple((WRegion*)genframe, reg, 0);
	}
}


/*}}}*/


/*{{{ Focus  */


void genframe_focus(WGenFrame *genframe, bool warp)
{
	if(warp)
		do_move_pointer_to((WRegion*)genframe);
	
	if(genframe->current_input!=NULL)
		region_set_focus_to((WRegion*)genframe->current_input, FALSE);
	else if(genframe->current_sub!=NULL)
		region_set_focus_to(genframe->current_sub, FALSE);
	else
		SET_FOCUS(WGENFRAME_WIN(genframe));
}


void genframe_inactivated(WGenFrame *genframe)
{
	genframe_draw(genframe, FALSE);
}


void genframe_activated(WGenFrame *genframe)
{
	genframe_draw(genframe, FALSE);
}


/*}}}*/


/*{{{ Inputs */


WRegion *genframe_current_input(WGenFrame *genframe)
{
	return genframe->current_input;
}


WRegion *genframe_add_input(WGenFrame *genframe, WRegionAddFn *fn, void *fnp)
{
	WRectangle geom;
	WRegion *sub;
	
	if(genframe->current_input!=NULL)
		return NULL;
	
	genframe_managed_geom(genframe, &geom);
	sub=fn((WWindow*)genframe, geom, fnp);
	
	if(sub==NULL)
		return NULL;
	
	genframe->current_input=sub;
	region_set_manager(sub, (WRegion*)genframe, NULL);
	region_keep_on_top(sub);
	
	return sub;
}


/*}}}*/


/*{{{ Misc. */


static bool set_genframe_background(WGenFrame *genframe, bool set_always)
{
	XSetWindowAttributes attr;
	ulong attrflags=0;
	bool tr=FALSE, chg=FALSE;
	WGRData *grdata=GRDATA_OF(genframe);
	
	if(genframe->managed_count==0){
		tr=grdata->transparent_background;
	}else if(genframe->current_sub!=NULL &&
			 WOBJ_IS(genframe->current_sub, WClientWin)){
		tr=((WClientWin*)genframe->current_sub)->flags&CWIN_PROP_TRANSPARENT;
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
		XChangeWindowAttributes(wglobal.dpy, genframe->win.win, attrflags,
								&attr);
		genframe_draw(genframe, TRUE);
		return TRUE;
	}
	
	return FALSE;
}


EXTL_EXPORT
void genframe_toggle_tab(WGenFrame *genframe)
{
	if(genframe->flags&WGENFRAME_TAB_HIDE)
		genframe->flags&=~WGENFRAME_TAB_HIDE;
	else
		genframe->flags|=WGENFRAME_TAB_HIDE;
	genframe_fit_managed(genframe);
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
	
	genframe_recalc_bar(genframe, FALSE);
	
	set_genframe_background(genframe, TRUE);
}


void genframe_notify_managed_change(WGenFrame *genframe, WRegion *sub)
{
	genframe_recalc_bar(genframe, TRUE);
}


static void genframe_size_changed_default(WGenFrame *genframe,
										  bool wchg, bool hchg)
{
	if(wchg)
		genframe_recalc_bar(genframe, FALSE);
}


EXTL_EXPORT
WRegion *genframe_current(WGenFrame *genframe)
{
	return genframe->current_sub;
}


static bool genframe_handle_drop(WGenFrame *genframe, int x, int y,
								 WRegion *dropped)
{
	return region_add_managed_simple((WRegion*)genframe, dropped,
									 REGION_ATTACH_SWITCHTO);
}


/*}}}*/


/*{{{ Dynfuns */


void genframe_recalc_bar(WGenFrame *genframe, bool draw)
{
	CALL_DYN(genframe_recalc_bar, genframe, (genframe, draw));
	
}


void genframe_draw_bar(const WGenFrame *genframe, bool complete)
{
	CALL_DYN(genframe_draw_bar, genframe, (genframe, complete));
}


void genframe_bar_geom(const WGenFrame *genframe, WRectangle *geom)
{
	CALL_DYN(genframe_bar_geom, genframe, (genframe, geom));
}


void genframe_managed_geom(const WGenFrame *genframe, WRectangle *geom)
{
	CALL_DYN(genframe_managed_geom, genframe, (genframe, geom));
}


void genframe_border_inner_geom(const WGenFrame *genframe, WRectangle *geom)
{
	CALL_DYN(genframe_border_inner_geom, genframe, (genframe, geom));
}


void genframe_size_changed(WGenFrame *genframe, bool wchg, bool hchg)
{
	CALL_DYN(genframe_size_changed, genframe, (genframe, wchg, hchg));
}


static WRegion *genframe_managed_enter_to_focus(WGenFrame *genframe,
												WRegion *reg)
{
	return genframe->current_input;
}


/*}}}*/


/*{{{ genframe_draw_bar_default */


void genframe_draw_bar_default(const WGenFrame *genframe, bool complete)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WRegion *sub, *next;
	WScreen *scr=SCREEN_OF(genframe);
	WGRData *grdata=&(scr->grdata);
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
	
	if(complete)
		XClearArea(wglobal.dpy, WIN, X, Y, W, H, False);
	
	if(genframe->managed_count==0){
		if(REGION_IS_ACTIVE(genframe))
			COLORS=&(grdata->act_tab_sel_colors);
		else
			COLORS=&(grdata->tab_sel_colors);
		draw_textbox(dinfo, "<empty frame>", CF_TAB_TEXT_ALIGN, TRUE);
		return;
	}
	
	n=0;
	FOR_ALL_MANAGED_ON_LIST(genframe->managed_list, sub){
		dinfo->geom.w=genframe_nth_tab_w(genframe, n);
		dinfo->geom.x=genframe_nth_tab_x(genframe, n);
		n++;
		
		if(REGION_IS_ACTIVE(genframe)){
			if(sub==genframe->current_sub)
				COLORS=&(grdata->act_tab_sel_colors);
			else
				COLORS=&(grdata->act_tab_colors);
		}else{
			if(sub==genframe->current_sub)
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
	{genframe_size_changed, genframe_size_changed_default},
	{(DynFun*)reparent_region, (DynFun*)genframe_reparent},
	{region_resize_hints, genframe_resize_hints},

	{genframe_draw_bar, genframe_draw_bar_default},
	
	{(DynFun*)window_press, (DynFun*)genframe_press},
	{(DynFun*)window_release, (DynFun*)genframe_release},
	
	{region_set_focus_to, genframe_focus},
	{region_activated, genframe_activated},
	{region_inactivated, genframe_inactivated},
	{(DynFun*)region_managed_enter_to_focus,
	 (DynFun*)genframe_managed_enter_to_focus},
	
	{(DynFun*)region_do_add_managed, (DynFun*)genframe_do_add_managed},
	{region_remove_managed, genframe_remove_managed},
	{region_notify_managed_change, genframe_notify_managed_change},
	{region_request_managed_geom, genframe_request_managed_geom},
	{(DynFun*)region_display_managed, (DynFun*)genframe_display_managed},

	{(DynFun*)region_handle_drop, (DynFun*)genframe_handle_drop},
	
	{region_draw_config_updated, genframe_draw_config_updated},
	
	{region_map, genframe_map},
	{region_unmap, genframe_unmap},
	
	END_DYNFUNTAB
};
									   

IMPLOBJ(WGenFrame, WWindow, genframe_deinit, genframe_dynfuntab);


/*}}}*/
