/*
 * ion/ioncore/frame.c
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
#include "framep.h"
#include "frame-pointer.h"
#include "frame-draw.h"
#include "sizehint.h"
#include "stacking.h"
#include "extlconv.h"
#include "mplex.h"
#include "bindmaps.h"
#include "regbind.h"
#include "gr.h"
#include "genws.h"
#include "activity.h"
#include "region-iter.h"


#define SET_SHADE_FLAG(F) ((F)->flags|=FRAME_SHADED, \
						   (F)->mplex.flags|=MPLEX_MANAGED_UNVIEWABLE)
#define UNSET_SHADE_FLAG(F) ((F)->flags&=~FRAME_SHADED, \
							 (F)->mplex.flags&=~MPLEX_MANAGED_UNVIEWABLE)


extern bool frame_set_background(WFrame *frame, bool set_always);
extern void frame_initialise_gr(WFrame *frame);

static bool frame_initialise_titles(WFrame *frame);
static void frame_free_titles(WFrame *frame);


/*{{{ Destroy/create frame */


bool frame_init(WFrame *frame, WWindow *parent, 
				   const WRectangle *geom)
{
	WRectangle mg;
	
	frame->flags=0;
	frame->saved_w=0;
	frame->saved_h=0;
	frame->saved_x=0;
	frame->saved_y=0;
	frame->tab_dragged_idx=-1;
	frame->titles=NULL;
	frame->titles_n=0;
	frame->bar_h=0;
	frame->tr_mode=GR_TRANSPARENCY_DEFAULT;
	frame->brush=NULL;
	frame->bar_brush=NULL;

	if(!mplex_init_new((WMPlex*)frame, parent, geom))
		return FALSE;
	
	frame_initialise_gr(frame);
	frame_initialise_titles(frame);
	
	XSelectInput(ioncore_g.dpy, FRAME_WIN(frame), IONCORE_EVENTMASK_FRAME);

	region_add_bindmap((WRegion*)frame, &ioncore_frame_bindmap);

	mplex_managed_geom((WMPlex*)frame, &mg);
	
	if(mg.h<=1)
		SET_SHADE_FLAG(frame);
	
	return TRUE;
}


WFrame *create_frame(WWindow *parent, const WRectangle *geom)
{
	CREATEOBJ_IMPL(WFrame, frame, (p, parent, geom));
}


void frame_deinit(WFrame *frame)
{
	frame_free_titles(frame);
	frame_release_brushes(frame);
	mplex_deinit((WMPlex*)frame);
}


/*}}}*/


/*{{{ Tabs */


int frame_tab_at_x(const WFrame *frame, int x)
{
	WRectangle bg;
	int tab, tx;
	
	frame_bar_geom(frame, &bg);
	
	if(x>=bg.x+bg.w || x<bg.x)
		return -1;
	
	tx=bg.x;

	for(tab=0; tab<FRAME_MCOUNT(frame); tab++){
		tx+=frame_nth_tab_w(frame, tab);
		if(x<tx)
			break;
	}
	
	return tab;
}


int frame_nth_tab_x(const WFrame *frame, int n)
{
	uint x=0;
	int i;
	
	for(i=0; i<n; i++)
		x+=frame_nth_tab_w(frame, i);
	
	return x;
}


static int frame_nth_tab_w_iw(const WFrame *frame, int n, bool inner)
{
	WRectangle bg;
	GrBorderWidths bdw=GR_BORDER_WIDTHS_INIT;
	int m=FRAME_MCOUNT(frame);
	uint w;
	
	frame_bar_geom(frame, &bg);

	if(m==0)
		m=1;

	if(frame->bar_brush!=NULL)
		grbrush_get_border_widths(frame->bar_brush, &bdw);
	
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


int frame_nth_tab_w(const WFrame *frame, int n)
{
	return frame_nth_tab_w_iw(frame, n, FALSE);
}


int frame_nth_tab_iw(const WFrame *frame, int n)
{
	return frame_nth_tab_w_iw(frame, n, TRUE);
}



static void update_attr(WFrame *frame, int i, WRegion *reg)
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

	if(i>=frame->titles_n){
		/* Might happen when deinitialising */
		return;
	}
	
	if(reg==FRAME_CURRENT(frame))
		flags|=0x01;
	if(reg!=NULL && reg->flags&REGION_TAGGED)
		flags|=0x02;
	if(i==frame->tab_dragged_idx)
		flags|=0x04;
	if(reg!=NULL && region_activity(reg))
		flags|=0x08;
	
	frame->titles[i].attr=attrs[flags];
}


void frame_update_attr_nth(WFrame *frame, int i)
{
	WRegion *reg;
	
	if(i<0 || i>=frame->titles_n)
		return;

	update_attr(frame, i, mplex_nth_managed((WMPlex*)frame, i));
}


static void update_attrs(WFrame *frame)
{
	int i=0;
	WRegion *sub;
	
	FOR_ALL_MANAGED_ON_LIST(FRAME_MLIST(frame), sub){
		update_attr(frame, i, sub);
		i++;
	}
}


static void frame_free_titles(WFrame *frame)
{
	int i;
	
	if(frame->titles!=NULL){
		for(i=0; i<frame->titles_n; i++){
			if(frame->titles[i].text)
				free(frame->titles[i].text);
		}
		free(frame->titles);
		frame->titles=NULL;
	}
	frame->titles_n=0;
}


static void do_init_title(WFrame *frame, int i, WRegion *sub)
{
	frame->titles[i].text=NULL;
	frame->titles[i].iw=frame_nth_tab_iw(frame, i);
	update_attr(frame, i, sub);
}

	
static bool frame_initialise_titles(WFrame *frame)
{
	int i, n=FRAME_MCOUNT(frame);
	WRegion *sub;
	
	frame_free_titles(frame);

	if(n==0)
		n=1;
	
	frame->titles=ALLOC_N(GrTextElem, n);
	if(frame->titles==NULL)
		return FALSE;
	frame->titles_n=n;
	
	if(FRAME_MCOUNT(frame)==0){
		do_init_title(frame, 0, NULL);
	}else{
		i=0;
		FOR_ALL_MANAGED_ON_LIST(FRAME_MLIST(frame), sub){
			do_init_title(frame, i, sub);
			i++;
		}
	}
	
	frame_recalc_bar(frame);

	return TRUE;
}


/*}}}*/


/*{{{ Resize and reparent */


static void reparent_or_fit(WFrame *frame, const WRectangle *geom,
							WWindow *parent)
{
	WRectangle old_geom, mg;
	bool wchg=(REGION_GEOM(frame).w!=geom->w);
	bool hchg=(REGION_GEOM(frame).h!=geom->h);
	bool move=(REGION_GEOM(frame).x!=geom->x ||
			   REGION_GEOM(frame).y!=geom->y);
	
	if(parent!=NULL){
		region_detach_parent((WRegion*)frame);
		XReparentWindow(ioncore_g.dpy, FRAME_WIN(frame), parent->win,
						geom->x, geom->y);
		XResizeWindow(ioncore_g.dpy, FRAME_WIN(frame), geom->w, geom->h);
		region_attach_parent((WRegion*)frame, (WRegion*)parent);
	}else{
		XMoveResizeWindow(ioncore_g.dpy, FRAME_WIN(frame),
						  geom->x, geom->y, geom->w, geom->h);
	}
	
	old_geom=REGION_GEOM(frame);
	REGION_GEOM(frame)=*geom;

	if(hchg){
		frame->flags|=FRAME_SAVED_VERT;
		frame->saved_y=old_geom.y;
		frame->saved_h=old_geom.h;
	}

	if(wchg){
		frame->flags|=FRAME_SAVED_HORIZ;
		frame->saved_x=old_geom.x;
		frame->saved_w=old_geom.w;
	}

	mplex_managed_geom((WMPlex*)frame, &mg);
	if(hchg && mg.h<=1){
		if(!(frame->flags&(FRAME_SHADED|FRAME_TAB_HIDE))){
			SET_SHADE_FLAG(frame);
			if(FRAME_CURRENT(frame)!=NULL)
				region_unmap(FRAME_CURRENT(frame));
		}
	}else if(hchg){
		if(frame->flags&FRAME_SHADED && REGION_IS_MAPPED(frame)){
			if(FRAME_CURRENT(frame)!=NULL)
				region_map(FRAME_CURRENT(frame));
		}
		UNSET_SHADE_FLAG(frame);
	}

	if(move && !wchg && !hchg)
		region_notify_subregions_move((WRegion*)frame);
	else if(wchg || hchg)
		mplex_fit_managed((WMPlex*)frame);

	if(wchg || hchg)
		mplex_size_changed((WMPlex*)frame, wchg, hchg);
}


bool frame_reparent(WFrame *frame, WWindow *parent, 
					   const WRectangle *geom)
{
	if(!region_same_rootwin((WRegion*)frame, (WRegion*)parent))
		return FALSE;
	
	reparent_or_fit(frame, geom, parent);
	return TRUE;
}


void frame_fit(WFrame *frame, const WRectangle *geom)
{
	reparent_or_fit(frame, geom, NULL);
}


void frame_resize_hints(WFrame *frame, XSizeHints *hints_ret,
						   uint *relw_ret, uint *relh_ret)
{
	WRectangle subgeom;
	uint wdummy, hdummy;
	
	/*if(FRAME_CURRENT(frame)==NULL){*/
		mplex_managed_geom((WMPlex*)frame, &subgeom);
		if(relw_ret!=NULL)
			*relw_ret=subgeom.w;
		if(relh_ret!=NULL)
			*relh_ret=subgeom.h;
	/*}else{
		if(relw_ret!=NULL)
			*relw_ret=REGION_GEOM(FRAME_CURRENT(frame)).w;
		if(relh_ret!=NULL)
			*relh_ret=REGION_GEOM(FRAME_CURRENT(frame)).h;
	}*/
	
	if(FRAME_CURRENT(frame)!=NULL){
		region_resize_hints(FRAME_CURRENT(frame), hints_ret,
							&wdummy, &hdummy);
	}else{
		hints_ret->flags=0;
	}
	
	xsizehints_adjust_for(hints_ret, FRAME_MLIST(frame));
}


/*}}}*/


/*{{{ Focus  */


void frame_inactivated(WFrame *frame)
{
	window_draw((WWindow*)frame, FALSE);
	extl_call_named("call_hook", "soo", NULL,
					"frame_inactivated",
					frame, FRAME_CURRENT(frame));
}


void frame_activated(WFrame *frame)
{
	window_draw((WWindow*)frame, FALSE);
	extl_call_named("call_hook", "soo", NULL,
					"frame_activated",
					frame, FRAME_CURRENT(frame));
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Toggle tab visibility.
 */
EXTL_EXPORT_MEMBER
void frame_toggle_tab(WFrame *frame)
{
	if(frame->flags&FRAME_SHADED)
		return;
	
	frame->flags^=FRAME_TAB_HIDE;
    mplex_size_changed(&(frame->mplex), FALSE, TRUE);
	mplex_fit_managed(&(frame->mplex));
	XClearWindow(ioncore_g.dpy, FRAME_WIN(frame));
	window_draw((WWindow*)frame, TRUE);
}


void frame_notify_managed_change(WFrame *frame, WRegion *sub)
{
	/* TODO: Should only draw/update the affected tab.*/
	update_attrs(frame);
	frame_recalc_bar(frame);
	frame_draw_bar(frame, FALSE);
}


static void frame_size_changed_default(WFrame *frame,
										  bool wchg, bool hchg)
{
	if(wchg)
		frame_recalc_bar(frame);
	/* We should get a request from X to draw the frame... */
}



static void frame_managed_changed(WFrame *frame, int mode, bool sw,
									 WRegion *reg)
{
	if(mode!=MPLEX_CHANGE_SWITCHONLY)
		frame_initialise_titles(frame);
	else
		update_attrs(frame);

	if(sw){
		extl_call_named("call_hook", "soo", NULL,
						"frame_managed_switched",
						frame, FRAME_CURRENT(frame));
		if(frame_set_background(frame, FALSE))
			return;
	}

	frame_draw_bar(frame, mode!=MPLEX_CHANGE_SWITCHONLY);
}


/*}}}*/


/*{{{ Save/load */


bool frame_save_to_file(WFrame *frame, FILE *file, int lvl)
{
	WRegion *sub;
	
	region_save_identity((WRegion*)frame, file, lvl);
	file_indent(file, lvl);
	fprintf(file, "flags = %d,\n", frame->flags);
	
	if(frame->flags&FRAME_SAVED_VERT){
		file_indent(file, lvl);
		fprintf(file, "saved_y = %d, saved_h = %d,\n", 
				frame->saved_y, frame->saved_h);
	}
	if(frame->flags&FRAME_SAVED_HORIZ){
		file_indent(file, lvl);
		fprintf(file, "saved_x = %d, saved_w = %d,\n", 
				frame->saved_x, frame->saved_w);
	}
	
	file_indent(file, lvl);
	fprintf(file, "subs = {\n");
	FOR_ALL_MANAGED_ON_LIST(FRAME_MLIST(frame), sub){
		file_indent(file, lvl+1);
		fprintf(file, "{\n");
		region_save_to_file((WRegion*)sub, file, lvl+2);
		if(sub==FRAME_CURRENT(frame)){
			file_indent(file, lvl+2);
			fprintf(file, "switchto = true,\n");
		}
		file_indent(file, lvl+1);
		fprintf(file, "},\n");
	}
	file_indent(file, lvl);
	fprintf(file, "},\n");
	
	return TRUE;
}


void frame_load_saved_geom(WFrame* frame, ExtlTab tab)
{
	int p=0, s=0;
	
	if(extl_table_gets_i(tab, "saved_x", &p) &&
	   extl_table_gets_i(tab, "saved_w", &s)){
		frame->saved_x=p;
		frame->saved_w=s;
		frame->flags|=FRAME_SAVED_HORIZ;
	}

	if(extl_table_gets_i(tab, "saved_y", &p) &&
	   extl_table_gets_i(tab, "saved_h", &s)){
		frame->saved_y=p;
		frame->saved_h=s;
		frame->flags|=FRAME_SAVED_VERT;
	}
}


void frame_do_load(WFrame *frame, ExtlTab tab)
{
	int flags=0;
	ExtlTab substab, subtab;
	int n, i;
	
	extl_table_gets_i(tab, "flags", &flags);
	
	if(flags&FRAME_TAB_HIDE)
		frame_toggle_tab((WFrame*)frame);

	frame_load_saved_geom((WFrame*)frame, tab);
	
	if(extl_table_gets_t(tab, "subs", &substab)){
		n=extl_table_get_n(substab);
		for(i=1; i<=n; i++){
			if(extl_table_geti_t(substab, i, &subtab)){
				mplex_attach_new((WMPlex*)frame, subtab);
				extl_unref_table(subtab);
			}
		}
		extl_unref_table(substab);
	}
}


/*}}}*/


/*{{{ Dynfuntab and class info */


static DynFunTab frame_dynfuntab[]={
	{region_fit, frame_fit},
	{(DynFun*)region_reparent, (DynFun*)frame_reparent},
	{region_resize_hints, frame_resize_hints},

	{mplex_managed_changed, frame_managed_changed},
	{mplex_size_changed, frame_size_changed_default},
	{region_notify_managed_change, frame_notify_managed_change},
	
	{region_activated, frame_activated},
	{region_inactivated, frame_inactivated},

	{(DynFun*)window_press, (DynFun*)frame_press},
	
	{(DynFun*)region_save_to_file,
	 (DynFun*)frame_save_to_file},

	{(DynFun*)frame_style, 
	 (DynFun*)frame_style_default},
	{(DynFun*)frame_tab_style,
	 (DynFun*)frame_tab_style_default},
	{window_draw, 
	 frame_draw_default},
	{frame_draw_bar, 
	 frame_draw_bar_default},
	{frame_recalc_bar, 
	 frame_recalc_bar_default},
	{frame_bar_geom, 
	 frame_bar_geom_default},
	{frame_border_geom, 
	 frame_border_geom_default},
	{frame_border_inner_geom, 
	 frame_border_inner_geom_default},
	{mplex_managed_geom, 
	 frame_managed_geom_default},

	{region_draw_config_updated, 
	 frame_draw_config_updated},
	
	END_DYNFUNTAB
};
									   

IMPLCLASS(WFrame, WMPlex, frame_deinit, frame_dynfuntab);


/*}}}*/
