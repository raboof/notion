/*
 * ion/ioncore/fullscreen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
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
#include "group-cw.h"
#include "return.h"



/*{{{ Generic full screen mode code */


bool region_fullscreen_scr(WRegion *reg, WScreen *scr, bool switchto)
{
    int rootx, rooty;
    bool wasfs=TRUE;
    int swf=(switchto ? MPLEX_ATTACH_SWITCHTO : 0);
    WPHolder *ph;
    
    ph=region_make_return_pholder(reg);
    
    if(!mplex_attach_simple((WMPlex*)scr, reg, swf)){
        warn(TR("Failed to enter full screen mode."));
        if(ph!=NULL)
            destroy_obj((Obj*)ph);
        return FALSE;
    }
    
    if(ph!=NULL){
        if(!region_do_set_return(reg, ph))
            destroy_obj((Obj*)ph);
    }
    
    return TRUE;
}


bool region_enter_fullscreen(WRegion *reg, bool switchto)
{
    WScreen *scr=region_screen_of(reg);
    
    if(scr==NULL){
        scr=rootwin_current_scr(region_rootwin_of(reg));
        if(scr==NULL)
            return FALSE;
    }
    
    return region_fullscreen_scr(reg, scr, switchto);
}


bool region_leave_fullscreen(WRegion *reg, bool switchto)
{    
    int swf=(switchto ? PHOLDER_ATTACH_SWITCHTO : 0);
    WPHolder *ph=region_unset_get_return(reg);
    bool cf;
    
    if(ph==NULL)
        return FALSE;
    
    cf=region_may_control_focus(reg);
    
    if(!pholder_attach(ph, swf, reg)){
        warn(TR("Failed to return from full screen mode; remaining manager "
                "or parent from previous location refused to manage us."));
        return FALSE;
    }
    
    destroy_obj((Obj*)ph);
        
    if(cf)
        region_goto(reg);
    
    return TRUE;
}


static bool region_set_fullscreen(WRegion *reg, int sp)
{
    bool set=REGION_IS_FULLSCREEN(reg);
    bool nset=libtu_do_setparam(sp, set);
    
    if(!XOR(nset, set))
        return set;

    if(nset)
        region_enter_fullscreen(reg, TRUE);
    else
        region_leave_fullscreen(reg, TRUE);
    
    return REGION_IS_FULLSCREEN(reg);
}


/*}}}*/


/*{{{ Client window requests */


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
            if(!region_fullscreen_scr((WRegion*)cwin, (WScreen*)scr, sw)){
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
        if(region_enter_fullscreen((WRegion*)cwin, sw))
            return TRUE;
        cwin->flags&=~CLIENTWIN_FS_RQ;
    }

    return FALSE;
}


/*}}}*/



/*{{{ Client window exports */


static WRegion *get_group(WClientWin *cwin)
{
    WGroupCW *cwg=OBJ_CAST(REGION_MANAGER(cwin), WGroupCW);
    
    return ((cwg!=NULL && group_bottom(&cwg->grp)==(WRegion*)cwin)
            ? (WRegion*)cwg
            : (WRegion*)cwin);
}


bool clientwin_set_fullscreen(WClientWin *cwin, int sp)
{
    return region_set_fullscreen(get_group(cwin), sp);
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
    return REGION_IS_FULLSCREEN(cwin);
}


/*}}}*/

