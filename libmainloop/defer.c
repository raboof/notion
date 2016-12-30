/*
 * ion/libmainloop/defer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

/* This file contains routines for deferred execution of potentially
 * dangerous actions. They're called upon returning to the main
 * loop.
 */

#include <libtu/obj.h>
#include <libtu/objp.h>
#include <libtu/types.h>
#include <libtu/misc.h>
#include <libtu/dlist.h>
#include <libtu/output.h>
#include <libtu/locale.h>
#include <libtu/debug.h>
#include "defer.h"


DECLSTRUCT(WDeferred){
    Watch watch;
    WDeferredAction *action;
    ExtlFn fn;
    WDeferred *next, *prev;
    WDeferred **list;
};


static WDeferred *deferred=NULL;


#define N_DBUF 16

/* To avoid allocating memory all the time, we use a small
 * buffer that should be able to contain the small expected
 * number of simultaneous deferred actions.
 */
static WDeferred dbuf[N_DBUF];
static int dbuf_used=0;


static WDeferred *alloc_defer()
{
    int i;

    /* Keeping it simple -- this naive loop should do it
     * as N_DBUF is small.
     */
    for(i=0; i<N_DBUF; i++){
        if(!(dbuf_used&(1<<i))){
            dbuf_used|=(1<<i);
            return dbuf+i;
        }
    }
    return ALLOC(WDeferred);
}


static void free_defer(WDeferred *d)
{
    if(d>=dbuf && d<dbuf+N_DBUF){
        dbuf_used&=~(1<<(d-dbuf));
        return;
    }
    FREE(d);
}


static void defer_watch_handler(Watch *w, Obj *UNUSED(obj))
{
    WDeferred *d=(WDeferred*)w;

    UNLINK_ITEM(*(WDeferred**)(d->list), d, next, prev);

    free_defer(d);

    D(warn(TR("Object destroyed while deferred actions are still pending.")));
}


static bool already_deferred(Obj *obj, WDeferredAction *action,
                             WDeferred *list)
{
    WDeferred *d;

    for(d=list; d!=NULL; d=d->next){
        if(d->action==action && d->watch.obj==obj)
            return TRUE;
    }

    return FALSE;
}


bool mainloop_defer_action_on_list(Obj *obj, WDeferredAction *action,
                                   WDeferred **list)
{
    WDeferred *d;

    if(already_deferred(obj, action, *list))
        return TRUE;

    d=alloc_defer();

    if(d==NULL){
        warn_err();
        return FALSE;
    }

    d->action=action;
    d->list=list;
    d->fn=extl_fn_none();
    watch_init(&(d->watch));

    if(obj!=NULL)
        watch_setup(&(d->watch), obj, defer_watch_handler);

    LINK_ITEM(*list, d, next, prev);

    return TRUE;
}


bool mainloop_defer_action(Obj *obj, WDeferredAction *action)
{
    return mainloop_defer_action_on_list(obj, action, &deferred);
}


bool mainloop_defer_destroy(Obj *obj)
{
    if(OBJ_IS_BEING_DESTROYED(obj))
        return FALSE;

    return mainloop_defer_action(obj, destroy_obj);
}


bool mainloop_defer_extl_on_list(ExtlFn fn, WDeferred **list)
{
    WDeferred *d;

    d=alloc_defer();

    if(d==NULL){
        warn_err();
        return FALSE;
    }

    d->action=NULL;
    d->list=list;
    d->fn=extl_ref_fn(fn);
    watch_init(&(d->watch));

    LINK_ITEM(*list, d, next, prev);

    return TRUE;
}


/*EXTL_DOC
 * Defer execution of \var{fn} until the main loop.
 */
EXTL_SAFE
EXTL_EXPORT_AS(mainloop, defer)
bool mainloop_defer_extl(ExtlFn fn)
{
    return mainloop_defer_extl_on_list(fn, &deferred);
}


static void do_execute(WDeferred *d)
{
    Obj *obj=d->watch.obj;
    WDeferredAction *a=d->action;
    ExtlFn fn=d->fn;

    watch_reset(&(d->watch));
    free_defer(d);

    if(a!=NULL){
        /* The deferral should not be on the list, if there
         * was an object, and it got destroyed.
         */
        /*if(obj!=NULL)*/
        a(obj);
    }else if(fn!=extl_fn_none()){
        extl_call(fn, NULL, NULL);
        extl_unref_fn(fn);
    }
}


void mainloop_execute_deferred_on_list(WDeferred **list)
{
    while(*list!=NULL){
        WDeferred *d=*list;
        UNLINK_ITEM(*list, d, next, prev);
        do_execute(d);
    }
}


void mainloop_execute_deferred()
{
    mainloop_execute_deferred_on_list(&deferred);
}

