/*
 * ion/ioncore/screen-notify.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libmainloop/defer.h>

#include "common.h"
#include "global.h"
#include "infowin.h"
#include "activity.h"
#include "tags.h"
#include "gr.h"
#include "gr-util.h"
#include "stacking.h"
#include "names.h"
#include "screen.h"
#include "screen-notify.h"


static void do_notify(WScreen *scr, Watch *watch, bool right,
                      const char *str, char *style)
{

    WInfoWin *iw=(WInfoWin*)(watch->obj);
    WFitParams fp;
    
    if(iw==NULL){
        WMPlexAttachParams param=MPLEXATTACHPARAMS_INIT;
        
        param.flags=(MPLEX_ATTACH_UNNUMBERED|
                     MPLEX_ATTACH_SIZEPOLICY|
                     MPLEX_ATTACH_GEOM|
                     MPLEX_ATTACH_LEVEL);
        param.level=STACKING_LEVEL_ON_TOP;
        
        if(!right){
            param.szplcy=SIZEPOLICY_GRAVITY_NORTHWEST;
            param.geom.x=0;
        }else{
            param.szplcy=SIZEPOLICY_GRAVITY_NORTHEAST;
            param.geom.x=REGION_GEOM(scr).w-1;
        }
        
        param.geom.y=0;
        param.geom.w=1;
        param.geom.h=1;
        
        iw=(WInfoWin*)mplex_do_attach_new(&scr->mplex, &param,
                                          (WRegionCreateFn*)create_infowin, 
                                          style);
        
        if(iw==NULL)
            return;

        watch_setup(watch, (Obj*)iw, NULL);
    }

    infowin_set_text(iw, str);
}


void screen_notify(WScreen *scr, const char *str)
{
    do_notify(scr, &scr->notifywin_watch, FALSE, str, "actnotify");
}


void screen_windowinfo(WScreen *scr, const char *str)
{
    do_notify(scr, &scr->infowin_watch, TRUE, str, "tab-info");
}


void screen_unnotify(WScreen *scr)
{
    Obj *iw=scr->notifywin_watch.obj;
    if(iw!=NULL){
        watch_reset(&(scr->notifywin_watch));
        mainloop_defer_destroy((Obj*)iw);
    }
}


void screen_nowindowinfo(WScreen *scr)
{
    Obj *iw=scr->infowin_watch.obj;
    if(iw!=NULL){
        watch_reset(&(scr->infowin_watch));
        mainloop_defer_destroy((Obj*)iw);
    }
}


static char *addnot(char *str, WRegion *reg)
{
    const char *nm=region_name(reg);
    char *nstr=NULL;
    
    if(nm==NULL)
        return str;
    
    if(str==NULL)
        return scat(TR("act: "), nm);

    nstr=scat3(str, ", ", nm);
    if(nstr!=NULL)
        free(str);
    return nstr;
}


static char *screen_managed_activity(WScreen *scr)
{
    char *notstr=NULL;
    WMPlexIterTmp tmp;
    WRegion *reg;
    
    FOR_ALL_MANAGED_BY_MPLEX(&scr->mplex, reg, tmp){
        if(region_is_activity_r(reg) && !REGION_IS_MAPPED(reg))
            notstr=addnot(notstr, reg);
    }
    
    return notstr;
}


static void screen_notify_activity(WScreen *scr)
{
    if(ioncore_g.screen_notify){
        char *notstr=screen_managed_activity(scr);
        if(notstr!=NULL){
            screen_notify(scr, notstr);
            free(notstr);
            return;
        }
    }

    screen_unnotify(scr);
    
    screen_update_infowin(scr);
}


static void screen_notify_tag(WScreen *scr)
{
    screen_update_infowin(scr);
}


GR_DEFATTR(active);
GR_DEFATTR(inactive);
GR_DEFATTR(selected);
GR_DEFATTR(tagged);
GR_DEFATTR(not_tagged);
GR_DEFATTR(not_dragged);
GR_DEFATTR(activity);
GR_DEFATTR(no_activity);


static void init_attr()
{
    GR_ALLOCATTR_BEGIN;
    GR_ALLOCATTR(active);
    GR_ALLOCATTR(inactive);
    GR_ALLOCATTR(selected);
    GR_ALLOCATTR(tagged);
    GR_ALLOCATTR(not_tagged);
    GR_ALLOCATTR(not_dragged);
    GR_ALLOCATTR(no_activity);
    GR_ALLOCATTR(activity);
    GR_ALLOCATTR_END;
}


void screen_update_infowin(WScreen *scr)
{
    WRegion *reg=mplex_mx_current(&(scr->mplex));
    bool tag=(reg!=NULL && region_is_tagged(reg));
    bool act=(reg!=NULL && region_is_activity_r(reg) && !REGION_IS_ACTIVE(scr));
    bool sac=REGION_IS_ACTIVE(scr);
    
    if(tag || act){
        const char *n=region_displayname(reg);
        WInfoWin *iw;
                
        screen_windowinfo(scr, n);
        
        iw=(WInfoWin*)scr->infowin_watch.obj;
        
        if(iw!=NULL){
            GrStyleSpec *spec=infowin_stylespec(iw);
            
            init_attr();
            
            gr_stylespec_unalloc(spec);
            
            gr_stylespec_set(spec, GR_ATTR(selected));
            gr_stylespec_set(spec, GR_ATTR(not_dragged));
            gr_stylespec_set(spec, sac ? GR_ATTR(active) : GR_ATTR(inactive));
            gr_stylespec_set(spec, tag ? GR_ATTR(tagged) : GR_ATTR(not_tagged));
            gr_stylespec_set(spec, act ? GR_ATTR(activity) : GR_ATTR(no_activity));
        }
            
    }else{
        screen_nowindowinfo(scr);
    }
}


void screen_managed_notify(WScreen *scr, WRegion *reg, WRegionNotify how)
{
    if(how==ioncore_g.notifies.sub_activity){
        /* TODO: multiple calls */
        mainloop_defer_action((Obj*)scr, 
                              (WDeferredAction*)screen_notify_activity);
    }else if(how==ioncore_g.notifies.tag){
        mainloop_defer_action((Obj*)scr, 
                              (WDeferredAction*)screen_notify_tag);
    }
}


