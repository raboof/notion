/*
 * ion/frame.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <libtu/parser.h>
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
#include <wmcore/tags.h>
#include <wmcore/names.h>
#include <wmcore/saveload.h>
#include "framep.h"
#include "frame-pointer.h"
#include "bindmaps.h"
#include "splitframe.h"
#include "funtabs.h"


static void deinit_frame(WFrame *frame);
static bool reparent_frame(WFrame *frame, WRegion *parent, WRectangle geom);
static void fit_frame(WFrame *frame, WRectangle geom);
static void focus_frame(WFrame *frame, bool warp);
static bool frame_display_managed(WFrame *frame, WRegion *sub);
static void frame_activated(WFrame *frame);
static void frame_inactivated(WFrame *frame);
static void frame_notify_managed_change(WFrame *frame, WRegion *sub);
static void frame_resize_hints(WFrame *frame, XSizeHints *hints_ret,
							   uint *relw_ret, uint *relh_ret);
static WRegion *frame_selected_sub(WFrame *frame);
static void frame_draw_config_updated(WFrame *frame);


static void frame_managed_geom(const WFrame *frame, WRectangle *geom);
static void frame_add_managed_params(const WFrame *frame, WRegion **par,
									 WRectangle *geom);
static void frame_add_managed_doit(WFrame *frame, WRegion *sub, int flags);
static void frame_remove_managed(WFrame *frame, WRegion *sub);
static void frame_request_managed_geom(WFrame *frame, WRegion *sub,
									   WRectangle geom, WRectangle *geomret,
									   bool tryonly);

static bool frame_save_to_file(WFrame *frame, FILE *file, int lvl);
	
static DynFunTab frame_dynfuntab[]={
	{fit_region, fit_frame},
	{(DynFun*)reparent_region, (DynFun*)reparent_frame},
	{focus_region, focus_frame},
	{(DynFun*)region_display_managed, (DynFun*)frame_display_managed},
	
	{draw_window, draw_frame},
	{(DynFun*)window_press, (DynFun*)frame_press},
	{(DynFun*)window_release, (DynFun*)frame_release},
	
	{region_activated, frame_activated},
	{region_inactivated, frame_inactivated},
	
	{region_resize_hints, frame_resize_hints},
	
	{(DynFun*)region_selected_sub, (DynFun*)frame_selected_sub},

	{region_add_managed_params, frame_add_managed_params},
	{region_add_managed_doit, frame_add_managed_doit},
	{region_remove_managed, frame_remove_managed},
	{region_notify_managed_change, frame_notify_managed_change},
	{region_request_managed_geom, frame_request_managed_geom},
	
	{region_draw_config_updated, frame_draw_config_updated},

	{(DynFun*)region_save_to_file, (DynFun*)frame_save_to_file},
	
	END_DYNFUNTAB
};
									   

IMPLOBJ(WFrame, WWindow, deinit_frame, frame_dynfuntab, &ion_frame_funclist)


/*{{{ Helpers */

#define REGION_LABEL(REG)	((REG)->mgr_data)

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


/*}}}*/


/*{{{ Destroy/create frame */


static bool init_frame(WFrame *frame, WRegion *parent, WRectangle geom,
					   int id, int flags)
{
	Window win;
	XSetWindowAttributes attr;
	WGRData *grdata=GRDATA_OF(parent);
	int sp=grdata->spacing;
	ulong attrflags=0;
	
	if(!WTHING_IS(parent, WWindow))
		return FALSE;
	
	frame->flags=flags;
	frame->managed_count=0;
	frame->managed_list=NULL;
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
	
	win=XCreateWindow(wglobal.dpy, ((WWindow*)parent)->win,
					  geom.x, geom.y, geom.w, geom.h,
					  0, CopyFromParent, InputOutput,
					  CopyFromParent, attrflags, &attr);
	
	if(!init_window((WWindow*)frame, (WWindow*)parent, win, geom)){
		XDestroyWindow(wglobal.dpy, win);
		return FALSE;
	}
	((WRegion*)frame)->flags|=REGION_HANDLES_MANAGED_ENTER_FOCUS;
	
	frame->target_id=use_target_id((WRegion*)frame, id);
	
	XSelectInput(wglobal.dpy, win, FRAME_MASK);
	
	region_add_bindmap((WRegion*)frame, &(ion_frame_bindmap), TRUE);
	
	return TRUE;
}


WFrame *create_frame(WRegion *parent, WRectangle geom, int id, int flags)
{
	CREATETHING_IMPL(WFrame, frame, (p, parent, geom, id, flags));
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


void frame_managed_geom(const WFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	
	geom->x=CLIENT_X(frame, grdata);
	geom->y=CLIENT_Y(frame, grdata);
	geom->w=CLIENT_W(frame, grdata);
	geom->h=CLIENT_H(frame, grdata);
}


/*}}}*/


/*{{{ Draw */


int frame_tab_at_x(const WFrame *frame, int x)
{
	WGRData *grdata=GRDATA_OF(frame);
	int bar_w=BAR_W(frame, grdata);
	int bar_x=BAR_X(frame, grdata);
	
	x-=bar_x;
	
	if(x<0 || bar_w==0)
		return -1;
	
	return (x*frame->managed_count)/bar_w;
}


int frame_nth_tab_x(const WFrame *frame, int n)
{
	WGRData *grdata=GRDATA_OF(frame);
	int bar_w=BAR_W(frame, grdata);
	int bar_x=BAR_X(frame, grdata);
	
	if(frame->managed_count==0)
		return bar_x;
	
	return (n*bar_w)/frame->managed_count+bar_x;
}


int frame_nth_tab_w(const WFrame *frame, int n)
{
	WGRData *grdata=GRDATA_OF(frame);
	int spc=grdata->tab_spacing;
	int bar_w=BAR_W(frame, grdata);
	int bar_x=BAR_X(frame, grdata);
	int start=frame_nth_tab_x(frame, n);
	
	if(n==frame->managed_count-1)
		return bar_w+bar_x-start;
	else
		return frame_nth_tab_x(frame, n+1)-spc-start;
}


void frame_recalc_bar(WFrame *frame)
{
	WScreen *scr=SCREEN_OF(frame);
	int bar_w=BAR_W(frame, &(scr->grdata));
	int tab_w, textw, n;
	WRegion *sub;
	
	if(frame->managed_count==0){
		frame->tab_w=bar_w;
	}else{
		n=0;
		FOR_ALL_MANAGED_ON_LIST(frame->managed_list, sub){
			tab_w=frame_nth_tab_w(frame, n++);
			textw=BORDER_IW(&(scr->grdata.tab_border), tab_w);
			REGION_LABEL(sub)=region_make_label(sub, textw,
												scr->grdata.tab_font);
		}
	}
}


static void frame_notify_managed_change(WFrame *frame, WRegion *sub)
{
	frame_recalc_bar(frame);
	draw_frame_bar(frame, FALSE);
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
	int n;
	
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
	
	if(frame->managed_count==0){
		if(REGION_IS_ACTIVE(frame))
			COLORS=&(grdata->act_tab_sel_colors);
		else
			COLORS=&(grdata->tab_sel_colors);
		draw_textbox(dinfo, "<empty frame>", CF_TAB_TEXT_ALIGN, TRUE);
		return;
	}
	
	n=0;
	FOR_ALL_MANAGED_ON_LIST(frame->managed_list, sub){
		dinfo->geom.w=frame_nth_tab_w(frame, n);
		dinfo->geom.x=frame_nth_tab_x(frame, n);
		n++;
		
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
	}
}


static void frame_draw_config_updated(WFrame *frame)
{
	WGRData *grdata=GRDATA_OF(frame);
	XSetWindowAttributes attr;
	ulong attrflags;
	WRegion *sub;
	WRectangle geom;
	
	if(grdata->transparent_background){
		attr.background_pixmap=ParentRelative;
		attrflags=CWBackPixmap;
	}else{
		attr.background_pixel=COLOR_PIXEL(grdata->frame_bgcolor);
		attrflags=CWBackPixel;
	}
	
	XChangeWindowAttributes(wglobal.dpy, frame->win.win,
							attrflags, &attr);
	
	frame_managed_geom(frame, &geom);
	
	FOR_ALL_TYPED(frame, sub, WRegion){
		region_draw_config_updated(sub);
		if(REGION_MANAGER(sub)==(WRegion*)frame)
			fit_region(sub, geom);
	}
	
	frame_recalc_bar(frame);
	draw_frame(frame, TRUE);
}


/*}}}*/


/*{{{ Resize and reparent */


static void reparent_or_fit(WFrame *frame, WRectangle geom, WWindow *parent)
{
	bool wchg=(FRAME_W(frame)!=geom.w);
	bool hchg=(FRAME_H(frame)!=geom.h);
	bool move=(FRAME_X(frame)!=geom.x ||
			   FRAME_Y(frame)!=geom.y);
	
	if(parent!=NULL){
		XReparentWindow(wglobal.dpy, FRAME_WIN(frame), parent->win,
						geom.x, geom.y);
		XResizeWindow(wglobal.dpy, FRAME_WIN(frame), geom.w, geom.h);
	}else{
		XMoveResizeWindow(wglobal.dpy, FRAME_WIN(frame),
						  geom.x, geom.y, geom.w, geom.h);
	}
	
	REGION_GEOM(frame)=geom;

	if(move && !wchg && !hchg)
		notify_subregions_move(&(frame->win.region));
	else if(wchg || hchg)
		frame_fit_managed(frame);

	if(wchg){
		frame->saved_w=FRAME_NO_SAVED_WH;
		frame_recalc_bar(frame);
		/* We should get an exposure event so this should not be needed. */
		/* draw_frame(frame, TRUE); */
	}
	
	if(hchg)
		frame->saved_h=FRAME_NO_SAVED_WH;
}


static bool reparent_frame(WFrame *frame, WRegion *parent, WRectangle geom)
{
	if(!WTHING_IS(parent, WWindow) || !same_screen((WRegion*)frame, parent))
		return FALSE;
	
	region_detach_parent((WRegion*)frame);
	region_set_parent((WRegion*)frame, parent);
	reparent_or_fit(frame, geom, (WWindow*)parent);
	
	return TRUE;
}


static void fit_frame(WFrame *frame, WRectangle geom)
{
	reparent_or_fit(frame, geom, NULL);
}



static void frame_request_managed_geom(WFrame *frame, WRegion *sub,
									   WRectangle geom, WRectangle *geomret,
									   bool tryonly)
{
	/* Just try to give it the maximum size */
	frame_managed_geom(frame, &geom);
	
	if(geomret!=NULL)
		*geomret=geom;
	
	if(!tryonly)
		fit_region(sub, geom);
}


static void frame_resize_hints(WFrame *frame, XSizeHints *hints_ret,
							   uint *relw_ret, uint *relh_ret)
{
	WRectangle subgeom;
	XSizeHints hints2;
	uint wdummy, hdummy;
	WScreen *scr=SCREEN_OF(frame);
	
	frame_managed_geom(frame, &subgeom);
	
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


WRegion *frame_nth_managed(WFrame *frame, uint n)
{
	WRegion *reg=FIRST_MANAGED(frame->managed_list);
	
	while(n-->0 && reg!=NULL)
		reg=NEXT_MANAGED(frame->managed_list, reg);

	return reg;
}


void frame_switch_nth(WFrame *frame, uint n)
{
	WRegion *sub=frame_nth_managed(frame, n);
	if(sub!=NULL)
		display_region_sp(sub);
}

	   
void frame_switch_next(WFrame *frame)
{
	WRegion *sub=NEXT_MANAGED_WRAP(frame->managed_list, frame->current_sub);
	if(sub!=NULL)
		display_region_sp(sub);
}


void frame_switch_prev(WFrame *frame)
{
	WRegion *sub=PREV_MANAGED_WRAP(frame->managed_list, frame->current_sub);
	if(sub!=NULL)
		display_region_sp(sub);
}


/*}}}*/


/*{{{ Add/remove managed */


static void frame_add_managed_params(const WFrame *frame, WRegion **par,
									 WRectangle *geom)
{
	frame_managed_geom(frame, geom);
	*par=(WRegion*)frame;
}


static void frame_add_managed_doit(WFrame *frame, WRegion *sub, int flags)
{
	set_target_id(sub, frame->target_id);
	
	if(frame->current_sub!=NULL && wglobal.opmode!=OPMODE_INIT){
		region_set_manager(sub, (WRegion*)frame, NULL);
		LINK_ITEM_AFTER(frame->managed_list, frame->current_sub,
						sub, mgr_next, mgr_prev);
	}else{
		region_set_manager(sub, (WRegion*)frame, &(frame->managed_list));
	}
	
	frame->managed_count++;
	
	if(frame->managed_count==1)
		flags|=REGION_ATTACH_SWITCHTO;

	frame_recalc_bar(frame);

	if(flags&REGION_ATTACH_SWITCHTO){
		frame_display_managed(frame, sub);
	}else{
		unmap_region(sub);
		draw_frame_bar(frame, TRUE);
	}

}


/*
void frame_move_managed(WFrame *dest, WFrame *src)
{
	WRegion *sub, *next, *cur;
	
	cur=src->current_sub;
	
	for(sub=FIRST_MANAGED(src->managed_list); sub!=NULL; sub=next){
		next=NEXT_MANAGED(src->managed_list, sub);
		region_add_managed((WRegion*)dest, sub,
						   sub==cur ? REGION_ATTACH_SWITCHTO : 0);
	}
}
*/

void frame_fit_managed(WFrame *frame)
{
	WRectangle geom;
	WRegion *sub;
	
	frame_managed_geom(frame, &geom);
	
	FOR_ALL_MANAGED_ON_LIST(frame->managed_list, sub){
		fit_region(sub, geom);
	}
	
	if(frame->current_input!=NULL)
		fit_region(frame->current_input, geom);
}


static void frame_do_remove(WFrame *frame, WRegion *sub)
{
	WRegion *next=NULL;

	if(frame->tab_pressed_sub==sub)
		frame->tab_pressed_sub=NULL;
	
	if(frame->current_sub==sub){
		next=PREV_MANAGED(frame->managed_list, sub);
		if(next==NULL)
			next=NEXT_MANAGED(frame->managed_list, sub);
		frame->current_sub=NULL;
	}
	
	region_unset_manager(sub, (WRegion*)frame, &(frame->managed_list));
	frame->managed_count--;
	
	if(wglobal.opmode!=OPMODE_DEINIT){
		frame_recalc_bar(frame);
		if(next!=NULL)
			frame_display_managed(frame, next);
		else
			draw_frame_bar(frame, TRUE);
	}

	if(REGION_LABEL(sub)!=NULL){
		free(REGION_LABEL(sub));
		REGION_LABEL(sub)=NULL;
	}
}


static void frame_remove_managed(WFrame *frame, WRegion *reg)
{
	if(frame->current_input==reg){
		region_unset_manager(reg, (WRegion*)frame, NULL);
		frame->current_input=NULL;
		if(REGION_IS_ACTIVE(frame))
			set_focus((WRegion*)frame);
	}else{
		frame_do_remove(frame, reg);
	}
}


/*}}}*/


/*{{{ Focus and managed switching */


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


static bool frame_display_managed(WFrame *frame, WRegion *sub)
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


/*}}}*/


/*{{{ Inputs */


WRegion *frame_current_input(WFrame *frame)
{
	return frame->current_input;
}


WRegion *frame_attach_input_new(WFrame *frame, WRegionCreateFn *fn,
								void *fnp)
{
	WRectangle geom;
	WRegion *sub;
	
	if(frame->current_input!=NULL)
		return NULL;
	
	frame_managed_geom(frame, &geom);
	sub=fn((WRegion*)frame, geom, fnp);

	if(sub==NULL)
		return NULL;
	
	frame->current_input=sub;
	region_set_manager(sub, (WRegion*)frame, NULL);

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
		region_add_managed((WRegion*)frame, reg, 0);
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
	if((next=NEXT_MANAGED(frame->managed_list, reg))==NULL)
		return;
	
	UNLINK_ITEM(frame->managed_list, reg, mgr_next, mgr_prev);
	LINK_ITEM_AFTER(frame->managed_list, next, reg, mgr_next, mgr_prev);
	
	draw_frame_bar(frame, TRUE);
}


void frame_move_current_tab_left(WFrame *frame)
{
	WRegion *reg, *prev;
	
	if((reg=frame->current_sub)==NULL)
		return;
	if((prev=PREV_MANAGED(frame->managed_list, reg))==NULL)
		return;

	UNLINK_ITEM(frame->managed_list, reg, mgr_next, mgr_prev);
	LINK_ITEM_BEFORE(frame->managed_list, prev, reg, mgr_next, mgr_prev);

	draw_frame_bar(frame, TRUE);
}

/*}}}*/


/*{{{ Save/load */


static bool frame_save_to_file(WFrame *frame, FILE *file, int lvl)
{
	begin_saved_region((WRegion*)frame, file, lvl);
	save_indent_line(file, lvl);
	fprintf(file, "target_id %d\n", frame->target_id);
	end_saved_region((WRegion*)frame, file, lvl);
	return TRUE;
}


static int load_target_id;


static bool opt_target_id(Tokenizer *tokz, int n, Token *toks)
{
	load_target_id=TOK_LONG_VAL(&(toks[1]));
	return TRUE;
}


static ConfOpt frame_opts[]={
	{"target_id", "l", opt_target_id, NULL},
	END_CONFOPTS
};


WRegion *frame_load(WRegion *par, WRectangle geom, Tokenizer *tokz)
{
	load_target_id=0;
	
	if(!parse_config_tokz(tokz, frame_opts))
		return NULL;
	
	return (WRegion*)create_frame(par, geom, load_target_id, 0);
}


/*}}}*/

