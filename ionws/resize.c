/*
 * ion/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */


#include <wmcore/global.h>
#include <wmcore/resize.h>
#include <wmcore/grab.h>
#include "resize.h"
#include "split.h"
#include "bindmaps.h"


/*{{{ keyboard handling */


static bool resize_handler(WRegion *thing, XEvent *xev)
{
	XKeyEvent *ev=&xev->xkey;
	WScreen *scr;
	WBinding *binding=NULL;
	WBindmap **bindptr;
	
	if(ev->type==KeyRelease)
		return FALSE;
	
	assert(thing && WTHING_IS(thing, WWindow));
	binding=lookup_binding(&ion_moveres_bindmap, ACT_KEYPRESS,
						   ev->state, ev->keycode);
	
	if(!binding)
		return FALSE;
	
	if(binding){
		/* Get the screen now for waitrel grab - the thing might
		 * have been destroyed when call_binding returns.
		 */
		scr=SCREEN_OF(thing);
		call_binding(binding, (WThing *)thing);
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


void grow(WRegion *reg)
{
	int wu=W_UNIT(reg), hu=H_UNIT(reg);
		
	if(!may_resize(reg))
		return;
	
	delta_resize(reg, -(wu-wu/2), wu/2, -(hu-hu/2), hu/2, NULL);
	set_resize_timer(reg, wglobal.resize_delay);
}


void shrink(WRegion *reg)
{
	int wu=W_UNIT(reg), hu=H_UNIT(reg);
	
	if(!may_resize(reg))
		return;
	
	delta_resize(reg, (wu-wu/2), -wu/2, (hu-hu/2), -hu/2, NULL);
	set_resize_timer(reg, wglobal.resize_delay);
}


void resize_vert(WRegion *reg)
{
	begin_keyresize(reg, VERTICAL);
}


void resize_horiz(WRegion *reg)
{
	begin_keyresize(reg, HORIZONTAL);
}


/*}}}*/


/*{{{ Maximize */


#define DO_MAXIMIZE(FRAME, WH, POS)                              \
	WRegion *par=FIND_PARENT1(FRAME, WRegion);                   \
	WRectangle geom=REGION_GEOM(FRAME);                          \
	int tmp, tmp2;                                               \
                                                                 \
	if(par==NULL)                                                \
		return;                                                  \
                                                                 \
	if((FRAME)->saved_##WH!=FRAME_NO_SAVED_WH){                  \
		geom.WH=(FRAME)->saved_##WH;                             \
		geom.POS=(FRAME)->saved_##POS;                           \
		region_request_geom((WRegion*)FRAME, geom, NULL, FALSE); \
		(FRAME)->saved_##WH=FRAME_NO_SAVED_WH;                   \
	}else{                                                       \
		tmp=geom.WH;                                             \
		tmp2=geom.POS;                                           \
		geom.WH=REGION_GEOM(par).WH;                             \
		geom.POS=0; /* Needed to resize both up and down */		 \
		region_request_geom((WRegion*)FRAME, geom, NULL, FALSE); \
		(FRAME)->saved_##WH=tmp;                                 \
		(FRAME)->saved_##POS=tmp2;                               \
	}

						
void maximize_vert(WFrame *frame)
{
	DO_MAXIMIZE(frame, h, y);
}


void maximize_horiz(WFrame *frame)
{
	DO_MAXIMIZE(frame, w, x);
}


/*}}}*/

