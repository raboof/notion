/*
 * ion/ioncore/fullscreen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "global.h"
#include "sizehint.h"
#include "clientwin.h"
#include "attach.h"
#include "screen.h"
#include "manage.h"
#include "fullscreen.h"
#include "mwmhints.h"
#include "focus.h"



bool clientwin_check_fullscreen_request(WClientWin *cwin, int w, int h,
                                        bool sw)
{
    WScreen *scr;
    WMwmHints *mwm;
    WRectangle *rwgeom;
    
    mwm=xwindow_get_mwmhints(cwin->win);
    if(mwm==NULL || !(mwm->flags&MWM_HINTS_DECORATIONS) ||
       mwm->decorations!=0)
        return FALSE;
    
    FOR_ALL_SCREENS(scr){
        if(!region_same_rootwin((WRegion*)scr, (WRegion*)cwin))
            continue;
        /* TODO: if there are multiple possible rootwins, use the one with
         * requested position, if any.
         */
        if(REGION_GEOM(scr).w==w && REGION_GEOM(scr).h==h){
            return clientwin_fullscreen_scr(cwin, (WScreen*)scr, sw);
        }
    }
    
    rwgeom=&REGION_GEOM(region_rootwin_of((WRegion*)cwin));
                        
    /* Catch Xinerama-unaware apps here */
    if(rwgeom->w==w && rwgeom->h==h)
        return clientwin_enter_fullscreen(cwin, sw);

    return FALSE;
}

static void destroy_pholder(WClientWin *cwin)
{
    WPHolder *ph=cwin->fs_pholder;
    cwin->fs_pholder=NULL;
    destroy_obj((Obj*)ph);
}


bool clientwin_fullscreen_scr(WClientWin *cwin, WScreen *scr, bool switchto)
{
    int rootx, rooty;
    bool wasfs=TRUE;
    int swf=(switchto ? MPLEX_ATTACH_SWITCHTO : 0);
    WRegion *mgr=REGION_MANAGER(cwin);
    
    if(cwin->fs_pholder!=NULL)
        destroy_pholder(cwin);
        
    if(cwin->fs_pholder==NULL && mgr!=NULL)
        cwin->fs_pholder=region_managed_get_pholder(mgr, (WRegion*)cwin);
    
    if(!mplex_attach_simple((WMPlex*)scr, (WRegion*)cwin, swf)){
        warn(TR("Failed to enter full screen mode."));
        if(cwin->fs_pholder!=NULL)
            destroy_pholder(cwin);
        return FALSE;
    }

    return TRUE;
}


bool clientwin_enter_fullscreen(WClientWin *cwin, bool switchto)
{
    WScreen *scr=region_screen_of((WRegion*)cwin);
    
    if(scr==NULL){
        scr=rootwin_current_scr(region_rootwin_of((WRegion*)cwin));
        if(scr==NULL)
            return FALSE;
    }
    
    return clientwin_fullscreen_scr(cwin, scr, switchto);
}


bool clientwin_leave_fullscreen(WClientWin *cwin, bool switchto)
{    
    bool cf;
    
    if(cwin->fs_pholder==NULL)
        return FALSE;
    
    cf=region_may_control_focus((WRegion*)cwin);
    
    if(!pholder_attach(cwin->fs_pholder, (WRegion*)cwin)){
        warn(TR("WClientWin failed to return from full screen mode; "
                "remaining manager or parent from previous location "
                "refused to manage us."));
        return FALSE;
    }
    
    if(cwin->fs_pholder!=NULL){
        /* Should be destroyed already in clientwin_fitrep. */
        destroy_pholder(cwin);
    }
        
    if(cf)
        region_goto((WRegion*)cwin);
    
    return TRUE;
}


/*EXTL_DOC
 * Toggle between full screen and normal (framed) mode.
 */
EXTL_EXPORT_MEMBER
bool clientwin_toggle_fullscreen(WClientWin *cwin)
{
    if(cwin->fs_pholder!=NULL)
        return clientwin_leave_fullscreen(cwin, TRUE);
    
    return clientwin_enter_fullscreen(cwin, TRUE);
}


