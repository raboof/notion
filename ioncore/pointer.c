/*
 * ion/ioncore/pointer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "pointer.h"
#include "cursor.h"
#include "event.h"
#include "global.h"
#include "focus.h"
#include "regbind.h"
#include "grab.h"


/*{{{ Variables */


static uint p_button=0, p_state=0;
static int p_x=-1, p_y=-1;
static int p_orig_x=-1, p_orig_y=-1;
static bool p_motion=FALSE;
static int p_clickcnt=0;
static Time p_time=0;
static bool p_motiontmp_dirty;
static WBinding *p_motiontmp=NULL;
static int p_area=0;

static WButtonHandler *p_button_handler=NULL;
static WMotionHandler *p_motion_handler=NULL;
static WMotionHandler *p_motion_begin_handler=NULL;

static WWatch p_regwatch=WWATCH_INIT, p_subregwatch=WWATCH_INIT;

#define p_reg ((WRegion*)p_regwatch.obj)
#define p_subreg ((WRegion*)p_subregwatch.obj)


/*}}}*/


/*{{{ Handler setup */


bool set_button_handler(WRegion *reg,
						WButtonHandler *handler)
{
	if(reg!=p_reg)
		return FALSE;
	
	p_button_handler=handler;
	return TRUE;
}


bool set_drag_handlers(WRegion *reg,
					   WMotionHandler *begin,
					   WMotionHandler *motion,
					   WButtonHandler *end)
{
	if(reg!=p_reg || p_motion==FALSE)
		return FALSE;
	
	p_motion_begin_handler=begin;
	p_motion_handler=motion;
	p_button_handler=end;
	
	return TRUE;
}


/*}}}*/


/*{{{ Misc. */


bool coords_in_rect(const WRectangle *g, int x, int y)
{
	return (x>=g->x && x<g->x+g->w && y>=g->y && y<g->y+g->h);
}


static bool time_in_threshold(Time time)
{
	Time t;
	
	if(time<p_time)
		t=p_time-time;
	else
		t=time-p_time;
	
	return t<wglobal.dblclick_delay;
}


static bool motion_in_threshold(int x, int y)
{
	return (x>p_x-CF_DRAG_TRESHOLD && x<p_x+CF_DRAG_TRESHOLD &&
			y>p_y-CF_DRAG_TRESHOLD && y<p_y+CF_DRAG_TRESHOLD);
}


/*}}}*/


/*{{{ Call handlers */


static void call_button(WBinding *binding, XButtonEvent *ev)
{
	WButtonHandler *fn;

	if(binding==NULL)
		return;

	extl_call(binding->func, "oo", NULL, p_reg, p_subreg);
	
	if(p_button_handler!=NULL && p_reg!=NULL)
		p_button_handler(p_reg, ev);
	
	p_button_handler=NULL;
}


static void call_motion(WBinding *binding, XMotionEvent *ev,
						int dx, int dy)
{
	if(p_motion_handler!=NULL && p_reg!=NULL)
		p_motion_handler(p_reg, ev, dx, dy);
}


static void call_motion_begin(WBinding *binding, XMotionEvent *ev,
							  int dx, int dy)
{
	WMotionHandler *fn;
	
	if(binding==NULL)
		return;

	extl_call(binding->func, "oo", NULL, p_reg, p_subreg);
	
	if(p_motion_begin_handler!=NULL && p_reg!=NULL)
		p_motion_begin_handler(p_reg, ev, dx, dy);
	
	p_motion_begin_handler=NULL;
}


/*}}}*/


/*{{{ handle_button_press/release/motion */


bool handle_key_dummy(WRegion *reg, XEvent *ev)
{
	return FALSE;
}
	

bool handle_button_press(XButtonEvent *ev)
{
	WBinding *pressbind=NULL;
	WRegion *reg=NULL;
	WRegion *subreg=NULL;
	uint button, state;
	int area=0;
	p_motiontmp=NULL;
	
	state=ev->state;
	button=ev->button;
	
	reg=(WRegion*)FIND_WINDOW_T(ev->window, WWindow);
	
	if(reg==NULL)
		return FALSE;

	if(ev->subwindow!=None){
		XButtonEvent ev2=*ev;
		ev2.window=ev->subwindow;
		if(XTranslateCoordinates(wglobal.dpy, ev->window, ev2.window,
								 ev->x, ev->y, &(ev2.x), &(ev2.y),
								 &(ev2.subwindow))){
			if(handle_button_press(&ev2))
				return TRUE;
		}
	}

	subreg=reg;
	area=window_press((WWindow*)reg, ev, &subreg);

	if(p_clickcnt==1 && time_in_threshold(ev->time) && p_button==button &&
	   p_state==state && reg==p_reg){
		pressbind=region_lookup_binding_area(reg, ACT_BUTTONDBLCLICK, state,
											 button, area);
	}
	
	if(pressbind==NULL){
		pressbind=region_lookup_binding_area(reg, ACT_BUTTONPRESS, state,
											 button, area);
	}
	
	setup_watch(&p_regwatch, (WObj*)reg, NULL);
	setup_watch(&p_subregwatch, (WObj*)subreg, NULL);

	/*do_grab_kb_ptr(ev->root, reg, FocusChangeMask);*/
	grab_establish(reg, handle_key_dummy, 0);
	
end:
	/*p_reg=reg;*/
	p_button=button;
	p_state=state;
	p_orig_x=p_x=ev->x_root;
	p_orig_y=p_y=ev->y_root;
	p_time=ev->time;
	p_motion=FALSE;
	p_clickcnt=0;
	p_motiontmp_dirty=TRUE;
	p_area=area;
	p_button_handler=NULL;
	p_motion_handler=NULL;
	p_motion_begin_handler=NULL;
	
	call_button(pressbind, ev);
	
	return TRUE;
}


bool handle_button_release(XButtonEvent *ev)
{
	WBinding *binding=NULL;
	
	if(p_button!=ev->button)
	   	return FALSE;

	if(p_reg!=NULL){
		if(p_motion==FALSE){
			p_clickcnt=1;
			binding=region_lookup_binding_area(p_reg, ACT_BUTTONCLICK,
											   p_state, p_button, p_area);
			call_button(binding,  ev);
		}else{
			if(p_button_handler!=NULL)
				p_button_handler(p_reg, ev);
		}
		
		/* Allow any temporary settings to be cleared */
		grab_remove(handle_key_dummy);
		window_release((WWindow*)p_reg);
	}
	
	/*reset_watch(&p_regwatch);*/
	reset_watch(&p_subregwatch);

	return TRUE;
}


void handle_pointer_motion(XMotionEvent *ev)
{
	int dx, dy;
	
	if(p_reg==NULL)
		return;
	
	if(p_motion==FALSE && motion_in_threshold(ev->x_root, ev->y_root))
		return;
	
	if(p_motiontmp_dirty){
		p_motiontmp=region_lookup_binding_area(p_reg, ACT_BUTTONMOTION,
											   p_state, p_button, p_area);
		p_motiontmp_dirty=FALSE;
	}

	p_time=ev->time;
	dx=ev->x_root-p_x;
	dy=ev->y_root-p_y;
	p_x=ev->x_root;
	p_y=ev->y_root;	
	
	if(p_motion==FALSE){
		p_motion=TRUE;
		call_motion_begin(p_motiontmp, ev, dx, dy);
	}else{
		call_motion(p_motiontmp, ev, dx, dy);
	}
}


/*}}}*/

