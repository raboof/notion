/*
 * ion/ioncore/thing.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "thing.h"
#include "thingp.h"
#include "global.h"
#include "region.h"


IMPLOBJ(WThing, WObj, NULL, NULL, NULL)


static void call_watches(WThing *thing);


/*{{{ Destroy */


void destroy_thing(WThing *t)
{
	WThing *p;

	call_watches(t);
	
	if(t->obj.obj_type->destroy_fn!=NULL)
		t->obj.obj_type->destroy_fn(t);

	destroy_subthings(t);

	unlink_thing(t);

	free(t);

	/* TODO: These should be "watches" */
	
	if(wglobal.focus_next==(WRegion*)t){
		/*warn("Region to be focused next destroyed.");*/
		wglobal.focus_next=NULL;
	}
}


void destroy_subthings(WThing *parent)
{
	WThing *t, *prev=NULL;

	assert(!(parent->flags&WTHING_SUBDEST));
	
	parent->flags|=WTHING_SUBDEST;
	
	/* destroy children */
	while(1){
		t=parent->t_children;
		if(t==NULL)
			break;
		assert(t!=prev);
		prev=t;
		destroy_thing(t);
	}
	
	parent->flags&=~WTHING_SUBDEST;
}


/*}}}*/


/*{{{ Linking */


void link_thing(WThing *parent, WThing *thing)
{
	LINK_ITEM(parent->t_children, thing, t_next, t_prev);
	thing->t_parent=parent;
}


void link_thing_before(WThing *before, WThing *thing)
{
	WThing *parent=before->t_parent;
	LINK_ITEM_BEFORE(parent->t_children, before, thing, t_next, t_prev);
	thing->t_parent=parent;
}


void link_thing_after(WThing *after, WThing *thing)
{
	WThing *parent=after->t_parent;
	LINK_ITEM_AFTER(parent->t_children, after, thing, t_next, t_prev);
	thing->t_parent=parent;
}


void unlink_thing(WThing *thing)
{
	WThing *parent=thing->t_parent;

	if(parent==NULL)
		return;
	
	UNLINK_ITEM(parent->t_children, thing, t_next, t_prev);
	thing->t_parent=NULL;
}


/*}}}*/


/*{{{ Scan */


static WThing *get_next_thing(WThing *first, const WObjDescr *descr)
{
	while(first!=NULL){
		if(wobj_is((WObj*)first, descr))
			break;
		first=first->t_next;
	}
	
	return first;
}


static WThing *get_prev_thing(WThing *first, const WObjDescr *descr)
{
	if(first==NULL)
		return NULL;
	
	while(1){
		first=first->t_prev;
		if(first->t_next==NULL)
			return NULL;
		if(wobj_is((WObj*)first, descr))
			break;
	}
	
	return first;
}


WThing *next_thing(WThing *first, const WObjDescr *descr)
{
	if(first==NULL)
		return NULL;
	
	return get_next_thing(first->t_next, descr);
}


WThing *next_thing_fb(WThing *first, const WObjDescr *descr, WThing *fb)
{
	WThing *r=NULL;
	if(first!=NULL)
		r=next_thing(first, descr);
	if(r==NULL)
		r=fb;
	return r;
}


WThing *prev_thing(WThing *first, const WObjDescr *descr)
{
	if(first==NULL)
		return NULL;
	
	return get_prev_thing(first, descr);
}


WThing *prev_thing_fb(WThing *first, const WObjDescr *descr, WThing *fb)
{
	WThing *r=NULL;
	if(first!=NULL)
		r=prev_thing(first, descr);
	if(r==NULL)
		r=fb;
	return r;
}


WThing *first_thing(WThing *parent, const WObjDescr *descr)
{
	if(parent==NULL)
		return NULL;
	
	return get_next_thing(parent->t_children, descr);
}


WThing *last_thing(WThing *parent, const WObjDescr *descr)
{
	WThing *p;
	
	if(parent==NULL)
		return NULL;
	
	p=parent->t_children;
	
	if(p==NULL)
		return NULL;
	
	p=p->t_prev;
	
	if(wobj_is((WObj*)p, descr))
		return p;
	
	return get_prev_thing(p, descr);
}


WThing *find_parent(WThing *p, const WObjDescr *descr)
{
	while(p!=NULL){
		if(wobj_is((WObj*)p, descr))
			break;
		p=p->t_parent;
	}
	
	return p;
}


WThing *find_parent1(WThing *p, const WObjDescr *descr)
{
	if(p==NULL || p->t_parent==NULL)
		return NULL;
	if(wobj_is((WObj*)p->t_parent, descr))
		return p->t_parent;
	return NULL;
}


WThing *nth_thing(WThing *parent, int n, const WObjDescr *descr)
{
	WThing *p;
	
	if(n<0)
		return NULL;
	
	p=first_thing(parent, descr);
	   
	while(n-- && p!=NULL)
		p=next_thing(p, descr);

	return p;
}


bool thing_is_ancestor(WThing *thing, WThing *thing2)
{
	while(thing!=NULL){
		if(thing==thing2)
			return TRUE;
		thing=thing->t_parent;
	}
	
	return FALSE;
}


bool thing_is_child(WThing *thing, WThing *thing2)
{
	return thing2->t_parent==thing;
}


/*}}}*/


/*{{{ Watches */


void setup_watch(WWatch *watch, WThing *thing, WWatchHandler *handler)
{
	reset_watch(watch);
	
	watch->handler=handler;
	LINK_ITEM(thing->t_watches, watch, next, prev);
	watch->thing=thing;
}

void do_reset_watch(WWatch *watch, bool call)
{
	WWatchHandler *handler=watch->handler;
	WThing *thing=watch->thing;
	
	watch->handler=NULL;
	
	if(thing==NULL)
		return;
	
	UNLINK_ITEM(thing->t_watches, watch, next, prev);
	watch->thing=NULL;
	
	if(call && handler!=NULL)
		handler(watch, thing);
}


void reset_watch(WWatch *watch)
{
	do_reset_watch(watch, FALSE);
}


bool watch_ok(WWatch *watch)
{
	return watch->thing!=NULL;
}


static void call_watches(WThing *thing)
{
	WWatch *watch, *next;

	watch=thing->t_watches;
	
	while(watch!=NULL){
		next=watch->next;
		do_reset_watch(watch, TRUE);
		watch=next;
	}
}


void init_watch(WWatch *watch)
{
	watch->thing=NULL;
	watch->next=NULL;
	watch->prev=NULL;
	watch->handler=NULL;
}


/*}}}*/

