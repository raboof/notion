/*
 * ion/ioncore/fullscreen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include "region-iter.h"



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


static void lastmgr_watchhandler(Watch *watch, Obj *obj)
{
    WClientWinFSInfo *fsinfo=(WClientWinFSInfo*)watch;
    WRegion *r;
    
    assert(OBJ_IS(obj, WRegion));
    
    /* TODO: It'd be nicer if we didn't access obj. */
    r=region_manager_or_parent((WRegion*)obj);
    
    if(r!=NULL)
        watch_setup(&(fsinfo->last_mgr_watch), (Obj*)r, lastmgr_watchhandler);
}


bool clientwin_fullscreen_scr(WClientWin *cwin, WScreen *scr, bool switchto)
{
    int rootx, rooty;
    bool wasfs=TRUE;
    int swf=(switchto ? MPLEX_ATTACH_SWITCHTO : 0);
    
    if(cwin->fsinfo.last_mgr_watch.obj==NULL){
        wasfs=FALSE;
        
        if(REGION_MANAGER(cwin)!=NULL){
            watch_setup(&(cwin->fsinfo.last_mgr_watch),
                        (Obj*)REGION_MANAGER(cwin),
                        lastmgr_watchhandler);
        }else if(scr->mplex.l1_current!=NULL){
            watch_setup(&(cwin->fsinfo.last_mgr_watch),
                        (Obj*)scr->mplex.l1_current, 
                        lastmgr_watchhandler);
        }
        region_rootpos((WRegion*)cwin, &rootx, &rooty);
        cwin->fsinfo.saved_rootrel_geom.x=rootx;
        cwin->fsinfo.saved_rootrel_geom.y=rooty;
        cwin->fsinfo.saved_rootrel_geom.w=REGION_GEOM(cwin).w;
        cwin->fsinfo.saved_rootrel_geom.h=REGION_GEOM(cwin).h;
    }
    
    if(!mplex_attach_simple((WMPlex*)scr, (WRegion*)cwin, swf)){
        warn(TR("Failed to enter full screen mode."));
        if(!wasfs)
            watch_reset(&(cwin->fsinfo.last_mgr_watch));
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
    WRegion *reg;
    WManageParams param=MANAGEPARAMS_INIT;
    int rootx, rooty;
    bool cf;
    
    if(cwin->fsinfo.last_mgr_watch.obj==NULL)
        return FALSE;
    
    cf=region_may_control_focus((WRegion*)cwin);

    reg=(WRegion*)cwin->fsinfo.last_mgr_watch.obj;
    watch_reset(&(cwin->fsinfo.last_mgr_watch));
    assert(OBJ_IS(reg, WRegion));
    
    param.geom=cwin->fsinfo.saved_rootrel_geom;
    param.tfor=NULL;
    param.userpos=FALSE;
    param.maprq=FALSE;
    param.switchto=switchto;
    param.dockapp=FALSE;
    param.gravity=StaticGravity;
    
    while(reg!=NULL){
        if(region_manage_clientwin(reg, cwin, &param, 
                                   MANAGE_REDIR_PREFER_NO)){
            if(!cf)
                return TRUE;
            return region_goto((WRegion*)cwin);
        }
        reg=region_manager_or_parent(reg);
    }

    warn(TR("WClientWin failed to return from full screen mode; remaining "
            "manager or parent from previous location refused to manage us."));
    return FALSE;
}


/*EXTL_DOC
 * Toggle between full screen and normal (framed) mode.
 */
EXTL_EXPORT_MEMBER
bool clientwin_toggle_fullscreen(WClientWin *cwin)
{
    if(cwin->fsinfo.last_mgr_watch.obj!=NULL)
        return clientwin_leave_fullscreen(cwin, TRUE);
    
    return clientwin_enter_fullscreen(cwin, TRUE);
}


