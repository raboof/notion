/*
 * ion/frame.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <wmcore/common.h>
#include <wmcore/window.h>
#include <wmcore/global.h>
#include <wmcore/thingp.h>
#include <wmcore/screen.h>
#include <wmcore/focus.h>
#include <wmcore/drawp.h>
#include <wmcore/event.h>
#include <wmcore/targetid.h>
#include <wmcore/attach.h>
#include <wmcore/resize.h>
#include <wmcore/close.h>
#include <wmcore/tags.h>
#include "framep.h"
#include "frame-pointer.h"
#include "bindmaps.h"
#include "splitframe.h"
#include "funtabs.h"


static void deinit_frame(WFrame *frame);
static void frame_remove_sub(WFrame *frame, WRegion *sub);
static bool reparent_frame(WFrame *frame, WWinGeomParams params);
static void fit_frame(WFrame *frame, WRectangle geom);
static void focus_frame(WFrame *frame, bool warp);
static bool frame_switch_subregion(WFrame *frame, WRegion *sub);
static void frame_activated(WFrame *frame);
static void frame_inactivated(WFrame *frame);
static void frame_notify_sub_change(WFrame *frame, WRegion *sub);
static void frame_request_sub_geom(WFrame *frame, WRegion *sub,
								   WRectangle geom, WRectangle *geomret,
								   bool tryonly);

static void frame_sub_params(const WFrame *frame, WWinGeomParams *ret);
static void frame_do_attach(WFrame *frame, WRegion *sub, int flags);
static void frame_do_detach(WFrame *frame, WRegion *sub);

static void frame_resize_hints(WFrame *frame, XSizeHints *hints_ret,
							   uint *relw_ret, uint *relh_ret);
	
static WRegion *frame_selected_sub(WFrame *frame);


static DynFunTab frame_dynfuntab[]={
	{fit_region, fit_frame},
	{(DynFun*)reparent_region, (DynFun*)reparent_frame},
	{focus_region, focus_frame},
	{(DynFun*)switch_subregion, (DynFun*)frame_switch_subregion},
	
	{draw_window, draw_frame},
	{(DynFun*)window_press, (DynFun*)frame_press},
	{(DynFun*)window_release, (DynFun*)frame_release},
	
	{region_activated, frame_activated},
	{region_inactivated, frame_inactivated},
	
	{region_notify_sub_change, frame_notify_sub_change},
	{region_request_sub_geom, frame_request_sub_geom},

	{region_do_attach_params, frame_sub_params},
	{region_do_attach, frame_do_attach},
	
	{region_resize_hints, frame_resize_hints},
	
	{region_remove_sub, frame_remove_sub},

	{(DynFun*)region_selected_sub, (DynFun*)frame_selected_sub},

	{region_request_close, destroy_frame},
	
	END_DYNFUNTAB
};
									   

IMPLOBJ(WFrame, WWindow, deinit_frame, frame_dynfuntab, &ion_frame_funclist)


/*{{{ Helpers */

#define REGION_LABEL(REG)	((REG)->uldata)

#define BAR_X(FRAME, GRDATA) ((GRDATA)->bar_off.x)
#define BAR_Y(FRAME, GRDATA) ((GRDATA)->bar_off.y)
#define BAR_W(FRAME, GRDATA) (REGION_GEOM(FRAME).w+(GRDATA)->bar_off.w)
#define BAR_H(FRAME, GRDATA) ((GRDATA)->bar_h)

#define FRAME_TO_CLIENT_W(W, GRDATA) ((W)+(GRDATA)->client_off.w)
#define FRAME_TO_CLIENT_H(H, GRDATA) ((H)+(GRDATA)->client_off.h)
#define CLIENT_TO_FRAME_W(W, GRDATA) ((W)-(GRDATA)->client_off.w)
#define CLIENT_TO_FRAME_H(H, GRDATA) ((H)-(GRDATA)->client_off.h)

#define CLIENT_X(FRAME, GRDATA) ((GRDATA)->client_off.x)
#define CLIENT_Y(FRAME, GRDATA) ((GRDATA)->client_off.y)
#define CLIENT_W(FRAME, GRDATA) FRAME_TO_CLIENT_W(FRAME_W(FRAME), GRDATA)
#define CLIENT_H(FRAME, GRDATA) FRAME_TO_CLIENT_H(FRAME_H(FRAME), GRDATA)


static WRegion *firstreg_ni(const WFrame *frame)
{
	WRegion *reg=FIRST_THING(frame, WRegion);
	if(reg==frame->current_input && reg!=NULL)
		reg=NEXT_THING(reg, WRegion);
	return reg;
}


static WRegion *lastreg_ni(const WFrame *frame)
{
	WRegion *reg=LAST_THING(frame, WRegion);
	
	if(reg==frame->current_input && reg!=NULL)
		reg=PREV_THING(reg, WRegion);
	return reg;
}


static WRegion *nextreg_ni(const WFrame *frame, const WRegion *reg)
{
	WRegion *rreg=NULL;
	if(reg!=NULL){
		rreg=NEXT_THING(reg, WRegion);
		if(rreg==frame->current_input && rreg!=NULL)
			rreg=NEXT_THING(rreg, WRegion);
	}
	return rreg;
}


static WRegion *prevreg_ni(const WFrame *frame, const WRegion *reg)
{
	WRegion *rreg=NULL;
	if(reg!=NULL){
		rreg=PREV_THING(reg, WRegion);
		if(rreg==frame->current_input && rreg!=NULL)
			rreg=PREV_THING(rreg, WRegion);
	}
	return rreg;
}


WRegion *frame_nthreg_ni(WFrame *frame, uint n)
{
	WRegion *reg=firstreg_ni(frame);
	
	while(n-->0 && reg!=NULL){
		reg=nextreg_ni(frame, reg);
	}
	return reg;
}


/*}}}*/


/*{{{ Destroy/create frame */


static bool init_frame(WFrame *frame, WScreen *scr, WWinGeomParams params,
					   int id, int flags)
{
	Window win;
	XSetWindowAttributes attr;
	WGRData *grdata=&(scr->grdata);
	int sp=grdata->spacing;
	int attrflags=0;
	
	frame->flags=flags;
	frame->sub_count=0;
	frame->current_sub=NULL;
	frame->current_input=NULL;
	frame->saved_w=FRAME_NO_SAVED_WH;
	frame->saved_h=FRAME_NO_SAVED_WH;
	frame->saved_x=FRAME_NO_SAVED_WH;
	frame->saved_y=FRAME_NO_SAVED_WH;
	frame->tab_pressed_sub=NULL;
	
	if(grdata->transparent_background){
		attr.background_pixmap=ParentRelative;
		attrflags=CWBackPixmap;
	}else{
		attr.background_pixel=COLOR_PIXEL(grdata->frame_bgcolor);
		attrflags=CWBackPixel;
	}
	
	win=XCreateWindow(wglobal.dpy, params.win,
					  params.win_x, params.win_y,
					  params.geom.w, params.geom.h,
					  0, CopyFromParent, InputOutput,
					  CopyFromParent, attrflags, &attr);
	
	if(!init_window((WWindow*)frame, scr, win, params.geom)){
		XDestroyWindow(wglobal.dpy, win);
		return FALSE;
	}
	
	frame->target_id=use_target_id((WRegion*)frame, id);
	
	frame->tab_w=BAR_W(frame, grdata);
	
	XSelectInput(wglobal.dpy, win, FRAME_MASK);
	
	region_add_bindmap((WRegion*)frame, &(ion_frame_bindmap), TRUE);
	
	return TRUE;
}


WFrame *create_frame(WScreen *scr, WWinGeomParams params,
					 int id, int flags)
{
	CREATETHING_IMPL(WFrame, frame, (p, scr, params, id, flags));
}


static void deinit_frame(WFrame *frame)
{
	deinit_window((WWindow*)frame);
	free_target_id(frame->target_id);
}


/*}}}*/


/*{{{ Geometry */


void frame_bar_geom(const WFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	
	geom->x=BAR_X(frame, grdata);
	geom->y=BAR_Y(frame, grdata);
	geom->w=BAR_W(frame, grdata);
	geom->h=BAR_H(frame, grdata);
}


void frame_sub_geom(const WFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	
	geom->x=CLIENT_X(frame, grdata);
	geom->y=CLIENT_Y(frame, grdata);
	geom->w=CLIENT_W(frame, grdata);
	geom->h=CLIENT_H(frame, grdata);
}


void frame_sub_params(const WFrame *frame, WWinGeomParams *ret)
{
	frame_sub_geom(frame, &(ret->geom));
	ret->win_x=ret->geom.x;
	ret->win_y=ret->geom.y;
	ret->win=FRAME_WIN(frame);
}


/*}}}*/


/*{{{ Draw */


void frame_recalc_bar(WFrame *frame)
{
	WScreen *scr=SCREEN_OF(frame);
	int bar_w=BAR_W(frame, &(scr->grdata));
	int spc=scr->grdata.frame_border.ipad;
	int tab_w;
	int textw;
	WRegion *sub;
	
	if(frame->sub_count==0){
		frame->tab_w=bar_w;
	}else{
		tab_w=(bar_w-(frame->sub_count-1)*spc)/frame->sub_count;
		frame->tab_w=tab_w;
		
		textw=BORDER_IW(&(scr->grdata.tab_border), tab_w);
		
		FOR_ALL_TYPED(frame, sub, WRegion){
			if(sub==frame->current_input)
				continue;
			REGION_LABEL(sub)=region_make_label(sub, textw,
												scr->grdata.tab_font);
		}
	}
}


void draw_frame(const WFrame *frame, bool complete)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WGRData *grdata=GRDATA_OF(frame);

	dinfo->win=FRAME_WIN(frame);
	dinfo->draw=FRAME_DRAW(frame);

	dinfo->grdata=grdata;
	dinfo->gc=grdata->gc;
	dinfo->geom=grdata->border_off;
	dinfo->geom.w+=FRAME_W(frame);
	dinfo->geom.h+=FRAME_H(frame);
	dinfo->border=&(grdata->frame_border);
	
	if(REGION_IS_ACTIVE(frame))
		dinfo->colors=&(grdata->act_frame_colors);
	else
		dinfo->colors=&(grdata->frame_colors);
	
	if(complete)
		XClearWindow(wglobal.dpy, FRAME_WIN(frame));
	
	do_draw_border(dinfo->win, dinfo->gc, 0, 0,
				   FRAME_W(frame), FRAME_H(frame),
				   grdata->spacing, grdata->spacing,
				   dinfo->colors->bg, dinfo->colors->bg);
	
	draw_border(dinfo);

	/*
	 XSetForeground(wglobal.dpy, XGC, grdata->frame_bgcolor);
	 XFillRectangle(wglobal.dpy, WIN, XGC, C_X, C_Y, C_W, C_H);
	 */
	
	draw_frame_bar(frame, !complete || !grdata->bar_inside_frame);
}


void draw_frame_bar(const WFrame *frame, bool complete)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WRegion *sub, *next;
	WScreen *scr=SCREEN_OF(frame);
	WGRData *grdata=&(scr->grdata);
	WRectangle bg;
	
	frame_bar_geom(frame, &bg);
	
	dinfo->win=FRAME_WIN(frame);
	dinfo->draw=FRAME_DRAW(frame);

	dinfo->grdata=grdata;
	dinfo->gc=grdata->tab_gc;
	dinfo->geom=bg;
	dinfo->border=&(grdata->tab_border);
	dinfo->font=grdata->tab_font;
	
	if(complete){
		if(!grdata->bar_inside_frame){
			if(REGION_IS_ACTIVE(frame))
				set_foreground(wglobal.dpy, XGC, grdata->act_frame_colors.bg);
			else
				set_foreground(wglobal.dpy, XGC, grdata->frame_colors.bg);
				
			XFillRectangle(wglobal.dpy, WIN, XGC, X, Y,
						   bg.w, H+grdata->spacing);
		}else{
			XClearArea(wglobal.dpy, WIN, X, Y, bg.w, H, False);
		}
	}
	
	if(frame->sub_count==0){
		if(REGION_IS_ACTIVE(frame))
			COLORS=&(grdata->act_tab_sel_colors);
		else
			COLORS=&(grdata->tab_sel_colors);
		draw_textbox(dinfo, "<empty frame>", CF_TAB_TEXT_ALIGN, TRUE);
		return;
	}
	
	dinfo->geom.w=frame->tab_w;
	
	for(sub=firstreg_ni(frame); sub!=NULL; sub=next){
		next=nextreg_ni(frame, sub);
		
		if(next==NULL)
			dinfo->geom.w=bg.w-(X-bg.x);
		
		if(REGION_IS_ACTIVE(frame)){
			if(sub==frame->current_sub)
				COLORS=&(grdata->act_tab_sel_colors);
			else
				COLORS=&(grdata->act_tab_colors);
		}else{
			if(sub==frame->current_sub)
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
		
		if(frame->flags&FRAME_TAB_DRAGGED &&
		   sub==frame->tab_pressed_sub){
			/* drag */
			if(grdata->bar_inside_frame){
				if(REGION_IS_ACTIVE(frame)){
					set_foreground(wglobal.dpy, grdata->stipple_gc,
								   grdata->act_frame_colors.bg);
				}else{
					set_foreground(wglobal.dpy, grdata->stipple_gc,
								   grdata->frame_colors.bg);
				}
			}
			XFillRectangle(wglobal.dpy, WIN, grdata->stipple_gc, X, Y, W, H);
		}
		
		X+=frame->tab_w+grdata->tab_spacing;
	}
}


/*}}}*/


/*{{{ Region dynfuns */


static bool reparent_or_fit(WFrame *frame, WWinGeomParams params, bool rep)
{
	bool wchg=(FRAME_W(frame)!=params.geom.w);
	bool hchg=(FRAME_H(frame)!=params.geom.h);
	bool move=(FRAME_X(frame)!=params.geom.x ||
			   FRAME_Y(frame)!=params.geom.y);
	
	if(rep){
		XReparentWindow(wglobal.dpy, FRAME_WIN(frame), params.win,
						params.win_x, params.win_y);
		XResizeWindow(wglobal.dpy, FRAME_WIN(frame),
					  params.geom.w, params.geom.h);
	}else{
		XMoveResizeWindow(wglobal.dpy, FRAME_WIN(frame),
						  params.win_x, params.win_y,
						  params.geom.w, params.geom.h);
	}
	
	REGION_GEOM(frame)=params.geom;

	if(move && !wchg && !hchg)
		notify_subregions_move(&(frame->win.region));
	else if(wchg || hchg)
		frame_fit_subs(frame);

	if(wchg){
		frame->saved_w=FRAME_NO_SAVED_WH;
		frame_recalc_bar(frame);
		/* We should get an exposure event so this should not be needed. */
		/* draw_frame(frame, TRUE); */
	}
	
	if(hchg)
		frame->saved_h=FRAME_NO_SAVED_WH;
	
	return TRUE;
}


static bool reparent_frame(WFrame *frame, WWinGeomParams params)
{
	return reparent_or_fit(frame, params, TRUE);
}


static void fit_frame(WFrame *frame, WRectangle geom)
{
	WWinGeomParams params;
	region_params2((WRegion*)frame, geom, &params);
	reparent_or_fit(frame, params, FALSE);
}


static void focus_frame(WFrame *frame, bool warp)
{
	if(warp)
		do_move_pointer_to((WRegion*)frame);
	
	if(frame->current_input!=NULL)
		focus_region((WRegion*)frame->current_input, FALSE);
	else if(frame->current_sub!=NULL)
		focus_region(frame->current_sub, FALSE);
	else
		SET_FOCUS(FRAME_WIN(frame));
}


static bool frame_switch_subregion(WFrame *frame, WRegion *sub)
{
	if(sub==frame->current_sub || sub==frame->current_input)
		return FALSE;

	if(frame->current_sub!=NULL)
		unmap_region(frame->current_sub);
	frame->current_sub=sub;
	map_region(sub);
	
	if(frame->current_input==NULL){
		if(REGION_IS_ACTIVE(frame))
			set_focus(sub);
	}else{
		region_restack(frame->current_input, None, Above);
#if 0
		if(REGION_IS_ACTIVE(frame)){
			/* For some reason se switch-to subregion seems to be
			 * getting the focus.
			 */
			set_focus(frame->current_input);
		}
#endif
	}

	/* Many programs will get upset if the visible, although only such,
	 * client window is not the lowest window in the frame. xprop/xwininfo
	 * will return the information for the lowest window. 'netscape -remote'
	 * will not work at all if there are no visible netscape windows.
	 */
	region_restack(sub, None, Below);

	draw_frame_bar(frame, TRUE);
	
	return TRUE;
}


static void frame_inactivated(WFrame *frame)
{
	draw_frame(frame, FALSE);
}


static void frame_activated(WFrame *frame)
{
	draw_frame(frame, FALSE);
}


static void frame_notify_sub_change(WFrame *frame, WRegion *sub)
{
	frame_recalc_bar(frame);
	draw_frame_bar(frame, FALSE);
}


static void frame_request_sub_geom(WFrame *frame, WRegion *sub,
								   WRectangle geom, WRectangle *geomret,
								   bool tryonly)
{
	/* Just try to give it the maximum size */
	frame_sub_geom(frame, &geom);
	
	if(geomret!=NULL)
		*geomret=geom;
	
	fit_region(sub, geom);
}


static void frame_resize_hints(WFrame *frame, XSizeHints *hints_ret,
							   uint *relw_ret, uint *relh_ret)
{
	WRectangle subgeom;
	XSizeHints hints2;
	uint wdummy, hdummy;
	WScreen *scr=SCREEN_OF(frame);
	
	frame_sub_geom(frame, &subgeom);
	
	*relw_ret=subgeom.w;
	*relh_ret=subgeom.h;
	
	hints_ret->flags=PResizeInc|PMinSize;
	hints_ret->width_inc=scr->w_unit;
	hints_ret->height_inc=scr->h_unit;
	hints_ret->min_width=FRAME_MIN_W(scr);
	hints_ret->min_height=FRAME_MIN_H(scr);
	
	if(frame->current_sub==NULL)
		return;
	
	region_resize_hints(frame->current_sub, &hints2, &wdummy, &hdummy);
		
	if(hints2.flags&PResizeInc){
		hints_ret->width_inc=hints2.width_inc;
		hints_ret->height_inc=hints2.height_inc;
	}
}


/*}}}*/


/*{{{ Client switching */


static WRegion *nth_sub(WFrame *frame, uint n)
{
	return frame_nthreg_ni(frame, n);
}


static WRegion *next_sub(WFrame *frame)
{
	WRegion *reg=NULL;
	if(frame->current_sub!=NULL)
		reg=nextreg_ni(frame, frame->current_sub);
	if(reg==NULL)
		reg=firstreg_ni(frame);
	return reg;
}


static WRegion *prev_sub(WFrame *frame)
{
	WRegion *reg=NULL;
	if(frame->current_sub!=NULL)
		reg=prevreg_ni(frame, frame->current_sub);
	if(reg==NULL)
		reg=lastreg_ni(frame);
	return reg;
}


void frame_switch_nth(WFrame *frame, uint n)
{
	WRegion *sub=nth_sub(frame, n);
	if(sub!=NULL)
		switch_region(sub);
}

	   
void frame_switch_next(WFrame *frame)
{
	WRegion *sub=next_sub(frame);
	if(sub!=NULL)
		switch_region(sub);
}


void frame_switch_prev(WFrame *frame)
{
	WRegion *sub=prev_sub(frame);
	if(sub!=NULL)
		switch_region(sub);
}


/*}}}*/


/*{{{ Attach, detach & fit */


static void frame_do_attach(WFrame *frame, WRegion *sub, int flags)
{
	set_target_id(sub, frame->target_id);
	
	if(frame->current_sub!=NULL && wglobal.opmode!=OPMODE_INIT)
		link_thing_after((WThing*)(frame->current_sub), (WThing*)sub);
	else
		link_thing((WThing*)frame, (WThing*)sub);
	frame->sub_count++;
	
	if(frame->sub_count==1)
		flags|=REGION_ATTACH_SWITCHTO;

	frame_recalc_bar(frame);

	if(flags&REGION_ATTACH_SWITCHTO){
		frame_switch_subregion(frame, sub);
	}else{
		unmap_region(sub);
		draw_frame_bar(frame, TRUE);
	}

}


void frame_attach_sub(WFrame *frame, WRegion *sub, int flags)
{
	region_attach_sub((WRegion*)frame, sub, flags);
	/*
	WWinGeomParams params;

	frame_sub_params(frame, &params);
	detach_reparent_region(sub, params);
	
	frame_do_attach(frame, sub, flags);
	 */
}


void frame_move_subs(WFrame *dest, WFrame *src)
{
	WRegion *sub, *next, *cur;
	
	cur=src->current_sub;
	
	for(sub=firstreg_ni(src); sub!=NULL; sub=next){
		next=nextreg_ni(src, sub);
		frame_attach_sub(dest, sub,
						 sub==cur ? REGION_ATTACH_SWITCHTO : 0);
	}
}


void frame_fit_subs(WFrame *frame)
{
	WRectangle geom;
	WRegion *sub;
	
	frame_sub_geom(frame, &geom);
	
	FOR_ALL_TYPED(frame, sub, WRegion){
		fit_region(sub, geom);
	}
}


static void frame_do_detach(WFrame *frame, WRegion *sub)
{
	WRegion *next=NULL;

	if(frame->tab_pressed_sub==sub)
		frame->tab_pressed_sub=NULL;
	
	if(frame->current_sub==sub){
		next=prevreg_ni(frame, sub);
		if(next==NULL)
			next=nextreg_ni(frame, sub);
		frame->current_sub=NULL;
	}
	
	unlink_thing((WThing*)sub);
	frame->sub_count--;
	
	if(wglobal.opmode!=OPMODE_DEINIT){
		frame_recalc_bar(frame);
		if(next!=NULL)
			frame_switch_subregion(frame, next);
		else
			draw_frame_bar(frame, TRUE);
	}

	if(REGION_LABEL(sub)!=NULL){
		free(REGION_LABEL(sub));
		REGION_LABEL(sub)=NULL;
	}
}


static void frame_remove_sub(WFrame *frame, WRegion *sub)
{
	if(frame->current_input==sub){
		frame->current_input=NULL;
		if(REGION_IS_ACTIVE(frame))
			set_focus((WRegion*)frame);
	}else{
		frame_do_detach(frame, sub);
	}
}


/*}}}*/


/*{{{ Inputs */


WRegion *frame_current_input(WFrame *frame)
{
	return frame->current_input;
}


WRegion *frame_attach_input_new(WFrame *frame, WRegionCreateFn *fn,
								void *fnp)
{
	WWinGeomParams params;
	WRegion *sub;
	
	if(frame->current_input!=NULL)
		return NULL;
	
	frame_sub_params(frame, &params);
	
	sub=fn(SCREEN_OF(frame), params, fnp);

	frame->current_input=sub;
	
	if(sub!=NULL)
		link_thing((WThing*)frame, (WThing*)sub);
	
	return sub;
}


/*}}}*/


/*{{{ Misc. */


void frame_attach_tagged(WFrame *frame)
{
	WRegion *reg;
	
	while((reg=tag_take_first())!=NULL){
		if(thing_is_ancestor((WThing*)frame, (WThing*)reg)){
			warn("Cannot attach tagged region: ancestor");
			continue;
		}
		frame_attach_sub(frame, reg, 0);
	}
}


WFrame *find_frame_of(Window win)
{
	WRegion *reg=FIND_WINDOW_T(win, WRegion);
	
	/* NOTE: FIND_PARENT may return reg. */
	return FIND_PARENT(reg, WFrame);
}


WRegion *frame_selected_sub(WFrame *frame)
{
	return frame->current_sub;
}


/*}}}*/


/*{{{ Tab reordering */


void frame_move_current_tab_right(WFrame *frame)
{
	WRegion *reg, *next;
	
	if((reg=frame->current_sub)==NULL)
		return;
	if((next=nextreg_ni(frame, reg))==NULL)
		return;
	
	unlink_thing((WThing*)reg);
	link_thing_after((WThing*)next, (WThing*)reg);
	draw_frame_bar(frame, TRUE);
}


void frame_move_current_tab_left(WFrame *frame)
{
	WRegion *reg, *prev;
	
	if((reg=frame->current_sub)==NULL)
		return;
	if((prev=prevreg_ni(frame, reg))==NULL)
		return;
	
	unlink_thing((WThing*)reg);
	link_thing_before((WThing*)prev, (WThing*)reg);
	draw_frame_bar(frame, TRUE);
}

/*}}}*/

