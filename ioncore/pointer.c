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
#include "binding.h"
#include "global.h"
#include "focus.h"


/* TODO: Monitor p_thing */

/*{{{ Variables */


static uint p_button=0, p_state=0;
static WBindmap *p_bindmap=NULL;
static int p_x=-1, p_y=-1;
static int p_orig_x=-1, p_orig_y=-1;
static bool p_motion=FALSE;
static int p_clickcnt=0;
static Time p_time=0;
static bool p_motiontmp_dirty;
static WBinding *p_motiontmp=NULL;
static WThing *p_thing=NULL, *p_dbltmp=NULL;
static WScreen *p_screen=NULL;
static int p_area=0;


/*}}}*/


/*{{{ Misc. */


static bool time_in_treshold(Time time)
{
	Time t;
	
	if(time<p_time)
		t=p_time-time;
	else
		t=time-p_time;
	
	return t<wglobal.dblclick_delay;
}


static bool motion_in_treshold(int x, int y)
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


void callhnd_button_void(WThing *thing, WFunction *func,
						 int n, const Token *args)
{
}


void callhnd_drag_void(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
}


static void call_button(WThing *thing, WBinding *binding, XButtonEvent *ev)
{
	WButtonHandler *fn;

	if(binding==NULL)
		return;

	if(binding->func->callhnd==callhnd_button_void){
		fn=(WButtonHandler*)binding->func->fn;
	}else if(binding->func->callhnd==callhnd_drag_void){
		fn=((WDragHandler*)binding->func->fn)->end;
	}else{
		call_binding(binding, thing);
		return;
	}
	
	thing=find_parent(thing, binding->func->objdescr);
	if(thing!=NULL)
		fn(thing, ev);
}


static void call_motion(WThing *thing, WBinding *binding, XMotionEvent *ev,
						int dx, int dy)
{
	WMotionHandler *fn;
	
	if(binding==NULL)
		return;

	if(binding->func->callhnd!=callhnd_drag_void){
		/*call_binding(binding, wglobal.grab_holder);*/
		return;
	}
	
	fn=((WDragHandler*)binding->func->fn)->motion;
	thing=find_parent(thing, binding->func->objdescr);
	if(thing!=NULL)
		fn(thing, ev, dx, dy);
}


static void call_motion_begin(WThing *thing, WBinding *binding,
							  XMotionEvent *ev, int dx, int dy)
{
	WMotionHandler *fn;
	
	if(binding==NULL)
		return;

	if(binding->func->callhnd!=callhnd_drag_void){
		/*call_binding(binding, wglobal.grab_holder);*/
		return;
	}
	
	fn=((WDragHandler*)binding->func->fn)->begin;
	thing=find_parent(thing, binding->func->objdescr);
	if(thing!=NULL)
		fn(thing, ev, dx, dy);
}


/*}}}*/


/*{{{ handle_button_press/release/motion */


bool handle_button_press(XButtonEvent *ev)
{
	WBinding *pressbind=NULL;
	WRegion *reg=NULL;
	uint button, state;
	WBindmap *bindmap=NULL;
	int area=0;
	
	p_thing=NULL;
	p_motiontmp=NULL;
	
	state=ev->state;
	button=ev->button;
	
	reg=(WRegion*)FIND_WINDOW_T(ev->window, WWindow);
	
	if(reg==NULL)
		return FALSE;
	
	do_grab_kb_ptr(ev->root, reg, FocusChangeMask);
	bindmap=((WWindow*)reg)->bindmap;
	
	area=window_press((WWindow*)reg, ev, &reg);
	
	p_thing=(WThing*)reg;
	
	if(p_clickcnt==1 && time_in_treshold(ev->time) && p_button==button &&
	   p_state==state && p_thing==p_dbltmp && p_bindmap==bindmap){
		pressbind=lookup_binding_area(bindmap, ACT_BUTTONDBLCLICK, state,
									  button, area);
	}
	
	if(pressbind==NULL)
		pressbind=lookup_binding_area(bindmap, ACT_BUTTONPRESS, state,
									  button, area);
	
end:
	p_dbltmp=p_thing;
	p_bindmap=bindmap;
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
		binding=lookup_binding_area(p_bindmap, ACT_BUTTONCLICK,
									p_state, p_button, p_area);
	}else if(p_motiontmp_dirty){
		binding=lookup_binding_area(p_bindmap, ACT_BUTTONMOTION,
									p_state, p_button, p_area);
	}else{
		binding=p_motiontmp;
	}

	call_button(p_thing, binding,  ev);
	
	return TRUE;
}


void handle_pointer_motion(XMotionEvent *ev)
{
	WThing *tmp;
	int dx, dy;
	
	if(p_motion==FALSE && motion_in_treshold(ev->x_root, ev->y_root))
		return;
	
	if(p_motiontmp_dirty){
		p_motiontmp=lookup_binding_area(p_bindmap, ACT_BUTTONMOTION,
										p_state, p_button, p_area);
		p_motiontmp_dirty=FALSE;
	}

	p_time=ev->time;
	dx=ev->x_root-p_x;
	dy=ev->y_root-p_y;
	p_x=ev->x_root;
	p_y=ev->y_root;	
	
	if(p_motion==FALSE)
		call_motion_begin(p_thing, p_motiontmp, ev, dx, dy);
	else
		call_motion(p_thing, p_motiontmp, ev, dx, dy);
	
	p_motion=TRUE;
}


/*}}}*/


