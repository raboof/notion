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

#include <libtu/setparam.h>
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


bool clientwin_fullscreen_may_switchto(WClientWin *cwin)
{
    return (region_may_control_focus((WRegion*)cwin)
            || !REGION_IS_ACTIVE(region_screen_of((WRegion*)cwin)));
}


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
            cwin->flags|=CLIENTWIN_FS_RQ;
            if(!clientwin_fullscreen_scr(cwin, (WScreen*)scr, sw)){
                cwin->flags&=~CLIENTWIN_FS_RQ;
                return FALSE;
            }
            return TRUE;
        }
    }
    
    rwgeom=&REGION_GEOM(region_rootwin_of((WRegion*)cwin));
                        
    /* Catch Xinerama-unaware apps here */
    if(rwgeom->w==w && rwgeom->h==h){
        cwin->flags|=CLIENTWIN_FS_RQ;
        if(clientwin_enter_fullscreen(cwin, sw))
            return TRUE;
        cwin->flags&=~CLIENTWIN_FS_RQ;
    }

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
    
    /* TODO: should use switchto param. */
    
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


bool clientwin_set_fullscreen(WClientWin *cwin, int sp)
{
    bool set=CLIENTWIN_IS_FULLSCREEN(cwin);
    bool nset=libtu_do_setparam(sp, set);
    
    if(!XOR(nset, set))
        return set;

    if(nset){
        clientwin_enter_fullscreen(cwin, TRUE);
    }else{
        if(cwin->fs_pholder==NULL){
            WPHolder *ph=region_get_rescue_pholder((WRegion*)cwin);
            
            if(ph!=NULL && pholder_target(ph)==REGION_MANAGER(cwin))
                destroy_obj((Obj*)ph);
            else
                cwin->fs_pholder=ph;
        }
        clientwin_leave_fullscreen(cwin, TRUE);
    }
    
    return CLIENTWIN_IS_FULLSCREEN(cwin);
}


/*EXTL_DOC
 * Set client window \var{cwin} full screen state according to the 
 * parameter \var{how} (set/unset/toggle). Resulting state is returned,
 * which may not be what was requested.
 */
EXTL_EXPORT_AS(WClientWin, set_fullscreen)
bool clientwin_set_fullscreen_extl(WClientWin *cwin, const char *how)
{
    return clientwin_set_fullscreen(cwin, libtu_string_to_setparam(how));
}


/*EXTL_DOC
 * Is \var{cwin} in full screen mode?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool clientwin_is_fullscreen(WClientWin *cwin)
{
    return CLIENTWIN_IS_FULLSCREEN(cwin);
}

