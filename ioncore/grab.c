/*
 * ion/grab.c
 *
 * Copyright (c) Lukas Schroeder 2002.
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "common.h"
#include "global.h"
#include "event.h"
#include "rootwin.h"
#include "cursor.h"
#include "grab.h"

/* {{{ definitions */

typedef struct _grab_status{
	WRegion *holder;
	GrabHandler *handler;
	WWatch watch;
	long events;
	long flags;

	bool remove;	/* TRUE, if entry marked for removal by do_grab_remove() */
	int suspended;
}GrabStatus;

#define MAX_GRABS 10
static GrabStatus grabs[MAX_GRABS];
static GrabStatus *current_grab;
static int idx_grab=0;

/*}}}*/


static void grab_watch_handler(WWatch *w, WObj *obj)
{
	grab_holder_remove((WRegion*)obj);
	/*warn("Object destroyed while it's a grab->holder!");*/
}

static void do_grab_install(GrabStatus *grab)
{
	setup_watch(&grab->watch, (WObj*)grab->holder, grab_watch_handler);
	do_grab_kb_ptr(ROOT_OF(grab->holder), grab->holder, ~grab->events);
	current_grab=grab;
	change_grab_cursor(CURSOR_WAITKEY);
}

static void do_grab_remove()
{
	current_grab=NULL;
	ungrab_kb_ptr();

	while(idx_grab>0 && grabs[idx_grab-1].remove==TRUE){
		reset_watch(&grabs[idx_grab-1].watch);
		idx_grab--;
	}

	assert(idx_grab>=0);

	if(idx_grab>0 && !grabs[idx_grab-1].suspended){
		current_grab=&grabs[idx_grab-1];
		do_grab_install(current_grab);
	}
}

bool call_grab_handler(XEvent *ev)
{
	if(current_grab && current_grab->remove){
		do_grab_remove();
		return FALSE;
	}

	if(current_grab!=NULL
		&& current_grab->holder!=NULL 
		&& current_grab->handler!=NULL){

		bool allow;
		bool remove=FALSE;
		GrabStatus *tmp_grab=current_grab;

		/* here we also catch luser's that tried to grab without
           KeyPressMask|KeyReleaseMask */
        allow=!(current_grab->events&(KeyPressMask|KeyReleaseMask));
        if(!allow && ev->type==KeyRelease && current_grab->events&KeyReleaseMask)
            allow=TRUE;
        else if(!allow && ev->type==KeyPress && current_grab->events&KeyPressMask)
            allow=TRUE;

		if(allow){
			remove=current_grab->handler(current_grab->holder, ev);
			if(remove)
				grab_remove(tmp_grab->handler);
		}
		return TRUE;
	}
	return FALSE;
}

bool grab_held()
{
	return (idx_grab>0 && !grabs[idx_grab-1].suspended);
}

void grab_establish(WRegion *reg, GrabHandler *func, long eventmask)
{
	if(idx_grab<MAX_GRABS){
		current_grab=&grabs[idx_grab++];
		current_grab->holder=reg;
		current_grab->handler=func;
		current_grab->events=~eventmask;
		current_grab->remove=FALSE;
		current_grab->suspended=0;

		do_grab_install(current_grab);
	}
}

void grab_remove(GrabHandler *func)
{
	int i;
	for(i=idx_grab-1; i>=0; i--){
		if(grabs[i].handler==func){
			grabs[i].remove=TRUE;
			break;
		}
	}

	if(grabs[idx_grab-1].remove){
		do_grab_remove();
	}
}

void grab_holder_remove(WRegion *holder)
{
	int i;
	for(i=idx_grab-1; i>=0; i--){
		if(grabs[i].holder==holder)
			grabs[i].remove=TRUE;
	}

	if(grabs[idx_grab-1].remove)
		do_grab_remove();
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

void grab_suspend()
{
	if(idx_grab>0){
		if(!grabs[idx_grab-1].suspended)
			ungrab_kb_ptr();
		grabs[idx_grab-1].suspended++;
	}
}

void grab_resume()
{
	if(idx_grab>0 && grabs[idx_grab-1].suspended){
		current_grab=&grabs[idx_grab-1];
		current_grab->suspended--;
		do_grab_install(current_grab);
	}
}

