/*
 * ion/ioncore/frame-pointer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include "common.h"
#include "global.h"
#include "pointer.h"
#include "cursor.h"
#include "focus.h"
#include "attach.h"
#include "resize.h"
#include "grab.h"
#include "frame.h"
#include "framep.h"
#include "frame-pointer.h"
#include "frame-draw.h"
#include "objp.h"
#include "bindmaps.h"
#include "infowin.h"
#include "defer.h"


static int p_tab_x=0, p_tab_y=0, p_tabnum=-1;
static int p_dx1mul=0, p_dx2mul=0, p_dy1mul=0, p_dy2mul=0;
static WInfoWin *tabdrag_infowin=NULL;


/*{{{ Frame press */

#define MINCORNER 16


static WRegion *sub_at_tab(WFrame *frame)
{
	return mplex_nth_managed((WMPlex*)frame, p_tabnum);
}


int frame_press(WFrame *frame, XButtonEvent *ev, WRegion **reg_ret)
{
	WRegion *sub;
	WRectangle g;
	
	p_dx1mul=0;
	p_dx2mul=0;
	p_dy1mul=0;
	p_dy2mul=0;
	p_tabnum=-1;


	/* Check resize directions */
	{
		int ww=REGION_GEOM(frame).w/2;
		int hh=REGION_GEOM(frame).h/2;
		int xdiv, ydiv;
		int tmpx, tmpy, atmpx, atmpy;
		
		tmpx=ev->x-ww;
		tmpy=hh-ev->y;
		xdiv=ww/2;
		ydiv=hh/2;
		atmpx=abs(tmpx);
		atmpy=abs(tmpy);
		
		if(xdiv<MINCORNER && xdiv>1){
			xdiv=ww-MINCORNER;
			if(xdiv<1)
				xdiv=1;
		}

		if(ydiv<MINCORNER && ydiv>1){
			ydiv=hh-MINCORNER;
			if(ydiv<1)
				ydiv=1;
		}

		if(xdiv==0){
			p_dx2mul=1;
		}else if(hh*atmpx/xdiv>=tmpy && -hh*atmpx/xdiv<=tmpy){
			p_dx1mul=(tmpx<0);
			p_dx2mul=(tmpx>=0);
		}

		if(ydiv==0){
			p_dy2mul=1;
		}else if(ww*atmpy/ydiv>=tmpx && -ww*atmpy/ydiv<=tmpx){
			p_dy1mul=(tmpy>0);
			p_dy2mul=(tmpy<=0);
		}
	}
	
	/* Check tab */
	
	frame_bar_geom(frame, &g);
	
	/* Borders act like tabs at top of the parent region */
	if(REGION_GEOM(frame).y==0){
		g.h+=g.y;
		g.y=0;
	}

	if(coords_in_rect(&g, ev->x, ev->y)){
		p_tabnum=frame_tab_at_x(frame, ev->x);

		region_rootpos((WRegion*)frame, &p_tab_x, &p_tab_y);
		p_tab_x+=frame_nth_tab_x(frame, p_tabnum);
		p_tab_y+=g.y;
		
		sub=mplex_nth_managed(&(frame->mplex), p_tabnum);

		if(reg_ret!=NULL)
			*reg_ret=sub;
		
		return WFRAME_AREA_TAB;
	}
	

	/* Check border */
	
	frame_border_inner_geom(frame, &g);
	
	if(coords_in_rect(&g, ev->x, ev->y))
		return WFRAME_AREA_CLIENT;
	
	return WFRAME_AREA_BORDER;
}


/*}}}*/


/*{{{ Tab drag */


static ExtlSafelist tabdrag_safe_funclist[]={
	(ExtlExportedFn*)&mplex_switch_nth,
	(ExtlExportedFn*)&mplex_switch_next,
	(ExtlExportedFn*)&mplex_switch_prev,
	NULL
};


#define BUTTONS_MASK \
	(Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)


static bool tabdrag_kbd_handler(WRegion *reg, XEvent *xev)
{
	XKeyEvent *ev=&xev->xkey;
	WBinding *binding=NULL;
	WBindmap **bindptr;
	
	if(ev->type==KeyRelease)
		return FALSE;
	
	assert(reg!=NULL);

	binding=lookup_binding(&ioncore_rootwin_bindmap, ACT_KEYPRESS,
						   ev->state&~BUTTONS_MASK, ev->keycode);
	
	if(binding!=NULL && binding->func!=extl_fn_none()){
		const ExtlSafelist *old_safelist=
			extl_set_safelist(tabdrag_safe_funclist);
		extl_call(binding->func, "o", NULL, region_screen_of(reg));
		extl_set_safelist(old_safelist);
	}
	
	return FALSE;
}


static void setup_dragwin(WFrame *frame, uint tab)
{
	WRectangle g;
	
	assert(tabdrag_infowin==NULL);
	
	g.x=p_tab_x;
	g.y=p_tab_y;
	g.w=frame_nth_tab_w(frame, tab);
	g.h=frame->bar_h;
	
	tabdrag_infowin=create_infowin((WWindow*)ROOTWIN_OF(frame), &g, 
								   frame_tab_style(frame));
	
	if(tabdrag_infowin==NULL)
		return;
	
	infowin_set_attr2(tabdrag_infowin, (REGION_IS_ACTIVE(frame) 
										? "active" : "inactive"),
					  frame->titles[tab].attr);
	
	if(frame->titles[tab].text!=NULL){
		char *buf=WINFOWIN_BUFFER(tabdrag_infowin);
		strncpy(buf, frame->titles[tab].text, WINFOWIN_BUFFER_LEN-1);
		buf[WINFOWIN_BUFFER_LEN-1]='\0';
	}
}


static void p_tabdrag_motion(WFrame *frame, XMotionEvent *ev,
							 int dx, int dy)
{
	WRootWin *rootwin=ROOTWIN_OF(frame);

	p_tab_x+=dx;
	p_tab_y+=dy;
	
	if(tabdrag_infowin!=NULL){
		WRectangle g;
		g.x=p_tab_x;
		g.y=p_tab_y;
		g.w=REGION_GEOM(tabdrag_infowin).w;
		g.h=REGION_GEOM(tabdrag_infowin).h;
		region_fit((WRegion*)tabdrag_infowin, &g);
	}
}


static void p_tabdrag_begin(WFrame *frame, XMotionEvent *ev,
							int dx, int dy)
{
	WRootWin *rootwin=ROOTWIN_OF(frame);

	if(p_tabnum<0)
		return;
	
	change_grab_cursor(CURSOR_DRAG);
		
	setup_dragwin(frame, p_tabnum);
	
	frame->tab_dragged_idx=p_tabnum;
	frame_update_attr_nth(frame, p_tabnum);

	frame_draw_bar(frame, FALSE);
	
	p_tabdrag_motion(frame, ev, dx, dy);
	
	if(tabdrag_infowin!=NULL)
		window_map((WWindow*)tabdrag_infowin);
}


static WRegion *fnd(Window root, int x, int y)
{
	Window win=root;
	int dstx, dsty;
	WRegion *reg=NULL;
	WWindow *w=NULL;
	WScreen *scr;
	
	FOR_ALL_SCREENS(scr){
		if(ROOT_OF(scr)==root &&
		   coords_in_rect(&REGION_GEOM(scr), x, y)){
			break;
		}
	}
	
	w=(WWindow*)scr;
	
	while(w!=NULL){
		if(HAS_DYN(w, region_handle_drop))
			reg=(WRegion*)w;
		
		if(!XTranslateCoordinates(wglobal.dpy, root, w->win,
								  x, y, &dstx, &dsty, &win)){
			return NULL;
		}
		
		w=FIND_WINDOW_T(win, WWindow);
		x=dstx;
		y=dsty;
	}

	return reg;
}


static void tabdrag_deinit(WFrame *frame)
{
	int idx=frame->tab_dragged_idx;
	frame->tab_dragged_idx=-1;
	frame_update_attr_nth(frame, idx);
	
	if(tabdrag_infowin!=NULL){
		destroy_obj((WObj*)tabdrag_infowin);
		tabdrag_infowin=NULL;
	}
}


static void tabdrag_killed(WFrame *frame)
{
	tabdrag_deinit(frame);
	if(!WOBJ_IS_BEING_DESTROYED(frame))
		frame_draw_bar(frame, TRUE);
}


static void p_tabdrag_end(WFrame *frame, XButtonEvent *ev)
{
	WRegion *sub=NULL;
	WRegion *dropped_on;
	Window win=None;

	sub=sub_at_tab(frame);
	
	tabdrag_deinit(frame);
	
	/* Must be same root window */
	if(sub==NULL || ev->root!=ROOT_OF(sub))
		return;
	
	dropped_on=fnd(ev->root, ev->x_root, ev->y_root);

	if(dropped_on==NULL || dropped_on==(WRegion*)frame || dropped_on==sub){
		frame_draw_bar(frame, TRUE);
		return;
	}
	
	if(region_handle_drop(dropped_on, p_tab_x, p_tab_y, sub))
		set_focus(dropped_on);
}


/*EXTL_DOC
 * Start dragging the tab that the user pressed on with the pointing device.
 * This function should only be used by binding it to \emph{mpress} or
 * \emph{mdrag} action with area ''tab''.
 */
EXTL_EXPORT_MEMBER
void frame_p_tabdrag(WFrame *frame)
{
	if(p_tabnum<0)
		return;
	
	p_set_drag_handlers((WRegion*)frame,
						(WMotionHandler*)p_tabdrag_begin,
						(WMotionHandler*)p_tabdrag_motion,
						(WButtonHandler*)p_tabdrag_end,
						tabdrag_kbd_handler,
						(GrabKilledHandler*)tabdrag_killed);
}


bool region_handle_drop(WRegion *reg, int x, int y, WRegion *dropped)
{
	bool ret=FALSE;
	CALL_DYN_RET(ret, bool, region_handle_drop, reg, (reg, x, y, dropped));
	return ret;
}
	

/*}}}*/


/*{{{ Resize */


static void p_moveres_end(WFrame *frame, XButtonEvent *ev)
{
	end_resize();
}


static void p_moveres_cancel(WFrame *frame)
{
	cancel_resize();
}


static void confine_to_parent(WFrame *frame)
{
	WRegion *par=region_parent((WRegion*)frame);
	if(par!=NULL)
		grab_confine_to(region_x_window(par));
}


static void p_resize_motion(WFrame *frame, XMotionEvent *ev, int dx, int dy)
{
	delta_resize((WRegion*)frame, p_dx1mul*dx, p_dx2mul*dx,
				 p_dy1mul*dy, p_dy2mul*dy, NULL);
}


static void p_resize_begin(WFrame *frame, XMotionEvent *ev, int dx, int dy)
{
	begin_resize((WRegion*)frame, NULL, TRUE);
	p_resize_motion(frame, ev, dx, dy);
}


/*EXTL_DOC
 * Start resizing \var{frame} with the mouse or other pointing device.
 * This function should only be used by binding it to \emph{mpress} or
 * \emph{mdrag} action.
 */
EXTL_EXPORT_MEMBER
void frame_p_resize(WFrame *frame)
{
	if(!p_set_drag_handlers((WRegion*)frame,
							(WMotionHandler*)p_resize_begin,
							(WMotionHandler*)p_resize_motion,
							(WButtonHandler*)p_moveres_end,
							NULL, 
							(GrabKilledHandler*)p_moveres_cancel))
		return;
	
	confine_to_parent(frame);
}


/*}}}*/


/*{{{ move */


static void p_move_motion(WFrame *frame, XMotionEvent *ev, int dx, int dy)
{
	delta_move((WRegion*)frame, dx, dy, NULL);
}


static void p_move_begin(WFrame *frame, XMotionEvent *ev, int dx, int dy)
{
	begin_move((WRegion*)frame, NULL, TRUE);
	p_move_motion(frame, ev, dx, dy);
}


void frame_p_move(WFrame *frame)
{
	if(!p_set_drag_handlers((WRegion*)frame,
							(WMotionHandler*)p_move_begin,
							(WMotionHandler*)p_move_motion,
							(WButtonHandler*)p_moveres_end,
							NULL, 
							(GrabKilledHandler*)p_moveres_cancel))
		return;
	
	confine_to_parent(frame);
}


/*}}}*/


/*{{{ switch_tab */


/*EXTL_DOC
 * Display the region corresponding to the tab that the user pressed on.
 * This function should only be used by binding it to a mouse action.
 */
EXTL_EXPORT_MEMBER
void frame_p_switch_tab(WFrame *frame)
{
	WRegion *sub;
	
	if(pointer_grab_region()!=(WRegion*)frame)
		return;
	
	sub=sub_at_tab(frame);
	
	if(sub!=NULL)
		region_display_sp(sub);
}


/*}}}*/

