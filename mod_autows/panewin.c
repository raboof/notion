/*
 * ion/autows/panewin.c
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
#include "panewin.h"


/*{{{ Init/deinit */


static void panewin_getbrush(WPaneWin *pwin)
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


bool panewin_init(WPaneWin *pwin, WWindow *parent, const WFitParams *fp)
{
    pwin->brush=NULL;
    pwin->bline=GR_BORDERLINE_NONE;
    
    if(!window_init(&(pwin->wwin), parent, fp))
        return FALSE;
    
    panewin_getbrush(pwin);
    
    if(pwin->brush==NULL){
        GrBorderWidths bdw=GR_BORDER_WIDTHS_INIT;
        memcpy(&(pwin->bdw), &bdw, sizeof(bdw));
    }

    XSelectInput(ioncore_g.dpy, pwin->wwin.win, IONCORE_EVENTMASK_FRAME);
    
    return TRUE;
}


WPaneWin *create_panewin(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WPaneWin, panewin, (p, parent, fp));
}


void panewin_deinit(WPaneWin *pwin)
{
    if(pwin->brush!=NULL){
        grbrush_release(pwin->brush, pwin->wwin.win);
        pwin->brush=NULL;
    }
    
    window_deinit(&(pwin->wwin));
}


/*}}}*/


/*{{{ Drawing */


static void panewin_updategr(WPaneWin *pwin)
{
    panewin_getbrush(pwin);
    region_updategr_default((WRegion*)pwin);
}


static void panewin_draw(WPaneWin *pwin, bool complete)
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


static DynFunTab panewin_dynfuntab[]={
    {region_updategr, panewin_updategr},
    {window_draw, panewin_draw},
    END_DYNFUNTAB,
};


IMPLCLASS(WPaneWin, WWindow, panewin_deinit, panewin_dynfuntab);


/*}}}*/

