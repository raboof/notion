/*
 * ion/ioncore/key.c
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
#include "extl.h"


static void waitrelease(WRegion *reg);


static void insstr(WWindow *wwin, XKeyEvent *ev)
{
	static XComposeStatus cs={NULL, 0};
	char buf[16]={0,};
	Status stat;
	int n, i;
	KeySym ksym;
	
	if(wwin->xic!=NULL){
		if(XFilterEvent((XEvent*)ev, ev->window))
		   return;
#ifdef CF_UTF8
		n=Xutf8LookupString(wwin->xic, ev, buf, 16, &ksym, &stat);
#else
		n=XmbLookupString(wwin->xic, ev, buf, 16, &ksym, &stat);
#endif
		if(stat!=XLookupChars && stat!=XLookupBoth)
			return;
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
static bool dispatch_binding(WRegion *mgr, WRegion *reg, WBinding *binding,
							 XKeyEvent *ev)
{
	WRootWin *rootwin;
	
	if(binding!=NULL){
		/* Get the screen now for waitrel grab - the object might
		 * have been destroyed when call_binding returns.
		 */
		extl_call(binding->func, "oo", NULL, mgr, reg);
		if(ev->state!=0 && binding->waitrel)
			waitrelease(reg);
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
 	if(ev->type!=KeyPress)
		return FALSE;
	if(ismod(ev->keycode))
		return FALSE;
	assert(WOBJ_IS(reg, WClientWin));
	send_key(xev, (WClientWin*)reg);
	return TRUE; /* remove the grab */
}


/*EXTL_DOC
 * Send next key press directly to \var{cwin}.
 */
EXTL_EXPORT
void clientwin_quote_next(WClientWin *cwin)
{
    grab_establish((WRegion*)cwin, quote_next_handler, FocusChangeMask);
	change_grab_cursor(CURSOR_WAITKEY);
}


static bool waitrelease_handler(WRegion *reg, XEvent *ev)
{
	if(!unmod(ev->xkey.state, ev->xkey.keycode))
		return TRUE;
	return FALSE;
}


static void waitrelease(WRegion *reg)
{
	grab_establish(reg, waitrelease_handler, FocusChangeMask|KeyPressMask);
	change_grab_cursor(CURSOR_WAITKEY);
}


static void free_subs(WSubmapState *p)
{
	WSubmapState *next;
	
	while(p!=NULL){
		next=p->next;
		free(p);
		p=next;
	}
}


static void clear_subs(WRegion *reg)
{
	reg->submapstat.key=None;
	free_subs(reg->submapstat.next);
	reg->submapstat.next=NULL;
}


static bool add_sub(WRegion *reg, uint key, uint state)
{
	WSubmapState *p=&(reg->submapstat);
	
	if(p->key!=None){
		if(p->next!=NULL)
			p=p->next;
	    p->next=ALLOC(WSubmapState);
		if(p->next==NULL)
			return FALSE;
		p=p->next;
		p->next=NULL;
	}
	
	p->key=key;
	p->state=state;
	
	return TRUE;

}


static bool submapgrab_handler(WRegion *reg, XEvent *ev)
{
	WRegion *oreg=reg;
	WRegion *binding_owner=NULL;
	WSubmapState *subchain=&(reg->submapstat);
	WBinding *binding=NULL;
	
	if(ev->type!=KeyPress)
		return FALSE;
	
	oreg=reg;
	
	while(reg!=NULL){
		binding=region_lookup_keybinding(reg, (XKeyEvent*)ev, subchain,
										 &binding_owner);
		if(binding!=NULL)
			break;
		if(WOBJ_IS(reg, WRootWin))
			break;
		reg=REGION_PARENT_CHK(reg, WRegion);
	}

	/* if it is just a modifier, then return. */
	if(binding==NULL){
		if(ismod(ev->xkey.keycode))
			return FALSE;
		clear_subs(oreg);
		return TRUE;
	}
	
	if(binding->submap!=NULL){
		if(add_sub(oreg, ev->xkey.keycode, ev->xkey.state)){
			return FALSE;
		}else{
			clear_subs(oreg);
			return TRUE;
		}
	}
	
	clear_subs(oreg);
	
	return dispatch_binding(binding_owner, reg, binding, &ev->xkey);
}


static void submapgrab(WRegion *reg)
{
	grab_establish(reg, submapgrab_handler, FocusChangeMask|KeyReleaseMask);
	change_grab_cursor(CURSOR_WAITKEY);
}


void handle_keypress(XKeyEvent *ev)
{
	WBinding *binding=NULL;
	WRegion *reg=NULL, *oreg=NULL, *binding_owner=NULL;

	reg=(WRegion*)FIND_WINDOW(ev->window);
	
	if(reg==NULL)
		return;
	
	while(reg->active_sub!=NULL)
		reg=reg->active_sub;
	
	oreg=reg;

	do{
		binding=region_lookup_keybinding(reg, ev, NULL, &binding_owner);
		if(binding!=NULL)
			break;
		if(WOBJ_IS(reg, WRootWin))
			break;
		reg=region_parent(reg);
	}while(reg!=NULL);
	
	if(binding!=NULL){
		if(binding->submap!=NULL){
			if(add_sub(reg, ev->keycode, ev->state))
				submapgrab(reg);
		}else{
			dispatch_binding(binding_owner, reg, binding, ev);
		}
	}else if(WOBJ_IS(oreg, WWindow)){
		insstr((WWindow*)oreg, ev);
	}
}
