/*
 * ion/ioncore/key.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <ctype.h>
#include <libtu/objp.h>
#include "common.h"
#include "key.h"
#include "binding.h"
#include "global.h"
#include "event.h"
#include "cursor.h"
#include "grab.h"
#include "regbind.h"
#include <libextl/extl.h>
#include "strings.h"
#include "xwindow.h"


static void waitrelease(WRegion *reg);
static void submapgrab(WRegion *reg);


static void insstr(WWindow *wwin, XKeyEvent *ev)
{
    static XComposeStatus cs={NULL, 0};
    char buf[32]={0,};
    Status stat;
    int n, i;
    KeySym ksym;
    
    if(wwin->xic!=NULL){
        if(XFilterEvent((XEvent*)ev, ev->window))
           return;
        n=XmbLookupString(wwin->xic, ev, buf, 16, &ksym, &stat);
        if(stat!=XLookupChars && stat!=XLookupBoth)
            return;
    }else{
        n=XLookupString(ev, buf, 32, &ksym, &cs);
    }
    
    if(n<=0)
        return;
    
    /* Won't catch bad strings, but should filter out most crap. */
    if(ioncore_g.use_mb){
        if(!iswprint(str_wchar_at(buf, 32)))
            return;
    }else{
        if(iscntrl(*buf))
            return;
    }
    
    window_insstr(wwin, buf, n);
}


static void send_key(XEvent *ev, WClientWin *cwin)
{
    Window win=cwin->win;
    ev->xkey.window=win;
    ev->xkey.subwindow=None;
    XSendEvent(ioncore_g.dpy, win, False, KeyPressMask, ev);
}


static bool quote_next_handler(WRegion *reg, XEvent *xev)
{
    XKeyEvent *ev=&xev->xkey;
    if(ev->type!=KeyPress)
        return FALSE;
    if(ioncore_ismod(ev->keycode))
        return FALSE;
    assert(OBJ_IS(reg, WClientWin));
    send_key(xev, (WClientWin*)reg);
    return TRUE; /* remove the grab */
}


/*EXTL_DOC
 * Send next key press directly to \var{cwin}.
 */
EXTL_EXPORT_MEMBER
void clientwin_quote_next(WClientWin *cwin)
{
    ioncore_grab_establish((WRegion*)cwin, quote_next_handler, NULL, 0);
    ioncore_change_grab_cursor(IONCORE_CURSOR_WAITKEY);
}


static bool waitrelease_handler(WRegion *reg, XEvent *ev)
{
    if(!ioncore_unmod(ev->xkey.state, ev->xkey.keycode))
        return TRUE;
    return FALSE;
}


static void waitrelease(WRegion *reg)
{
    if(ioncore_modstate()==0)
        return;
	
    /* We need to grab on the root window as <reg> might have been
     * ioncore_defer_destroy:ed by the binding handler (the most common case
     * for using this kpress_wait!). In such a case the grab may
     * be removed before the modifiers are released.
     */
    ioncore_grab_establish((WRegion*)region_rootwin_of(reg), 
                           waitrelease_handler, 
                           NULL, 0);
    ioncore_change_grab_cursor(IONCORE_CURSOR_WAITKEY);
}


static void free_subs(WSubmapState *p)
{
    WSubmapState *next;
    
    while(p!=NULL){
        next=p->next;
        free(p);
        p=next;
    }
}


static void clear_subs(WRegion *reg)
{
    while(reg->submapstat!=NULL){
        WSubmapState *tmp=reg->submapstat;
        reg->submapstat=tmp->next;
        free(tmp);
    }
}


static bool add_sub(WRegion *reg, uint key, uint state)
{
    WSubmapState **p;
    WSubmapState *s;
    
    if(reg->submapstat==NULL){
        p=&(reg->submapstat);
    }else{
        s=reg->submapstat;
        while(s->next!=NULL)
            s=s->next;
        p=&(s->next);
    }
    
    s=ALLOC(WSubmapState);
    
    if(s==NULL)
        return FALSE;
    
    s->key=key;
    s->state=state;
    
    *p=s;
    
    return TRUE;

}


static XKeyEvent *current_key_event=NULL;


XKeyEvent *ioncore_current_key_event()
{
    return current_key_event;
}


/* Return value TRUE = grab needed */
static bool do_key(WRegion *reg, XKeyEvent *ev)
{
    WBinding *binding=NULL;
    WRegion *oreg=NULL, *binding_owner=NULL, *subreg=NULL;
    bool grabbed;
    
    oreg=reg;
    grabbed=(oreg->flags&REGION_BINDINGS_ARE_GRABBED);
    
    if(grabbed){
        /* Find the deepest nested active window grabbing this key. */
        while(reg->active_sub!=NULL)
            reg=reg->active_sub;
        
        do{
            binding=region_lookup_keybinding(reg, ev, oreg->submapstat, 
                                             &binding_owner);
            
            if(binding!=NULL)
                break;
            if(OBJ_IS(reg, WRootWin))
                break;
            
            subreg=reg;
            reg=REGION_PARENT_REG(reg);
        }while(reg!=NULL);
    }else{
        binding=region_lookup_keybinding(oreg, ev, oreg->submapstat, 
                                         &binding_owner);
    }
    
    if(binding!=NULL){
        if(binding->submap!=NULL){
            if(add_sub(oreg, ev->keycode, ev->state))
                return grabbed;
            else
                clear_subs(oreg);
        }else if(binding_owner!=NULL){
            WRegion *mgd=region_managed_within(binding_owner, subreg);
            bool subs=(oreg->submapstat!=NULL);
            
            clear_subs(oreg);
            
            if(grabbed)
                XUngrabKeyboard(ioncore_g.dpy, CurrentTime);
            
            if(!subs)
                current_key_event=ev;
                
            /* TODO: having to pass both mgd and subreg for some handlers
             * to work is ugly and complex.
             */
            extl_call(binding->func, "ooo", NULL, binding_owner, mgd, subreg);
            
            current_key_event=NULL;
            
            if(ev->state!=0 && !subs && binding->wait)
                waitrelease(oreg);
        }
    }else if(oreg->submapstat!=NULL){
        clear_subs(oreg);
    }else if(OBJ_IS(oreg, WWindow)){
        insstr((WWindow*)oreg, ev);
    }
    
    return FALSE;
}


static bool submapgrab_handler(WRegion* reg, XEvent *ev)
{
    if(ev->type!=KeyPress)
        return FALSE;
    return !do_key(reg, &ev->xkey);
}


static void submapgrab(WRegion *reg)
{
    ioncore_grab_establish(reg, submapgrab_handler, clear_subs, 0);
    ioncore_change_grab_cursor(IONCORE_CURSOR_WAITKEY);
}


void ioncore_do_handle_keypress(XKeyEvent *ev)
{
    WRegion *reg=(WRegion*)XWINDOW_REGION_OF(ev->window);
    
    if(reg!=NULL){
        if(do_key(reg, ev))
            submapgrab(reg);
    }
}
