/*
 * ion/ioncore/kbresize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>

#include "global.h"
#include "resize.h"
#include "kbresize.h"
#include "grab.h"
#include "binding.h"
#include "signal.h"
#include "focus.h"
#include "bindmaps.h"
#include "framep.h"


enum{
	VERTICAL,
	HORIZONTAL
};


/*{{{ Resize accelerator */


static struct timeval last_action_tv={-1, 0};
static struct timeval last_update_tv={-1, 0};
static int last_mode=0;
static double accel=1, accelinc=30, accelmax=100*100;
static long actmax=200, uptmin=50;

static void accel_reset()
{
	last_mode=0;
	accel=1.0;
	last_action_tv.tv_sec=-1;
	last_action_tv.tv_usec=-1;
}


/*EXTL_DOC
 * Set keyboard resize acceleration parameters. When a keyboard resize
 * function is called, and at most \var{t_max} milliseconds has passed
 * from a previous call, acceleration factor is reset to 1.0. Otherwise,
 * if at least \var{t_min} milliseconds have passed from the from previous
 * acceleration update or reset the squere root of the acceleration factor
 * is incremented by \var{step}. The maximum acceleration factor (pixels/call 
 * modulo size hints) is given by \var{maxacc}. The default values are 
 * (200, 50, 30, 100). 
 * 
 * Notice the interplay with X keyboard acceleration parameters.
 * (Maybe insteed of \var{t_min} we should use a minimum number of
 * calls to the function/key presses between updated? Or maybe the
 * resize should be completely time-based with key presses triggering
 * the changes?)
 */
EXTL_EXPORT
void set_resize_accel_params(int t_max, int t_min, double step, double maxacc)
{
	actmax=(t_max>0 ? t_max : INT_MAX);
	uptmin=(t_min>0 ? t_min : INT_MAX);
	accelinc=(step>0 ? step : 1);
	accelmax=(maxacc>0 ? maxacc*maxacc : 1);
}


static int sign(int x)
{
	return (x>0 ? 1 : (x<0 ? -1 : 0));
}


static long tvdiffmsec(struct timeval *tv1, struct timeval *tv2)
{
	double t1=1000*(double)tv1->tv_sec+(double)tv1->tv_usec/1000;
	double t2=1000*(double)tv2->tv_sec+(double)tv2->tv_usec/1000;
	
	return (int)(t1-t2);
}

#define SIGN_NZ(X) ((X) < 0 ? -1 : 1)

static double max(double a, double b)
{
	return (a<b ? b : a);
}

void kbresize_accel(int *wu, int *hu, int mode)
{
	struct timeval tv;
	long adiff, udiff;
	
	gettimeofday(&tv, NULL);
	
	adiff=tvdiffmsec(&tv, &last_action_tv);
	udiff=tvdiffmsec(&tv, &last_update_tv);
	
	if(last_mode==mode && adiff<actmax){
		if(udiff>uptmin){
			accel+=accelinc;
			if(accel>accelmax)
				accel=accelmax;
			last_update_tv=tv;
		}
	}else{
		accel=1.0;
		last_update_tv=tv;
	}
	
	last_mode=mode;
	last_action_tv=tv;
	
	/*
	if(*wu!=0)
		*wu=SIGN_NZ(*wu)*ceil(max(sqrt(accel), abs(*wu)));
	if(*hu!=0)
		*hu=SIGN_NZ(*hu)*ceil(max(sqrt(accel), abs(*hu)));
	 */
	if(*wu!=0)
		*wu=(*wu)*ceil(sqrt(accel)/abs(*wu));
	if(*hu!=0)
		*hu=(*hu)*ceil(sqrt(accel)/abs(*hu));
}


/*}}}*/


/*{{{ Keyboard resize handler */


static ExtlSafelist moveres_safe_funclist[]={
	(ExtlExportedFn*)&kbresize_resize,
	(ExtlExportedFn*)&kbresize_move,
	(ExtlExportedFn*)&kbresize_end,
	(ExtlExportedFn*)&kbresize_cancel,
	NULL
};


static bool resize_handler(WRegion *reg, XEvent *xev)
{
	XKeyEvent *ev=&xev->xkey;
	WBinding *binding=NULL;
	WBindmap **bindptr;
	
	if(ev->type==KeyRelease)
		return FALSE;
	
	assert(reg!=NULL);
	
	binding=lookup_binding(&ioncore_kbresize_bindmap, ACT_KEYPRESS,
						   ev->state, ev->keycode);
	
	if(!binding)
		return FALSE;
	
	if(binding!=NULL){
		const ExtlSafelist *old_safelist=
			extl_set_safelist(moveres_safe_funclist);
		extl_call(binding->func, "o", NULL, reg);
		extl_set_safelist(old_safelist);
	}
	
	return (resize_target()==NULL);
}


/*}}}*/


/*{{{ Resize timer */


static void tmr_end_resize(WTimer *unused)
{
	if(end_resize())
		grab_remove(resize_handler);
}


static WTimer resize_timer=INIT_TIMER(tmr_end_resize);


/*}}}*/


/*{{{ Misc. */


static int limit_and_encode_mode(int *left, int *right, 
								 int *top, int *bottom)
{
	*left=sign(*left);
	*right=sign(*right);
	*top=sign(*top);
	*bottom=sign(*bottom);

	return (*left)+(*right)*3+(*top)*9+(*bottom)*27;
}


void frame_kbresize_units(WFrame *frame, int *wret, int *hret)
{
	/*WGRData *grdata=GRDATA_OF(frame);
	*wret=grdata->w_unit;
	*hret=grdata->h_unit;*/
	*wret=1;
	*hret=1;
	
	if(WFRAME_CURRENT(frame)!=NULL){
		XSizeHints hints;
		
		region_resize_hints(WFRAME_CURRENT(frame), &hints, NULL, NULL);
		
		if(hints.flags&PResizeInc &&
		   (hints.width_inc>1 || hints.height_inc>1)){
			*wret=hints.width_inc;
			*hret=hints.height_inc;
		}
	}
}


/*}}}*/


/*{{{ Keyboard resize interface */


/*EXTL_DOC
 * Shrink or grow resize mode target one step in each direction.
 * Acceptable values for the parameters \var{left}, \var{right}, \var{top}
 * and \var{bottom} are as follows: -1: shrink along,
 * 0: do not change, 1: grow along corresponding border.
 */
EXTL_EXPORT
void kbresize_resize(int left, int right, int top, int bottom)
{
	int wu=0, hu=0;
	int mode=0;
    WRegion *reg=resize_target();
    
    if(reg==NULL || !WOBJ_IS(reg, WFrame))
        return;
	
	frame_kbresize_units((WFrame*)reg, &wu, &hu);
	
	mode=3*limit_and_encode_mode(&left, &right, &top, &bottom);
	kbresize_accel(&wu, &hu, mode);

	delta_resize(reg, -left*wu, right*wu, -top*hu, bottom*hu, NULL);
	
	set_timer(&resize_timer, wglobal.resize_delay);
}


/*EXTL_DOC
 * Move resize mode target one step:
 *
 * \begin{tabular}{rl}
 * \hline
 * \var{horizmul}/\var{vertmul} & effect \\\hline
 * -1 & Move left/up \\
 * 0 & No effect \\
 * 1 & Move right/down \\
 * \end{tabular}
 */
EXTL_EXPORT
void kbresize_move(int horizmul, int vertmul)
{
	int mode=0, dummy=0;
    WRegion *reg=resize_target();
    
    if(reg==NULL || !WOBJ_IS(reg, WFrame))
        return;
	
	mode=1+3*limit_and_encode_mode(&horizmul, &vertmul, &dummy, &dummy);
	kbresize_accel(&horizmul, &vertmul, mode);

	delta_resize(reg, horizmul, horizmul, vertmul, vertmul, NULL);
	
	set_timer(&resize_timer, wglobal.resize_delay);
}


/*EXTL_DOC
 * Return from move/resize mode and apply changes unless opaque
 * move/resize is enabled.
 */
EXTL_EXPORT
void kbresize_end()
{
    WRegion *reg=resize_target();
	if(end_resize()){
		reset_timer(&resize_timer);
		warp(reg);
		grab_remove(resize_handler);
	}
}


/*EXTL_DOC
 * Return from move/resize cancelling changes if opaque
 * move/resize has not been enabled.
 */
EXTL_EXPORT
void kbresize_cancel()
{
    WRegion *reg=resize_target();
	if(cancel_resize()){
		reset_timer(&resize_timer);
		warp(reg);
		grab_remove(resize_handler);
	}
}


/*EXTL_DOC
 * Enter move/resize mode for \var{frame}. The bindings set with
 * \fnref{kbresize_bindings} are used in this mode and of
 * of the exported functions only \fnref{kbresize_resize}, 
 * \fnref{kbresize_move}, \fnref{kbresize_cancel} and
 * \fnref{kbresize_end} are allowed to be called.
 */
EXTL_EXPORT_MEMBER
void frame_begin_kbresize(WFrame *frame)
{
	if(!begin_resize((WRegion*)frame, NULL, FALSE))
		return;

	accel_reset();
	
	grab_establish((WRegion*)frame, resize_handler,
				   (GrabKilledHandler*)kbresize_cancel, 0);
	
	set_timer(&resize_timer, wglobal.resize_delay);
}


/*}}}*/

