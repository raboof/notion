/*
 * ion/ioncore/defer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

/* This file contains routines for deferred execution of potentially
 * dangerous actions. They're called upon returning to the main
 * loop.
 */

#include "common.h"
#include "obj.h"
#include "defer.h"

INTRSTRUCT(Defer);
	
DECLSTRUCT(Defer){
	WWatch watch;
	DeferredAction *action;
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


static bool get_next(WObj **obj, DeferredAction **action, Defer **list)
{
	Defer *d=*list;
	
	if(d==NULL)
		return FALSE;
	
	UNLINK_ITEM(*list, d, next, prev);
	*obj=d->watch.obj;
	*action=d->action;
	reset_watch(&(d->watch));
	free_defer(d);
	return TRUE;
}


static void defer_watch_handler(WWatch *w, WObj *obj)
{
	Defer *d=(Defer*)w;
	
	UNLINK_ITEM(*(Defer**)(d->list), d, next, prev);
	free_defer(d);
	
	warn("Object destroyed while deferred actions pending.");
}

	
bool defer_action_on_list(WObj *obj, DeferredAction *action, void **list)
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
		setup_watch(&(d->watch), obj, defer_watch_handler);
	
	LINK_ITEM(*(Defer**)list, d, next, prev);
	
	return TRUE;
}


bool defer_action(WObj *obj, DeferredAction *action)
{
	return defer_action_on_list(obj, action, (void**)&deferred);
}


bool defer_destroy(WObj *obj)
{
	return defer_action(obj, destroy_obj);
}
	

void execute_deferred_on_list(void **list)
{
	WObj *obj;
	void (*action)(WObj*);
	
	while(get_next(&obj, &action, (Defer**)list))
		action(obj);
}


void execute_deferred()
{
	execute_deferred_on_list((void**)&deferred);
}
