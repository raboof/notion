/*
 * ion/panews/panehandle.c
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
#include "panehandle.h"
#include "main.h"


/*{{{ Init/deinit */


static void panehandle_getbrush(WPaneHandle *pwin)
{
    GrBrush *brush=gr_get_brush(region_rootwin_of((WRegion*)pwin),
                                pwin->wwin.win, "pane");

    if(brush!=NULL){
        if(pwin->brush!=NULL)
            grbrush_release(pwin->brush, pwin->wwin.win);
        
        pwin->brush=brush;
        
        grbrush_get_border_widths(brush, &(pwin->bdw));
        grbrush_enable_transparency(brush, pwin->wwin.win,
                                    GR_TRANSPARENCY_YES);
    }
}


bool panehandle_init(WPaneHandle *pwin, WWindow *parent, const WFitParams *fp)
{
    pwin->brush=NULL;
    pwin->bline=GR_BORDERLINE_NONE;
    pwin->splitfloat=NULL;
    
    if(!window_init(&(pwin->wwin), parent, fp))
        return FALSE;
    
    panehandle_getbrush(pwin);
    
    if(pwin->brush==NULL){
        GrBorderWidths bdw=GR_BORDER_WIDTHS_INIT;
        memcpy(&(pwin->bdw), &bdw, sizeof(bdw));
    }

    XSelectInput(ioncore_g.dpy, pwin->wwin.win, IONCORE_EVENTMASK_FRAME);
    
    return TRUE;
}


WPaneHandle *create_panehandle(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WPaneHandle, panehandle, (p, parent, fp));
}


void panehandle_deinit(WPaneHandle *pwin)
{
    assert(pwin->splitfloat==NULL);
    
    if(pwin->brush!=NULL){
        grbrush_release(pwin->brush, pwin->wwin.win);
        pwin->brush=NULL;
    }
    
    window_deinit(&(pwin->wwin));
}


/*}}}*/


/*{{{ Drawing */


static void panehandle_updategr(WPaneHandle *pwin)
{
    panehandle_getbrush(pwin);
    region_updategr_default((WRegion*)pwin);
}


static void panehandle_draw(WPaneHandle *pwin, bool complete)
{
    WRectangle g;
    
    if(pwin->brush==NULL)
        return;
    
    g.x=0;
    g.y=0;
    g.w=REGION_GEOM(pwin).w;
    g.h=REGION_GEOM(pwin).h;
    
    grbrush_draw_borderline(pwin->brush, pwin->wwin.win, &g, NULL, 
                            pwin->bline);
}


/*}}}*/


/*{{{ The class */


static DynFunTab panehandle_dynfuntab[]={
    {region_updategr, panehandle_updategr},
    {window_draw, panehandle_draw},
    END_DYNFUNTAB,
};


IMPLCLASS(WPaneHandle, WWindow, panehandle_deinit, panehandle_dynfuntab);


/*}}}*/

