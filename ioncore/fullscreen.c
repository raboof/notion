/*
 * ion/ioncore/fullscreen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
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
    WPHolder *ph=NULL;
    bool newph=FALSE, ret;

    ph=region_unset_get_return(reg);
    
    if(ph==NULL){
        ph=region_make_return_pholder(reg);
        newph=TRUE;
    }
    
    ret=(mplex_attach_simple((WMPlex*)scr, reg, swf)!=NULL);
    
    if(!ret)
        warn(TR("Failed to enter full screen mode."));
    
    if(!ret && newph)
        destroy_obj((Obj*)ph);
    else if(!region_do_set_return(reg, ph))
        destroy_obj((Obj*)ph);
    
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
    
    if(ph==NULL)
        return FALSE;
    
    if(!pholder_attach_mcfgoto(ph, swf, reg)){
        warn(TR("Failed to return from full screen mode; remaining manager "
                "or parent from previous location refused to manage us."));
        return FALSE;
    }
    
    destroy_obj((Obj*)ph);
    
    return TRUE;
}


/*#undef REGION_IS_FULLSCREEN
#define REGION_IS_FULLSCREEN(REG) (region_get_return((WRegion*)REG)!=NULL)*/


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
        /* Only Mplayer supports single Xinerama region FS to my knowledge, 
         * and doesn't set position, so we also don't check position here, 
         * and instead take the first screen with matching size.
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
    
    return FALSE;
}


/*}}}*/


/*{{{ Group exports */


/*EXTL_DOC
 * Set client window \var{reg} full screen state according to the 
 * parameter \var{how} (set/unset/toggle). Resulting state is returned,
 * which may not be what was requested.
 */
EXTL_EXPORT_AS(WGroup, set_fullscreen)
bool group_set_fullscreen_extl(WGroup *grp, const char *how)
{
    return region_set_fullscreen((WRegion*)grp, libtu_string_to_setparam(how));
}


/*EXTL_DOC
 * Is \var{reg} in full screen mode?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool group_is_fullscreen(WGroup *grp)
{
    return REGION_IS_FULLSCREEN(grp);
}


/*}}}*/

