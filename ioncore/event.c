/*
 * ion/ioncore/event.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <X11/Xmd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "common.h"
#include "global.h"
#include "event.h"
#include "eventh.h"
#include "focus.h"
#include "signal.h"



/*{{{ Hooks */


WHooklist *ioncore_handle_event_alt=NULL;


/*}}}*/


/*{{{ Timestamp management */

#define CHKEV(E, T) case E: tm=((T*)ev)->time; break;
#define CLOCK_SKEW_MS 30000

static Time last_timestamp=CurrentTime;

void ioncore_update_timestamp(XEvent *ev)
{
	Time tm;
	
	switch(ev->type){
	CHKEV(ButtonPress, XButtonPressedEvent);
	CHKEV(ButtonRelease, XButtonReleasedEvent);
	CHKEV(EnterNotify, XEnterWindowEvent);
	CHKEV(KeyPress, XKeyPressedEvent);
	CHKEV(KeyRelease, XKeyReleasedEvent);
	CHKEV(LeaveNotify, XLeaveWindowEvent);
	CHKEV(MotionNotify, XPointerMovedEvent);
	CHKEV(PropertyNotify, XPropertyEvent);
	CHKEV(SelectionClear, XSelectionClearEvent);
	CHKEV(SelectionNotify, XSelectionEvent);
	CHKEV(SelectionRequest, XSelectionRequestEvent);
	default:
		return;
	}

	if(tm>last_timestamp || last_timestamp - tm > CLOCK_SKEW_MS)
		last_timestamp=tm;
}


Time ioncore_get_timestamp()
{
	if(last_timestamp==CurrentTime){
		/* Idea blatantly copied from wmx */
		XEvent ev;
		Atom dummy;
		
		D(fprintf(stderr, "Attempting to get time from X server."));
		
		dummy=XInternAtom(ioncore_g.dpy, "_ION_TIMEREQUEST", False);
		if(dummy==None){
			warn("Time request failed.");
			return 0;
		}
		/* TODO: use some other window that should also function as a
		 * NET_WM support check window.
		 */
		XChangeProperty(ioncore_g.dpy, ioncore_g.rootwins->dummy_win,
						dummy, dummy, 8, PropModeAppend,
						(unsigned char*)"", 0);
		ioncore_get_event(&ev, PropertyChangeMask);
		XPutBackEvent(ioncore_g.dpy, &ev);
	}
	
	return last_timestamp;
}


/*}}}*/


/*{{{ Event reading */


void ioncore_get_event(XEvent *ev, long mask)
{
	fd_set rfds;
	
	while(1){
		ioncore_check_signals();
		
		if(XCheckMaskEvent(ioncore_g.dpy, mask, ev)){
			ioncore_update_timestamp(ev);
			return;
		}
		
		FD_ZERO(&rfds);
		FD_SET(ioncore_g.conn, &rfds);

        /* Other FD:s are _not_ to be handled! */
		select(ioncore_g.conn+1, &rfds, NULL, NULL, NULL);
	}
}


/*}}}*/


/*{{{ X connection FD handler */


static void skip_focusenter()
{
	XEvent ev;
	WRegion *r;
	
	XSync(ioncore_g.dpy, False);
	
	while(XCheckMaskEvent(ioncore_g.dpy,
						  EnterWindowMask|FocusChangeMask, &ev)){
		ioncore_update_timestamp(&ev);
		if(ev.type==FocusOut)
			ioncore_handle_focus_out(&(ev.xfocus));
		else if(ev.type==FocusIn)
			ioncore_handle_focus_in(&(ev.xfocus));
		/*else if(ev.type==EnterNotify)
			handle_enter_window(&ev);*/
	}
}



void ioncore_x_connection_handler(int conn, void *unused)
{
	XEvent ev;
    bool more=TRUE;

    while(more){
        XNextEvent(ioncore_g.dpy, &ev);
        ioncore_update_timestamp(&ev);
        
        CALL_ALT_B_NORET(ioncore_handle_event_alt, (&ev));

		XSync(ioncore_g.dpy, False);
		if(ioncore_g.focus_next!=NULL && 
           ioncore_g.input_mode==IONCORE_INPUTMODE_NORMAL){
			bool warp=ioncore_g.warp_next;
			WRegion *next=ioncore_g.focus_next;
			ioncore_g.focus_next=NULL;
			skip_focusenter();
			region_do_set_focus(next, warp);
        }

		/*ioncore_check_signals();
		ioncore_execute_deferred();*/
        
        more=(QLength(ioncore_g.dpy)>0);
    }
}


/*}}}*/

