/*
 * ion/ioncore/key.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ctype.h>
#include "common.h"
#include "key.h"
#include "binding.h"
#include "global.h"
#include "event.h"
#include "cursor.h"
#include <libtu/objp.h>
#include "grab.h"
#include "regbind.h"
#include "extl.h"
#include "region-iter.h"
#include "strings.h"


static void waitrelease(WRegion *reg);


static void insstr(WWindow *wwin, XKeyEvent *ev)
{
	static XComposeStatus cs={NULL, 0};
	char buf[32]={0,};
	Status stat;
	int n, i;
	KeySym ksym;
	
	if(wwin->xic!=NULL){
		if(XFilterEvent((XEvent*)ev, ev->window))
		   return;
		n=XmbLookupString(wwin->xic, ev, buf, 16, &ksym, &stat);
		if(stat!=XLookupChars && stat!=XLookupBoth)
			return;
	}else{
		n=XLookupString(ev, buf, 32, &ksym, &cs);
	}
	
	if(n<=0)
		return;
	
	/* Won't catch bad strings, but should filter out most crap. */
	if(ioncore_g.use_mb){
		if(!iswprint(str_wchar_at(buf, 32)))
			return;
	}else{
		if(iscntrl(*buf))
			return;
	}
	
	window_insstr(wwin, buf, n);
}


/* dispatch_binding
 * the return values are those expected by GrabHandler's, i.e.
 * you can just pass through the retval obtained from this function
 */
static bool dispatch_binding(WRegion *binding_owner, 
                             WRegion *grab_reg, WBinding *binding,
							 XKeyEvent *ev)
{
	WRootWin *rootwin;
	WRegion *subctx;
    
	if(binding!=NULL){
		/* Get the screen now for waitrel grab - the object might
		 * have been destroyed when call_binding returns.
		 */
        if(grab_reg==binding_owner || grab_reg==NULL)
            subctx=region_current(binding_owner);
        else
            subctx=grab_reg;
        
		extl_call(binding->func, "oo", NULL, binding_owner, subctx);
        
		if(ev->state!=0 && binding->waitrel)
			waitrelease(grab_reg);
	}
	return TRUE;
}


static void send_key(XEvent *ev, WClientWin *cwin)
{
	Window win=cwin->win;
	ev->xkey.window=win;
	ev->xkey.subwindow=None;
	XSendEvent(ioncore_g.dpy, win, False, KeyPressMask, ev);
}


static bool quote_next_handler(WRegion *reg, XEvent *xev)
{
	XKeyEvent *ev=&xev->xkey;
 	if(ev->type!=KeyPress)
		return FALSE;
	if(ioncore_ismod(ev->keycode))
		return FALSE;
	assert(OBJ_IS(reg, WClientWin));
	send_key(xev, (WClientWin*)reg);
	return TRUE; /* remove the grab */
}


/*EXTL_DOC
 * Send next key press directly to \var{cwin}.
 */
EXTL_EXPORT_MEMBER
void clientwin_quote_next(WClientWin *cwin)
{
    ioncore_grab_establish((WRegion*)cwin, quote_next_handler, NULL, 0);
	ioncore_change_grab_cursor(IONCORE_CURSOR_WAITKEY);
}


static bool waitrelease_handler(WRegion *reg, XEvent *ev)
{
	if(!ioncore_unmod(ev->xkey.state, ev->xkey.keycode))
		return TRUE;
	return FALSE;
}


static void waitrelease(WRegion *reg)
{
	/* We need to grab on the root window as <reg> might have been
	 * ioncore_defer_destroy:ed by the binding handler (the most common case
	 * for using this kpress_waitrel!). In such a case the grab may
	 * be removed before the modifiers are released.
	 */
	ioncore_grab_establish((WRegion*)region_rootwin_of(reg), 
                           waitrelease_handler, 
                           NULL, 0);
	ioncore_change_grab_cursor(IONCORE_CURSOR_WAITKEY);
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
		if(OBJ_IS(reg, WRootWin))
			break;
		reg=REGION_PARENT(reg);
	}

	/* if it is just a modifier, then return. */
	if(binding==NULL){
		if(ioncore_ismod(ev->xkey.keycode))
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
	ioncore_grab_establish(reg, submapgrab_handler, clear_subs, 0);
	ioncore_change_grab_cursor(IONCORE_CURSOR_WAITKEY);
}


void ioncore_do_handle_keypress(XKeyEvent *ev)
{
	WBinding *binding=NULL;
	WRegion *reg=NULL, *oreg=NULL, *binding_owner=NULL;
	bool grabbed;

	reg=(WRegion*)XWINDOW_REGION_OF(ev->window);
	
	if(reg==NULL)
		return;
	
	grabbed=(reg->flags&REGION_BINDINGS_ARE_GRABBED);
	
	oreg=reg;
	
	if(grabbed){
		/* Find the deepest nested active window grabbing this key. */
		while(reg->active_sub!=NULL)
			reg=reg->active_sub;
		
		do{
			if(reg->flags&REGION_BINDINGS_ARE_GRABBED){
				binding=region_lookup_keybinding(reg, ev, NULL, 
												 &binding_owner);
			}
			if(binding!=NULL)
				break;
			if(OBJ_IS(reg, WRootWin))
				break;
			reg=region_parent(reg);
		}while(reg!=NULL);
	}else{
		if(reg->submapstat.key!=None){
			submapgrab_handler(reg, (XEvent*)ev);
			return;
		}
		binding=region_lookup_keybinding(reg, ev, NULL, &binding_owner);
	}
	
	if(binding!=NULL){
		if(binding->submap!=NULL){
			if(add_sub(reg, ev->keycode, ev->state)){
				if(grabbed)
					submapgrab(reg);
			}
		}else{
			if(grabbed)
				XUngrabKeyboard(ioncore_g.dpy, CurrentTime);
			dispatch_binding(binding_owner, reg, binding, ev);
		}
	}else if(OBJ_IS(oreg, WWindow)){
		insstr((WWindow*)oreg, ev);
	}
}
