/*
 * ion/ioncore/kbresize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>

#include "global.h"
#include "resize.h"
#include "kbresize.h"
#include "grab.h"
#include "binding.h"
#include "signal.h"
#include "focus.h"
#include "bindmaps.h"
#include "framep.h"


/*{{{ Resize accelerator */


static struct timeval last_action_tv={-1, 0};
static struct timeval last_update_tv={-1, 0};
static int last_accel_mode=0;
static double accel=1, accelinc=30, accelmax=100*100;
static long actmax=200, uptmin=50;

static void accel_reset()
{
    last_accel_mode=0;
    accel=1.0;
    last_action_tv.tv_sec=-1;
    last_action_tv.tv_usec=-1;
}


/*EXTL_DOC
 * Set keyboard resize acceleration parameters. When a keyboard resize
 * function is called, and at most \var{t_max} milliseconds has passed
 * from a previous call, acceleration factor is reset to 1.0. Otherwise,
 * if at least \var{t_min} milliseconds have passed from the from previous
 * acceleration update or reset the squere root of the acceleration factor
 * is incremented by \var{step}. The maximum acceleration factor (pixels/call 
 * modulo size hints) is given by \var{maxacc}. The default values are 
 * (200, 50, 30, 100). 
 * 
 * Notice the interplay with X keyboard acceleration parameters.
 * (Maybe insteed of \var{t_min} we should use a minimum number of
 * calls to the function/key presses between updated? Or maybe the
 * resize should be completely time-based with key presses triggering
 * the changes?)
 */
EXTL_EXPORT
void ioncore_set_moveres_accel(int t_max, int t_min, 
                               double step, double maxacc)
{
    actmax=(t_max>0 ? t_max : INT_MAX);
    uptmin=(t_min>0 ? t_min : INT_MAX);
    accelinc=(step>0 ? step : 1);
    accelmax=(maxacc>0 ? maxacc*maxacc : 1);
}


static int sign(int x)
{
    return (x>0 ? 1 : (x<0 ? -1 : 0));
}


static long tvdiffmsec(struct timeval *tv1, struct timeval *tv2)
{
    double t1=1000*(double)tv1->tv_sec+(double)tv1->tv_usec/1000;
    double t2=1000*(double)tv2->tv_sec+(double)tv2->tv_usec/1000;
    
    return (int)(t1-t2);
}

#define SIGN_NZ(X) ((X) < 0 ? -1 : 1)

static double max(double a, double b)
{
    return (a<b ? b : a);
}

void moveresmode_accel(WMoveresMode *mode, int *wu, int *hu, int accel_mode)
{
    struct timeval tv;
    long adiff, udiff;
    
    gettimeofday(&tv, NULL);
    
    adiff=tvdiffmsec(&tv, &last_action_tv);
    udiff=tvdiffmsec(&tv, &last_update_tv);
    
    if(last_accel_mode==accel_mode && adiff<actmax){
        if(udiff>uptmin){
            accel+=accelinc;
            if(accel>accelmax)
                accel=accelmax;
            last_update_tv=tv;
        }
    }else{
        accel=1.0;
        last_update_tv=tv;
    }
    
    last_accel_mode=accel_mode;
    last_action_tv=tv;
    
    if(*wu!=0)
        *wu=(*wu)*ceil(sqrt(accel)/abs(*wu));
    if(*hu!=0)
        *hu=(*hu)*ceil(sqrt(accel)/abs(*hu));
}


/*}}}*/


/*{{{ Keyboard resize handler */


static ExtlSafelist moveres_safe_funclist[]={
    (ExtlExportedFn*)&moveresmode_resize,
    (ExtlExportedFn*)&moveresmode_move,
    (ExtlExportedFn*)&moveresmode_finish,
    (ExtlExportedFn*)&moveresmode_cancel,
    NULL
};


static bool resize_handler(WRegion *reg, XEvent *xev)
{
    XKeyEvent *ev=&xev->xkey;
    WBinding *binding=NULL;
    WBindmap **bindptr;
    WMoveresMode *mode;
    
    if(ev->type==KeyRelease)
        return FALSE;
    
    if(reg==NULL)
        return FALSE;
    
    mode=moveres_mode(reg);
    
    if(mode==NULL)
        return FALSE;
    
    binding=bindmap_lookup_binding(ioncore_moveres_bindmap, 
                                   BINDING_KEYPRESS,
                                   ev->state, ev->keycode);
    
    if(!binding)
        return FALSE;
    
    if(binding!=NULL){
        const ExtlSafelist *old_safelist=
            extl_set_safelist(moveres_safe_funclist);
        extl_call(binding->func, "o", NULL, mode);
        extl_set_safelist(old_safelist);
    }
    
    return (moveres_mode(reg)==NULL);
}


/*}}}*/


/*{{{ Resize timer */


static void tmr_end_resize(WTimer *unused)
{
    WMoveresMode *mode=moveres_mode(NULL);
    if(mode!=NULL)
        moveresmode_cancel(mode);
}


static WTimer resize_timer=TIMER_INIT(tmr_end_resize);


/*}}}*/


/*{{{ Misc. */


static int limit_and_encode_mode(int *left, int *right, 
                                 int *top, int *bottom)
{
    *left=sign(*left);
    *right=sign(*right);
    *top=sign(*top);
    *bottom=sign(*bottom);

    return (*left)+(*right)*3+(*top)*9+(*bottom)*27;
}


static void resize_units(WMoveresMode *mode, int *wret, int *hret)
{
    /**wret=1;
    *hret=1;
    
    if(FRAME_CURRENT(frame)!=NULL){
        XSizeHints hints;
        
        region_resize_hints(FRAME_CURRENT(frame), &hints, NULL, NULL);
        
        if(hints.flags&PResizeInc &&
           (hints.width_inc>1 || hints.height_inc>1)){
            *wret=hints.width_inc;
            *hret=hints.height_inc;
        }
    }*/

    XSizeHints *h=&(mode->hints);
    *wret=1;
    *hret=1;
    if(h->flags&PResizeInc && (h->width_inc>1 || h->height_inc>1)){
        *wret=h->width_inc;
        *hret=h->height_inc;
    }
}


/*}}}*/


/*{{{ Keyboard resize interface */


/*EXTL_DOC
 * Shrink or grow resize mode target one step in each direction.
 * Acceptable values for the parameters \var{left}, \var{right}, \var{top}
 * and \var{bottom} are as follows: -1: shrink along,
 * 0: do not change, 1: grow along corresponding border.
 */
EXTL_EXPORT_MEMBER
void moveresmode_resize(WMoveresMode *mode, 
                        int left, int right, int top, int bottom)
{
    int wu=0, hu=0;
    int accel_mode=0;
    
    accel_mode=3*limit_and_encode_mode(&left, &right, &top, &bottom);
    resize_units(mode, &wu, &hu);
    moveresmode_accel(mode, &wu, &hu, accel_mode);

    moveresmode_delta_resize(mode, -left*wu, right*wu, -top*hu, bottom*hu, 
                             NULL);
    
    timer_set(&resize_timer, ioncore_g.resize_delay);
}


/*EXTL_DOC
 * Move resize mode target one step:
 *
 * \begin{tabular}{rl}
 * \hline
 * \var{horizmul}/\var{vertmul} & effect \\\hline
 * -1 & Move left/up \\
 * 0 & No effect \\
 * 1 & Move right/down \\
 * \end{tabular}
 */
EXTL_EXPORT_MEMBER
void moveresmode_move(WMoveresMode *mode, int horizmul, int vertmul)
{
    int accel_mode=0, dummy=0;
    
    accel_mode=1+3*limit_and_encode_mode(&horizmul, &vertmul, &dummy, &dummy);
    moveresmode_accel(mode, &horizmul, &vertmul, accel_mode);

    moveresmode_delta_resize(mode, horizmul, horizmul, vertmul, vertmul, 
                             NULL);
    
    timer_set(&resize_timer, ioncore_g.resize_delay);
}


/*EXTL_DOC
 * Return from move/resize mode and apply changes unless opaque
 * move/resize is enabled.
 */
EXTL_EXPORT_MEMBER
void moveresmode_finish(WMoveresMode *mode)
{
    WRegion *reg=moveresmode_target(mode);
    if(moveresmode_do_end(mode, TRUE)){
        timer_reset(&resize_timer);
        region_warp(reg);
        ioncore_grab_remove(resize_handler);
    }
}


/*EXTL_DOC
 * Return from move/resize cancelling changes if opaque
 * move/resize has not been enabled.
 */
EXTL_EXPORT_MEMBER
void moveresmode_cancel(WMoveresMode *mode)
{
    WRegion *reg=moveresmode_target(mode);
    if(moveresmode_do_end(mode, FALSE)){
        timer_reset(&resize_timer);
        region_warp(reg);
        ioncore_grab_remove(resize_handler);
    }
}


static void cancel_moveres(WRegion *reg)
{
    WMoveresMode *mode=moveres_mode(reg);
    if(mode!=NULL)
        moveresmode_cancel(mode);
}

    
/*EXTL_DOC
 * Enter move/resize mode for \var{frame}. The bindings set with
 * \fnref{WMoveresMode.bindings} are used in this mode and of
 * of the exported functions only \fnref{WMoveresMode.resize}, 
 * \fnref{WMoveresMode.move}, \fnref{WMoveresMode.cancel} and
 * \fnref{WMoveresMode.end} are allowed to be called.
 */
EXTL_EXPORT_MEMBER
WMoveresMode *frame_begin_moveres(WFrame *frame)
{
    WMoveresMode *mode=region_begin_resize((WRegion*)frame, NULL, FALSE);

    if(mode==NULL)
        return NULL;
    
    accel_reset();
    
    ioncore_grab_establish((WRegion*)frame, resize_handler,
                           (GrabKilledHandler*)cancel_moveres, 0);
    
    timer_set(&resize_timer, ioncore_g.resize_delay);
    
    return mode;
    
}


/*}}}*/
