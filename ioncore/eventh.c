/*
 * ion/ioncore/eventh.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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
#include "region-iter.h"
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
                   region_notify_activity((WRegion*)cwin);
            }else{
                region_clear_activity((WRegion*)cwin, TRUE);
            }
        }
        XFree(hints);
    }else if(ev->atom==XA_WM_NORMAL_HINTS){
        clientwin_get_size_hints(cwin);
    }else if(ev->atom==XA_WM_NAME){
        if(!(cwin->flags&CLIENTWIN_USE_NET_WM_NAME))
            clientwin_get_set_name(cwin);
    }else if(ev->atom== XA_WM_TRANSIENT_FOR){
        clientwin_tfor_changed(cwin);
    }else if(ev->atom==ioncore_g.atom_wm_protocols){
        clientwin_get_protocols(cwin);
    }else{
        netwm_handle_property(cwin, ev);
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
    WRegion *reg=NULL, *freg=NULL, *mgr=NULL;
    bool more=TRUE;
    
    if(ioncore_g.input_mode!=IONCORE_INPUTMODE_NORMAL)
        return;

    do{
        if(eev->mode!=NotifyNormal && !ioncore_g.warp_enabled)
            continue;
        /*if(eev->detail==NotifyNonlinearVirtual)
            continue;*/

        reg=XWINDOW_REGION_OF_T(eev->window, WRegion);
        
        if(reg==NULL)
            continue;

        D(fprintf(stderr, "E: %p %s %d %d\n", reg, OBJ_TYPESTR(reg),
                  eev->mode, eev->detail));
        
        /* If an EnterWindow event was already found that we're going to
         * handle, do not note subsequent events if they are into an 
         * ancestor of the window of freg.
         */
        if(freg!=NULL){
            WRegion *r2=freg;
            while(r2!=NULL){
                if(r2==reg)
                    break;
                r2=REGION_PARENT_REG(r2);
            }
            if(r2!=NULL)
                continue;
        }
        
        if(!REGION_IS_ACTIVE(reg))
            freg=reg;
    }while(freg!=NULL && XCheckMaskEvent(ioncore_g.dpy, EnterWindowMask, ev));

    if(freg==NULL)
        return;
    
    if(!region_skip_focus(freg)){
        region_goto_flags(freg, (REGION_GOTO_FOCUS|
                                 REGION_GOTO_NOWARP|
                                 REGION_GOTO_ENTERWINDOW));
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
    WWindow *wwin, *tmp;
    Colormap cmap=None;

    reg=XWINDOW_REGION_OF_T(ev->window, WRegion);
    
    if(reg==NULL)
        return;

    D(fprintf(stderr, "FI: %s %p %d %d\n", OBJ_TYPESTR(reg), reg, ev->mode, ev->detail);)

    if(ev->mode==NotifyGrab)
        return;

    if(ev->detail==NotifyPointer)
        return;
    
    /* Root windows appear either as WRootWins or WScreens */
    if(ev->window==region_root_of(reg)){
        D(fprintf(stderr, "scr-in %d %d %d\n", ROOTWIN_OF(reg)->xscr,
                  ev->mode, ev->detail));
        if((ev->detail==NotifyPointerRoot || ev->detail==NotifyDetailNone) &&
           pointer_in_root(ev->window) && ioncore_g.focus_next==NULL){
            /* Restore focus */
            region_set_focus(reg);
            return;
        }
        /*return;*/
    }

    /* Input contexts */
    if(OBJ_IS(reg, WWindow)){
        wwin=(WWindow*)reg;
        if(wwin->xic!=NULL)
            XSetICFocus(wwin->xic);
    }
    
    region_got_focus(reg);
    
    if(ev->detail!=NotifyInferior)
        netwm_set_active(reg);
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


