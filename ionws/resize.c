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
#include "resize.h"
#include "split.h"
#include "bindmaps.h"
#include "ionframe.h"


/*{{{ Keyboard resize handler */


static const char* moveres_safe_funclist[]={
	"ionframe_do_resize",
	"ionframe_end_resize",
	"ionframe_cancel_resize",
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
		const char **old_safelist=extl_set_safelist(moveres_safe_funclist);
		extl_call(binding->func, "o", NULL, reg);
		extl_set_safelist(old_safelist);
	}
	
	return !is_resizing();
}


/*}}}*/


/*{{{ Keyboard resize interface */


static void end_keyresize()
{
	grab_remove(resize_handler);
}


/*EXTL_DOC
 * Enter resize mode for \var{frame}.
 */
EXTL_EXPORT
void ionframe_begin_resize(WIonFrame *frame)
{
	grab_establish((WRegion*)frame, resize_handler,
				   FocusChangeMask|KeyReleaseMask);
	begin_resize_atexit((WRegion*)frame, NULL, FALSE, end_keyresize);
}


/*EXTL_DOC
 * Shrink or grow \var{frame} one or more steps in each direction.
 * Negative values for the parameters \var{left}, \var{right}, \var{top}
 * and \var{bottom} attempt to shrink the frame by altering the corresponding
 * border and positive values grow.
 */
EXTL_EXPORT
void ionframe_do_resize(WIonFrame *frame, int left, int right,
						int top, int bottom)
{
	int wu=0, hu=0;

	/* TODO: should use WEAK_ stuff? */
	
	if(!may_resize((WRegion*)frame))
		return;
	
	genframe_resize_units((WGenFrame*)frame, &wu, &hu);
	
	delta_resize((WRegion*)frame, -left*wu, right*wu, -top*hu, bottom*hu,
				 NULL);
	set_resize_timer((WRegion*)frame, wglobal.resize_delay);
}


/*EXTL_DOC
 * Return from move/resize mode and apply changes unless opaque
 * move/resize is enabled.
 */
EXTL_EXPORT
void ionframe_end_resize(WIonFrame *frame)
{
	end_resize((WRegion*)frame);
}


/*EXTL_DOC
 * Return from move/resize cancelling changes if opaque
 * move/resize has not been enabled.
 */
EXTL_EXPORT
void ionframe_cancel_resize(WIonFrame *frame)
{
	cancel_resize((WRegion*)frame);
}


/*}}}*/

