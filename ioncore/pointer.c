/*
 * wmcore/pointer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "pointer.h"
#include "cursor.h"
#include "event.h"
#include "global.h"
#include "focus.h"
#include "regbind.h"


/* TODO: Monitor p_thing */

/*{{{ Variables */


static uint p_button=0, p_state=0;
static int p_x=-1, p_y=-1;
static int p_orig_x=-1, p_orig_y=-1;
static bool p_motion=FALSE;
static int p_clickcnt=0;
static Time p_time=0;
static bool p_motiontmp_dirty;
static WBinding *p_motiontmp=NULL;
static WThing *p_thing=NULL;
static WRegion *p_reg=NULL;
static WScreen *p_screen=NULL;
static int p_area=0;

static WButtonHandler *p_button_handler=NULL;
static WMotionHandler *p_motion_handler=NULL;
static WMotionHandler *p_motion_begin_handler=NULL;


/*}}}*/


/*{{{ Handler setup */


bool set_button_handler(WButtonHandler *handler)
{
	p_button_handler=handler;
	return TRUE;
}


bool set_drag_handlers(WMotionHandler *begin, WMotionHandler *motion,
					   WButtonHandler *end)
{
	if(p_motion==FALSE)
		return FALSE;
	
	p_motion_begin_handler=begin;
	p_motion_handler=motion;
	p_button_handler=end;
	
	return TRUE;
}


/*}}}*/


/*{{{ Misc. */


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


bool find_window_at(Window rootwin, int x, int y, Window *childret)
{
	int dstx, dsty;
	
	if(!XTranslateCoordinates(wglobal.dpy, rootwin, rootwin,
							  x, y, &dstx, &dsty, childret))
		return FALSE;
	
	if(*childret==None)
		return FALSE;
	
	return TRUE;
}


/*}}}*/


/*{{{ Call handlers */


static void call_button(WThing *thing, WBinding *binding, XButtonEvent *ev)
{
	WButtonHandler *fn;

	if(binding==NULL)
		return;

	call_binding(binding, thing);
	
	if(p_button_handler!=NULL)
		p_button_handler(thing, ev);
	
	p_button_handler=NULL;
}


static void call_motion(WThing *thing, WBinding *binding, XMotionEvent *ev,
						int dx, int dy)
{
	if(p_motion_handler!=NULL)
		p_motion_handler(thing, ev, dx, dy);
}


static void call_motion_begin(WThing *thing, WBinding *binding,
							  XMotionEvent *ev, int dx, int dy)
{
	WMotionHandler *fn;
	
	if(binding==NULL)
		return;

	call_binding(binding, thing);
	
	if(p_motion_begin_handler!=NULL)
		p_motion_begin_handler(thing, ev, dx, dy);
	
	p_motion_begin_handler=NULL;
}


/*}}}*/


/*{{{ handle_button_press/release/motion */


bool handle_button_press(XButtonEvent *ev)
{
	WBinding *pressbind=NULL;
	WRegion *reg=NULL;
	uint button, state;
	/*WBindmap *bindmap=NULL;*/
	int area=0;
	
	p_motiontmp=NULL;
	p_thing=NULL;
	
	state=ev->state;
	button=ev->button;
	
	reg=(WRegion*)FIND_WINDOW_T(ev->window, WWindow);
	
	if(reg==NULL)
		return FALSE;

	do_grab_kb_ptr(ev->root, reg, FocusChangeMask);
	
	p_thing=(WThing*)reg;
	area=window_press((WWindow*)reg, ev, &p_thing);
	
	if(p_clickcnt==1 && time_in_threshold(ev->time) && p_button==button &&
	   p_state==state && reg==p_reg){
		pressbind=region_lookup_binding_area(reg, ACT_BUTTONDBLCLICK, state,
											 button, area);
	}
	
	if(pressbind==NULL){
		pressbind=region_lookup_binding_area(reg, ACT_BUTTONPRESS, state,
											 button, area);
	}
	
end:
	p_reg=reg;
	p_button=button;
	p_state=state;
	p_orig_x=p_x=ev->x_root;
	p_orig_y=p_y=ev->y_root;
	p_time=ev->time;
	p_motion=FALSE;
	p_clickcnt=0;
	p_motiontmp_dirty=TRUE;
	p_screen=SCREEN_OF(reg);
	p_area=area;
	p_button_handler=NULL;
	p_motion_handler=NULL;
	p_motion_begin_handler=NULL;
	
	call_button(p_thing, pressbind, ev);
	
	return TRUE;
}


bool handle_button_release(XButtonEvent *ev)
{
	WBinding *binding=NULL;
	
	if(p_button!=ev->button)
	   	return FALSE;

	if(p_motion==FALSE){
		p_clickcnt=1;
		binding=region_lookup_binding_area(p_reg, ACT_BUTTONCLICK,
										   p_state, p_button, p_area);
		call_button(p_thing, binding,  ev);
	}else{
		if(p_button_handler!=NULL)
			p_button_handler(p_thing, ev);
	}

	return TRUE;
}


void handle_pointer_motion(XMotionEvent *ev)
{
	WThing *tmp;
	int dx, dy;
	
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
		call_motion_begin(p_thing, p_motiontmp, ev, dx, dy);
	}else{
		call_motion(p_thing, p_motiontmp, ev, dx, dy);
	}
}


/*}}}*/

