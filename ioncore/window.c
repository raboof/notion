/*
 * ion/ioncore/window.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include "common.h"
#include "global.h"
#include "window.h"
#include "focus.h"
#include "rootwin.h"
#include "region.h"
#include "stacking.h"
#include "xwindow.h"


/*{{{ Dynfuns */


void window_draw(WWindow *wwin, bool complete)
{
    CALL_DYN(window_draw, wwin, (wwin, complete));
}


void window_insstr(WWindow *wwin, const char *buf, size_t n)
{
    CALL_DYN(window_insstr, wwin, (wwin, buf, n));
}


int window_press(WWindow *wwin, XButtonEvent *ev, WRegion **reg_ret)
{
    int area=0;
    CALL_DYN_RET(area, int, window_press, wwin, (wwin, ev, reg_ret));
    return area;
}


void window_release(WWindow *wwin)
{
    CALL_DYN(window_release, wwin, (wwin));
}


/*}}}*/
    

/*{{{ Init, create */


bool window_init(WWindow *wwin, WWindow *par, Window win, 
                 const WFitParams *fp)
{
    wwin->win=win;
    wwin->xic=NULL;
    wwin->keep_on_top_list=NULL;
    region_init(&(wwin->region), par, fp);
    if(win!=None){
        XSaveContext(ioncore_g.dpy, win, ioncore_g.win_context, 
                     (XPointer)wwin);
        if(par!=NULL)
            window_init_sibling_stacking(par, win);
    }
    return TRUE;
}


bool window_init_new(WWindow *wwin, WWindow *par, const WFitParams *fp)
{
    Window win;
    
    win=create_xwindow(region_rootwin_of((WRegion*)par),
                       par->win, &(fp->g));
    if(win==None)
        return FALSE;
    /* window_init does not fail */
    return window_init(wwin, par, win, fp);
}


void window_deinit(WWindow *wwin)
{
    region_deinit((WRegion*)wwin);
    
    if(wwin->xic!=NULL)
        XDestroyIC(wwin->xic);

    if(wwin->win!=None){
        XDeleteContext(ioncore_g.dpy, wwin->win, ioncore_g.win_context);
        XDestroyWindow(ioncore_g.dpy, wwin->win);
    }
}


/*}}}*/


/*{{{ Region dynfuns */


void window_do_fitrep(WWindow *wwin, WWindow *par, const WRectangle *geom)
{
    bool move=(REGION_GEOM(wwin).x!=geom->x ||
               REGION_GEOM(wwin).y!=geom->y);
    int w=maxof(1, geom->w);
    int h=maxof(1, geom->h);

    if(par!=NULL){
        region_detach_parent((WRegion*)wwin);
        XReparentWindow(ioncore_g.dpy, wwin->win, par->win, geom->x, geom->y);
        XResizeWindow(ioncore_g.dpy, wwin->win, w, h);
        region_attach_parent((WRegion*)wwin, (WRegion*)par);
    }else{
        XMoveResizeWindow(ioncore_g.dpy, wwin->win, geom->x, geom->y, w, h);
    }
    
    REGION_GEOM(wwin)=*geom;

    if(move)
        region_notify_subregions_move(&(wwin->region));
}


bool window_fitrep(WWindow *wwin, WWindow *par, const WFitParams *fp)
{
    if(par!=NULL && !region_same_rootwin((WRegion*)wwin, (WRegion*)par))
        return FALSE;
    window_do_fitrep(wwin, par, &(fp->g));
    return TRUE;
}


void window_map(WWindow *wwin)
{
    XMapWindow(ioncore_g.dpy, wwin->win);
    REGION_MARK_MAPPED(wwin);
}


void window_unmap(WWindow *wwin)
{
    XUnmapWindow(ioncore_g.dpy, wwin->win);
    REGION_MARK_UNMAPPED(wwin);
}


void window_do_set_focus(WWindow *wwin, bool warp)
{
    if(warp)
        region_do_warp((WRegion*)wwin);
    xwindow_do_set_focus(wwin->win);
}


Window window_restack(WWindow *wwin, Window other, int mode)
{
    xwindow_restack(wwin->win, other, mode);
    return wwin->win;
}


Window window_xwindow(const WWindow *wwin)
{
    return wwin->win;
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab window_dynfuntab[]={
    {region_map, window_map},
    {region_unmap, window_unmap},
    {region_do_set_focus, window_do_set_focus},
    {(DynFun*)region_fitrep, (DynFun*)window_fitrep},
    {(DynFun*)region_restack, (DynFun*)window_restack},
    {(DynFun*)region_xwindow, (DynFun*)window_xwindow},
    END_DYNFUNTAB
};


IMPLCLASS(WWindow, WRegion, window_deinit, window_dynfuntab);

    
/*}}}*/

