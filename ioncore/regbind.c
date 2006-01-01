/*
 * ion/regbind.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
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
#include <libtu/objp.h>


/*{{{ Grab/ungrab */


static void do_binding_grab_on_ungrab_on(const WRegion *reg, 
                                         const WBinding *binding,
                                         const WBindmap *bindmap, bool grab)
{
    Window win=region_xwindow(reg);
    WRegBindingInfo *r;
    
    for(r=reg->bindings; r!=NULL; r=r->next){
        if(r->bindmap==bindmap)
            continue;
        if(bindmap_lookup_binding(r->bindmap, binding->act, binding->state,
                                  binding->kcb)!=NULL)
            break;
    }
    if(r==NULL && binding->area==0){
        if(grab)
            binding_grab_on(binding, win);
        else
            binding_ungrab_on(binding, win);
    }
}


static void do_binding_grab_on_ungrab_ons(const WRegion *reg, 
                                          const WBindmap *bindmap,
                                          bool grab)
{
    Rb_node node=NULL;
    WBinding *binding=NULL;
    
    if(!(reg->flags&REGION_BINDINGS_ARE_GRABBED) ||
       bindmap->bindings==NULL){
        return;
    }
    
    FOR_ALL_BINDINGS(binding, node, bindmap->bindings){
        do_binding_grab_on_ungrab_on(reg, binding, bindmap, grab);
    }
}


static void grab_ungrabbed_bindings(const WRegion *reg, const WBindmap *bindmap)
{
    do_binding_grab_on_ungrab_ons(reg, bindmap, TRUE);
}


static void ungrab_freed_bindings(const WRegion *reg, const WBindmap *bindmap)
{
    do_binding_grab_on_ungrab_ons(reg, bindmap, FALSE);
}


void rbind_binding_added(const WRegBindingInfo *rbind, 
                         const WBinding *binding,
                         const WBindmap *bindmap)
{
    if(binding->area==0 && rbind->reg->flags&REGION_BINDINGS_ARE_GRABBED)
        do_binding_grab_on_ungrab_on(rbind->reg, binding, rbind->bindmap, TRUE);
}


void rbind_binding_removed(const WRegBindingInfo *rbind, 
                           const WBinding *binding,
                           const WBindmap *bindmap)
{
    if(binding->area==0 && rbind->reg->flags&REGION_BINDINGS_ARE_GRABBED)
        do_binding_grab_on_ungrab_on(rbind->reg, binding, rbind->bindmap, FALSE);
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


bool region_add_bindmap_owned(WRegion *reg, WBindmap *bindmap, WRegion *owner)
{
    WRegBindingInfo *rbind;
    
    if(region_xwindow(reg)==None)
        return FALSE;
    
    if(bindmap==NULL)
        return FALSE;
    
    if(find_rbind(reg, bindmap)!=NULL)
        return FALSE;
    
    rbind=ALLOC(WRegBindingInfo);
    
    if(rbind==NULL)
        return FALSE;
    
    rbind->bindmap=bindmap;
    rbind->owner=owner;
    rbind->reg=reg;
    LINK_ITEM(bindmap->rbind_list, rbind, bm_next, bm_prev);

    grab_ungrabbed_bindings(reg, bindmap);
    
    /* Link to reg's rbind list*/ {
        WRegBindingInfo *b=reg->bindings;
        if(owner==NULL){
            LINK_ITEM_FIRST(b, rbind, next, prev);
        }else{
            LINK_ITEM_LAST(b, rbind, next, prev);
        }
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
    WRegBindingInfo *rbind=NULL;
    WBinding *binding=NULL;
    const WSubmapState *s=NULL;
    WBindmap *bindmap=NULL;
    int i;
    
    *binding_owner_ret=reg;
    
    for(rbind=(WRegBindingInfo*)reg->bindings; rbind!=NULL; rbind=rbind->next){
        bindmap=rbind->bindmap;
        
        for(s=sc; s!=NULL && bindmap!=NULL; s=s->next){
            binding=bindmap_lookup_binding(bindmap, BINDING_KEYPRESS, s->state, s->key);

            if(binding==NULL){
                bindmap=NULL;
                break;
            }
            
            bindmap=binding->submap;
        }
        
        if(bindmap==NULL){
            /* There may be no next iteration so we must reset binding here
             * because we have not found a proper binding.
             */
            binding=NULL;
            continue;
        }

        binding=bindmap_lookup_binding(bindmap, BINDING_KEYPRESS, ev->state, ev->keycode);
        
        if(binding!=NULL)
            break;
    }
    
    if(binding!=NULL && rbind->owner!=NULL)
        *binding_owner_ret=rbind->owner;
    
    return binding;
}


WBinding *region_lookup_binding(WRegion *reg, int act, uint state,
                                     uint kcb, int area)
{
    WRegBindingInfo *rbind;
    WBinding *binding=NULL;
    
    for(rbind=(WRegBindingInfo*)reg->bindings; rbind!=NULL; rbind=rbind->next){
        if(rbind->owner!=NULL)
            continue;
        binding=bindmap_lookup_binding_area(rbind->bindmap, act, state, kcb, area);
        if(binding!=NULL)
            break;
    }
    
    return binding;
}


/*}}}*/

