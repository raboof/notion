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


/*{{{ Keyboard resize handler */


static const char* moveres_safe_funclist[]={
	"ionframe_grow_vert",
	"ionframe_shrink_vert",
	"ionframe_grow_horiz",
	"ionframe_shrink_horiz",
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


EXTL_EXPORT
void ionframe_begin_resize(WRegion *reg)
{
	grab_establish((WRegion*)reg, resize_handler,
				   FocusChangeMask|KeyReleaseMask);
	begin_resize_atexit(reg, NULL, end_keyresize);
}


static void ionframe_grow(WIonFrame *frame, int dir)
{
	int wu=0, hu=0;

	if(!may_resize((WRegion*)frame))
		return;
	
	genframe_resize_units((WGenFrame*)frame, &wu, &hu);
	
	if(dir==VERTICAL)
		wu=0;
	else
		hu=0;
		
	delta_resize((WRegion*)frame, -(wu-wu/2), wu/2, -(hu-hu/2), hu/2, NULL);
	set_resize_timer((WRegion*)frame, wglobal.resize_delay);
}


static void ionframe_shrink(WIonFrame *frame, int dir)
{
	int wu, hu;
	
	if(!may_resize((WRegion*)frame))
		return;

	genframe_resize_units((WGenFrame*)frame, &wu, &hu);
	
	if(dir==VERTICAL)
		wu=0;
	else
		hu=0;
	
	delta_resize((WRegion*)frame, (wu-wu/2), -wu/2, (hu-hu/2), -hu/2, NULL);
	set_resize_timer((WRegion*)frame, wglobal.resize_delay);
}


EXTL_EXPORT
void ionframe_grow_vert(WIonFrame *frame)
{
	ionframe_grow(frame, VERTICAL);
}


EXTL_EXPORT
void ionframe_shrink_vert(WIonFrame *frame)
{
	ionframe_shrink(frame, VERTICAL);
}


EXTL_EXPORT
void ionframe_grow_horiz(WIonFrame *frame)
{
	ionframe_grow(frame, HORIZONTAL);
}


EXTL_EXPORT
void ionframe_shrink_horiz(WIonFrame *frame)
{
	ionframe_shrink(frame, HORIZONTAL);
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

