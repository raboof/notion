/*
 * ion/ioncore/eventh.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include <libtu/objp.h>
#include "common.h"
#include "global.h"
#include "rootwin.h"
#include "property.h"
#include "pointer.h"
#include "key.h"
#include "focus.h"
#include "selection.h"
#include "event.h"
#include "eventh.h"
#include "clientwin.h"
#include "colormap.h"
#include "grab.h"
#include "bindmaps.h"
#include "activity.h"
#include "netwm.h"
#include "xwindow.h"


/*{{{ ioncore_handle_event */


#define CASE_EVENT(EV) case EV:  /*\
    fprintf(stderr, "[%#lx] %s\n", ev->xany.window, #EV);*/


bool ioncore_handle_event(XEvent *ev)
{
    
    switch(ev->type){
    CASE_EVENT(MapRequest)
        ioncore_handle_map_request(&(ev->xmaprequest));
        break;
    CASE_EVENT(ConfigureRequest)
        ioncore_handle_configure_request(&(ev->xconfigurerequest));
        break;
    CASE_EVENT(UnmapNotify)
        ioncore_handle_unmap_notify(&(ev->xunmap));
        break;
    CASE_EVENT(DestroyNotify)
        ioncore_handle_destroy_notify(&(ev->xdestroywindow));
        break;
    CASE_EVENT(ClientMessage)
        ioncore_handle_client_message(&(ev->xclient));
        break;
    CASE_EVENT(PropertyNotify)
        ioncore_handle_property(&(ev->xproperty));
        break;
    CASE_EVENT(FocusIn)
        ioncore_handle_focus_in(&(ev->xfocus));
        break;
    CASE_EVENT(FocusOut)
        ioncore_handle_focus_out(&(ev->xfocus));
        break;
    CASE_EVENT(EnterNotify)
        ioncore_handle_enter_window(ev);
        break;
    CASE_EVENT(Expose)        
        ioncore_handle_expose(&(ev->xexpose));
        break;
    CASE_EVENT(KeyPress)
        ioncore_handle_keyboard(ev);
        break;
    CASE_EVENT(KeyRelease)
        ioncore_handle_keyboard(ev);
        break;
    CASE_EVENT(ButtonPress)
        ioncore_handle_buttonpress(ev);
        break;
    CASE_EVENT(ColormapNotify)
        ioncore_handle_colormap_notify(&(ev->xcolormap));
        break;
    CASE_EVENT(MappingNotify)
        ioncore_handle_mapping_notify(ev);
        break;
    CASE_EVENT(SelectionClear)
        ioncore_clear_selection();
        break;
    CASE_EVENT(SelectionNotify)
        ioncore_handle_selection(&(ev->xselection));
        break;
    CASE_EVENT(SelectionRequest)
        ioncore_handle_selection_request(&(ev->xselectionrequest));
        break;
    default:
        return FALSE;
    }
    
    return TRUE;
}


/*}}}*/


/*{{{ Map, unmap, destroy */


void ioncore_handle_map_request(const XMapRequestEvent *ev)
{
    WRegion *reg;
    
    reg=XWINDOW_REGION_OF(ev->window);
    
    if(reg!=NULL)
        return;
    
    ioncore_manage_clientwin(ev->window, TRUE);
}


void ioncore_handle_unmap_notify(const XUnmapEvent *ev)
{
    WClientWin *cwin;

    /* We are not interested in SubstructureNotify -unmaps. */
    if(ev->event!=ev->window && ev->send_event!=True)
        return;

    cwin=XWINDOW_REGION_OF_T(ev->window, WClientWin);
    
    if(cwin!=NULL)
        clientwin_unmapped(cwin);
}


void ioncore_handle_destroy_notify(const XDestroyWindowEvent *ev)
{
    WClientWin *cwin;

    cwin=XWINDOW_REGION_OF_T(ev->window, WClientWin);
    
    if(cwin!=NULL)
        clientwin_destroyed(cwin);
}


/*}}}*/


/*{{{ Client configure/property/message */


void ioncore_handle_configure_request(XConfigureRequestEvent *ev)
{
    WClientWin *cwin;
    XWindowChanges wc;

    cwin=XWINDOW_REGION_OF_T(ev->window, WClientWin);
    
    if(cwin==NULL){
        wc.border_width=ev->border_width;
        wc.sibling=ev->above;
        wc.stack_mode=ev->detail;
        wc.x=ev->x;
        wc.y=ev->y;
        wc.width=ev->width;
        wc.height=ev->height;
        XConfigureWindow(ioncore_g.dpy, ev->window, ev->value_mask, &wc);
        return;
    }

    clientwin_handle_configure_request(cwin, ev);
}


void ioncore_handle_client_message(const XClientMessageEvent *ev)
{
    netwm_handle_client_message(ev);

#if 0
    WClientWin *cwin;

    if(ev->message_type!=ioncore_g.atom_wm_change_state)
        return;
    
    cwin=XWINDOW_REGION_OF_T(ev->window, WClientWin);

    if(cwin==NULL)
        return;
    
    if(ev->format==32 && ev->data.l[0]==IconicState){
        if(cwin->state==NormalState)
            iconify_clientwin(cwin);
    }
#endif
}


static bool pchg_mrsh_extl(ExtlFn fn, void **p)
{
    extl_call(fn, "oi", NULL, p[0], ((XPropertyEvent*)p[1])->atom);
    return TRUE;
}

static bool pchg_mrsh(void (*fn)(void *p1, void *p2), void **p)
{
    fn(p[0], p[1]);
    return TRUE;
}
                             

void ioncore_handle_property(const XPropertyEvent *ev)
{
    WClientWin *cwin;
    
    cwin=XWINDOW_REGION_OF_T(ev->window, WClientWin);
    
    if(cwin==NULL)
        return;
    
    if(ev->atom==XA_WM_HINTS){
        XWMHints *hints;
        hints=XGetWMHints(ioncore_g.dpy, ev->window);
        /* region_notify/clear_activity take care of checking current state */
        if(hints!=NULL){
            if(hints->flags&XUrgencyHint){
                if(!region_skip_focus((WRegion*)cwin))
                    region_set_activity((WRegion*)cwin, SETPARAM_SET);
            }else{
                region_set_activity((WRegion*)cwin, SETPARAM_UNSET);
            }
        }
        XFree(hints);
    }else if(ev->atom==XA_WM_NORMAL_HINTS){
        clientwin_get_size_hints(cwin);
    }else if(ev->atom==XA_WM_NAME){
        if(!(cwin->flags&CLIENTWIN_USE_NET_WM_NAME))
            clientwin_get_set_name(cwin);
    }else if(ev->atom==XA_WM_TRANSIENT_FOR){
        clientwin_tfor_changed(cwin);
    }else if(ev->atom==ioncore_g.atom_wm_protocols){
        clientwin_get_protocols(cwin);
    }else{
        netwm_handle_property(cwin, ev);
    }
    
    /* Call property hook */
    {
        void *p[2];
        p[0]=(void*)cwin;
        p[1]=(void*)ev;
        hook_call(clientwin_property_change_hook, p,
                  (WHookMarshall*)pchg_mrsh,
                  (WHookMarshallExtl*)pchg_mrsh_extl);
    }
}


/*}}}*/


/*{{{ Misc. notifies */


void ioncore_handle_mapping_notify(XEvent *ev)
{
    do{
        XRefreshKeyboardMapping(&(ev->xmapping));
    }while(XCheckTypedEvent(ioncore_g.dpy, MappingNotify, ev));
    
    ioncore_refresh_bindmaps();
}


/*}}}*/


/*{{{ Expose */


void ioncore_handle_expose(const XExposeEvent *ev)
{
    WWindow *wwin;
    WRootWin *rootwin;
    XEvent tmp;
    
    while(XCheckWindowEvent(ioncore_g.dpy, ev->window, ExposureMask, &tmp))
        /* nothing */;

    wwin=XWINDOW_REGION_OF_T(ev->window, WWindow);

    if(wwin!=NULL)
        window_draw(wwin, FALSE);
}


/*}}}*/


/*{{{ Enter window, focus */


void ioncore_handle_enter_window(XEvent *ev)
{
    XEnterWindowEvent *eev=&(ev->xcrossing);
    WRegion *reg=NULL;
    
    if(ioncore_g.input_mode!=IONCORE_INPUTMODE_NORMAL ||
       ioncore_g.no_mousefocus){
        return;
    }
        
    if(eev->mode!=NotifyNormal && !ioncore_g.warp_enabled)
        return;
                
    reg=XWINDOW_REGION_OF_T(eev->window, WRegion);
    
    if(reg==NULL)
        return;
        
    if(REGION_IS_ACTIVE(reg))
        return;
        
    if(region_skip_focus(reg))
        return;
    
    if(ioncore_g.focus_next!=NULL &&
       ioncore_g.focus_next_source<IONCORE_FOCUSNEXT_ENTERWINDOW){
        return;
    }
    
    /* If a child of 'reg' is to be focused, do not process this
     * event. (ioncore_g.focus_next should only be set here by
     * another call to use from ioncore_handle_enter_window below.)
     */
    if(ioncore_g.focus_next!=NULL){
        WRegion *r2=ioncore_g.focus_next;
        while(r2!=NULL){
            if(r2==reg)
                return;
            r2=REGION_PARENT_REG(r2);
        }
    }
    
    if(region_goto_flags(reg, (REGION_GOTO_FOCUS|
                               REGION_GOTO_NOWARP|
                               REGION_GOTO_ENTERWINDOW))){
        ioncore_g.focus_next_source=IONCORE_FOCUSNEXT_ENTERWINDOW;
    }
}


static bool pointer_in_root(Window root1)
{
    Window root2=None, win;
    int x, y, wx, wy;
    uint mask;
    
    XQueryPointer(ioncore_g.dpy, root1, &root2, &win,
                  &x, &y, &wx, &wy, &mask);
    
    return (root1==root2);
}


void ioncore_handle_focus_in(const XFocusChangeEvent *ev)
{
    WRegion *reg;
    WWindow *wwin;
    
    reg=XWINDOW_REGION_OF_T(ev->window, WRegion);
    
    if(reg==NULL)
        return;

    D(fprintf(stderr, "FI: %s %p %d %d\n", OBJ_TYPESTR(reg), reg, ev->mode, ev->detail);)

    if(ev->mode==NotifyGrab)
        return;

    if(ev->detail==NotifyPointer)
        return;
    
    /* Input contexts */
    if(OBJ_IS(reg, WWindow)){
        wwin=(WWindow*)reg;
        if(wwin->xic!=NULL)
            XSetICFocus(wwin->xic);
    }

    if(ev->detail!=NotifyInferior)
        netwm_set_active(reg);
    
    region_got_focus(reg);
    
    if(ioncore_g.focus_next!=NULL && 
       ioncore_g.focus_next_source<IONCORE_FOCUSNEXT_FALLBACK){
        return;
    }
    
    if((ev->detail==NotifyPointerRoot || ev->detail==NotifyDetailNone) 
       && ev->window==region_root_of(reg) /* OBJ_IS(reg, WRootWin) */){
        /* Restore focus if it was returned to a root window and we don't
         * know of a pending focus change.
         */
        if(pointer_in_root(ev->window)){
            region_set_focus(reg);
            ioncore_g.focus_next_source=IONCORE_FOCUSNEXT_FALLBACK;
        }
    }else{
        /* Something got the focus, don't use fallback. */
        ioncore_g.focus_next=NULL;
    }
}


void ioncore_handle_focus_out(const XFocusChangeEvent *ev)
{
    WRegion *reg;
    WWindow *wwin;
    
    reg=XWINDOW_REGION_OF_T(ev->window, WRegion);
    
    if(reg==NULL)
        return;

    D(fprintf(stderr, "FO: %s %p %d %d\n", OBJ_TYPESTR(reg), reg, ev->mode, ev->detail);)

    if(ev->mode==NotifyGrab)
        return;

    if(ev->detail==NotifyPointer)
        return;

    D(if(OBJ_IS(reg, WRootWin))
      fprintf(stderr, "scr-out %d %d %d\n", ((WRootWin*)reg)->xscr, ev->mode, ev->detail));

    if(OBJ_IS(reg, WWindow)){
        wwin=(WWindow*)reg;
        if(wwin->xic!=NULL)
            XUnsetICFocus(wwin->xic);
    }
    
    if(ev->detail!=NotifyInferior)
        region_lost_focus(reg);
    else
        region_got_focus(reg);
}


/*}}}*/


/*{{{ Pointer, keyboard */


void ioncore_handle_buttonpress(XEvent *ev)
{
    XEvent tmp;
    Window win_pressed;
    bool finished=FALSE;

    if(ioncore_grab_held())
        return;

    win_pressed=ev->xbutton.window;
    
    if(!ioncore_do_handle_buttonpress(&(ev->xbutton)))
        return;

    while(!finished && ioncore_grab_held()){
        XFlush(ioncore_g.dpy);
        ioncore_get_event(ev, IONCORE_EVENTMASK_PTRLOOP);
        
        if(ev->type==MotionNotify){
            /* Handle sequences of MotionNotify (possibly followed by button
             * release) as one.
             */
            if(XPeekEvent(ioncore_g.dpy, &tmp)){
                if(tmp.type==MotionNotify || tmp.type==ButtonRelease)
                    XNextEvent(ioncore_g.dpy, ev);
            }
        }
        
        switch(ev->type){
        CASE_EVENT(ButtonRelease)
            if(ioncore_do_handle_buttonrelease(&ev->xbutton))
                finished=TRUE;
            break;
        CASE_EVENT(MotionNotify)
            ioncore_do_handle_motionnotify(&ev->xmotion);
            break;
        CASE_EVENT(Expose)
            ioncore_handle_expose(&(ev->xexpose));
            break;
        CASE_EVENT(KeyPress)
        CASE_EVENT(KeyRelease)
            ioncore_handle_grabs(ev);
            break;
        CASE_EVENT(FocusIn)
            ioncore_handle_focus_in(&(ev->xfocus));
            break;
        CASE_EVENT(FocusOut)
            ioncore_handle_focus_out(&(ev->xfocus));
            break;
        }
    }
}


void ioncore_handle_keyboard(XEvent *ev)
{
    if(ioncore_handle_grabs(ev))
        return;
    
    if(ev->type==KeyPress)
        ioncore_do_handle_keypress(&(ev->xkey));
}


/*}}}*/


