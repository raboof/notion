/*
 * wmcore/event.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

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


/*{{{ Event reading */


void get_event(XEvent *ev)
{
	fd_set rfds;
	int nfds=wglobal.conn;
	
	while(1){
		check_signals();
	
		if(QLength(wglobal.dpy)>0){
			XNextEvent(wglobal.dpy, ev);
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
 				return;
 			}
		}
	}
}


void get_event_mask(XEvent *ev, long mask)
{
	fd_set rfds;
	bool found=FALSE;
	
	while(1){
		check_signals();
		
		while(XCheckMaskEvent(wglobal.dpy, mask, ev)){
			if(ev->type!=MotionNotify)
				return;
			found=TRUE;
		}

		if(found)
			return;
		
		FD_ZERO(&rfds);
		FD_SET(wglobal.conn, &rfds);

		select(wglobal.conn+1, &rfds, NULL, NULL, NULL);
	}
}


/*}}}*/


/*{{{ Grab */


void do_grab_kb_ptr(Window win, WRegion *reg, long eventmask)
{
	wglobal.input_mode=INPUT_GRAB;
	
	XSelectInput(wglobal.dpy, win, ROOT_MASK&~eventmask);
	XGrabPointer(wglobal.dpy, win, True, GRAB_POINTER_MASK,
				 GrabModeAsync, GrabModeAsync, win,
				 x_cursor(CURSOR_DEFAULT), CurrentTime);
	XGrabKeyboard(wglobal.dpy, win, False, GrabModeAsync,
				  GrabModeAsync, CurrentTime);
	XSelectInput(wglobal.dpy, win, ROOT_MASK);
}


void grab_kb_ptr(WRegion *reg)
{
	do_grab_kb_ptr(ROOT_OF(reg), reg, FocusChangeMask);
}


void ungrab_kb_ptr()
{
	XUngrabKeyboard(wglobal.dpy, CurrentTime);
	XUngrabPointer(wglobal.dpy, CurrentTime);
	
	wglobal.input_mode=INPUT_NORMAL;
	wglobal.draw_dragwin=NULL;
}


/*}}}*/

