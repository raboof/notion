/*
 * ion/autows/unusedwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/event.h>
#include <ioncore/gr.h>
#include <ioncore/regbind.h>
#include "unusedwin.h"
#include "main.h"


/*{{{ Init/deinit */


static void unusedwin_getbrush(WUnusedWin *uwin)
{
    GrBrush *brush=gr_get_brush(region_rootwin_of((WRegion*)uwin),
                                uwin->wwin.win, "frame-tiled-autows-unused");

    if(brush!=NULL){
        if(uwin->brush!=NULL)
            grbrush_release(uwin->brush, uwin->wwin.win);
        
        uwin->brush=brush;
        
        grbrush_enable_transparency(brush, uwin->wwin.win,
                                    GR_TRANSPARENCY_YES);
    }
}


bool unusedwin_init(WUnusedWin *uwin, WWindow *parent, const WFitParams *fp)
{
    uwin->brush=NULL;
    
    if(!window_init(&(uwin->wwin), parent, fp))
        return FALSE;
    
    unusedwin_getbrush(uwin);
    
    region_add_bindmap((WRegion*)uwin, mod_autows_unusedwin_bindmap);

    XSelectInput(ioncore_g.dpy, uwin->wwin.win, IONCORE_EVENTMASK_FRAME);
    
    return TRUE;
}


WUnusedWin *create_unusedwin(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WUnusedWin, unusedwin, (p, parent, fp));
}


void unusedwin_deinit(WUnusedWin *uwin)
{
    if(uwin->brush!=NULL){
        grbrush_release(uwin->brush, uwin->wwin.win);
        uwin->brush=NULL;
    }
    
    window_deinit(&(uwin->wwin));
}


/*}}}*/


/*{{{ Drawing */


static void unusedwin_updategr(WUnusedWin *uwin)
{
    unusedwin_getbrush(uwin);
    region_updategr_default((WRegion*)uwin);
}


static void unusedwin_draw(WUnusedWin *uwin, bool complete)
{
    WRectangle g;
    const char *substyle=(REGION_IS_ACTIVE(uwin)
                          ? "active"
                          : "inactive");
    
    if(uwin->brush==NULL)
        return;
    
    g.x=0;
    g.y=0;
    g.w=REGION_GEOM(uwin).w;
    g.h=REGION_GEOM(uwin).h;
    
    grbrush_draw_border(uwin->brush, uwin->wwin.win, &g, substyle);
}


/*}}}*/


/*{{{ The class */


static DynFunTab unusedwin_dynfuntab[]={
    {region_updategr, unusedwin_updategr},
    {window_draw, unusedwin_draw},
    END_DYNFUNTAB,
};


IMPLCLASS(WUnusedWin, WWindow, unusedwin_deinit, unusedwin_dynfuntab);


/*}}}*/

