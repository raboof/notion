/*
 * ion/ioncore/genframe-pointer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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
#include "genframe.h"
#include "genframe-pointer.h"
#include "genframep.h"
#include "objp.h"
#include "bindmaps.h"
#include "infowin.h"
#include "defer.h"


static int p_tab_x=0, p_tab_y=0, p_tabnum=-1;
static int p_dx1mul=0, p_dx2mul=0, p_dy1mul=0, p_dy2mul=0;
static WInfoWin *tabdrag_infowin=NULL;


/*{{{ Frame press */

#define MINCORNER 16


static WRegion *sub_at_tab(WGenFrame *genframe)
{
	return mplex_nth_managed((WMPlex*)genframe, p_tabnum);
}


int genframe_press(WGenFrame *genframe, XButtonEvent *ev, WRegion **reg_ret)
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
		int ww=REGION_GEOM(genframe).w/2;
		int hh=REGION_GEOM(genframe).h/2;
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
	
	genframe_bar_geom(genframe, &g);
	
	/* Borders act like tabs at top of the parent region */
	if(REGION_GEOM(genframe).y==0){
		g.h+=g.y;
		g.y=0;
	}

	if(coords_in_rect(&g, ev->x, ev->y)){
		p_tabnum=genframe_tab_at_x(genframe, ev->x);

		region_rootpos((WRegion*)genframe, &p_tab_x, &p_tab_y);
		p_tab_x+=genframe_nth_tab_x(genframe, p_tabnum);
		p_tab_y+=g.y;
		
		sub=mplex_nth_managed(&(genframe->mplex), p_tabnum);

		if(reg_ret!=NULL)
			*reg_ret=sub;
		
		return WGENFRAME_AREA_TAB;
	}
	

	/* Check border */
	
	genframe_border_inner_geom(genframe, &g);
	
	if(coords_in_rect(&g, ev->x, ev->y))
		return WGENFRAME_AREA_CLIENT;
	
	return WGENFRAME_AREA_BORDER;
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


static void setup_dragwin(WGenFrame *genframe, uint tab)
{
	WRectangle g;
	
	assert(tabdrag_infowin==NULL);
	
	g.x=p_tab_x;
	g.y=p_tab_y;
	g.w=genframe_nth_tab_w(genframe, tab);
	g.h=genframe->bar_h;
	
	tabdrag_infowin=create_infowin((WWindow*)ROOTWIN_OF(genframe), &g, 
								   genframe_tab_style(genframe));
	
	if(tabdrag_infowin==NULL)
		return;
	
	infowin_set_attr2(tabdrag_infowin, (REGION_IS_ACTIVE(genframe) 
										? "active" : "inactive"),
					  genframe->titles[tab].attr);
	
	if(genframe->titles[tab].text!=NULL){
		char *buf=WINFOWIN_BUFFER(tabdrag_infowin);
		strncpy(buf, genframe->titles[tab].text, WINFOWIN_BUFFER_LEN-1);
		buf[WINFOWIN_BUFFER_LEN-1]='\0';
	}
}


static void p_tabdrag_motion(WGenFrame *genframe, XMotionEvent *ev,
							 int dx, int dy)
{
	WRootWin *rootwin=ROOTWIN_OF(genframe);

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


static void p_tabdrag_begin(WGenFrame *genframe, XMotionEvent *ev,
							int dx, int dy)
{
	WRootWin *rootwin=ROOTWIN_OF(genframe);

	if(p_tabnum<0)
		return;
	
	change_grab_cursor(CURSOR_DRAG);
		
	setup_dragwin(genframe, p_tabnum);
	
	genframe->tab_dragged_idx=p_tabnum;
	genframe_update_attr_nth(genframe, p_tabnum);

	genframe_draw_bar(genframe, FALSE);
	
	p_tabdrag_motion(genframe, ev, dx, dy);
	
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


static void tabdrag_deinit(WGenFrame *genframe)
{
	int idx=genframe->tab_dragged_idx;
	genframe->tab_dragged_idx=-1;
	genframe_update_attr_nth(genframe, idx);
	
	if(tabdrag_infowin!=NULL){
		destroy_obj((WObj*)tabdrag_infowin);
		tabdrag_infowin=NULL;
	}
}


static void tabdrag_killed(WGenFrame *genframe)
{
	tabdrag_deinit(genframe);
	if(!WOBJ_IS_BEING_DESTROYED(genframe))
		genframe_draw_bar(genframe, TRUE);
}


static void p_tabdrag_end(WGenFrame *genframe, XButtonEvent *ev)
{
	WRegion *sub=NULL;
	WRegion *dropped_on;
	Window win=None;

	sub=sub_at_tab(genframe);
	
	tabdrag_deinit(genframe);
	
	/* Must be same root window */
	if(ev->root!=ROOT_OF(sub))
		return;
	
	dropped_on=fnd(ev->root, ev->x_root, ev->y_root);

	if(dropped_on==NULL || dropped_on==(WRegion*)genframe || dropped_on==sub){
		genframe_draw_bar(genframe, TRUE);
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
void genframe_p_tabdrag(WGenFrame *genframe)
{
	p_set_drag_handlers((WRegion*)genframe,
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


static void p_moveres_end(WGenFrame *genframe, XButtonEvent *ev)
{
	end_resize();
}


static void p_moveres_cancel(WGenFrame *genframe)
{
	cancel_resize();
}


static void confine_to_parent(WGenFrame *genframe)
{
	WRegion *par=region_parent((WRegion*)genframe);
	if(par!=NULL)
		grab_confine_to(region_x_window(par));
}


static void p_resize_motion(WGenFrame *genframe, XMotionEvent *ev, int dx, int dy)
{
	delta_resize((WRegion*)genframe, p_dx1mul*dx, p_dx2mul*dx,
				 p_dy1mul*dy, p_dy2mul*dy, NULL);
}


static void p_resize_begin(WGenFrame *genframe, XMotionEvent *ev, int dx, int dy)
{
	begin_resize((WRegion*)genframe, NULL, TRUE);
	p_resize_motion(genframe, ev, dx, dy);
}


/*EXTL_DOC
 * Start resizing \var{genframe} with the mouse or other pointing device.
 * This function should only be used by binding it to \emph{mpress} or
 * \emph{mdrag} action.
 */
EXTL_EXPORT_MEMBER
void genframe_p_resize(WGenFrame *genframe)
{
	if(!p_set_drag_handlers((WRegion*)genframe,
							(WMotionHandler*)p_resize_begin,
							(WMotionHandler*)p_resize_motion,
							(WButtonHandler*)p_moveres_end,
							NULL, 
							(GrabKilledHandler*)p_moveres_cancel))
		return;
	
	confine_to_parent(genframe);
}


/*}}}*/


/*{{{ move */


static void p_move_motion(WGenFrame *genframe, XMotionEvent *ev, int dx, int dy)
{
	delta_move((WRegion*)genframe, dx, dy, NULL);
}


static void p_move_begin(WGenFrame *genframe, XMotionEvent *ev, int dx, int dy)
{
	begin_move((WRegion*)genframe, NULL, TRUE);
	p_move_motion(genframe, ev, dx, dy);
}


void genframe_p_move(WGenFrame *genframe)
{
	if(!p_set_drag_handlers((WRegion*)genframe,
							(WMotionHandler*)p_move_begin,
							(WMotionHandler*)p_move_motion,
							(WButtonHandler*)p_moveres_end,
							NULL, 
							(GrabKilledHandler*)p_moveres_cancel))
		return;
	
	confine_to_parent(genframe);
}


/*}}}*/


/*{{{ switch_tab */


/*EXTL_DOC
 * Display the region corresponding to the tab that the user pressed on.
 * This function should only be used by binding it to a mouse action.
 */
EXTL_EXPORT_MEMBER
void genframe_p_switch_tab(WGenFrame *genframe)
{
	WRegion *sub;
	
	if(pointer_grab_region()!=(WRegion*)genframe)
		return;
	
	sub=sub_at_tab(genframe);
	
	if(sub!=NULL)
		region_display_sp(sub);
}


/*}}}*/

