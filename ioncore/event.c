/*
 * ion/ioncore/event.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <X11/Xmd.h>

#include "common.h"
#include "global.h"
#include "signal.h"
#include "event.h"
#include "cursor.h"
#include "pointer.h"
#include "key.h"
#include "readfds.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>


/*{{{ Time updating */

#define CHKEV(E, T) case E: tm=((T*)ev)->time; break;
#define CLOCK_SKEW_MS 30000

static Time last_timestamp=CurrentTime;

void update_timestamp(XEvent *ev)
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


Time get_timestamp()
{
	if(last_timestamp==CurrentTime){
		/* Idea blatantly copied from wmx */
		XEvent ev;
		Atom dummy;
		
		D(fprintf(stderr, "Attempting to get time from X server."));
		
		dummy=XInternAtom(wglobal.dpy, "_ION_TIMEREQUEST", False);
		if(dummy==None){
			warn("Time request failed.");
			return 0;
		}
		/* TODO: use some other window that should also function as a
		 * NET_WM support check window.
		 */
		XChangeProperty(wglobal.dpy, wglobal.rootwins->dummy_win,
						dummy, dummy, 8, PropModeAppend,
						(unsigned char*)"", 0);
		get_event_mask(&ev, PropertyChangeMask);
		XPutBackEvent(wglobal.dpy, &ev);
	}
	
	return last_timestamp;
}


/*}}}*/


/*{{{ Event reading */


void get_event(XEvent *ev)
{
	fd_set rfds;
	int nfds=wglobal.conn;
	
	while(1){
		check_signals();
		
		if(QLength(wglobal.dpy)>0){
			XNextEvent(wglobal.dpy, ev);
			update_timestamp(ev);
			return;
		}
		
		XFlush(wglobal.dpy);
		
		FD_ZERO(&rfds);
		FD_SET(wglobal.conn, &rfds);
		
		set_input_fds(&rfds, &nfds);
		
		if(select(nfds+1, &rfds, NULL, NULL, NULL)>0){
			check_input_fds(&rfds);
			if(FD_ISSET(wglobal.conn, &rfds)){
				XNextEvent(wglobal.dpy, ev);
				update_timestamp(ev);
				return;
			}
		}
	}
}


void get_event_mask(XEvent *ev, long mask)
{
	fd_set rfds;
	
	while(1){
		check_signals();
		
		if(XCheckMaskEvent(wglobal.dpy, mask, ev)){
			update_timestamp(ev);
			return;
		}
		
		FD_ZERO(&rfds);
		FD_SET(wglobal.conn, &rfds);
		
		select(wglobal.conn+1, &rfds, NULL, NULL, NULL);
	}
}


/*}}}*/


/*{{{ Grab */


void do_grab_kb_ptr(Window win, Window confine_to, int cursor, long eventmask)
{
	wglobal.input_mode=INPUT_GRAB;
	
	XSelectInput(wglobal.dpy, win, ROOT_MASK&~eventmask);
	XGrabPointer(wglobal.dpy, win, True, GRAB_POINTER_MASK,
				 GrabModeAsync, GrabModeAsync, confine_to,
				 x_cursor(cursor), CurrentTime);
	XGrabKeyboard(wglobal.dpy, win, False, GrabModeAsync,
				  GrabModeAsync, CurrentTime);
	XSync(wglobal.dpy, False);
	XSelectInput(wglobal.dpy, win, ROOT_MASK);
}


void ungrab_kb_ptr()
{
	XUngrabKeyboard(wglobal.dpy, CurrentTime);
	XUngrabPointer(wglobal.dpy, CurrentTime);
	
	wglobal.input_mode=INPUT_NORMAL;
}


/*}}}*/

