/*
 * ion/ioncore/obj.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "obj.h"
#include "objp.h"


WObjDescr OBJDESCR(WObj)={"WObj", NULL, NULL, NULL, NULL};


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
