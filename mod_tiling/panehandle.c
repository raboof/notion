/*
 * ion/mod_tiling/panehandle.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
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
    GrBrush *brush=gr_get_brush(pwin->wwin.win, 
                                region_rootwin_of((WRegion*)pwin), 
                                "pane");

    if(brush!=NULL){
        if(pwin->brush!=NULL)
            grbrush_release(pwin->brush);
        
        pwin->brush=brush;
        
        grbrush_get_border_widths(brush, &(pwin->bdw));
        grbrush_enable_transparency(brush, GR_TRANSPARENCY_YES);
    }
}


bool panehandle_init(WPaneHandle *pwin, WWindow *parent, const WFitParams *fp)
{
    pwin->brush=NULL;
    pwin->bline=GR_BORDERLINE_NONE;
    pwin->splitfloat=NULL;
    
    if(!window_init(&(pwin->wwin), parent, fp, "WPanelHandle"))
        return FALSE;
    
    ((WRegion*)pwin)->flags|=REGION_SKIP_FOCUS;
    
    panehandle_getbrush(pwin);
    
    if(pwin->brush==NULL){
        GrBorderWidths bdw=GR_BORDER_WIDTHS_INIT;
        memcpy(&(pwin->bdw), &bdw, sizeof(bdw));
    }

    window_select_input(&(pwin->wwin), IONCORE_EVENTMASK_NORMAL);
    
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
        grbrush_release(pwin->brush);
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
    
    grbrush_begin(pwin->brush, &g, (complete ? 0 : GRBRUSH_NO_CLEAR_OK));
    
    grbrush_draw_borderline(pwin->brush, &g, pwin->bline);
    
    grbrush_end(pwin->brush);
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

