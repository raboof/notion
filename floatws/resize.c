/*
 * ion/floatws/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */


#include <ioncore/global.h>
#include <ioncore/resize.h>
#include <ioncore/grab.h>
#include <ioncore/binding.h>
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


/*{{{ Keyboard resize interface */


static void end_keyresize()
{
	grab_remove(resize_handler);
}


/*EXTL_DOC
 * Enter resize mode for \var{frame}.
 */
EXTL_EXPORT
void floatframe_begin_resize(WFloatFrame *frame)
{
	grab_establish((WRegion*)frame, resize_handler,
				   FocusChangeMask|KeyReleaseMask);
	begin_resize_atexit((WRegion*)frame, NULL, FALSE, end_keyresize);
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
void floatframe_do_resize(WFloatFrame *frame, int horizmul, int vertmul)
{
	int wu=0, hu=0;

	if(!may_resize((WRegion*)frame))
		return;
	
	genframe_resize_units((WGenFrame*)frame, &wu, &hu);
	
	wu=wu*horizmul;
	hu=hu*vertmul;

	delta_resize((WRegion*)frame, 0, wu, 0, hu, NULL);
	set_resize_timer((WRegion*)frame, wglobal.resize_delay);
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
	set_resize_timer((WRegion*)frame, wglobal.resize_delay);
}


/*EXTL_DOC
 * Return from move/resize mode and apply changes unless opaque
 * move/resize is enabled.
 */
EXTL_EXPORT
void floatframe_end_resize(WFloatFrame *frame)
{
	end_resize((WRegion*)frame);
}

/*EXTL_DOC
 * Return from move/resize cancelling changes if opaque
 * move/resize has not been enabled.
 */
EXTL_EXPORT
void floatframe_cancel_resize(WFloatFrame *frame)
{
	cancel_resize((WRegion*)frame);
}

/*}}}*/

