/*
 * ion/ioncore/defer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

/* This file contains routines for deferred execution of potentially
 * dangerous actions. They're called upon returning to the main
 * loop.
 */

#include "common.h"
#include <libtu/obj.h>
#include <libtu/objp.h>
#include "defer.h"

INTRSTRUCT(Defer);
	
DECLSTRUCT(Defer){
	Watch watch;
	WDeferredAction *action;
	Defer *next, *prev;
	void **list;
};

static Defer *deferred=NULL;

#define N_DBUF 16

/* To avoid allocating memory all the time, we use a small
 * buffer that should be able to contain the small expected
 * number of simultaneous deferred actions.
 */
static Defer dbuf[N_DBUF];
static int dbuf_used=0;


static Defer *alloc_defer()
{
	int i;
	
	/* Keeping it simple -- this naive loop should do it
	 * as N_DBUF is small.
	 */
	for(i=0; i<N_DBUF; i++){
		if(!dbuf_used&(1<<i)){
			dbuf_used|=(1<<i);
			return dbuf+i;
		}
	}
	return ALLOC(Defer);
}


static void free_defer(Defer *d)
{
	if(d>=dbuf && d<dbuf+N_DBUF){
		dbuf_used&=~1<<((d-dbuf)/sizeof(Defer));
		return;
	}
	FREE(d);
}


static bool get_next(Obj **obj, WDeferredAction **action, Defer **list)
{
	Defer *d=*list;
	
	if(d==NULL)
		return FALSE;
	
	UNLINK_ITEM(*list, d, next, prev);
	*obj=d->watch.obj;
	*action=d->action;
	watch_reset(&(d->watch));
	free_defer(d);
	return TRUE;
}


static void defer_watch_handler(Watch *w, Obj *obj)
{
	Defer *d=(Defer*)w;
	
	UNLINK_ITEM(*(Defer**)(d->list), d, next, prev);
	free_defer(d);
	
	warn("Object destroyed while deferred actions pending.");
}

	
bool ioncore_defer_action_on_list(Obj *obj, WDeferredAction *action, 
                                  void **list)
{
	Defer *d;
	
	d=alloc_defer();
	
	if(d==NULL){
		warn("Unable to allocate a Defer structure for deferred action.");
		return FALSE;
	}
	
	d->action=action;
	d->list=list;
	
	if(obj!=NULL)
		watch_setup(&(d->watch), obj, defer_watch_handler);
	
	LINK_ITEM(*(Defer**)list, d, next, prev);
	
	return TRUE;
}


bool ioncore_defer_action(Obj *obj, WDeferredAction *action)
{
	return ioncore_defer_action_on_list(obj, action, (void**)&deferred);
}


bool ioncore_defer_destroy(Obj *obj)
{
	if(OBJ_IS_BEING_DESTROYED(obj))
		return FALSE;
	
	return ioncore_defer_action(obj, destroy_obj);
}
	

void ioncore_execute_deferred_on_list(void **list)
{
	Obj *obj;
	void (*action)(Obj*);
	
	while(get_next(&obj, &action, (Defer**)list))
		action(obj);
}


void ioncore_execute_deferred()
{
	ioncore_execute_deferred_on_list((void**)&deferred);
}

