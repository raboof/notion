/*
 * ion/ioncore/grab.c
 *
 * Copyright (c) Lukas Schroeder 2002,
 *               Tuomo Valkonen 2003-2007.
 *
 * See the included file LICENSE for details.
 *
 * Alternatively, you may apply the Clarified Artistic License to this file,
 * since Lukas' contributions were originally under that.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include "common.h"
#include "global.h"
#include "event.h"
#include "cursor.h"
#include "grab.h"


/*{{{ Definitions */


typedef struct _grab_status{
    WRegion *holder;
    GrabHandler *handler;
    GrabKilledHandler *killedhandler;
    Watch watch;
    long eventmask;
    long flags;

    bool remove;    /* TRUE, if entry marked for removal by do_grab_remove() */
    int cursor;
    Window confine_to;
    int sqid;
}GrabStatus;

#define MAX_GRABS 4
static GrabStatus grabs[MAX_GRABS];
static GrabStatus *current_grab;
static int idx_grab=0;
static int last_sqid=0;


/*}}}*/


/*{{{ do_grab/ungrab */


static void grab_kb_ptr(Window win, Window confine_to, int cursor,
                        long eventmask)
{
    ioncore_g.input_mode=IONCORE_INPUTMODE_GRAB;

    XSelectInput(ioncore_g.dpy, win, IONCORE_EVENTMASK_ROOT&~eventmask);
    XGrabPointer(ioncore_g.dpy, win, True, IONCORE_EVENTMASK_PTRGRAB,
                 GrabModeAsync, GrabModeAsync, confine_to,
                 ioncore_xcursor(cursor), CurrentTime);
    XGrabKeyboard(ioncore_g.dpy, win, False, GrabModeAsync,
                  GrabModeAsync, CurrentTime);
    XSync(ioncore_g.dpy, False);
    XSelectInput(ioncore_g.dpy, win, IONCORE_EVENTMASK_ROOT);
}


static void ungrab_kb_ptr()
{
    XUngrabKeyboard(ioncore_g.dpy, CurrentTime);
    XUngrabPointer(ioncore_g.dpy, CurrentTime);

    ioncore_g.input_mode=IONCORE_INPUTMODE_NORMAL;
}


/*}}}*/


/*{{{ Functions for installing grabs */


static void do_holder_remove(WRegion *holder, bool killed);


static void grab_watch_handler(Watch *UNUSED(w), Obj *obj)
{
    do_holder_remove((WRegion*)obj, TRUE);
}


static void do_grab_install(GrabStatus *grab)
{
    watch_setup(&grab->watch, (Obj*)grab->holder, grab_watch_handler);
    grab_kb_ptr(region_root_of(grab->holder), grab->confine_to,
                grab->cursor, grab->eventmask);
    current_grab=grab;
}


void ioncore_grab_establish(WRegion *reg, GrabHandler *func,
                            GrabKilledHandler *kh,
                            long eventmask)
{
    assert((~eventmask)&(KeyPressMask|KeyReleaseMask));

    if(idx_grab<MAX_GRABS){
        current_grab=&grabs[idx_grab++];
        current_grab->holder=reg;
        current_grab->handler=func;
        current_grab->killedhandler=kh;
        current_grab->eventmask=eventmask;
        current_grab->remove=FALSE;
        current_grab->cursor=IONCORE_CURSOR_DEFAULT;
        current_grab->confine_to=None; /*region_root_of(reg);*/
        current_grab->sqid=last_sqid++;
        do_grab_install(current_grab);
    }
}


/*}}}*/


/*{{{ Grab removal functions */


static void do_grab_remove()
{
    current_grab=NULL;
    ungrab_kb_ptr();

    while(idx_grab>0 && grabs[idx_grab-1].remove==TRUE){
        watch_reset(&grabs[idx_grab-1].watch);
        idx_grab--;
    }

    assert(idx_grab>=0);

    if(idx_grab>0){
        current_grab=&grabs[idx_grab-1];
        do_grab_install(current_grab);
    }
}


static void mark_for_removal(GrabStatus *grab, bool killed)
{
    if(!grab->remove){
        grab->remove=TRUE;
        if(killed && grab->killedhandler!=NULL && grab->holder!=NULL)
            grab->killedhandler(grab->holder);
    }

    if(grabs[idx_grab-1].remove)
        do_grab_remove();
}


static void do_holder_remove(WRegion *holder, bool killed)
{
    int i;

    for(i=idx_grab-1; i>=0; i--){
        if(grabs[i].holder==holder)
            mark_for_removal(grabs+i, killed);
    }
}


void ioncore_grab_holder_remove(WRegion *holder)
{
    do_holder_remove(holder, FALSE);
}


void ioncore_grab_remove(GrabHandler *func)
{
    int i;
    for(i=idx_grab-1; i>=0; i--){
        if(grabs[i].handler==func){
            mark_for_removal(grabs+i, FALSE);
            break;
        }
    }
}


/*}}}*/


/*{{{ Grab handler calling */


bool ioncore_handle_grabs(XEvent *ev)
{
    GrabStatus *gr;
    int gr_sqid;

    while(current_grab && current_grab->remove)
        do_grab_remove();

    if(current_grab==NULL || current_grab->holder==NULL ||
       current_grab->handler==NULL){
        return FALSE;
    }

    /* Escape key is harcoded to always kill active grab. */
    if(ev->type==KeyPress && XLookupKeysym(&(ev->xkey), 0)==XK_Escape){
        mark_for_removal(current_grab, TRUE);
        return TRUE;
    }

    if(ev->type!=KeyRelease && ev->type!=KeyPress)
        return FALSE;

    /* We must check that the grab pointed to by current_grab still
     * is the same grab and not already released or replaced by
     * another grab.
     */
    gr=current_grab;
    gr_sqid=gr->sqid;
    if(gr->handler(gr->holder, ev) && gr->sqid==gr_sqid)
        mark_for_removal(gr, FALSE);

    return TRUE;
}


/*}}}*/


/*{{{ Misc. */


bool ioncore_grab_held()
{
    return idx_grab>0;
}


void ioncore_change_grab_cursor(int cursor)
{
    if(current_grab!=NULL){
        current_grab->cursor=cursor;
        XChangeActivePointerGrab(ioncore_g.dpy, IONCORE_EVENTMASK_PTRGRAB,
                                 ioncore_xcursor(cursor), CurrentTime);
    }
}


void ioncore_grab_confine_to(Window confine_to)
{
    if(current_grab!=NULL){
        current_grab->confine_to=confine_to;
        XGrabPointer(ioncore_g.dpy, region_root_of(current_grab->holder),
                     True, IONCORE_EVENTMASK_PTRGRAB, GrabModeAsync,
                     GrabModeAsync, confine_to,
                     ioncore_xcursor(IONCORE_CURSOR_DEFAULT),
                     CurrentTime);
    }
}


WRegion *ioncore_grab_get_holder()
{
    if (ioncore_grab_held())
        return grabs[idx_grab-1].holder;
    return NULL;
}


WRegion *ioncore_grab_get_my_holder(GrabHandler *func)
{
    int i;
    for(i=idx_grab-1; i>=0; i--)
        if(grabs[i].handler==func)
            return grabs[i].holder;
    return NULL;
}


/*}}}*/

