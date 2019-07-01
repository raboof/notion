/*
 * ion/ioncore/key.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
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
    int n;
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
    ioncore_grab_establish((WRegion*)cwin,
                           quote_next_handler, NULL,
                           0, GRAB_DEFAULT_FLAGS);
    ioncore_change_grab_cursor(IONCORE_CURSOR_WAITKEY);
}


static bool waitrelease_handler(WRegion *UNUSED(reg), XEvent *ev)
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
                           waitrelease_handler, NULL,
                           0, GRAB_DEFAULT_FLAGS);
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


static uint current_kcb, current_state;
static bool current_submap;

/* Note: state set to AnyModifier for submaps */
bool ioncore_current_key(uint *kcb, uint *state, bool *sub)
{
    if(current_kcb==0)
        return FALSE;

    *kcb=current_kcb;
    *state=current_state;
    *sub=current_submap;

    return TRUE;
}

static bool submap_defined(WRegion *reg, XKeyEvent *ev)
{
    WRegion *oreg=reg;

    /* Find the deepest nested active window grabbing this key. */
    while(reg->active_sub!=NULL)
        reg=reg->active_sub;

    do{
        WRegion *binding_owner=NULL;
        WBinding *binding=region_lookup_submap(reg, ev, oreg->submapstat,
                                               &binding_owner);
        
        if(binding!=NULL && binding->submap!=NULL)
            return TRUE;

        reg=REGION_PARENT_REG(reg);
    }while(reg!=NULL);

    return FALSE;
}

/* Return value TRUE = grab needed */
static bool do_key(WRegion *reg, XKeyEvent *ev)
{
    WBinding *binding=NULL;

    WRegion *oreg=NULL, *binding_owner=NULL, *subreg=NULL;
    bool grabbed;
    bool grab_needed = FALSE;
    bool has_submap = FALSE;

    oreg=reg;
    grabbed=(oreg->flags&REGION_BINDINGS_ARE_GRABBED);

    if(grabbed){
        /* Find the deepest nested active window grabbing this key. */
        while(reg->active_sub!=NULL)
            reg=reg->active_sub;

        do{
            binding=region_lookup_keybinding(reg, ev, oreg->submapstat,
                                             &binding_owner);

            if(binding!=NULL && binding->func!=extl_fn_none())
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

    has_submap = submap_defined(oreg, ev);
    if(has_submap && add_sub(oreg, ev->keycode, ev->state))
        grab_needed = TRUE;
    else
        clear_subs(oreg);

    if(binding!=NULL){
        if(binding_owner!=NULL && binding->func!=extl_fn_none()){
            WRegion *mgd=region_managed_within(binding_owner, subreg);
            bool subs=(oreg->submapstat!=NULL);

            if(grabbed)
                XUngrabKeyboard(ioncore_g.dpy, CurrentTime);

            current_kcb=ev->keycode;
            current_state=ev->state;
            current_submap=oreg->submapstat!=NULL;

            /* TODO: having to pass both mgd and subreg for some handlers
             * to work is ugly and complex.
             */
            extl_call(binding->func, "ooo", NULL, binding_owner, mgd, subreg);

            current_kcb=0;

            if(ev->state!=0 && !subs && binding->wait)
                waitrelease(oreg);
        }
    }else if(OBJ_IS(oreg, WWindow)){
        insstr((WWindow*)oreg, ev);
    }

    return grab_needed;
}


static bool submapgrab_handler(WRegion* reg, XEvent *xev)
{
    XKeyEvent *ev=&xev->xkey;
    if(ev->type!=KeyPress)
        return FALSE;
    if(ioncore_ismod(ev->keycode))
        return FALSE;
    return !do_key(reg, ev);
}


static void submapgrab(WRegion *reg)
{
    ioncore_grab_establish(reg, submapgrab_handler, clear_subs,
                           0, GRAB_DEFAULT_FLAGS);
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
