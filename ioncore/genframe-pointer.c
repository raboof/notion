/*
 * ion/ioncore/genframe-pointer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "global.h"
#include "pointer.h"
#include "grdata.h"
#include "cursor.h"
#include "focus.h"
#include "drawp.h"
#include "attach.h"
#include "resize.h"
#include "grab.h"
#include "genframe.h"
#include "genframe-pointer.h"
#include "genframep.h"
#include "objp.h"


static int p_tab_x, p_tab_y, p_tabnum=-1;
static bool p_tabdrag_active=FALSE;
static bool p_tabdrag_selected=FALSE;
static int p_dx1mul=0, p_dx2mul=0, p_dy1mul=0, p_dy2mul=0;


/*{{{ Frame press */


static bool inrect(WRectangle g, int x, int y)
{
	return (x>=g.x && x<g.x+g.w && y>=g.y && y<g.y+g.h);
}


#define RESB 8


int genframe_press(WGenFrame *genframe, XButtonEvent *ev, WRegion **reg_ret)
{
	WScreen *scr=SCREEN_OF(genframe);
	WRegion *sub;
	WRectangle g;
	
	p_dx1mul=0;
	p_dx2mul=0;
	p_dy1mul=0;
	p_dy2mul=0;
	p_tabnum=-1;
	
	/* Check tab */
	
	genframe_bar_geom(genframe, &g);
		
	if(inrect(g, ev->x, ev->y)){
		p_dy1mul=1;

		if(ev->x<g.x+RESB)
			p_dx1mul=1;
		else if(ev->x>g.x+g.w-RESB)
			p_dx2mul=1;
		
		p_tabnum=genframe_tab_at_x(genframe, ev->x);

		region_rootpos((WRegion*)genframe, &p_tab_x, &p_tab_y);
		p_tab_x+=genframe_nth_tab_x(genframe, p_tabnum);
		p_tab_y+=g.y;
		
		sub=genframe_nth_managed(genframe, p_tabnum);

		if(sub==NULL)
			return WGENFRAME_AREA_EMPTY_TAB;

		genframe->tab_pressed_sub=sub;

		if(reg_ret!=NULL)
			*reg_ret=sub;
		
		return WGENFRAME_AREA_TAB;
	}
	
	/* Check borders */

	genframe_border_inner_geom(genframe, &g);
	
	if(ev->x<g.x+RESB)
		p_dx1mul=1;
	else if(ev->x>g.x+g.w-RESB)
		p_dx2mul=1;
	if(ev->y<g.y+RESB)
		p_dy1mul=1;
	else if(ev->y>g.y+g.h-RESB)
		p_dy2mul=1;
	
	
	if(p_dx1mul+p_dx2mul+p_dy1mul+p_dy2mul!=0)
		return WGENFRAME_AREA_BORDER;
	
	/* Neither of those */
	
	if(ev->x<REGION_GEOM(genframe).w/2)
		p_dx1mul=1;
	else
		p_dx2mul=1;
	if(ev->y<REGION_GEOM(genframe).h/2)
		p_dy1mul=1;
	else
		p_dy2mul=1;

	return 0;
}


void genframe_release(WGenFrame *genframe)
{
	genframe->tab_pressed_sub=NULL;
}


/*}}}*/


/*{{{ Tab drag */


static const char *tabdrag_safe_funclist[]={
	"screen_switch_nth_on_cvp",
	"screen_switch_next_on_cvp",
	"screen_switch_prev_on_cvp",
	"viewport_switch_nth",
	"viewport_switch_next",
	"viewport_switch_prev",
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

	binding=lookup_binding(&ioncore_screen_bindmap, ACT_KEYPRESS,
						   ev->state&~BUTTONS_MASK, ev->keycode);
	
	if(binding!=NULL && binding->func!=extl_fn_none()){
		const char **old_safelist=extl_set_safelist(tabdrag_safe_funclist);
		extl_call(binding->func, "o", NULL, SCREEN_OF(reg));
		extl_set_safelist(old_safelist);
	}
	
	return FALSE;
}


static void draw_tabdrag(const WRegion *reg)
{
	DrawInfo _dinfo, *dinfo=&_dinfo;
	WGRData *grdata=GRDATA_OF(reg);
	const char *label=NULL;
	
	dinfo->win=grdata->drag_win;
	dinfo->draw=grdata->drag_draw;
	dinfo->grdata=grdata;
	dinfo->gc=grdata->tab_gc;
	dinfo->geom.x=0;
	dinfo->geom.y=0;
	dinfo->geom.w=grdata->drag_geom.w;
	dinfo->geom.h=grdata->drag_geom.h;
	dinfo->border=&(grdata->tab_border);
	dinfo->font=grdata->tab_font;

	if(p_tabdrag_active){
		if(p_tabdrag_selected)
			dinfo->colors=&(grdata->act_tab_sel_colors);
		else
			dinfo->colors=&(grdata->act_tab_colors);
	}else{
		if(p_tabdrag_selected)
			dinfo->colors=&(grdata->tab_sel_colors);
		else
			dinfo->colors=&(grdata->tab_colors);
	}
	
	
	if(((WGenFrame*)reg)->tab_pressed_sub!=NULL)
		label=REGION_LABEL(((WGenFrame*)reg)->tab_pressed_sub);
	if(label==NULL)
		label="";
	
	draw_textbox(dinfo, label, ALIGN_CENTER, TRUE);
}


static void p_tabdrag_motion(WGenFrame *genframe, XMotionEvent *ev,
							 int dx, int dy)
{
	WGRData *grdata=GRDATA_OF(genframe);

	p_tab_x+=dx;
	p_tab_y+=dy;
	
	XMoveWindow(wglobal.dpy, grdata->drag_win, p_tab_x, p_tab_y);
}	


static void p_tabdrag_begin(WGenFrame *genframe, XMotionEvent *ev,
							int dx, int dy)
{
	WGRData *grdata=GRDATA_OF(genframe);

	change_grab_cursor(CURSOR_DRAG);
		
	grdata->drag_geom.w=genframe_nth_tab_w(genframe, p_tabnum);
	grdata->drag_geom.h=grdata->bar_h;
	XResizeWindow(wglobal.dpy, grdata->drag_win,
				  grdata->drag_geom.w, grdata->drag_geom.h);
	/*XSelectInput(wglobal.dpy, grdata->drag_win, ExposureMask);*/
	
	wglobal.draw_dragwin=(WDrawDragwinFn*)draw_tabdrag;
	wglobal.draw_dragwin_arg=(WRegion*)genframe;
	
	p_tabdrag_active=REGION_IS_ACTIVE(genframe);
	p_tabdrag_selected=(genframe->current_sub==genframe->tab_pressed_sub);
	
	genframe->flags|=WGENFRAME_TAB_DRAGGED;
		
	genframe_draw_bar(genframe, FALSE);
	
	p_tabdrag_motion(genframe, ev, dx, dy);
	
	XMapRaised(wglobal.dpy, grdata->drag_win);
}


static WRegion *fnd(WWindow *w, int x, int y)
{
	int dstx, dsty;
	Window rootwin=w->win;
	Window childret;
	WRegion *reg=NULL, *reg2;
	
	do{
		if(!XTranslateCoordinates(wglobal.dpy, rootwin, w->win,
								  x, y, &dstx, &dsty, &childret))
			return NULL;
	
		if(childret==None){
			x-=REGION_GEOM(w).x;
			y-=REGION_GEOM(w).y;
			FOR_ALL_TYPED_CHILDREN(w, reg2, WRegion){
				if(region_is_fully_mapped(reg2) &&
				   inrect(REGION_GEOM(reg2), x, y) &&
				   HAS_DYN(reg2, region_handle_drop)){
					return reg2;
				}
			}
			break;
		}
		w=FIND_WINDOW_T(childret, WWindow);
		if(w==NULL)
			break;
		if(HAS_DYN(w, region_handle_drop))
			reg=(WRegion*)w;
	}while(1);
	
	return reg;
}


static void p_tabdrag_end(WGenFrame *genframe, XButtonEvent *ev)
{
	WGRData *grdata=GRDATA_OF(genframe);
	WRegion *sub=NULL;
	WRegion *dropped_on;
	Window win=None;
	
	grab_remove(tabdrag_kbd_handler);

	wglobal.draw_dragwin=NULL;

	sub=genframe->tab_pressed_sub;
	genframe->tab_pressed_sub=NULL;
	genframe->flags&=~WGENFRAME_TAB_DRAGGED;
	
	XUnmapWindow(wglobal.dpy, grdata->drag_win);	
	/*XSelectInput(wglobal.dpy, grdata->drag_win, 0);*/
	
	/* Must be same screen */
	
	if(ev->root!=ROOT_OF(sub))
		return;
	
	dropped_on=fnd((WWindow*)SCREEN_OF(sub), ev->x_root, ev->y_root);

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
EXTL_EXPORT
void genframe_p_tabdrag(WGenFrame *genframe)
{
	if(genframe->tab_pressed_sub==NULL)
		return;

	set_drag_handlers((WRegion*)genframe,
					  (WMotionHandler*)p_tabdrag_begin,
					  (WMotionHandler*)p_tabdrag_motion,
					  (WButtonHandler*)p_tabdrag_end);
	
	grab_establish((WRegion*)genframe, tabdrag_kbd_handler, 0);
}


bool region_handle_drop(WRegion *reg, int x, int y, WRegion *dropped)
{
	bool ret=FALSE;
	CALL_DYN_RET(ret, bool, region_handle_drop, reg, (reg, x, y, dropped));
	return ret;
}
	

/*}}}*/


/*{{{ Resize */


static void p_resize_motion(WGenFrame *genframe, XMotionEvent *ev, int dx, int dy)
{
	delta_resize((WRegion*)genframe, p_dx1mul*dx, p_dx2mul*dx,
				 p_dy1mul*dy, p_dy2mul*dy, NULL);
}


static void p_resize_begin(WGenFrame *genframe, XMotionEvent *ev, int dx, int dy)
{
	begin_resize((WRegion*)genframe, NULL);
	p_resize_motion(genframe, ev, dx, dy);
}


static void p_resize_end(WGenFrame *genframe, XButtonEvent *ev)
{
	end_resize((WRegion*)genframe);
}


/*EXTL_DOC
 * Start resizing \var{genframe} with the mouse or other pointing device.
 * This function should only be used by binding it to \emph{mpress} or
 * \emph{mdrag} action.
 */
EXTL_EXPORT
void genframe_p_resize(WGenFrame *genframe)
{
	set_drag_handlers((WRegion*)genframe,
					  (WMotionHandler*)p_resize_begin,
					  (WMotionHandler*)p_resize_motion,
					  (WButtonHandler*)p_resize_end);
}


/*}}}*/


/*{{{ move */


static void p_move_motion(WGenFrame *genframe, XMotionEvent *ev, int dx, int dy)
{
	delta_move((WRegion*)genframe, dx, dy, NULL);
}


static void p_move_begin(WGenFrame *genframe, XMotionEvent *ev, int dx, int dy)
{
	begin_move((WRegion*)genframe, NULL);
	p_move_motion(genframe, ev, dx, dy);
}


static void p_move_end(WGenFrame *genframe, XButtonEvent *ev)
{
	end_resize((WRegion*)genframe);
}


void genframe_p_move(WGenFrame *genframe)
{
	set_drag_handlers((WRegion*)genframe,
					  (WMotionHandler*)p_move_begin,
					  (WMotionHandler*)p_move_motion,
					  (WButtonHandler*)p_move_end);
}


/*}}}*/


/*{{{ switch_tab */

/*EXTL_DOC
 * Display the region corresponding to the tab that the user pressed on.
 * This function should only be used by binding it to a mouse action.
 */
EXTL_EXPORT
void genframe_p_switch_tab(WGenFrame *genframe)
{
	if(genframe->tab_pressed_sub!=NULL)
		region_display_sp(genframe->tab_pressed_sub);
}


/*}}}*/

