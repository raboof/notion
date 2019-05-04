/*
 * ion/ioncore/presize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#include "presize.h"
#include "resize.h"
#include "window.h"
#include "pointer.h"
#include "grab.h"


/*{{{ Resize */


static int p_dx1mul=0, p_dx2mul=0, p_dy1mul=0, p_dy2mul=0;


#define MINCORNER 16


void window_p_resize_prepare(WWindow *wwin, XButtonEvent *ev)
{
    int ww=REGION_GEOM(wwin).w/2;
    int hh=REGION_GEOM(wwin).h/2;
    int xdiv, ydiv;
    int tmpx, tmpy, atmpx, atmpy;

    p_dx1mul=0;
    p_dx2mul=0;
    p_dy1mul=0;
    p_dy2mul=0;
    
    tmpx=ev->x-ww;
    tmpy=hh-ev->y;
    xdiv=ww/2;
    ydiv=hh/2;
    atmpx=abs(tmpx);
    atmpy=abs(tmpy);
    
    if(xdiv<MINCORNER && xdiv>1){
        xdiv=ww-MINCORNER;
        if(xdiv<1)
            xdiv=1;
    }
    
    if(ydiv<MINCORNER && ydiv>1){
        ydiv=hh-MINCORNER;
        if(ydiv<1)
            ydiv=1;
    }
    
    if(xdiv==0){
        p_dx2mul=1;
    }else if(hh*atmpx/xdiv>=tmpy && -hh*atmpx/xdiv<=tmpy){
        p_dx1mul=(tmpx<0);
        p_dx2mul=(tmpx>=0);
    }
    
    if(ydiv==0){
        p_dy2mul=1;
    }else if(ww*atmpy/ydiv>=tmpx && -ww*atmpy/ydiv<=tmpx){
        p_dy1mul=(tmpy>0);
        p_dy2mul=(tmpy<=0);
    }
}


static void p_moveres_end(WWindow *wwin, XButtonEvent *UNUSED(ev))
{
    WMoveresMode *mode=moveres_mode((WRegion*)wwin);
    if(mode!=NULL)
        moveresmode_do_end(mode, TRUE);
}


static void p_moveres_cancel(WWindow *wwin)
{
    WMoveresMode *mode=moveres_mode((WRegion*)wwin);
    if(mode!=NULL)
        moveresmode_do_end(mode, FALSE);
}


static void confine_to_parent(WWindow *wwin)
{
    WRegion *par=REGION_PARENT_REG(wwin);
    if(par!=NULL)
        ioncore_grab_confine_to(region_xwindow(par));
}


static void p_resize_motion(WWindow *wwin, XMotionEvent *UNUSED(ev), int dx, int dy)
{
    WMoveresMode *mode=moveres_mode((WRegion*)wwin);
    if(mode!=NULL){
        moveresmode_delta_resize(mode, p_dx1mul*dx, p_dx2mul*dx,
                                 p_dy1mul*dy, p_dy2mul*dy, NULL);
    }
}


static void p_resize_begin(WWindow *wwin, XMotionEvent *ev, int dx, int dy)
{
    region_begin_resize((WRegion*)wwin, NULL, TRUE);
    p_resize_motion(wwin, ev, dx, dy);
}


/*EXTL_DOC
 * Start resizing \var{wwin} with the mouse or other pointing device.
 * This function should only be used by binding it to \emph{mpress} or
 * \emph{mdrag} action.
 */
EXTL_EXPORT_MEMBER
void window_p_resize(WWindow *wwin)
{
    if(!ioncore_set_drag_handlers((WRegion*)wwin,
                                  (WMotionHandler*)p_resize_begin,
                                  (WMotionHandler*)p_resize_motion,
                                  (WButtonHandler*)p_moveres_end,
                                  NULL, 
                                  (GrabKilledHandler*)p_moveres_cancel))
        return;
    
    confine_to_parent(wwin);
}


/*}}}*/


/*{{{ Move */


static void p_move_motion(WWindow *wwin, XMotionEvent *UNUSED(ev), int dx, int dy)
{
    WMoveresMode *mode=moveres_mode((WRegion*)wwin);
    if(mode!=NULL)
        moveresmode_delta_move(mode, dx, dy, NULL);
}


static void p_move_begin(WWindow *wwin, XMotionEvent *ev, int dx, int dy)
{
    region_begin_move((WRegion*)wwin, NULL, TRUE);
    p_move_motion(wwin, ev, dx, dy);
}


/*EXTL_DOC
 * Start moving \var{wwin} with the mouse or other pointing device.
 * This function should only be used by binding it to \emph{mpress} or
 * \emph{mdrag} action.
 */
EXTL_EXPORT_MEMBER
void window_p_move(WWindow *wwin)
{
    if(!ioncore_set_drag_handlers((WRegion*)wwin,
                                  (WMotionHandler*)p_move_begin,
                                  (WMotionHandler*)p_move_motion,
                                  (WButtonHandler*)p_moveres_end,
                                  NULL, 
                                  (GrabKilledHandler*)p_moveres_cancel))
        return;
    
    confine_to_parent(wwin);
}


/*}}}*/
