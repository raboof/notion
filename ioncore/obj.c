/*
 * ion/ioncore/obj.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "obj.h"
#include "objp.h"


WObjDescr OBJDESCR(WObj)={"WObj", NULL, NULL, NULL};


/*{{{ Destroy */


void destroy_obj(WObj *obj)
{
	WObjDescr *d;
	
	call_watches(obj);
	
	d=obj->obj_type;
	
	while(d!=NULL){
		if(d->destroy_fn!=NULL){
			d->destroy_fn(obj);
			break;
		}
		d=d->ancestor;
	}
	
	free(obj);
}


/*}}}*/


/*{{{ is/cast */


bool wobj_is(const WObj *obj, const WObjDescr *descr)
{
	WObjDescr *d;
	
	if(obj==NULL)
		return FALSE;
	
	d=obj->obj_type;
	
	while(d!=NULL){
		if(d==descr)
			return TRUE;
		d=d->ancestor;
	}
	return FALSE;
}


const void *wobj_cast(const WObj *obj, const WObjDescr *descr)
{
	WObjDescr *d;
	
	if(obj==NULL)
		return FALSE;
	
	d=obj->obj_type;
	
	while(d!=NULL){
		if(d==descr)
			return (void*)obj;
		d=d->ancestor;
	}
	return NULL;
}


/*}}}*/


/*{{{ Dynamic functions */


/* This function is called when no handler is found.
 */
static void dummy_dyn()
{
}


DynFun *lookup_dynfun(const WObj *obj, DynFun *func,
					  bool *funnotfound)
{
	const WObjDescr *descr=obj->obj_type;
	const DynFunTab *df;
	
	for(; descr!=&WObj_objdescr; descr=descr->ancestor){
		df=descr->funtab;
	
		if(df==NULL)
			continue;
		
		/* Naïve linear search --- could become too slow if there are a
		 * lot of "dynamic" functions. Let's hope there aren't. A simple
		 * but ugly solution would to sort the table runtime at first
		 * occurence of the object (yuck!) with qsort and then search
		 * with bsearch.
		 */
		while(df->func!=NULL){
			if(df->func==func){
				*funnotfound=FALSE;
				return df->handler;
			}
			df++;
		}
	}
	
	*funnotfound=TRUE;
	return dummy_dyn;
}


bool has_dynfun(const WObj *obj, DynFun *func)
{
	bool funnotfound;
	lookup_dynfun(obj, func, &funnotfound);
	return !funnotfound;
}


/*}}}*/


/*{{{ Watches */


void setup_watch(WWatch *watch, WObj *obj, WWatchHandler *handler)
{
	reset_watch(watch);
	
	watch->handler=handler;
	LINK_ITEM(obj->obj_watches, watch, next, prev);
	watch->obj=obj;
}

void do_reset_watch(WWatch *watch, bool call)
{
	WWatchHandler *handler=watch->handler;
	WObj *obj=watch->obj;
	
	watch->handler=NULL;
	
	if(obj==NULL)
		return;
	
	UNLINK_ITEM(obj->obj_watches, watch, next, prev);
	watch->obj=NULL;
	
	if(call && handler!=NULL)
		handler(watch, obj);
}


void reset_watch(WWatch *watch)
{
	do_reset_watch(watch, FALSE);
}


bool watch_ok(WWatch *watch)
{
	return watch->obj!=NULL;
}


void call_watches(WObj *obj)
{
	WWatch *watch, *next;

	watch=obj->obj_watches;
	
	while(watch!=NULL){
		next=watch->next;
		do_reset_watch(watch, TRUE);
		watch=next;
	}
}


void init_watch(WWatch *watch)
{
	watch->obj=NULL;
	watch->next=NULL;
	watch->prev=NULL;
	watch->handler=NULL;
}


/*}}}*/

