/*
 * ion/regbind.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include "common.h"
#include "region.h"
#include "binding.h"
#include "regbind.h"
#include "objp.h"


/*{{{ Grab/ungrab */


static void do_grab_ungrab_binding(const WRegion *reg, const WBinding *binding,
								   const WBindmap *bindmap, bool grab)
{
	Window win=region_x_window(reg);
	WRegBindingInfo *r;
	
	for(r=reg->bindings; r!=NULL; r=r->next){
		if(r->bindmap==bindmap)
			continue;
		if(lookup_binding(r->bindmap, binding->act, binding->state,
						  binding->kcb)!=NULL)
			break;
	}
	if(r==NULL && binding->area==0){
		if(grab)
			grab_binding(binding, win);
		else
			ungrab_binding(binding, win);
	}
}


static void do_grab_ungrab_bindings(const WRegion *reg, const WBindmap *bindmap,
									bool grab)
{
	WBinding *binding=bindmap->bindings;
	int i;
	
	for(i=0; i<bindmap->nbindings; i++, binding++)
		do_grab_ungrab_binding(reg, binding, bindmap, grab);
}


static void grab_ungrabbed_bindings(const WRegion *reg, const WBindmap *bindmap)
{
	do_grab_ungrab_bindings(reg, bindmap, TRUE);
}


static void ungrab_freed_bindings(const WRegion *reg, const WBindmap *bindmap)
{
	do_grab_ungrab_bindings(reg, bindmap, FALSE);
}


void rbind_binding_added(const WRegBindingInfo *rbind, const WBinding *binding,
						 const WBindmap *bindmap)
{
	if(rbind->reg->flags&REGION_BINDINGS_ARE_GRABBED && binding->area==0)
		do_grab_ungrab_binding(rbind->reg, binding, rbind->bindmap, TRUE);
}


/*}}}*/


/*{{{ Find */


static WRegBindingInfo *find_rbind(WRegion *reg, WBindmap *bindmap)
{
	WRegBindingInfo *rbind;
	
	for(rbind=(WRegBindingInfo*)reg->bindings; rbind!=NULL; rbind=rbind->next){
		if(rbind->bindmap==bindmap)
			return rbind;
	}
	
	return NULL;
}


/*}}}*/


/*{{{ Interface */

#define REGION_SUPPORTS_BINDINGS(reg) WOBJ_IS(reg, WWindow)

bool region_add_bindmap_owned(WRegion *reg, WBindmap *bindmap, WRegion *owner)
{
	WRegBindingInfo *rbind;
	
	if(region_x_window(reg)==None)
		return FALSE;
	
	if(bindmap==NULL)
		return FALSE;
	
	if(find_rbind(reg, bindmap)!=NULL)
		return FALSE;
	
	rbind=ALLOC(WRegBindingInfo);
	
	if(rbind==NULL){
		warn_err();
		return FALSE;
	}
	
	rbind->bindmap=bindmap;
	rbind->owner=owner;
	rbind->reg=reg;
	LINK_ITEM(bindmap->rbind_list, rbind, bm_next, bm_prev);

	if(reg->flags&REGION_BINDINGS_ARE_GRABBED)
		grab_ungrabbed_bindings(reg, bindmap);
	
	/* Link to reg's rbind list*/ {
		WRegBindingInfo *b=reg->bindings;
		LINK_ITEM(b, rbind, next, prev);
		reg->bindings=b;
	}
	
	return TRUE;
}


bool region_add_bindmap(WRegion *reg, WBindmap *bindmap)
{
	return region_add_bindmap_owned(reg, bindmap, NULL);
}


static void remove_rbind(WRegion *reg, WRegBindingInfo *rbind)
{
	UNLINK_ITEM(rbind->bindmap->rbind_list, rbind, bm_next, bm_prev);
	
	/* Unlink from reg's rbind list*/ {
		WRegBindingInfo *b=reg->bindings;
		UNLINK_ITEM(b, rbind, next, prev);
		reg->bindings=b;
	}

	if(reg->flags&REGION_BINDINGS_ARE_GRABBED)
		ungrab_freed_bindings(reg, rbind->bindmap);

	free(rbind);
}


void region_remove_bindmap_owned(WRegion *reg, WBindmap *bindmap,
								 WRegion *owner)
{
	WRegBindingInfo *rbind=find_rbind(reg, bindmap);
	
	if(rbind!=NULL /*&& rbind->owner==owner*/)
		remove_rbind(reg, rbind);
}


void region_remove_bindmap(WRegion *reg, WBindmap *bindmap)
{
	region_remove_bindmap_owned(reg, bindmap, NULL);
}


void region_remove_bindings(WRegion *reg)
{
	WRegBindingInfo *rbind;
	
	while((rbind=(WRegBindingInfo*)reg->bindings)!=NULL)
		remove_rbind(reg, rbind);
}


WBinding *region_lookup_keybinding(WRegion *reg, const XKeyEvent *ev,
								   const WSubmapState *sc,
								   WRegion **binding_owner_ret)
{
	WRegBindingInfo *rbind;
	WBinding *binding=NULL;
	const WSubmapState *s=NULL;
	WBindmap *bindmap=NULL;
	int i;
	
	*binding_owner_ret=reg;
	
	for(rbind=(WRegBindingInfo*)reg->bindings; rbind!=NULL; rbind=rbind->next){
		bindmap=rbind->bindmap;
		
		for(s=sc; s!=NULL && bindmap!=NULL; s=s->next){
			binding=lookup_binding(bindmap, ACT_KEYPRESS, s->state, s->key);

			if(binding==NULL){
				bindmap=NULL;
				break;
			}
			
			bindmap=binding->submap;
		}
		
		if(bindmap==NULL)
			continue;

		binding=lookup_binding(bindmap, ACT_KEYPRESS, ev->state, ev->keycode);
		
		if(binding!=NULL)
			break;
	}
	
	if(binding!=NULL && rbind->owner!=NULL)
		*binding_owner_ret=rbind->owner;
	
	return binding;
}


WBinding *region_lookup_binding_area(WRegion *reg, int act, uint state,
									 uint kcb, int area)
{
	WRegBindingInfo *rbind;
	WBinding *binding=NULL;
	
	for(rbind=(WRegBindingInfo*)reg->bindings; rbind!=NULL; rbind=rbind->next){
		if(rbind->owner!=NULL)
			continue;
		binding=lookup_binding_area(rbind->bindmap, act, state, kcb, area);
		if(binding!=NULL)
			break;
	}
	
	return binding;
}


/*}}}*/

