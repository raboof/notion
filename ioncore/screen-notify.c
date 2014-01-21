/*
 * ion/ioncore/screen-notify.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/minmax.h>
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
#include "strings.h"


/*{{{ Generic stuff */


static WInfoWin *do_get_notifywin(WScreen *scr, Watch *watch, uint pos,
                                  char *style)
{

    WInfoWin *iw=(WInfoWin*)(watch->obj);
    WFitParams fp;
    
    if(iw==NULL){
        WMPlexAttachParams param=MPLEXATTACHPARAMS_INIT;
        
        param.flags=(MPLEX_ATTACH_UNNUMBERED|
                     MPLEX_ATTACH_SIZEPOLICY|
                     MPLEX_ATTACH_GEOM|
                     MPLEX_ATTACH_LEVEL|
                     MPLEX_ATTACH_PASSIVE);
        param.level=STACKING_LEVEL_ON_TOP;
        
        param.geom.x=0;
        param.geom.y=0;
        param.geom.w=1;
        param.geom.h=1;
        
        switch(pos){
        case MPLEX_STDISP_TL:
            param.szplcy=SIZEPOLICY_GRAVITY_NORTHWEST;
            param.geom.x=0;
            break;
            
        case MPLEX_STDISP_TR:
            param.szplcy=SIZEPOLICY_GRAVITY_NORTHEAST;
            param.geom.x=REGION_GEOM(scr).w-1;
            break;
            
        case MPLEX_STDISP_BL:
            param.szplcy=SIZEPOLICY_GRAVITY_SOUTHWEST;
            param.geom.x=0;
            param.geom.y=REGION_GEOM(scr).h-1;
            break;
            
        case MPLEX_STDISP_BR:
            param.szplcy=SIZEPOLICY_GRAVITY_SOUTHEAST;
            param.geom.x=REGION_GEOM(scr).w-1;
            param.geom.y=REGION_GEOM(scr).h-1;
            break;
        }
        

        iw=(WInfoWin*)mplex_do_attach_new(&scr->mplex, &param,
                                          (WRegionCreateFn*)create_infowin, 
                                          style);
        
        if(iw!=NULL)
            watch_setup(watch, (Obj*)iw, NULL);
    }
    
    return iw;
}


static void do_unnotify(Watch *watch)
{    
    Obj *iw=watch->obj;
    if(iw!=NULL){
        watch_reset(watch);
        mainloop_defer_destroy((Obj*)iw);
    }
}


/*}}}*/


/*{{{ Notifywin */


static WInfoWin *get_notifywin(WScreen *scr)
{
    WRegion *stdisp=NULL;
    WMPlexSTDispInfo info;
    uint pos=MPLEX_STDISP_TL;
    
    mplex_get_stdisp(&scr->mplex, &stdisp, &info);
    if(stdisp!=NULL)
        pos=info.pos;
    
    return do_get_notifywin(scr, &scr->notifywin_watch, pos, "actnotify");
}


void screen_notify(WScreen *scr, const char *str)
{
    WInfoWin *iw=get_notifywin(scr);
    
    if(iw!=NULL){
        int maxw=REGION_GEOM(scr).w/3;
        infowin_set_text(iw, str, maxw);
    }
}


void screen_unnotify(WScreen *scr)
{
    do_unnotify(&scr->notifywin_watch);
}


static bool ws_mapped(WScreen *scr, WRegion *reg)
{
    while(1){
        WRegion *mgr=REGION_MANAGER(reg);
        
        if(mgr==NULL)
            return FALSE;
        
        if(mgr==(WRegion*)scr)
            return REGION_IS_MAPPED(reg);
        
        reg=mgr;
    }
}


static void screen_managed_activity(WScreen *scr)
{
    char *notstr=NULL;
    WRegion *reg;
    ObjListIterTmp tmp;
    PtrListIterTmp tmp2;
    ObjList *actlist=ioncore_activity_list();
    WInfoWin *iw=NULL;
    PtrList *found=NULL;
    int nfound=0, nadded=0;
    int w=0, maxw=REGION_GEOM(scr).w/4;
    
    /* Lisäksi minimipituus (10ex tms.), ja sen yli menevät jätetään
     * pois (+ n) 
     */
    FOR_ALL_ON_OBJLIST(WRegion*, reg, actlist, tmp){
        if(region_screen_of(reg)!=scr || ws_mapped(scr, reg))
            continue;
        if(region_name(reg)==NULL)
            continue;
        if(ptrlist_insert_last(&found, reg))
            nfound++;
    }
    
    if(found==NULL)
        goto unnotify;
    
    iw=get_notifywin(scr);
    
    if(iw==NULL)
        return;
        
    if(iw->brush==NULL)
        goto unnotify;
    
    notstr=scopy(TR("act: "));
    
    if(notstr==NULL)
        goto unnotify;
    
    FOR_ALL_ON_PTRLIST(WRegion*, reg, found, tmp2){
        const char *nm=region_name(reg);
        char *nstr=NULL, *label=NULL;
        
        w=grbrush_get_text_width(iw->brush, notstr, strlen(notstr));

        if(w>=maxw)
            break;
        
        label=grbrush_make_label(iw->brush, nm, maxw-w);
        if(label!=NULL){
            nstr=(nadded>0
                  ? scat3(notstr, ", ", label)
                  : scat(notstr, label));
            
            if(nstr!=NULL){
                free(notstr);
                notstr=nstr;
                nadded++;
            }
            free(label);
        }
    }
    
    if(nfound > nadded){
        char *nstr=NULL;
        
        libtu_asprintf(&nstr, "%s  +%d", notstr, nfound-nadded);
        
        if(nstr!=NULL){
            free(notstr);
            notstr=nstr;
        }
    }
    
    ptrlist_clear(&found);
    
    infowin_set_text(iw, notstr, 0);
    
    free(notstr);
    
    return;

unnotify:    
    screen_unnotify(scr);
}


static void screen_do_update_notifywin(WScreen *scr)
{
    if(ioncore_g.screen_notify)
        screen_managed_activity(scr);
    else
        screen_unnotify(scr);
}


/*}}}*/


/*{{{ Infowin */


static WInfoWin *get_infowin(WScreen *scr)
{
    WRegion *stdisp=NULL;
    WMPlexSTDispInfo info;
    uint pos=MPLEX_STDISP_TR;
    
    mplex_get_stdisp(&scr->mplex, &stdisp, &info);
    if(stdisp!=NULL && info.pos==MPLEX_STDISP_TR)
        pos=MPLEX_STDISP_BR;
    
    return do_get_notifywin(scr, &scr->infowin_watch, pos, "tab-info");
}


void screen_windowinfo(WScreen *scr, const char *str)
{
    WInfoWin *iw=get_infowin(scr);
    
    if(iw!=NULL){
        int maxw=REGION_GEOM(scr).w/3;
        infowin_set_text(iw, str, maxw);
    }
}


void screen_nowindowinfo(WScreen *scr)
{
    do_unnotify(&scr->infowin_watch);
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


static void screen_do_update_infowin(WScreen *scr)
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



/*}}}*/


/*{{{ Notification callbacks and deferred updates*/


void screen_update_infowin(WScreen *scr)
{
    mainloop_defer_action((Obj*)scr, 
                          (WDeferredAction*)screen_do_update_infowin);
}


void screen_update_notifywin(WScreen *scr)
{
    mainloop_defer_action((Obj*)scr, 
                          (WDeferredAction*)screen_do_update_notifywin);
}


void screen_managed_notify(WScreen *scr, WRegion *reg, WRegionNotify how)
{
    if(how==ioncore_g.notifies.tag)
        screen_update_infowin(scr);
}


void ioncore_screen_activity_notify(WRegion *reg, WRegionNotify how)
{
    WScreen *scr=region_screen_of(reg);
    if(scr!=NULL){
        if(how==ioncore_g.notifies.activity){
            screen_update_notifywin(region_screen_of(reg));
        }else if(how==ioncore_g.notifies.name){
            if(region_is_activity(reg))
                screen_update_notifywin(scr);
            if((WRegion*)scr==REGION_MANAGER(reg))
                screen_do_update_infowin(scr);
        }
    }
}


/*}}}*/

