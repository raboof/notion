/*
 * ion/ionws/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */


#include <ioncore/global.h>
#include <ioncore/resize.h>
#include <ioncore/grab.h>
#include <ioncore/binding.h>
#include "resize.h"
#include "split.h"
#include "bindmaps.h"
#include "ionframe.h"


/*{{{ keyboard handling */


static const char* moveres_safe_funclist[]={
	"ionframe_grow", "ionframe_shrink", "ionframe_end_resize",
	"ionframe_cancel_resize", NULL
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
		extl_call_restricted(binding->func, moveres_safe_funclist,
							 "o", NULL, reg);
	}
	
	return !is_resizing();
}


/*}}}*/


/*{{{ Keyboard resize */


static int tmpdir=0;


static void end_keyresize()
{
	grab_remove(resize_handler);
}


static void begin_keyresize(WRegion *reg, int dir)
{
	XSizeHints hints;
	uint relw, relh;

	grab_establish((WRegion*)reg, resize_handler,
				   FocusChangeMask|KeyReleaseMask);
	
	tmpdir=dir;
	begin_resize_atexit(reg, NULL, end_keyresize);
}


#define W_UNIT(REG) (tmpdir==HORIZONTAL ? SCREEN_OF(REG)->w_unit : 0)
#define H_UNIT(REG) (tmpdir==VERTICAL ? SCREEN_OF(REG)->h_unit : 0)


EXTL_EXPORT
void ionframe_grow(WIonFrame *frame)
{
	WRegion *reg=(WRegion*)frame;
	int wu=W_UNIT(reg), hu=H_UNIT(reg);
		
	if(!may_resize(reg))
		return;
	
	delta_resize(reg, -(wu-wu/2), wu/2, -(hu-hu/2), hu/2, NULL);
	set_resize_timer(reg, wglobal.resize_delay);
}


EXTL_EXPORT
void ionframe_shrink(WIonFrame *frame)
{
	WRegion *reg=(WRegion*)frame;
	int wu=W_UNIT(reg), hu=H_UNIT(reg);
	
	if(!may_resize(reg))
		return;
	
	delta_resize(reg, (wu-wu/2), -wu/2, (hu-hu/2), -hu/2, NULL);
	set_resize_timer(reg, wglobal.resize_delay);
}


EXTL_EXPORT
void ionframe_resize_vert(WIonFrame *frame)
{
	begin_keyresize((WRegion*)frame, VERTICAL);
}


EXTL_EXPORT
void ionframe_resize_horiz(WIonFrame *frame)
{
	begin_keyresize((WRegion*)frame, HORIZONTAL);
}


EXTL_EXPORT
void ionframe_end_resize(WIonFrame *frame)
{
	end_resize((WRegion*)frame);
}


EXTL_EXPORT
void ionframe_cancel_resize(WIonFrame *frame)
{
	cancel_resize((WRegion*)frame);
}


/*}}}*/

