/*
 * ion/floatws/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include "floatframe.h"
#include "main.h"


enum{
	VERTICAL,
	HORIZONTAL
};


/*{{{ Keyboard resize handler */


static ExtlSafelist moveres_safe_funclist[]={
	(ExtlExportedFn*)&floatframe_do_resize,
	(ExtlExportedFn*)&floatframe_do_move,
	(ExtlExportedFn*)&floatframe_end_resize,
	(ExtlExportedFn*)&floatframe_cancel_resize,
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
	
	binding=lookup_binding(&floatframe_moveres_bindmap, ACT_KEYPRESS,
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
void floatframe_do_resize(WFloatFrame *frame, int left, int right,
						  int top, int bottom)
{
	int wu=0, hu=0;
	int mode=0;
	
	if(!may_resize((WRegion*)frame))
		return;
	
	genframe_resize_units((WGenFrame*)frame, &wu, &hu);
	
	mode=3*limit_and_encode_mode(&left, &right, &top, &bottom);
	resize_accel(&wu, &hu, mode);

	delta_resize((WRegion*)frame, -left*wu, right*wu, -top*hu, bottom*hu,
				 NULL);
	
	set_timer(&resize_timer, wglobal.resize_delay);
}


/*EXTL_DOC
 * Move \var{frame} one step
 * (must be in move/resize mode):
 *
 * \begin{tabular}{rl}
 * \hline
 * \var{horizmul}/\var{vertmul} & effect \\\hline
 * -1 & Move left/up \\
 * 0 & No effect \\
 * 1 & Move right/down \\
 * \end{tabular}
 */
EXTL_EXPORT_MEMBER
void floatframe_do_move(WFloatFrame *frame, int horizmul, int vertmul)
{
	int mode=0, dummy=0;
	
	if(!may_resize((WRegion*)frame))
		return;
	
	mode=1+3*limit_and_encode_mode(&horizmul, &vertmul, &dummy, &dummy);
	resize_accel(&horizmul, &vertmul, mode);

	delta_resize((WRegion*)frame, horizmul, horizmul, vertmul, vertmul, NULL);
	
	set_timer(&resize_timer, wglobal.resize_delay);
}


/*EXTL_DOC
 * Return from move/resize mode and apply changes unless opaque
 * move/resize is enabled.
 */
EXTL_EXPORT_MEMBER
void floatframe_end_resize(WFloatFrame *frame)
{
	if(end_resize()){
		reset_timer(&resize_timer);
		warp((WRegion*)frame);
		grab_remove(resize_handler);
	}
}


/*EXTL_DOC
 * Return from move/resize cancelling changes if opaque
 * move/resize has not been enabled.
 */
EXTL_EXPORT_MEMBER
void floatframe_cancel_resize(WFloatFrame *frame)
{
	if(cancel_resize()){
		reset_timer(&resize_timer);
		warp((WRegion*)frame);
		grab_remove(resize_handler);
	}
}


/*EXTL_DOC
 * Enter move/resize mode for \var{frame}. The bindings set with
 * \fnref{floatframe_moveres_bindings} are used in this mode and of
 * of the exported functions only \fnref{WFloatFrame.do_resize}, 
 * \fnref{WFloatFrame.do_move}, \fnref{WFloatFrame.cancel_resize} and
 * \fnref{WFloatFrame.end_resize} are allowed to be called.
 */
EXTL_EXPORT_MEMBER
void floatframe_begin_resize(WFloatFrame *frame)
{
	if(!begin_resize((WRegion*)frame, NULL, FALSE))
		return;
	
	grab_establish((WRegion*)frame, resize_handler,
				   (GrabKilledHandler*)floatframe_cancel_resize, 0);
	
	set_timer(&resize_timer, wglobal.resize_delay);
}


/*}}}*/

