/*
 * ion/frame-pointer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <wmcore/common.h>
#include <wmcore/global.h>
#include <wmcore/pointer.h>
#include <wmcore/grdata.h>
#include <wmcore/cursor.h>
#include <wmcore/focus.h>
#include <wmcore/drawp.h>
#include <wmcore/attach.h>
#include <wmcore/resize.h>
#include <wmcore/funtabs.h>
#include <wmcore/functionp.h>
#include <wmcore/grab.h>
#include "frame.h"
#include "frame-pointer.h"
#include "resize.h"


#define REGION_LABEL(REG)	((REG)->mgr_data)


static int p_tab_x, p_tab_y, p_tabnum=-1;
static bool p_tabdrag_active=FALSE;
static bool p_tabdrag_selected=FALSE;
static int p_dx1mul=0, p_dx2mul=0, p_dy1mul=0, p_dy2mul=0;


/*{{{ Frame press */


static bool inrect(WRectangle g, int x, int y)
{
	return (x>=g.x && x<g.x+g.w && y>=g.y && y<g.y+g.h);
}


static void frame_borders(WFrame *frame, WGRData *grdata, WRectangle *geom)
{
	*geom=grdata->border_off;
	geom->x+=BORDER_TL_TOTAL(&grdata->frame_border);
	geom->y+=BORDER_TL_TOTAL(&grdata->frame_border);
	geom->w-=BORDER_TL_TOTAL(&grdata->frame_border)*2;
	geom->h-=BORDER_TL_TOTAL(&grdata->frame_border)*2;
	geom->w+=FRAME_W(frame);
	geom->h+=FRAME_H(frame);
}

#define RESB 8

int frame_press(WFrame *frame, XButtonEvent *ev, WThing **thing_ret)
{
	WScreen *scr=SCREEN_OF(frame);
	WRegion *sub;
	WRectangle g;
	
	p_dx1mul=0;
	p_dx2mul=0;
	p_dy1mul=0;
	p_dy2mul=0;
	p_tabnum=-1;
	
	/* Check tab */
	
	frame_bar_geom(frame, &g);
		
	if(inrect(g, ev->x, ev->y)){
		p_dy1mul=1;

		if(ev->x<g.x+RESB)
			p_dx1mul=1;
		else if(ev->x>g.x+g.w-RESB)
			p_dx2mul=1;
		
		p_tabnum=frame_tab_at_x(frame, ev->x);

		region_rootpos((WRegion*)frame, &p_tab_x, &p_tab_y);
		p_tab_x+=frame_nth_tab_x(frame, p_tabnum);
		p_tab_y+=g.y;
		
		sub=frame_nth_managed(frame, p_tabnum);

		if(sub==NULL)
			return FRAME_AREA_EMPTY_TAB;

		frame->tab_pressed_sub=sub;

		if(thing_ret!=NULL)
			*thing_ret=(WThing*)sub;
		
		return FRAME_AREA_TAB;
	}
	
	/* Check borders */

	frame_borders(frame, &scr->grdata, &g);
	
	if(ev->x<g.x+RESB)
		p_dx1mul=1;
	else if(ev->x>g.x+g.w-RESB)
		p_dx2mul=1;
	if(ev->y<g.y+RESB)
		p_dy1mul=1;
	else if(ev->y>g.y+g.h-RESB)
		p_dy2mul=1;
	
	
	if(p_dx1mul+p_dx2mul+p_dy1mul+p_dy2mul!=0)
		return FRAME_AREA_BORDER;
	
	/* Neither of those */
	
	if(ev->x<REGION_GEOM(frame).w/2)
		p_dx1mul=1;
	else
		p_dx2mul=1;
	if(ev->y<REGION_GEOM(frame).h/2)
		p_dy1mul=1;
	else
		p_dy2mul=1;

	return 0;
}


void frame_release(WFrame *frame)
{
	frame->tab_pressed_sub=NULL;
}


/*}}}*/


/*{{{ Tab drag */


static WFunction tabdrag_safe_funtab[]={
	FN_GLOBAL(l,   "switch_ws_nth",			switch_ws_nth),
	FN_GLOBAL_VOID("switch_ws_next",		switch_ws_next),
	FN_GLOBAL_VOID("switch_ws_prev",		switch_ws_prev),

	END_FUNTAB
};


static WFunclist tabdrag_safe_funclist=INIT_FUNCLIST;


#define BUTTONS_MASK \
	(Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)

static bool tabdrag_kbd_handler(WRegion *thing, XEvent *xev)
{
	XKeyEvent *ev=&xev->xkey;
	WScreen *scr;
	WBinding *binding=NULL;
	WBindmap **bindptr;
	
	if(ev->type==KeyRelease)
		return FALSE;
	
	assert(thing && WTHING_IS(thing, WWindow));

	binding=lookup_binding(&wmcore_screen_bindmap, ACT_KEYPRESS,
						   ev->state&~BUTTONS_MASK, ev->keycode);
	
	if(binding!=NULL){
		call_binding_restricted(binding, (WThing*)viewport_of(thing),
								&tabdrag_safe_funclist);
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
	
	
	if(((WFrame*)reg)->tab_pressed_sub!=NULL)
		label=REGION_LABEL(((WFrame*)reg)->tab_pressed_sub);
	if(label==NULL)
		label="";
	
	draw_textbox(dinfo, label, ALIGN_CENTER, TRUE);
}


static void p_tabdrag_motion(WFrame *frame, XMotionEvent *ev, int dx, int dy)
{
	WGRData *grdata=GRDATA_OF(frame);

	p_tab_x+=dx;
	p_tab_y+=dy;
	
	XMoveWindow(wglobal.dpy, grdata->drag_win, p_tab_x, p_tab_y);
}	


static void p_tabdrag_begin(WFrame *frame, XMotionEvent *ev, int dx, int dy)
{
	WGRData *grdata=GRDATA_OF(frame);

	change_grab_cursor(CURSOR_DRAG);
		
	grdata->drag_geom.w=frame_nth_tab_w(frame, p_tabnum);
	grdata->drag_geom.h=grdata->bar_h;
	XResizeWindow(wglobal.dpy, grdata->drag_win,
				  grdata->drag_geom.w, grdata->drag_geom.h);
	/*XSelectInput(wglobal.dpy, grdata->drag_win, ExposureMask);*/
	
	/* TODO: drawlist */
	wglobal.draw_dragwin=(WDrawDragwinFn*)draw_tabdrag;
	wglobal.draw_dragwin_arg=(WRegion*)frame;
	
	p_tabdrag_active=REGION_IS_ACTIVE(frame);
	p_tabdrag_selected=(frame->current_sub==frame->tab_pressed_sub);
	
	frame->flags|=FRAME_TAB_DRAGGED;
		
	draw_frame_bar(frame, FALSE);
	
	p_tabdrag_motion(frame, ev, dx, dy);
	
	XMapRaised(wglobal.dpy, grdata->drag_win);
}


static WFrame *fnd(Window rootwin, int x, int y)
{
	int dstx, dsty;
	Window childret;
	WFrame *frame=NULL;
	WWindow *w;
	
	do{
		if(!XTranslateCoordinates(wglobal.dpy, rootwin, rootwin,
								  x, y, &dstx, &dsty, &childret))
			return NULL;
	
		if(childret==None)
			return frame;
		w=FIND_WINDOW_T(childret, WWindow);
		if(w==NULL)
			break;
		if(WTHING_IS(w, WFrame))
			frame=(WFrame*)w;
		rootwin=childret;
	}while(1);
	
	return frame;
}


static void p_tabdrag_end(WFrame *frame, XButtonEvent *ev)
{
	WGRData *grdata=GRDATA_OF(frame);
	WFrame *newframe=NULL;
	WRegion *sub=NULL;
	Window win=None;
	
	grab_remove(tabdrag_kbd_handler);

	wglobal.draw_dragwin=NULL;

	sub=frame->tab_pressed_sub;
	frame->tab_pressed_sub=NULL;
	frame->flags&=~FRAME_TAB_DRAGGED;
	
	XUnmapWindow(wglobal.dpy, grdata->drag_win);	
	/*XSelectInput(wglobal.dpy, grdata->drag_win, 0);*/
	
	/* Must be same screen */
	
	if(ev->root!=ROOT_OF(sub))
		return;
	
	/*if(find_window_at(ev->root, ev->x_root, ev->y_root, &win))
		newframe=find_frame_of(win);*/
	newframe=fnd(ev->root, ev->x_root, ev->y_root);

	if(newframe==NULL || frame==newframe){
		draw_frame_bar(frame, TRUE);
		return;
	}
	
	region_add_managed((WRegion*)newframe, sub, REGION_ATTACH_SWITCHTO);
	set_focus((WRegion*)newframe);
}


void p_tabdrag_setup(WFrame *frame)
{
	if(frame->tab_pressed_sub==NULL)
		return;

	if(tabdrag_safe_funclist.funtabs==NULL){
		add_to_funclist(&tabdrag_safe_funclist,
						tabdrag_safe_funtab);
	}
	
	set_drag_handlers((WRegion*)frame,
					  (WMotionHandler*)p_tabdrag_begin,
					  (WMotionHandler*)p_tabdrag_motion,
					  (WButtonHandler*)p_tabdrag_end);
	
	grab_establish((WRegion*)frame, tabdrag_kbd_handler, 0);
}


/*}}}*/


/*{{{ Resize */


static void p_resize_motion(WFrame *frame, XMotionEvent *ev, int dx, int dy)
{
	delta_resize((WRegion*)frame, p_dx1mul*dx, p_dx2mul*dx,
				 p_dy1mul*dy, p_dy2mul*dy, NULL);
}


static void p_resize_begin(WFrame *frame, XMotionEvent *ev, int dx, int dy)
{
	begin_resize((WRegion*)frame, NULL);
	p_resize_motion(frame, ev, dx, dy);
}


static void p_resize_end(WFrame *frame, XButtonEvent *ev)
{
	end_resize((WRegion*)frame);
}

#include <wmcore/objp.h>

void p_resize_setup(WFrame *frame)
{
	set_drag_handlers((WRegion*)frame,
					  (WMotionHandler*)p_resize_begin,
					  (WMotionHandler*)p_resize_motion,
					  (WButtonHandler*)p_resize_end);
}


/*}}}*/


/*{{{ switch_tab */


void frame_switch_tab(WFrame *frame)
{
	if(frame->tab_pressed_sub!=NULL)
		display_region_sp(frame->tab_pressed_sub);
	
}

/*}}}*/

