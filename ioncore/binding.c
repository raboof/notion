/*
 * wmcore/binding.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#ifndef CF_NO_LOCK_HACK
#define CF_HACK_IGNORE_EVIL_LOCKS
#endif

#ifdef CF_HACK_IGNORE_EVIL_LOCKS
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#endif

#include "common.h"
#include "event.h"
#include "binding.h"
#include "global.h"
#include "objp.h"


/* */

#define N_MODS 8

static const uint modmasks[N_MODS]={
	ShiftMask, LockMask, ControlMask, Mod1Mask, Mod2Mask, Mod3Mask,
	Mod4Mask, Mod5Mask
};

static XModifierKeymap *modmap=NULL;

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

#endif


#define CVAL(A, B, V) ( A->V < B->V ? -1 : (A->V > B->V ? 1 : 0))

static int compare_bindings(const void *a_, const void *b_)
{
	const WBinding *a=(WBinding*)a_, *b=(WBinding*)b_;
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
	bindmap->nbindings=0;
	bindmap->bindings=NULL;
	bindmap->parent=NULL;
	bindmap->extends=NULL;
	bindmap->confdefmod=0;
	return TRUE;
}


WBindmap *create_bindmap()
{
	SIMPLECREATESTRUCT_IMPL(WBindmap, bindmap, (p));
}


void destroy_binding(WBinding *binding)
{
	int i;
	
	if(binding->submap!=NULL){
		destroy_bindmap(binding->submap);
		free(binding->submap);
	}
	
	for(i=1; i<binding->nargs; i++)
		tok_free(binding->args+i);
}


void destroy_bindmap(WBindmap *bindmap)
{
	int i;
	WBinding *binding=bindmap->bindings;
	
	for(i=0; i<bindmap->nbindings; i++, binding++)
		destroy_binding(binding);
	
	free(bindmap->bindings);
}


	
bool add_binding(WBindmap *bindmap, const WBinding *b)
{
	WBinding *binding;
	int i, j;
	
	if(bindmap==NULL || (b->func==NULL && b->submap==NULL))
		return FALSE;

	binding=bindmap->bindings;
	
	for(i=0; i<bindmap->nbindings; i++){
		switch(compare_bindings((void*)b, (void*)(binding+i))){
		case 1:
			continue;
		case 0:
			destroy_binding(binding+i);
			goto subst;
		}
		break;
	}

	binding=REALLOC_N(binding, WBinding, bindmap->nbindings,
					  bindmap->nbindings+1);
	
	if(binding==NULL){
		warn_err();
		return FALSE;
	}
	
	memmove(&(binding[i+1]), &(binding[i]),
			sizeof(WBinding)*(bindmap->nbindings-i));
	
	bindmap->bindings=binding;
	bindmap->nbindings++;
	
subst:
	memcpy(&(binding[i]), b, sizeof(WBinding));
	
	return TRUE;
}


#if 0
static void bind_simple(WBindmap *bindmap, int ftab,
						int state, int key, char *fn)
{
	WBinding binding;
	
	binding.func=lookup_func(fn, ftab);
	binding.state=state;
	binding.kcb=XKeysymToKeycode(wglobal.dpy, key);
	binding.act=ACT_KEYPRESS;
	binding.nargs=0;
	binding.submap=NULL;
	
	add_binding(bindmap, &binding);
}
#endif


void init_bindings()
{
	modmap=XGetModifierMapping(wglobal.dpy);
	
	assert(modmap!=NULL);
#if 0
	init_bindmap(&(wglobal.main_bindmap));
	init_bindmap(&(wglobal.tab_bindmap));
	init_bindmap(&(wglobal.input_bindmap));
	init_bindmap(&(wglobal.moveres_bindmap));
	
	/* There must be a way to get out of resize mode */
	bind_simple(&(wglobal.moveres_bindmap), FUNTAB_MOVERES,
				0, XK_Escape, "cancel_resize");
#endif

#ifdef CF_HACK_IGNORE_EVIL_LOCKS
	lookup_evil_locks();
#endif
}


void update_modmap()
{
	XModifierKeymap *nm=XGetModifierMapping(wglobal.dpy);
	
	if(nm!=NULL){
		XFreeModifiermap(modmap);
		modmap=nm;
	}
}


/* */


void grab_bindings(WBindmap *bindmap, Window win)
{
	WBinding *binding;
	int i;
	
	for(; bindmap!=NULL; bindmap=bindmap->extends){
		binding=bindmap->bindings;
		
		for(i=0; i<bindmap->nbindings; i++, binding++){
			if(binding->act==ACT_KEYPRESS){
#ifndef CF_HACK_IGNORE_EVIL_LOCKS			
				XGrabKey(wglobal.dpy, binding->kcb, binding->state, win,
						 True, GrabModeAsync, GrabModeAsync);
#else		
				evil_grab_key(wglobal.dpy, binding->kcb, binding->state, win,
							  True, GrabModeAsync, GrabModeAsync);
#endif			
			}
			
			if(binding->act!=ACT_BUTTONPRESS &&
			   binding->act!=ACT_BUTTONCLICK &&
			   binding->act!=ACT_BUTTONDBLCLICK &&
			   binding->act!=ACT_BUTTONMOTION)
				continue;
			
			if(binding->state==0)
				continue;
			
#ifndef CF_HACK_IGNORE_EVIL_LOCKS			
			XGrabButton(wglobal.dpy, binding->kcb, binding->state, win,
						True, GRAB_POINTER_MASK, GrabModeAsync, GrabModeAsync,
						None, None);
#else			
			evil_grab_button(wglobal.dpy, binding->kcb, binding->state, win,
							 True, GRAB_POINTER_MASK, GrabModeAsync, GrabModeAsync,
							 None, None);
#endif
			
		}
	}
}


/* */


static WBinding *search_binding(WBindmap *bindmap, WBinding *binding)
{
	WBinding *bindingr=NULL;
	
	for(; bindmap!=NULL; bindmap=bindmap->extends){
		bindingr=(WBinding*)bsearch((void*)binding, (void*)(bindmap->bindings),
									bindmap->nbindings, sizeof(WBinding),
									compare_bindings);
		if(bindingr!=NULL)
			break;
	}
	
	return bindingr;
}


static WBinding *do_lookup_binding(WBindmap *bindmap,
								   int act, uint state, uint kcb, int area)
{
	WBinding *binding, tmp;

#ifdef CF_HACK_IGNORE_EVIL_LOCKS
	state&=~evilignoremask;
#endif
	
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
			tmp.kcb=(act==ACT_KEYPRESS ? AnyKey : AnyButton);
			binding=search_binding(bindmap, &tmp);

			if(binding==NULL){
				tmp.state=AnyModifier;
				binding=search_binding(bindmap, &tmp);
			}
		}
	}
				
	return binding;
}


WBinding *lookup_binding(WBindmap *bindmap, int act, uint state, uint kcb)
{
	return do_lookup_binding(bindmap, act, state, kcb, 0);
}


WBinding *lookup_binding_area(WBindmap *bindmap,
							  int act, uint state, uint kcb, int area)
{
	WBinding *binding;
	
	binding=do_lookup_binding(bindmap, act, state, kcb, area);
	
	if(binding==NULL)
		binding=do_lookup_binding(bindmap, act, state, kcb, 0);
	
	return binding;
}

	
void call_binding(WBinding *binding, WThing *thing)
{
	if(binding->func==NULL)
		return;
	
	if(binding->func->callhnd==NULL)
		return;
	
	binding->func->callhnd(thing, binding->func,
						   binding->nargs, binding->args);
}


/*
 * A dirty hack to deal with (==ignore) evil locking modifier keys.
 */


int unmod(int state, int keycode)
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


bool ismod(int keycode)
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
		keycodes[i]=XKeysymToKeycode(wglobal.dpy, evillocks[i]);
	
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

#endif /* CF_HACK_IGNORE_EVIL_LOCKS */

