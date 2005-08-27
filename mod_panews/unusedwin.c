/*
 * ion/panews/unusedwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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
#include <ioncore/framep.h>
#include <ioncore/presize.h>
#include <ioncore/frame-pointer.h>
#include "unusedwin.h"
#include "splitext.h"
#include "placement.h"
#include "main.h"


/*{{{ Init/deinit */


static void unusedwin_getbrush(WUnusedWin *uwin)
{
    GrBrush *brush=gr_get_brush(uwin->wwin.win,
                                region_rootwin_of((WRegion*)uwin),
                                "frame-tiled-panews-unused");

    if(brush!=NULL){
        if(uwin->brush!=NULL)
            grbrush_release(uwin->brush);
        
        uwin->brush=brush;
        
        grbrush_enable_transparency(brush, GR_TRANSPARENCY_YES);
    }
}


bool unusedwin_init(WUnusedWin *uwin, WWindow *parent, const WFitParams *fp)
{
    uwin->brush=NULL;
    
    if(!window_init(&(uwin->wwin), parent, fp))
        return FALSE;
    
    unusedwin_getbrush(uwin);
    
    region_add_bindmap((WRegion*)uwin, mod_panews_unusedwin_bindmap);

    window_select_input(&(uwin->wwin), IONCORE_EVENTMASK_NORMAL);

    ((WRegion*)uwin)->flags|=REGION_PLEASE_WARP;
    
    return TRUE;
}


WUnusedWin *create_unusedwin(WWindow *parent, const WFitParams *fp)
{
    CREATEOBJ_IMPL(WUnusedWin, unusedwin, (p, parent, fp));
}


void unusedwin_deinit(WUnusedWin *uwin)
{
    if(uwin->brush!=NULL){
        grbrush_release(uwin->brush);
        uwin->brush=NULL;
    }
    
    window_deinit(&(uwin->wwin));
}


/*}}}*/


/*{{{ unusedwin_press */


static void unusedwin_border_inner_geom(const WUnusedWin *uwin, 
                                        WRectangle *geom)
{
    GrBorderWidths bdw;
    
    geom->x=0;
    geom->y=0;
    geom->w=REGION_GEOM(uwin).w;
    geom->h=REGION_GEOM(uwin).h;
    
    if(uwin->brush!=NULL){
        grbrush_get_border_widths(uwin->brush, &bdw);

        geom->x+=bdw.left;
        geom->y+=bdw.top;
        geom->w-=bdw.left+bdw.right;
        geom->h-=bdw.top+bdw.bottom;
    }
    
    geom->w=maxof(geom->w, 0);
    geom->h=maxof(geom->h, 0);
}


static int unusedwin_press(WUnusedWin *uwin, XButtonEvent *ev, 
                           WRegion **reg_ret)
{
    WRectangle g;
    
    *reg_ret=NULL;
    
    window_p_resize_prepare(&uwin->wwin, ev);
    
    /* Check border */
    
    unusedwin_border_inner_geom(uwin, &g);
    
    if(rectangle_contains(&g, ev->x, ev->y))
        return FRAME_AREA_CLIENT;
    
    return FRAME_AREA_BORDER;
}


/*}}}*/


/*{{{ unusedwin_handle_drop */


static bool unusedwin_handle_drop(WUnusedWin *uwin, int x, int y,
                                  WRegion *dropped)
{
    WSplitUnused *us=OBJ_CAST(splittree_node_of((WRegion*)uwin),
                              WSplitUnused);
    WPaneWS *ws=REGION_MANAGER_CHK(uwin, WPaneWS);
    
    if(us==NULL || ws==NULL)
        return FALSE;
    
    return panews_handle_unused_drop(ws, us, dropped);
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
    
    grbrush_begin(uwin->brush, &g, (complete ? 0 : GRBRUSH_NO_CLEAR_OK));
    
    grbrush_draw_border(uwin->brush, &g, substyle);
    
    grbrush_end(uwin->brush);
}


/*}}}*/


/*{{{ The class */


static DynFunTab unusedwin_dynfuntab[]={
    {region_updategr, unusedwin_updategr},
    {window_draw, unusedwin_draw},
    {(DynFun*)window_press, (DynFun*)unusedwin_press},
    {(DynFun*)region_handle_drop, (DynFun*)unusedwin_handle_drop},
    END_DYNFUNTAB,
};


IMPLCLASS(WUnusedWin, WWindow, unusedwin_deinit, unusedwin_dynfuntab);


/*}}}*/

