/*
 * ion/regbind.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <libtu/objp.h>
#include <libmainloop/defer.h>
#include "common.h"
#include "region.h"
#include "binding.h"
#include "regbind.h"


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
                         const WBindmap *UNUSED(bindmap))
{
    if(binding->area==0 && rbind->reg->flags&REGION_BINDINGS_ARE_GRABBED)
        do_binding_grab_on_ungrab_on(rbind->reg, binding, rbind->bindmap, TRUE);
}


void rbind_binding_removed(const WRegBindingInfo *rbind, 
                           const WBinding *binding,
                           const WBindmap *UNUSED(bindmap))
{
    if(binding->area==0 && rbind->reg->flags&REGION_BINDINGS_ARE_GRABBED)
        do_binding_grab_on_ungrab_on(rbind->reg, binding, rbind->bindmap, FALSE);
}


/*}}}*/


/*{{{ Find */


static WRegBindingInfo *find_rbind(WRegion *reg, WBindmap *bindmap,
                                   WRegion *owner)
{
    WRegBindingInfo *rbind;
    
    for(rbind=(WRegBindingInfo*)reg->bindings; rbind!=NULL; rbind=rbind->next){
        if(rbind->bindmap==bindmap && rbind->owner==owner)
            return rbind;
    }
    
    return NULL;
}


/*}}}*/


/*{{{ Interface */


static WRegBindingInfo *region_do_add_bindmap_owned(WRegion *reg, 
                                                    WBindmap *bindmap, 
                                                    WRegion *owner,
                                                    bool first)
{
    WRegBindingInfo *rbind;
    
    if(bindmap==NULL)
        return NULL;
    
    rbind=ALLOC(WRegBindingInfo);
    
    if(rbind==NULL)
        return NULL;
    
    rbind->bindmap=bindmap;
    rbind->owner=owner;
    rbind->reg=reg;
    rbind->tmp=0;
    
    LINK_ITEM(bindmap->rbind_list, rbind, bm_next, bm_prev);

    if(region_xwindow(reg)!=None && !(reg->flags&REGION_GRAB_ON_PARENT))
        grab_ungrabbed_bindings(reg, bindmap);
    
    /* Link to reg's rbind list*/ {
        WRegBindingInfo *b=reg->bindings;
        if(first){
            LINK_ITEM_FIRST(b, rbind, next, prev);
        }else{
            LINK_ITEM_LAST(b, rbind, next, prev);
        }
        reg->bindings=b;
    }
    
    return rbind;
}


bool region_add_bindmap(WRegion *reg, WBindmap *bindmap)
{
    if(find_rbind(reg, bindmap, NULL)!=NULL)
        return FALSE;
    return (region_do_add_bindmap_owned(reg, bindmap, NULL, TRUE)!=NULL);
}


static void remove_rbind(WRegion *reg, WRegBindingInfo *rbind)
{
    UNLINK_ITEM(rbind->bindmap->rbind_list, rbind, bm_next, bm_prev);
    
    /* Unlink from reg's rbind list*/ {
        WRegBindingInfo *b=reg->bindings;
        UNLINK_ITEM(b, rbind, next, prev);
        reg->bindings=b;
    }
    
    if(region_xwindow(reg)!=None && !(reg->flags&REGION_GRAB_ON_PARENT))
        ungrab_freed_bindings(reg, rbind->bindmap);

    free(rbind);
}


void region_remove_bindmap(WRegion *reg, WBindmap *bindmap)
{
    WRegBindingInfo *rbind=find_rbind(reg, bindmap, NULL);
    if(rbind!=NULL)
        remove_rbind(reg, rbind);
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


/*{{{ Update */


static void add_bindings(WRegion *reg, WRegion *r2)
{
    WRegion *rx=REGION_MANAGER(r2);
    WRegBindingInfo *rbind, *rb2;
    
    if(rx!=NULL && REGION_PARENT_REG(rx)==reg){
        /* The recursion is here to get the bindmaps correctly ordered. */
        add_bindings(reg, rx);
    }
    
    if(r2->flags&REGION_GRAB_ON_PARENT){
        for(rb2=(WRegBindingInfo*)r2->bindings; rb2!=NULL; rb2=rb2->next){
            rbind=find_rbind(reg, rb2->bindmap, r2);
            if(rbind==NULL){
                rbind=region_do_add_bindmap_owned(reg, rb2->bindmap, 
                                                  r2, TRUE);
            }
            if(rbind!=NULL)
                rbind->tmp=1;
        }
    }
}


void region_do_update_owned_grabs(WRegion *reg)
{
    WRegBindingInfo *rbind, *rb2;

    reg->flags&=~REGION_BINDING_UPDATE_SCHEDULED;
    
    /* clear flags */
    for(rbind=(WRegBindingInfo*)reg->bindings; rbind!=NULL; rbind=rbind->next)
        rbind->tmp=0;
    
    /* make new grabs */
    if(reg->active_sub!=NULL)
        add_bindings(reg, reg->active_sub);
    
    /* remove old grabs */
    for(rbind=(WRegBindingInfo*)reg->bindings; rbind!=NULL; rbind=rb2){
        rb2=rbind->next;
        if(rbind->tmp!=1 && rbind->owner!=NULL)
            remove_rbind(reg, rbind);
    }
}

void region_update_owned_grabs(WRegion *reg)
{
    if(reg->flags&REGION_BINDING_UPDATE_SCHEDULED 
       || OBJ_IS_BEING_DESTROYED(reg)
       || ioncore_g.opmode==IONCORE_OPMODE_DEINIT){
        return;
    }
    
    if(mainloop_defer_action((Obj*)reg, 
                             (WDeferredAction*)region_do_update_owned_grabs)){
        reg->flags|=REGION_BINDING_UPDATE_SCHEDULED;
    }else{
        region_do_update_owned_grabs(reg);
    }
}


/*}}}*/
