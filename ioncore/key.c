/*
 * wmcore/key.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
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

static WBinding *lookup_binding_from_event(WWindow *thing, XKeyEvent *ev)
{   
	WBinding *binding;
	WBindmap **bindptr;
	
	bindptr=&thing->bindmap;
	assert(*bindptr!=NULL);
	
	binding=lookup_binding(*bindptr, ACT_KEYPRESS, ev->state, ev->keycode);
	return binding;
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


static bool submapgrab_handler(WRegion *reg, XEvent *ev)
{
	WBinding *binding;
	
	if(ev->type==KeyRelease)
		return FALSE;
	
	binding=lookup_binding_from_event((WWindow*)reg, &ev->xkey);
	
	/* if it is just a modifier, then return
	 */
	if(binding==NULL)
		if(ismod(ev->xkey.keycode)){
			return FALSE;
		}
	
	return dispatch_binding(reg, binding, &ev->xkey);
}
    
static void submapgrab(WRegion *reg)
{
	grab_establish(reg, submapgrab_handler, FocusChangeMask|KeyReleaseMask);
}


void handle_keypress(XKeyEvent *ev)
{
	WRegion *reg=NULL;
	WBinding *binding=NULL;
	WBindmap **bindptr;
	WScreen *scr;
	bool topmap=TRUE;
	bool toplvl=FALSE;
	
	/* Lookup the object that should receive the event and
	 * the action.
	 */
	/* this function gets called with grab_holder==NULL
	 */

		reg=(WRegion*)FIND_WINDOW(ev->window);
	
	if(reg==NULL || !WTHING_IS(reg, WWindow))
		return;
	
	bindptr=&(((WWindow*)reg)->bindmap);
	
	if(*bindptr==NULL)
		return;
	
	toplvl=((WWindow*)reg)->flags&WWINDOW_CLIENTCONT;
	
	/* Restore object's bindmap pointer to the its toplevel bindmap
	 * (if in submap mode).
	 */
	while((*bindptr)->parent!=NULL){
		topmap=FALSE;
		*bindptr=(*bindptr)->parent;
	}
	
	binding=lookup_binding_from_event((WWindow*)reg, ev);

	/* Is it a submap? Then handle it accordingly...
	 */
	if(binding!=NULL && binding->submap!=NULL){
		*bindptr=binding->submap;
		if(toplvl)
			submapgrab(reg);
		return;
	}

	/* Call the handler.
	 */
	if(binding!=NULL)
		dispatch_binding(reg, binding, ev);
	else if(topmap){
		insstr((WWindow*)reg, ev);
	}
}


