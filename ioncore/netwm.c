/*
 * ion/ioncore/netwm.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
 */

#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include <libtu/util.h>
#include "common.h"
#include "global.h"
#include "fullscreen.h"
#include "clientwin.h"
#include "netwm.h"
#include "property.h"
#include "activity.h"
#include "focus.h"
#include "xwindow.h"
#include "extlconv.h"
#include "group.h"


/*{{{ Atoms */

static Atom atom_net_wm_name=0;
static Atom atom_net_wm_state=0;
static Atom atom_net_wm_state_fullscreen=0;
static Atom atom_net_supporting_wm_check=0;
static Atom atom_net_virtual_roots=0;
static Atom atom_net_active_window=0;

#define N_NETWM 6

static Atom atom_net_supported=0;

/*}}}*/


/*{{{ Initialisation */


void netwm_init()
{
    atom_net_wm_name=XInternAtom(ioncore_g.dpy, "_NET_WM_NAME", False);
    atom_net_wm_state=XInternAtom(ioncore_g.dpy, "_NET_WM_STATE", False);
    atom_net_wm_state_fullscreen=XInternAtom(ioncore_g.dpy, "_NET_WM_STATE_FULLSCREEN", False);
    atom_net_supported=XInternAtom(ioncore_g.dpy, "_NET_SUPPORTED", False);
    atom_net_supporting_wm_check=XInternAtom(ioncore_g.dpy, "_NET_SUPPORTING_WM_CHECK", False);
    atom_net_virtual_roots=XInternAtom(ioncore_g.dpy, "_NET_VIRTUAL_ROOTS", False);
    atom_net_active_window=XInternAtom(ioncore_g.dpy, "_NET_ACTIVE_WINDOW", False);
}


void netwm_init_rootwin(WRootWin *rw)
{
    Atom atoms[N_NETWM];
        const char *p[1];

    atoms[0]=atom_net_wm_name;
    atoms[1]=atom_net_wm_state;
    atoms[2]=atom_net_wm_state_fullscreen;
    atoms[3]=atom_net_supporting_wm_check;
    atoms[4]=atom_net_virtual_roots;
    atoms[5]=atom_net_active_window;
    
    XChangeProperty(ioncore_g.dpy, WROOTWIN_ROOT(rw),
                    atom_net_supporting_wm_check, XA_WINDOW,
                    32, PropModeReplace, (uchar*)&(rw->dummy_win), 1);
    XChangeProperty(ioncore_g.dpy, WROOTWIN_ROOT(rw),
                    atom_net_supported, XA_ATOM,
                    32, PropModeReplace, (uchar*)atoms, N_NETWM);

    p[0]=libtu_progbasename();
    xwindow_set_text_property(rw->dummy_win, atom_net_wm_name, p, 1);
}


/*}}}*/


/*{{{ _NET_WM_STATE */


WScreen *netwm_check_initial_fullscreen(WClientWin *cwin)
{

    int i, n;
    int ret=0;
    long *data;
    
    n=xwindow_get_property(cwin->win, atom_net_wm_state, XA_ATOM,
                   1, TRUE, (uchar**)&data);
    
    if(n<0)
        return NULL;
    
    for(i=0; i<n; i++){
        if(data[i]==(long)atom_net_wm_state_fullscreen)
            return region_screen_of((WRegion*)cwin);
    }
    
    XFree((void*)data);

    return NULL;
}


void netwm_update_state(WClientWin *cwin)
{
    CARD32 data[1];
    int n=0;
    
    if(REGION_IS_FULLSCREEN(cwin))
        data[n++]=atom_net_wm_state_fullscreen;

    XChangeProperty(ioncore_g.dpy, cwin->win, atom_net_wm_state, 
                    XA_ATOM, 32, PropModeReplace, (uchar*)data, n);
}


void netwm_delete_state(WClientWin *cwin)
{
    XDeleteProperty(ioncore_g.dpy, cwin->win, atom_net_wm_state);
}



static void netwm_state_change_rq(WClientWin *cwin, 
                                  const XClientMessageEvent *ev)
{
    if((ev->data.l[1]==0 ||
        ev->data.l[1]!=(long)atom_net_wm_state_fullscreen) &&
       (ev->data.l[2]==0 ||
        ev->data.l[2]!=(long)atom_net_wm_state_fullscreen)){
        return;
    }
    
    /* Ok, full screen add/remove/toggle */
    if(!REGION_IS_FULLSCREEN(cwin)){
        if(ev->data.l[0]==_NET_WM_STATE_ADD || 
           ev->data.l[0]==_NET_WM_STATE_TOGGLE){
            WRegion *grp=region_groupleader_of((WRegion*)cwin);
            bool sw=clientwin_fullscreen_may_switchto(cwin);
            cwin->flags|=CLIENTWIN_FS_RQ;
            if(!region_enter_fullscreen(grp, sw))
                cwin->flags&=~CLIENTWIN_FS_RQ;
        }else{
            /* Should not be set.. */
            cwin->flags&=~CLIENTWIN_FS_RQ;
        }
    }else{
        if(ev->data.l[0]==_NET_WM_STATE_REMOVE || 
           ev->data.l[0]==_NET_WM_STATE_TOGGLE){
            WRegion *grp=region_groupleader_of((WRegion*)cwin);
            bool sw=clientwin_fullscreen_may_switchto(cwin);
            cwin->flags&=~CLIENTWIN_FS_RQ;
            region_leave_fullscreen(grp, sw);
        }else{
            /* Set the flag */
            cwin->flags|=CLIENTWIN_FS_RQ;
        }
    }
}


/*}}}*/


/*{{{ _NET_ACTIVE_WINDOW */


void netwm_set_active(WRegion *reg)
{
    CARD32 data[1]={None};
    
    if(OBJ_IS(reg, WClientWin))
        data[0]=region_xwindow(reg);

    /* The spec doesn't say how multihead should be handled, so
     * we just update the root window the window is on.
     */
    XChangeProperty(ioncore_g.dpy, region_root_of(reg), 
                    atom_net_active_window, XA_WINDOW, 
                    32, PropModeReplace, (uchar*)data, 1);
}


static void netwm_active_window_rq(WClientWin *cwin, 
                                   const XClientMessageEvent *ev)
{
    bool ignore=TRUE;
    
    extl_table_gets_b(cwin->proptab, "ignore_net_active_window", &ignore);
    
    if(!ignore)
        region_goto((WRegion*)cwin);
    else
        region_set_activity((WRegion*)cwin, SETPARAM_SET);
}


/*}}}*/


/*{{{ _NET_WM_NAME */


char **netwm_get_name(WClientWin *cwin)
{
    return xwindow_get_text_property(cwin->win, atom_net_wm_name, NULL);
}


/*}}}*/


/*{{{ netwm_handle_client_message */


void netwm_handle_client_message(const XClientMessageEvent *ev)
{
    /* Check _NET_WM_STATE fullscreen request */
    if(ev->message_type==atom_net_wm_state && ev->format==32){
        WClientWin *cwin=XWINDOW_REGION_OF_T(ev->window, WClientWin);
        if(cwin!=NULL)
            netwm_state_change_rq(cwin, ev);
    }
    /* Check _NET_ACTIVE_WINDOW request */
    else if(ev->message_type==atom_net_active_window && ev->format==32){
        WClientWin *cwin=XWINDOW_REGION_OF_T(ev->window, WClientWin);
        if(cwin!=NULL)
            netwm_active_window_rq(cwin, ev);
    }
}


/*}}}*/


/*{{{ netwm_handle_property */


bool netwm_handle_property(WClientWin *cwin, const XPropertyEvent *ev)
{
    if(ev->atom!=atom_net_wm_name)
        return FALSE;
    
    clientwin_get_set_name(cwin);
    return TRUE;
}


/*}}}*/
