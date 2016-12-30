/*
 * ion/ioncore/key.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#include <ctype.h>

#include <libtu/objp.h>
#include <libextl/extl.h>
#include <libmainloop/defer.h>

#include "common.h"
#include "key.h"
#include "binding.h"
#include "global.h"
#include "event.h"
#include "cursor.h"
#include "grab.h"
#include "regbind.h"
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
    ioncore_grab_establish((WRegion*)cwin, quote_next_handler, NULL, 0);
    ioncore_change_grab_cursor(IONCORE_CURSOR_WAITKEY);
}


static bool waitrelease_handler(WRegion *UNUSED(reg), XEvent *ev)
{
    return (ioncore_unmod(ev->xkey.state, ev->xkey.keycode)==0);
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


static void free_sub(WSubmapState *p)
{
    /*extl_unref_fn(p->leave);
    watch_reset(&p->leave_reg);
    */

    free(p);
}


void region_free_submapstat(WRegion *reg)
{
    while(reg->submapstat!=NULL){
        WSubmapState *p=reg->submapstat;
        reg->submapstat=p->next;
        free_sub(p);
    }
}


WHook *ioncore_submap_ungrab_hook=NULL;


static void call_submap_ungrab_hook()
{
    hook_call_v(ioncore_submap_ungrab_hook);
}


static void clear_subs(WRegion *reg)
{
    region_free_submapstat(reg);
    mainloop_defer_action(NULL, (WDeferredAction*)call_submap_ungrab_hook);
/*
    while(reg!=NULL && reg->submapstat!=NULL){
        WSubmapState *p=reg->submapstat;
        reg->submapstat=p->next;

        if(p->leave!=extl_fn_none() && p->leave_reg.obj!=NULL){
            Watch regw=WATCH_INIT;

            watch_setup(&regw, (Obj*)reg, NULL);

            extl_call(p->leave, "o", NULL, p->leave_reg.obj);

            reg=(WRegion*)regw.obj;

            watch_reset(&regw);
        }

        free_sub(p);
    }
*/
}


static WSubmapState *add_sub(WRegion *reg, uint key, uint state)
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
        return NULL;

    s->key=key;
    s->state=state;
    /*s->leave=extl_fn_none();
    watch_init(&s->leave_reg);*/

    *p=s;

    return s;

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


enum{GRAB_NONE, GRAB_NONE_SUBMAP, GRAB_SUBMAP, GRAB_WAITRELEASE};


static WBinding *lookup_binding_(WRegion *reg,
                                 int act, uint state, uint kcb,
                                 WSubmapState *st,
                                 WRegion **binding_owner, WRegion **subreg)
{
    WBinding *binding;

    *subreg=NULL;

    do{
        binding=region_lookup_keybinding(reg, act, state, kcb, st,
                                         binding_owner);

        if(binding!=NULL)
            break;

        if(OBJ_IS(reg, WRootWin))
            break;

        *subreg=reg;
        reg=REGION_PARENT_REG(reg);
    }while(reg!=NULL);

    return binding;
}

static WBinding *lookup_binding(WRegion *oreg,
                                int act, uint state, uint kcb,
                                WRegion **binding_owner, WRegion **subreg)
{
    WRegion *reg=oreg;

    /* Find the deepest nested active window grabbing this key. */
    while(reg->active_sub!=NULL)
        reg=reg->active_sub;

    return lookup_binding_(reg, act, state, kcb, oreg->submapstat,
                           binding_owner, subreg);
}


static void do_call_binding(WBinding *binding, WRegion *reg, WRegion *subreg)
{
    WRegion *mgd=region_managed_within(reg, subreg);

    /* TODO: having to pass both mgd and subreg for some handlers
     * to work is ugly and complex.
     */
    extl_call(binding->func, "ooo", NULL, reg, mgd, subreg);
}


static int do_key(WRegion *oreg, XKeyEvent *ev)
{
    WBinding *binding=NULL;
    WRegion *binding_owner=NULL, *subreg=NULL;
    bool grabbed=(oreg->flags&REGION_BINDINGS_ARE_GRABBED);
    int ret=GRAB_NONE;

    if(grabbed){
        binding=lookup_binding(oreg, BINDING_KEYPRESS, ev->state, ev->keycode,
                               &binding_owner, &subreg);
    }else{
        binding=region_lookup_keybinding(oreg, BINDING_KEYPRESS,
                                         ev->state, ev->keycode,
                                         oreg->submapstat,
                                         &binding_owner);
    }

    if(binding!=NULL){
        bool subs=(oreg->submapstat!=NULL);
        WBinding *call=NULL;

        if(binding->submap!=NULL){
            WSubmapState *s=add_sub(oreg, ev->keycode, ev->state);
            if(s!=NULL){
                /*WRegion *own2, *subreg2;

                call=lookup_binding(binding_owner, BINDING_SUBMAP_LEAVE, 0, 0,
                                    oreg->submapstat, &own2, &subreg2);

                if(call!=NULL){
                    s->leave=extl_ref_fn(call->func);
                    watch_setup(&s->leave_reg, (Obj*)own2, NULL);
                }*/

                call=lookup_binding_(binding_owner, BINDING_SUBMAP_ENTER, 0, 0,
                                     oreg->submapstat,
                                     &binding_owner, &subreg);

                ret=(grabbed ? GRAB_SUBMAP : GRAB_NONE_SUBMAP);
            }
        }else{
            call=binding;

            if(grabbed)
                XUngrabKeyboard(ioncore_g.dpy, CurrentTime);

            if(ev->state!=0 && !subs && binding->wait)
                ret=GRAB_WAITRELEASE;
        }

        if(call!=NULL){
            current_kcb=ev->keycode;
            current_state=ev->state;
            current_submap=subs;

            do_call_binding(call, binding_owner, subreg);

            current_kcb=0;
        }
    }else if(oreg->submapstat==NULL && OBJ_IS(oreg, WWindow)){
        insstr((WWindow*)oreg, ev);
    }

    return ret;
}


static bool submapgrab_handler(WRegion* reg, XEvent *xev)
{
    XKeyEvent *ev=&xev->xkey;
    if(ev->type!=KeyPress){
        if(ioncore_unmod(ev->state, ev->keycode)==0){
            WBinding *binding;
            WRegion *binding_owner, *subreg;

            binding=lookup_binding(reg,
                                   BINDING_SUBMAP_RELEASEMOD, 0, 0,
                                   &binding_owner, &subreg);

            if(binding!=NULL)
                do_call_binding(binding, binding_owner, subreg);
        }
        return FALSE;
    }

    if(ioncore_ismod(ev->keycode))
        return FALSE;
    if(do_key(reg, ev)!=GRAB_SUBMAP){
        clear_subs(reg);
        return TRUE;
    }else{
        return FALSE;
    }
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
        Watch w=WATCH_INIT;
        int grab;

        /* reg might be destroyed by binding handlers */
        watch_setup(&w, (Obj*)reg, NULL);

        grab=do_key(reg, ev);

        reg=(WRegion*)w.obj;

        if(reg!=NULL){
            if(grab==GRAB_SUBMAP)
                submapgrab(reg);
            else if(grab==GRAB_WAITRELEASE)
                waitrelease(reg);
            else if(grab==GRAB_NONE_SUBMAP)
                /* nothing */;
            else if(grab==GRAB_NONE && reg->submapstat!=NULL)
                clear_subs(reg);
        }

        watch_reset(&w);
    }
}
