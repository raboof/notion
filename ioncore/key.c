/*
 * wmcore/key.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <ctype.h>

#include "common.h"
#include "key.h"
#include "binding.h"
#include "global.h"
#include "event.h"
#include "cursor.h"
#include "objp.h"
#include "grab.h"
#include "regbind.h"


static void waitrelease(WScreen *screen);


static void insstr(WWindow *wwin, XKeyEvent *ev)
{
	static XComposeStatus cs={NULL, 0};
	char buf[16]={0,};
	Status stat;
	int n;
	KeySym ksym;
	
	if(wwin->xic!=NULL){
		if(XFilterEvent((XEvent*)ev, ev->window))
		   return;
		n=XmbLookupString(wwin->xic, ev, buf, 16, &ksym, &stat);
	}else{
		n=XLookupString(ev, buf, 16, &ksym, &cs);
	}
	
	if(n<=0 || *(uchar*)buf<32)
		return;

	window_insstr(wwin, buf, n);
}


/* dispatch_binding
 * the return values are those expected by GrabHandler's, i.e.
 * you can just pass through the retval obtained from this function
 */
static bool dispatch_binding(WRegion *reg, WBinding *binding, XKeyEvent *ev)
{
	WScreen *scr;
	
	if(binding){
		/* Get the screen now for waitrel grab - the thing might
		 * have been destroyed when call_binding returns.
		 */
		scr=SCREEN_OF(reg);
		call_binding(binding, (WThing *)reg);
		if(ev->state!=0 && binding->waitrel){
			waitrelease(scr);
			/* return FALSE here to prevent uninstalling the waitrelease handler
			   immediately after establishing it */
			return FALSE;
		}
	}
	return TRUE;
}


static void send_key(XEvent *ev, WClientWin *cwin)
{
	Window win=cwin->win;
	ev->xkey.window=win;
	ev->xkey.subwindow=None;
	XSendEvent(wglobal.dpy, win, False, KeyPressMask, ev);
}


static bool quote_next_handler(WRegion *reg, XEvent *xev)
{
	XKeyEvent *ev=&xev->xkey;
 	if(ev->type==KeyRelease)
		return FALSE;
	if(ismod(ev->keycode))
		return FALSE;
	assert(WTHING_IS(reg, WClientWin));
	send_key(xev, (WClientWin*)reg);
	return TRUE; /* remove the grab */
}


void quote_next(WClientWin *cwin)
{
    grab_establish((WRegion*)cwin, quote_next_handler, FocusChangeMask);
}


static bool waitrelease_handler(WRegion *thing, XEvent *ev)
{
	if(!unmod(ev->xkey.state, ev->xkey.keycode))
		return TRUE;
	return FALSE;
}


static void waitrelease(WScreen *screen)
{
	grab_establish((WRegion*)screen, waitrelease_handler,
				   FocusChangeMask|KeyPressMask);
}


static void kgrab_binding_and_reg(WBinding **binding_ret,
								  WRegion **binding_owner_ret,
								  WRegion *reg, XKeyEvent *ev)
{
	WBinding *binding;
	WSubmapState *subchain;
	
	*binding_ret=NULL;
	*binding_owner_ret=reg;
	
	subchain=&(reg->submapstat);
		/*(((WRegion*)SCREEN_OF(reg))->submapstat.key==None
			  ? NULL
			  : &(((WRegion*)SCREEN_OF(reg))->submapstat));*/
	
	while(reg!=NULL){
		binding=region_lookup_keybinding(reg, ev, subchain, binding_owner_ret);
		if(binding!=NULL){
			*binding_ret=binding;
			return;
		}
		if(WTHING_IS(reg, WScreen))
			break;
		reg=FIND_PARENT1(reg, WRegion);
	}
}


static void clear_subs(WRegion *reg)
{
	reg->submapstat.key=None;
}


static bool add_sub(WRegion *reg, uint key, uint state)
{
	if(reg->submapstat.key!=None)
		return FALSE;
	   
	reg->submapstat.key=key;
	reg->submapstat.state=state;
	
	return TRUE;
}


static bool submapgrab_handler(WRegion *reg, XEvent *ev)
{
	WBinding *binding=NULL;
	WRegion *binding_owner=NULL;
	
	if(ev->type==KeyRelease)
		return FALSE;
	
	kgrab_binding_and_reg(&binding, &binding_owner, reg, &ev->xkey);

	/* if it is just a modifier, then return. */
	if(binding==NULL){
		if(ismod(ev->xkey.keycode))
			return FALSE;
		clear_subs(reg);
		return TRUE;
	}
	
	if(binding->submap!=NULL){
		if(add_sub(reg, ev->xkey.keycode, ev->xkey.state)){
			return FALSE;
		}else{
			clear_subs(reg);
			return TRUE;
		}
	}
	
	clear_subs(reg);
	
	return dispatch_binding(binding_owner, binding, &ev->xkey);
}


static void submapgrab(WRegion *reg)
{
	grab_establish(reg, submapgrab_handler, FocusChangeMask|KeyReleaseMask);
}


void handle_keypress(XKeyEvent *ev)
{
	WBinding *binding=NULL;
	WRegion *reg=NULL, *oreg=NULL, *binding_owner=NULL;
	
	if(ev->subwindow!=None)
		reg=(WRegion*)FIND_WINDOW(ev->subwindow);
	if(reg==NULL)
		reg=(WRegion*)FIND_WINDOW(ev->window);
	if(reg==NULL)
		return;
	oreg=reg;

	do{
		binding=region_lookup_keybinding(reg, ev, NULL, &binding_owner);
		if(binding!=NULL)
			break;
		if(WTHING_IS(reg, WScreen))
			break;
		reg=FIND_PARENT1(reg, WRegion);
	}while(reg!=NULL);
	
	if(binding!=NULL){
		if(binding->submap!=NULL){
			if(add_sub(reg, ev->keycode, ev->state))
				submapgrab(reg);
		}else{
			dispatch_binding(binding_owner, binding, ev);
		}
	}else if(WTHING_IS(oreg, WWindow)){
		insstr((WWindow*)oreg, ev);
	}
}
