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
	begin_resize_atexit((WRegion*)frame, NULL, end_keyresize);
}


/*EXTL_DOC
 * Shrink or grow \var{frame} one step horizontally and/or vertically
 * (must be in move/resize mode):
 *
 * \begin{tabular}{rl}
 * \hline
 * \var{horizmul}/\var{vertmul} & effect \\\hline
 * -1 & Shrink horizontally/vertically \\
 * 0 & No effect \\
 * 1 & Grow horizontally/vertically \\
 * \end{tabular}
 */
EXTL_EXPORT
void ionframe_do_resize(WIonFrame *frame, int horizmul, int vertmul)
{
	int wu=0, hu=0;

	if(!may_resize((WRegion*)frame))
		return;
	
	genframe_resize_units((WGenFrame*)frame, &wu, &hu);
	
	wu=wu*horizmul;
	hu=hu*vertmul;
		
	delta_resize((WRegion*)frame, -(wu-wu/2), wu/2, -(hu-hu/2), hu/2, NULL);
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

