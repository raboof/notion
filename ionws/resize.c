/*
 * ion/ionws/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ioncore/global.h>
#include <ioncore/resize.h>
#include <ioncore/grab.h>
#include <ioncore/binding.h>
#include <ioncore/signal.h>
#include "resize.h"
#include "split.h"
#include "bindmaps.h"
#include "ionframe.h"


/*{{{ Keyboard resize handler */


static ExtlSafelist moveres_safe_funclist[]={
	(ExtlExportedFn*)&ionframe_do_resize,
	(ExtlExportedFn*)&ionframe_end_resize,
	(ExtlExportedFn*)&ionframe_cancel_resize,
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
	
	binding=lookup_binding(&ionframe_moveres_bindmap, ACT_KEYPRESS,
						   ev->state, ev->keycode);
	
	if(!binding)
		return FALSE;
	
	if(binding!=NULL){
		const ExtlSafelist *old_safelist=
			extl_set_safelist(moveres_safe_funclist);
		extl_call(binding->func, "o", NULL, reg);
		extl_set_safelist(old_safelist);
	}
	
	return !is_resizing();
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


/*{{{ Keyboard resize interface */


static int sign(int x)
{
	return (x>0 ? 1 : (x<0 ? -1 : 0));
}


static int limit_and_encode_mode(int *left, int *right, 
								 int *top, int *bottom)
{
	*left=sign(*left);
	*right=sign(*right);
	*top=sign(*top);
	*bottom=sign(*bottom);

	return (*left)+(*right)*3+(*top)*9+(*bottom)*27;
}


/*EXTL_DOC
 * Shrink or grow \var{frame} one step in each direction.
 * Acceptable values for the parameters \var{left}, \var{right}, \var{top}
 * and \var{bottom} are as follows: -1: shrink along,
 * 0: do not change, 1: grow along corresponding border.
 */
EXTL_EXPORT_MEMBER
void ionframe_do_resize(WIonFrame *frame, int left, int right,
						int top, int bottom)
{
	int wu=0, hu=0;
	int mode=0;
	
	/* TODO: should use WEAK_ stuff? */
	
	if(!may_resize((WRegion*)frame))
		return;
	
	genframe_resize_units((WGenFrame*)frame, &wu, &hu);
	
	mode=limit_and_encode_mode(&left, &right, &top, &bottom);
	resize_accel(&wu, &hu, mode);

	delta_resize((WRegion*)frame, -left*wu, right*wu, -top*hu, bottom*hu, 
				 NULL);
	
	set_timer(&resize_timer, wglobal.resize_delay);
}


/*EXTL_DOC
 * Return from move/resize mode and apply changes unless opaque
 * move/resize is enabled.
 */
EXTL_EXPORT_MEMBER
void ionframe_end_resize(WIonFrame *frame)
{
	if(end_resize()){
		reset_timer(&resize_timer);
		grab_remove(resize_handler);
	}
}


/*EXTL_DOC
 * Return from move/resize cancelling changes if opaque
 * move/resize has not been enabled.
 */
EXTL_EXPORT_MEMBER
void ionframe_cancel_resize(WIonFrame *frame)
{
	if(cancel_resize()){
		reset_timer(&resize_timer);
		grab_remove(resize_handler);
	}
}


/*EXTL_DOC
 * Enter resize mode for \var{frame}. The bindings set with
 * \fnref{ionframe_moveres_bindings} are used in this mode and of
 * of the exported functions only \fnref{WIonFrame.do_resize}, 
 * \fnref{ionframe_cancel_resize} and \fnref{WIonFrame.end_resize}
 * are allowed to be called.
 */
EXTL_EXPORT_MEMBER
void ionframe_begin_resize(WIonFrame *frame)
{
	if(!begin_resize((WRegion*)frame, NULL, FALSE))
		return;
	
	grab_establish((WRegion*)frame, resize_handler,
				   (GrabKilledHandler*)ionframe_cancel_resize, 0);

	set_timer(&resize_timer, wglobal.resize_delay);
}


/*}}}*/

