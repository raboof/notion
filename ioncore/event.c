/*
 * ion/ioncore/event.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <X11/Xmd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/signal.h>

#include <libmainloop/select.h>
#include <libmainloop/signal.h>
#include <libmainloop/defer.h>

#include "common.h"
#include "global.h"
#include "event.h"
#include "eventh.h"
#include "focus.h"
#include "exec.h"
#include "ioncore.h"



/*{{{ Hooks */


WHook *ioncore_handle_event_alt=NULL;


/*}}}*/


/*{{{ Signal check */
    

static void check_signals()
{
    int kill_sig=mainloop_check_signals();
    
    if(kill_sig!=0){
        if(kill_sig==SIGUSR1){
            ioncore_restart();
            assert(0);
        } 
        if(kill_sig==SIGTERM){
            /* Save state if not running under a session manager. */
            ioncore_emergency_snapshot();
            ioncore_resign();
            /* We may still return here if running under a session manager. */
        }else{
            ioncore_emergency_snapshot();
            ioncore_deinit();
            kill(getpid(), kill_sig);
        }
    }
}


/*}}}*/


/*{{{ Timestamp stuff */

#define CHKEV(E, T) case E: tm=((T*)ev)->time; break;
#define CLOCK_SKEW_MS 30000

static Time last_timestamp=CurrentTime;

void ioncore_update_timestamp(XEvent *ev)
{
    Time tm;
    
    switch(ev->type){
    CHKEV(ButtonPress, XButtonPressedEvent);
    CHKEV(ButtonRelease, XButtonReleasedEvent);
    CHKEV(EnterNotify, XEnterWindowEvent);
    CHKEV(KeyPress, XKeyPressedEvent);
    CHKEV(KeyRelease, XKeyReleasedEvent);
    CHKEV(LeaveNotify, XLeaveWindowEvent);
    CHKEV(MotionNotify, XPointerMovedEvent);
    CHKEV(PropertyNotify, XPropertyEvent);
    CHKEV(SelectionClear, XSelectionClearEvent);
    CHKEV(SelectionNotify, XSelectionEvent);
    CHKEV(SelectionRequest, XSelectionRequestEvent);
    default:
        return;
    }

    if(tm>last_timestamp || last_timestamp - tm > CLOCK_SKEW_MS)
        last_timestamp=tm;
}


Time ioncore_get_timestamp()
{
    if(last_timestamp==CurrentTime){
        /* Idea blatantly copied from wmx */
        XEvent ev;
        Atom dummy;
        
        D(fprintf(stderr, "Attempting to get time from X server."));
        
        dummy=XInternAtom(ioncore_g.dpy, "_ION_TIMEREQUEST", False);
        if(dummy==None){
            warn(TR("Time request from X server failed."));
            return 0;
        }
        /* TODO: use some other window that should also function as a
         * NET_WM support check window.
         */
        XChangeProperty(ioncore_g.dpy, ioncore_g.rootwins->dummy_win,
                        dummy, dummy, 8, PropModeAppend,
                        (unsigned char*)"", 0);
        ioncore_get_event(&ev, PropertyChangeMask);
        XPutBackEvent(ioncore_g.dpy, &ev);
    }
    
    return last_timestamp;
}


/*}}}*/


/*{{{ Event reading */


void ioncore_get_event(XEvent *ev, long mask)
{
    fd_set rfds;
    
    while(1){
        check_signals();
        
        if(XCheckMaskEvent(ioncore_g.dpy, mask, ev)){
            ioncore_update_timestamp(ev);
            return;
        }
        
        FD_ZERO(&rfds);
        FD_SET(ioncore_g.conn, &rfds);

        /* Other FD:s are _not_ to be handled! */
        select(ioncore_g.conn+1, &rfds, NULL, NULL, NULL);
    }
}


/*}}}*/


/*{{{ Flush */


static void skip_enterwindow()
{
    XEvent ev;
    
    XSync(ioncore_g.dpy, False);
    
    while(XCheckMaskEvent(ioncore_g.dpy, EnterWindowMask, &ev)){
        ioncore_update_timestamp(&ev);
    }
}


void ioncore_flushfocus()
{
    WRegion *next;
    bool warp;
    
    if(ioncore_g.input_mode!=IONCORE_INPUTMODE_NORMAL)
        return;
        
    next=ioncore_g.focus_next;
    warp=ioncore_g.warp_next;

    if(next==NULL)
        return;
        
    ioncore_g.focus_next=NULL;
        
    region_do_set_focus(next, warp);
        
    /* Just greedily eating it all away that X has to offer
     * seems to be the best we can do with Xlib.
     */
    if(warp)
        skip_enterwindow();
}


/*}}}*/


/*{{{ X connection FD handler */


void ioncore_x_connection_handler(int conn, void *unused)
{
    XEvent ev;

    XNextEvent(ioncore_g.dpy, &ev);
    ioncore_update_timestamp(&ev);

    hook_call_alt_p(ioncore_handle_event_alt, &ev, NULL);
}


/*}}}*/


/*{{{ Mainloop */


void ioncore_mainloop()
{
    mainloop_trap_signals(NULL);
    
    ioncore_g.opmode=IONCORE_OPMODE_NORMAL;

    while(1){
        check_signals();
        mainloop_execute_deferred();
        
        if(QLength(ioncore_g.dpy)==0){
            XSync(ioncore_g.dpy, False);
            
            if(QLength(ioncore_g.dpy)==0){
                ioncore_flushfocus();
                XSync(ioncore_g.dpy, False);
                
                if(QLength(ioncore_g.dpy)==0){
                    mainloop_select();
                    continue;
                }
            }
        }
        
        ioncore_x_connection_handler(ioncore_g.conn, NULL);
    }
}


/*}}}*/

