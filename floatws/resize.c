/*
 * ion/floatws/resize.c
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
#include "floatframe.h"
#include "main.h"


enum{
	VERTICAL,
	HORIZONTAL
};


/*{{{ Keyboard resize handler */


static const char* moveres_safe_funclist[]={
	"floatframe_do_resize",
	"floatframe_do_move",
	"floatframe_end_resize",
	"floatframe_cancel_resize",
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
		const char **old_safelist=extl_set_safelist(moveres_safe_funclist);
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


/*EXTL_DOC
 * Shrink or grow \var{frame} one or more steps in each direction.
 * Negative values for the parameters \var{left}, \var{right}, \var{top}
 * and \var{bottom} attempt to shrink the frame by altering the corresponding
 * border and positive values grow.
 */
EXTL_EXPORT
void floatframe_do_resize(WFloatFrame *frame, int left, int right,
						  int top, int bottom)
{
	int wu=0, hu=0;

	/* TODO: should use WEAK_ stuff? */
	
	if(!may_resize((WRegion*)frame))
		return;
	
	genframe_resize_units((WGenFrame*)frame, &wu, &hu);
	
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
EXTL_EXPORT
void floatframe_do_move(WFloatFrame *frame, int horizmul, int vertmul)
{
	int wu, hu;

	if(!may_resize((WRegion*)frame))
		return;
	
	wu=GRDATA_OF(frame)->w_unit*horizmul;
	hu=GRDATA_OF(frame)->h_unit*vertmul;

	delta_resize((WRegion*)frame, wu, wu, hu, hu, NULL);
	
	set_timer(&resize_timer, wglobal.resize_delay);
}


/*EXTL_DOC
 * Return from move/resize mode and apply changes unless opaque
 * move/resize is enabled.
 */
EXTL_EXPORT
void floatframe_end_resize(WFloatFrame *frame)
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
EXTL_EXPORT
void floatframe_cancel_resize(WFloatFrame *frame)
{
	if(cancel_resize()){
		reset_timer(&resize_timer);
		grab_remove(resize_handler);
	}
}


/*EXTL_DOC
 * Enter resize mode for \var{frame}.
 */
EXTL_EXPORT
void floatframe_begin_resize(WFloatFrame *frame)
{
	if(!begin_resize((WRegion*)frame, NULL, FALSE))
		return;
	
	grab_establish((WRegion*)frame, resize_handler,
				   (GrabKilledHandler*)floatframe_cancel_resize, 0);
	
	set_timer(&resize_timer, wglobal.resize_delay);
}


/*}}}*/

