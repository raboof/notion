/*
 * ion/ioncore/obj.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include "common.h"
#include "obj.h"
#include "objp.h"


WClassDescr CLASSDESCR(WObj)={"WObj", NULL, 0, NULL, NULL};


static void do_watches(WObj *obj, bool call);


/*{{{ Destroy */


void destroy_obj(WObj *obj)
{
	WClassDescr *d;
	
	if(OBJ_IS_BEING_DESTROYED(obj))
		return;
	
	obj->flags|=OBJ_DEST;
	
	do_watches(obj, TRUE);
	
	d=obj->obj_type;
	
	while(d!=NULL){
		if(d->destroy_fn!=NULL){
			d->destroy_fn(obj);
			break;
		}
		d=d->ancestor;
	}
	
	do_watches(obj, FALSE);

	free(obj);
}


/*}}}*/


/*{{{ is/cast */


bool obj_is(const WObj *obj, const WClassDescr *descr)
{
	WClassDescr *d;
	
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


bool obj_is_str(const WObj *obj, const char *str)
{
	WClassDescr *d;
	
	if(obj==NULL || str==NULL)
		return FALSE;
	
	d=obj->obj_type;
	
	while(d!=NULL){
		if(strcmp(d->name, str)==0)
			return TRUE;
		d=d->ancestor;
	}
	return FALSE;
}


const void *obj_cast(const WObj *obj, const WClassDescr *descr)
{
	WClassDescr *d;
	
	if(obj==NULL)
		return NULL;
	
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


static int comp_fun(const void *a, const void *b)
{
	void *af=(void*)((DynFunTab*)a)->func;
	void *bf=(void*)((DynFunTab*)b)->func;
	
	return (af<bf ? -1 : (af==bf ? 0 : 1));
}

			
DynFun *lookup_dynfun(const WObj *obj, DynFun *func,
					  bool *funnotfound)
{
	WClassDescr *descr;
	DynFunTab *df;
	/*DynFunTab dummy={NULL, NULL};*/
	/*dummy.func=func;*/
	
	if(obj==NULL)
		return NULL;
	
	descr=obj->obj_type;
	
	for(; descr!=&WObj_classdescr; descr=descr->ancestor){
		if(descr->funtab==NULL)
			continue;
		
		if(descr->funtab_n==-1){
			/* Need to sort the table. */
			descr->funtab_n=0;
			df=descr->funtab;
			while(df->func!=NULL){
				descr->funtab_n++;
				df++;
			}
			
			if(descr->funtab_n>0){
				qsort(descr->funtab, descr->funtab_n, sizeof(DynFunTab),
					  comp_fun);
			}
		}
		
		/*
		if(descr->funtab_n==0)
			continue;
		
		df=(DynFunTab*)bsearch(&dummy, descr->funtab, descr->funtab_n,
							   sizeof(DynFunTab), comp_fun);
		if(df!=NULL){
			*funnotfound=FALSE;
			return df->handler;
		}
		*/
		
		/* Using custom bsearch instead of the one in libc is probably 
		 * faster as the overhead of calling a comparison function would
		 * be significant given that the comparisons are simple and 
		 * funtab_n not that big.
		 */
		{
			int min=0, max=descr->funtab_n-1;
			int ndx;
			df=descr->funtab;
			while(max>=min){
				ndx=(max+min)/2;
				if((void*)df[ndx].func==(void*)func){
					*funnotfound=FALSE;
					return df[ndx].handler;
				}
				if((void*)df[ndx].func<(void*)func)
					min=ndx+1;
				else
					max=ndx-1;
			}
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


bool watch_setup(WWatch *watch, WObj *obj, WWatchHandler *handler)
{
	if(OBJ_IS_BEING_DESTROYED(obj))
		return FALSE;
	
	watch_reset(watch);
	
	watch->handler=handler;
	LINK_ITEM(obj->obj_watches, watch, next, prev);
	watch->obj=obj;
	
	return TRUE;
}

void do_watch_reset(WWatch *watch, bool call)
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


void watch_reset(WWatch *watch)
{
	do_watch_reset(watch, FALSE);
}


bool watch_ok(WWatch *watch)
{
	return watch->obj!=NULL;
}


static void do_watches(WObj *obj, bool call)
{
	WWatch *watch, *next;

	watch=obj->obj_watches;
	
	while(watch!=NULL){
		next=watch->next;
		do_watch_reset(watch, call);
		watch=next;
	}
}


void watch_calles(WObj *obj)
{
	do_watches(obj, FALSE);
}


void watch_init(WWatch *watch)
{
	watch->obj=NULL;
	watch->next=NULL;
	watch->prev=NULL;
	watch->handler=NULL;
}


/*}}}*/

