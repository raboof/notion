/*
 * ion/ioncore/grab.c
 * 
 * Copyright (c) Lukas Schroeder 2002,
 *				 Tuomo Valkonen 2003.
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include "common.h"
#include "global.h"
#include "event.h"
#include "cursor.h"
#include "grab.h"


/*{{{ Definitions */


typedef struct _grab_status{
	WRegion *holder;
	GrabHandler *handler;
	GrabKilledHandler *killedhandler;
	WWatch watch;
	long eventmask;
	long flags;

	bool remove;	/* TRUE, if entry marked for removal by do_grab_remove() */
	int cursor;
	Window confine_to;
	int sqid;
}GrabStatus;

#define MAX_GRABS 4
static GrabStatus grabs[MAX_GRABS];
static GrabStatus *current_grab;
static int idx_grab=0;
static int last_sqid=0;


/*}}}*/


/*{{{ Functions for installing grabs */


static void do_holder_remove(WRegion *holder, bool killed);

	
static void grab_watch_handler(WWatch *w, WObj *obj)
{
	do_holder_remove((WRegion*)obj, TRUE);
}


static void do_grab_install(GrabStatus *grab)
{
	setup_watch(&grab->watch, (WObj*)grab->holder, grab_watch_handler);
	do_grab_kb_ptr(ROOT_OF(grab->holder), grab->confine_to, grab->cursor,
				   grab->eventmask);
	current_grab=grab;
}


void grab_establish(WRegion *reg, GrabHandler *func, GrabKilledHandler *kh,
					long eventmask)
{
	assert((~eventmask)&(KeyPressMask|KeyReleaseMask));
	
	if(idx_grab<MAX_GRABS){
		current_grab=&grabs[idx_grab++];
		current_grab->holder=reg;
		current_grab->handler=func;
		current_grab->killedhandler=kh;
		current_grab->eventmask=eventmask;
		current_grab->remove=FALSE;
		current_grab->cursor=CURSOR_DEFAULT;
		current_grab->confine_to=ROOT_OF(reg);
		current_grab->sqid=last_sqid++;
		do_grab_install(current_grab);
	}
}


/*}}}*/


/*{{{ Grab removal functions */


static void do_grab_remove()
{
	current_grab=NULL;
	ungrab_kb_ptr();

	while(idx_grab>0 && grabs[idx_grab-1].remove==TRUE){
		reset_watch(&grabs[idx_grab-1].watch);
		idx_grab--;
	}

	assert(idx_grab>=0);

	if(idx_grab>0){
		current_grab=&grabs[idx_grab-1];
		do_grab_install(current_grab);
	}
}


static void mark_for_removal(GrabStatus *grab, bool killed)
{
	if(!grab->remove){
		grab->remove=TRUE;
		if(killed && grab->killedhandler!=NULL && grab->holder!=NULL)
			grab->killedhandler(grab->holder);
	}
	
	if(grabs[idx_grab-1].remove)
		do_grab_remove();
}


static void do_holder_remove(WRegion *holder, bool killed)
{
	int i;
	
	for(i=idx_grab-1; i>=0; i--){
		if(grabs[i].holder==holder)
			mark_for_removal(grabs+i, killed);
	}
}


void grab_holder_remove(WRegion *holder)
{
	do_holder_remove(holder, FALSE);
}


void grab_remove(GrabHandler *func)
{
	int i;
	for(i=idx_grab-1; i>=0; i--){
		if(grabs[i].handler==func){
			mark_for_removal(grabs+i, FALSE);
			break;
		}
	}
}


/*}}}*/


/*{{{ Grab handler calling */


bool call_grab_handler(XEvent *ev)
{
	GrabStatus *gr;
	int gr_sqid;
	
	while(current_grab && current_grab->remove)
		do_grab_remove();
	
	if(current_grab==NULL || current_grab->holder==NULL ||
	   current_grab->handler==NULL){
		return FALSE;
	}
	
	/* Escape key is harcoded to always kill active grab. */
	if(XLookupKeysym(&(ev->xkey), 0)==XK_Escape){
		mark_for_removal(current_grab, TRUE);
		return TRUE;
	}
	
	if(ev->type!=KeyRelease && ev->type!=KeyPress)
		return FALSE;
	
	/* We must check that the grab pointed to by current_grab still
	 * is the same grab and not already released or replaced by
	 * another grab.
	 */
	gr=current_grab;
	gr_sqid=gr->sqid;
	if(gr->handler(gr->holder, ev) && gr->sqid==gr_sqid)
		mark_for_removal(gr, FALSE);
	
	return TRUE;
}


/*}}}*/


/*{{{ Misc. */


bool grab_held()
{
	return idx_grab>0;
}


void change_grab_cursor(int cursor)
{
	if(current_grab!=NULL){
		current_grab->cursor=cursor;
		XChangeActivePointerGrab(wglobal.dpy, GRAB_POINTER_MASK,
								 x_cursor(cursor), CurrentTime);
	}
}


void grab_confine_to(Window confine_to)
{
	if(current_grab!=NULL){
		current_grab->confine_to=confine_to;
		XGrabPointer(wglobal.dpy, ROOT_OF(current_grab->holder), True,
					 GRAB_POINTER_MASK, GrabModeAsync, GrabModeAsync, 
					 confine_to, x_cursor(CURSOR_DEFAULT), CurrentTime);
	}
}


WRegion *grab_get_holder()
{
	if (grab_held())
		return grabs[idx_grab-1].holder;
	return NULL;
}


WRegion *grab_get_my_holder(GrabHandler *func)
{
	int i;
	for(i=idx_grab-1; i>=0; i--)
		if(grabs[i].handler==func)
			return grabs[i].holder;
	return NULL;
}


/*}}}*/

