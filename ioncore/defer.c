/*
 * wmcore/defer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

/* This file contains routines for deferred execution of potentially
 * dangerous actions. They're called upon returning to the main
 * loop.
 */

#include "common.h"
#include "thing.h"

typedef void Action(WThing*);

INTRSTRUCT(Defer)
	
DECLSTRUCT(Defer){
	WWatch watch;
	Action *action;
	Defer *next, *prev;
};

static Defer *deferred=NULL;

#define N_DBUF 4

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


static bool get_next(WThing **thing, Action **action)
{
	Defer *d=deferred;
	
	if(d==NULL)
		return FALSE;
	
	UNLINK_ITEM(deferred, d, next, prev);
	*thing=d->watch.thing;
	*action=d->action;
	reset_watch(&(d->watch));
	free_defer(d);
	return TRUE;
}


static void defer_watch_handler(WWatch *w, WThing *t)
{
	Defer *d=(Defer*)w;
	
	UNLINK_ITEM(deferred, d, next, prev);
	free_defer(d);
	
	warn("Thing destroyed while deferred actions pending.");
}

	
bool defer_action(WThing *thing, Action *action)
{
	Defer *d;
	
	/*fprintf(stderr, "defer: %p(%p)\n", action, thing);*/
	
	d=alloc_defer();
	
	if(d==NULL){
		warn("Unable to allocate a Defer structure for deferred action.");
		return FALSE;
	}
	
	d->action=action;
	
	if(thing!=NULL)
		setup_watch(&(d->watch), thing, defer_watch_handler);
	
	LINK_ITEM(deferred, d, next, prev);
	
	return TRUE;
}


bool defer_destroy(WThing *thing)
{
	return defer_action(thing, destroy_thing);
}
	

void execute_deferred()
{
	WThing *thing;
	void (*action)(WThing*);
	
	while(get_next(&thing, &action)){
		/*fprintf(stderr, "dexec: %p(%p)\n", action, thing);*/
		action(thing);
	}
}
