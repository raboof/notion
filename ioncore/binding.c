/*
 * ion/ioncore/binding.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include "common.h"
#include "event.h"
#include "binding.h"
#include "global.h"
#include <libtu/objp.h>
#include "regbind.h"
#include <libextl/extl.h>


#ifndef CF_NO_LOCK_HACK
#define CF_HACK_IGNORE_EVIL_LOCKS
#endif

#ifdef CF_HACK_IGNORE_EVIL_LOCKS
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#endif


/* */


#define N_MODS 8

static const uint modmasks[N_MODS]={
    ShiftMask, LockMask, ControlMask, Mod1Mask, Mod2Mask, Mod3Mask,
    Mod4Mask, Mod5Mask
};

static XModifierKeymap *modmap=NULL;

#define KNOWN_MODIFIERS_MASK (ShiftMask|LockMask|ControlMask|Mod1Mask|\
                              Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask)

#ifdef CF_HACK_IGNORE_EVIL_LOCKS

#define N_EVILLOCKS 3
#define N_LOOKUPEVIL 2

static uint evillockmasks[N_EVILLOCKS]={
     0, 0, LockMask
};

static const KeySym evillocks[N_LOOKUPEVIL]={
    XK_Num_Lock, XK_Scroll_Lock
};

static uint evilignoremask=LockMask;

static void lookup_evil_locks();

static void evil_grab_key(Display *display, uint keycode, uint modifiers,
                          Window grab_window, bool owner_events,
                          int pointer_mode, int keyboard_mode);

static void evil_grab_button(Display *display, uint button, uint modifiers,
                             Window grab_window, bool owner_events,
                             uint event_mask, int pointer_mode,
                             int keyboard_mode, Window confine_to,
                             Cursor cursor);

static void evil_ungrab_key(Display *display, uint keycode, uint modifiers,
                            Window grab_window);

static void evil_ungrab_button(Display *display, uint button, uint modifiers,
                               Window grab_window);

#endif


#define CVAL(A, B, V) ( A->V < B->V ? -1 : (A->V > B->V ? 1 : 0))

static int compare_bindings(const WBinding *a, const WBinding *b)
{
    int r=CVAL(a, b, act);
    if(r==0){
        r=CVAL(a, b, kcb);
        if(r==0){
            r=CVAL(a, b, state);
            if(r==0){
                r=CVAL(a, b, area);
            }
        }
    }
    return r;
}
                    
#undef CVAL


bool init_bindmap(WBindmap *bindmap)
{
    bindmap->rbind_list=NULL;
    bindmap->areamap=NULL;
    bindmap->nbindings=0;
    bindmap->bindings=make_rb();
    if(bindmap->bindings==NULL){
        warn_err();
        return FALSE;
    }
    return TRUE;
}


WBindmap *create_bindmap()
{
    WBindmap *bindmap=ALLOC(WBindmap);
    
    if(bindmap==NULL){
        warn_err();
        return NULL;
    }
    
    if(!init_bindmap(bindmap)){
        free(bindmap);
        return NULL;
    }
    
    return bindmap;
}


void binding_deinit(WBinding *binding)
{
    if(binding->submap!=NULL){
        bindmap_destroy(binding->submap);
        binding->submap=NULL;
    }

    binding->func=extl_unref_fn(binding->func);
}


static void do_destroy_binding(WBinding *binding)
{
    assert(binding!=NULL);
    binding_deinit(binding);
    free(binding);
}


static void bindmap_deinit(WBindmap *bindmap)
{
    WBinding *b=NULL;
    Rb_node node=NULL;

    while(bindmap->rbind_list!=NULL){
        region_remove_bindmap(bindmap->rbind_list->reg,
                              bindmap);
    }
        
    if(bindmap->bindings==NULL)
        return;
    
    FOR_ALL_BINDINGS(b, node, bindmap->bindings){
        do_destroy_binding((WBinding*)rb_val(node));
        bindmap->nbindings--;
    }

    assert(bindmap->nbindings==0);
    
    rb_free_tree(bindmap->bindings);
    bindmap->bindings=NULL;
}


void bindmap_destroy(WBindmap *bindmap)
{
    bindmap_deinit(bindmap);
    free(bindmap);
}


static void free_map(Rb_node map)
{
    Rb_node node;
    WBinding *b;
    
    FOR_ALL_BINDINGS(b, node, map)
        free(b);
    
    rb_free_tree(map);
}


void bindmap_refresh(WBindmap *bindmap)
{
    WRegBindingInfo *rbind;
    Rb_node newtree, node;
    WBinding *b, *b2;
    
    if(bindmap->bindings==NULL)
        return;
    
    newtree=make_rb();
    
    if(newtree==NULL){
        warn_err();
        return;
    }
    
    FOR_ALL_BINDINGS(b, node, bindmap->bindings){
        b2=ALLOC(WBinding);
        if(b2==NULL){
            warn_err();
            free_map(newtree);
            return;
        }
        
        *b2=*b;

        if(b->act==BINDING_KEYPRESS){
            for(rbind=bindmap->rbind_list; rbind!=NULL; rbind=rbind->bm_next)
                rbind_binding_removed(rbind, b, bindmap);
            b2->kcb=XKeysymToKeycode(ioncore_g.dpy, b->ksb);
        }
        
        if(!rb_insertg(newtree, b2, b2, (Rb_compfn*)compare_bindings)){
            warn_err();
            free(b2);
            free_map(newtree);
            return;
        }
    }

    free_map(bindmap->bindings);
    bindmap->bindings=newtree;

    FOR_ALL_BINDINGS(b, node, bindmap->bindings){
        if(b->act!=BINDING_KEYPRESS)
            continue;
        for(rbind=bindmap->rbind_list; rbind!=NULL; rbind=rbind->bm_next)
            rbind_binding_added(rbind, b, bindmap);
        if(b->submap!=NULL)
            bindmap_refresh(b->submap);
    }
}


bool bindmap_add_binding(WBindmap *bindmap, const WBinding *b)
{
    WRegBindingInfo *rbind=NULL;
    WBinding *binding=NULL;
    Rb_node node=NULL;
    int found=0;
    
    /* Handle adding the binding */
    binding=ALLOC(WBinding);
    
    if(binding==NULL){
        warn_err();
        return FALSE;
    }
    
    memcpy(binding, b, sizeof(*b));
    
    node=rb_find_gkey_n(bindmap->bindings, binding,
                        (Rb_compfn*)compare_bindings, &found);
    
    if(found){
        if(!rb_insert_a(node, binding, binding)){
            free(binding);
            return FALSE;
        }
        do_destroy_binding((WBinding*)rb_val(node));
        rb_delete_node(node);
        bindmap->nbindings--;
    }else{
        if(!rb_insertg(bindmap->bindings, binding, binding, 
                       (Rb_compfn*)compare_bindings)){
            free(binding);
            return FALSE;
        }
    }

    bindmap->nbindings++;
    
    for(rbind=bindmap->rbind_list; rbind!=NULL; rbind=rbind->bm_next)
        rbind_binding_added(rbind, binding, bindmap);

    return TRUE;
}


bool bindmap_remove_binding(WBindmap *bindmap, const WBinding *b)
{
    WRegBindingInfo *rbind=NULL;
    WBinding *binding=NULL;
    Rb_node node=NULL;
    int found=0;
    
    if(bindmap->bindings==NULL)
        return FALSE;
    
    node=rb_find_gkey_n(bindmap->bindings, b, (Rb_compfn*)compare_bindings,
                        &found);
    
    if(!found)
        return FALSE;
    
    binding=(WBinding*)rb_val(node);
    
    for(rbind=bindmap->rbind_list; rbind!=NULL; rbind=rbind->bm_next)
        rbind_binding_removed(rbind, binding, bindmap);

    do_destroy_binding(binding);
    rb_delete_node(node);
    
    bindmap->nbindings--;

    return TRUE;
}


void ioncore_init_bindings()
{
    modmap=XGetModifierMapping(ioncore_g.dpy);
    
    assert(modmap!=NULL);

#ifdef CF_HACK_IGNORE_EVIL_LOCKS
    lookup_evil_locks();
#endif
}


void ioncore_update_modmap()
{
    XModifierKeymap *nm=XGetModifierMapping(ioncore_g.dpy);
    
    if(nm!=NULL){
        XFreeModifiermap(modmap);
        modmap=nm;
    }
}


/* */


void binding_grab_on(const WBinding *binding, Window win)
{
    if(binding->act==BINDING_KEYPRESS){
#ifndef CF_HACK_IGNORE_EVIL_LOCKS            
        XGrabKey(ioncore_g.dpy, binding->kcb, binding->state, win,
                 True, GrabModeAsync, GrabModeAsync);
#else        
        evil_grab_key(ioncore_g.dpy, binding->kcb, binding->state, win,
                      True, GrabModeAsync, GrabModeAsync);
#endif            
    }
    
    if(binding->act!=BINDING_BUTTONPRESS &&
       binding->act!=BINDING_BUTTONCLICK &&
       binding->act!=BINDING_BUTTONDBLCLICK &&
       binding->act!=BINDING_BUTTONMOTION)
        return;
    
    if(binding->state==0)
        return;
    
#ifndef CF_HACK_IGNORE_EVIL_LOCKS            
    XGrabButton(ioncore_g.dpy, binding->kcb, binding->state, win,
                True, IONCORE_EVENTMASK_PTRGRAB, GrabModeAsync, GrabModeAsync,
                None, None);
#else            
    evil_grab_button(ioncore_g.dpy, binding->kcb, binding->state, win,
                     True, IONCORE_EVENTMASK_PTRGRAB, GrabModeAsync, GrabModeAsync,
                     None, None);
#endif
}


void binding_ungrab_on(const WBinding *binding, Window win)
{
    if(binding->act==BINDING_KEYPRESS){
#ifndef CF_HACK_IGNORE_EVIL_LOCKS
        XUngrabKey(ioncore_g.dpy, binding->kcb, binding->state, win);
#else
        evil_ungrab_key(ioncore_g.dpy, binding->kcb, binding->state, win);
#endif
    }
    
    if(binding->act!=BINDING_BUTTONPRESS &&
       binding->act!=BINDING_BUTTONCLICK &&
       binding->act!=BINDING_BUTTONDBLCLICK &&
       binding->act!=BINDING_BUTTONMOTION)
        return;
    
    if(binding->state==0)
        return;

#ifndef CF_HACK_IGNORE_EVIL_LOCKS
    XUngrabButton(ioncore_g.dpy, binding->kcb, binding->state, win);
#else
    evil_ungrab_button(ioncore_g.dpy, binding->kcb, binding->state, win);
#endif
}


/* */


static WBinding *search_binding(WBindmap *bindmap, WBinding *binding)
{
    Rb_node node;
    int found=0;

    if(bindmap->bindings==NULL)
        return NULL;
    
    node=rb_find_gkey_n(bindmap->bindings, binding,
                        (Rb_compfn*)compare_bindings, &found);
    
    if(found==0)
        return NULL;
    
    return (WBinding*)rb_val(node);
}


static WBinding *do_bindmap_lookup_binding(WBindmap *bindmap,
                                   int act, uint state, uint kcb, int area)
{
    WBinding *binding, tmp;

    if(bindmap->nbindings==0)
        return NULL;

#ifdef CF_HACK_IGNORE_EVIL_LOCKS
    state&=~evilignoremask;
#endif
    state&=KNOWN_MODIFIERS_MASK;
    
    tmp.act=act;
    tmp.kcb=kcb;
    tmp.state=state;
    tmp.area=area;
    
    binding=search_binding(bindmap, &tmp);

    if(binding==NULL){
        tmp.state=AnyModifier;
        binding=search_binding(bindmap, &tmp);

        if(binding==NULL){
            tmp.state=state;
            tmp.kcb=(act==BINDING_KEYPRESS ? AnyKey : AnyButton);
            binding=search_binding(bindmap, &tmp);

            if(binding==NULL){
                tmp.state=AnyModifier;
                binding=search_binding(bindmap, &tmp);
            }
        }
    }
                
    return binding;
}


WBinding *bindmap_lookup_binding(WBindmap *bindmap, int act, uint state, uint kcb)
{
    return do_bindmap_lookup_binding(bindmap, act, state, kcb, 0);
}


WBinding *bindmap_lookup_binding_area(WBindmap *bindmap,
                              int act, uint state, uint kcb, int area)
{
    WBinding *binding;
    
    binding=do_bindmap_lookup_binding(bindmap, act, state, kcb, area);
    
    if(binding==NULL)
        binding=do_bindmap_lookup_binding(bindmap, act, state, kcb, 0);
    
    return binding;
}

    
/*
 * A dirty hack to deal with (==ignore) evil locking modifier keys.
 */


int ioncore_unmod(int state, int keycode)
{
    int j;
    
#ifdef CF_HACK_IGNORE_EVIL_LOCKS
    state&=~evilignoremask;
#endif

    for(j=0; j<N_MODS*modmap->max_keypermod; j++){
        if(modmap->modifiermap[j]==keycode)
            return state&~modmasks[j/modmap->max_keypermod];
    }
    
    return state;
}


int ioncore_modstate()
{
	char keys[32];
	int state=0;
	int j;
	
	XQueryKeymap(ioncore_g.dpy, keys);
    
	for(j=0; j<N_MODS*modmap->max_keypermod; j++){
        int a=(modmap->modifiermap[j]&7);
		int b=(modmap->modifiermap[j]>>3);
		if(b<32){
			if(keys[b]&(1<<a))
				state|=modmasks[j/modmap->max_keypermod];
		}
    }

#ifdef CF_HACK_IGNORE_EVIL_LOCKS
    state&=~evilignoremask;
#endif
    return state;
}


bool ioncore_ismod(int keycode)
{
    int j;
    
    for(j=0; j<N_MODS*modmap->max_keypermod; j++){
        if(modmap->modifiermap[j]==keycode)
            return TRUE;
    }
    
    return FALSE;
}
    

#ifdef CF_HACK_IGNORE_EVIL_LOCKS

static void lookup_evil_locks()
{
    uint keycodes[N_LOOKUPEVIL];
    int i, j;
    
    for(i=0; i<N_LOOKUPEVIL; i++)
        keycodes[i]=XKeysymToKeycode(ioncore_g.dpy, evillocks[i]);
    
    for(j=0; j<N_MODS*modmap->max_keypermod; j++){
        for(i=0; i<N_LOOKUPEVIL; i++){
            if(keycodes[i]==None)
                continue;
            if(modmap->modifiermap[j]==keycodes[i]){
                evillockmasks[i]=modmasks[j/modmap->max_keypermod];
                evilignoremask|=evillockmasks[i];
            }
        }
    }
}


static void evil_grab_key(Display *display, uint keycode, uint modifiers,
                          Window grab_window, bool owner_events,
                          int pointer_mode, int keyboard_mode)
{
    uint mods;
    int i, j;
    
    XGrabKey(display, keycode, modifiers, grab_window, owner_events,
             pointer_mode, keyboard_mode);

    if(modifiers==AnyModifier)
        return;
    
    for(i=0; i<N_EVILLOCKS; i++){
        if(evillockmasks[i]==0)
            continue;
        mods=modifiers;
        for(j=i; j<N_EVILLOCKS; j++){
            if(evillockmasks[j]==0)
                continue;            
            mods|=evillockmasks[j];            
            XGrabKey(display, keycode, mods,
                     grab_window, owner_events, pointer_mode, keyboard_mode);
            if(i==j)
                continue;
            XGrabKey(display, keycode,
                     modifiers|evillockmasks[i]|evillockmasks[j],
                     grab_window, owner_events, pointer_mode, keyboard_mode);
        }
    }    
}


static void evil_grab_button(Display *display, uint button, uint modifiers,
                             Window grab_window, bool owner_events,
                             uint event_mask, int pointer_mode,
                             int keyboard_mode, Window confine_to,
                             Cursor cursor)
{
    uint mods;
    int i, j;

    XGrabButton(display, button, modifiers,
                grab_window, owner_events, event_mask, pointer_mode,
                keyboard_mode, confine_to, cursor);

    if(modifiers==AnyModifier)
        return;
    
    for(i=0; i<N_EVILLOCKS; i++){
        if(evillockmasks[i]==0)
            continue;
        mods=modifiers;
        for(j=i; j<N_EVILLOCKS; j++){            
            if(evillockmasks[j]==0)
                continue;            
            mods|=evillockmasks[j];            
            XGrabButton(display, button, mods,
                        grab_window, owner_events, event_mask, pointer_mode,
                        keyboard_mode, confine_to, cursor);
            if(i==j)
                continue;
            XGrabButton(display, button,
                        modifiers|evillockmasks[i]|evillockmasks[j],
                        grab_window, owner_events, event_mask, pointer_mode,
                        keyboard_mode, confine_to, cursor);
        }            
    }
}


static void evil_ungrab_key(Display *display, uint keycode, uint modifiers,
                            Window grab_window)
{
    uint mods;
    int i, j;
    
    XUngrabKey(display, keycode, modifiers, grab_window);

    if(modifiers==AnyModifier)
        return;
    
    for(i=0; i<N_EVILLOCKS; i++){
        if(evillockmasks[i]==0)
            continue;
        mods=modifiers;
        for(j=i; j<N_EVILLOCKS; j++){
            if(evillockmasks[j]==0)
                continue;            
            mods|=evillockmasks[j];            
            XUngrabKey(display, keycode, mods, grab_window);
            if(i==j)
                continue;
            XUngrabKey(display, keycode,
                       modifiers|evillockmasks[i]|evillockmasks[j],
                       grab_window);
        }
    }    
}


static void evil_ungrab_button(Display *display, uint button, uint modifiers,
                               Window grab_window)
{
    uint mods;
    int i, j;
    
    XUngrabButton(display, button, modifiers, grab_window);

    if(modifiers==AnyModifier)
        return;
    
    for(i=0; i<N_EVILLOCKS; i++){
        if(evillockmasks[i]==0)
            continue;
        mods=modifiers;
        for(j=i; j<N_EVILLOCKS; j++){            
            if(evillockmasks[j]==0)
                continue;            
            mods|=evillockmasks[j];            
            XUngrabButton(display, button, mods, grab_window);
            if(i==j)
                continue;
            XUngrabButton(display, button,
                          modifiers|evillockmasks[i]|evillockmasks[j], 
                          grab_window);
        }            
    }
    
}

#endif /* CF_HACK_IGNORE_EVIL_LOCKS */

