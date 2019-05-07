/*
 * ion/ioncore/screen-notify.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/minmax.h>
#include <libmainloop/defer.h>
#include <libmainloop/signal.h>

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


static WInfoWin *do_get_popup_win(WScreen *scr, Watch *watch, uint pos,
                                  char *style)
{

    WInfoWin *iw=(WInfoWin*)(watch->obj);

    if(iw==NULL){
        WMPlexAttachParams param=MPLEXATTACHPARAMS_INIT;

        param.flags=(MPLEX_ATTACH_UNNUMBERED|
                     MPLEX_ATTACH_SIZEPOLICY|
                     MPLEX_ATTACH_GEOM|
                     MPLEX_ATTACH_LEVEL);
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

// returns the position or the stdisp, or -1 if there isn't one
static int get_stdisp_pos(WScreen *scr)
{
    WRegion* stdisp=NULL;
    WMPlexSTDispInfo info;

    mplex_get_stdisp(&scr->mplex, &stdisp, &info);

    return (stdisp!=NULL) ? info.pos : -1;
}

/*}}}*/


/*{{{ Notifywin */


static WInfoWin *get_notifywin(WScreen *scr)
{
    int stdisp_pos = get_stdisp_pos(scr);
    return do_get_popup_win(scr,
                            &scr->notifywin_watch,
                            (stdisp_pos >= 0) ? stdisp_pos : MPLEX_STDISP_TL,
                            "actnotify");
}


void screen_notify(WScreen *scr, const char *str)
{
    WInfoWin *iw=get_notifywin(scr);

    if(iw!=NULL){
        int maxw=REGION_GEOM(scr).w/3;
        infowin_set_text(iw, str, maxw);
    }
}


void screen_unnotify_notifywin(WScreen *scr)
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
        if(!ioncore_g.activity_notification_on_all_screens &&
           (region_screen_of(reg)!=scr || ws_mapped(scr, reg)))
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
    screen_unnotify_notifywin(scr);
}


static void _screen_do_update_notifywin(WScreen *scr)
{
    if(ioncore_g.screen_notify)
        screen_managed_activity(scr);
    else
        screen_unnotify_notifywin(scr);
}

static void screen_do_update_notifywin(WScreen *scr)
{
    if(ioncore_g.activity_notification_on_all_screens) {
        WScreen* scr_iterated;
        FOR_ALL_SCREENS(scr_iterated)
        {
            _screen_do_update_notifywin(scr_iterated);
        }
    }else{
        _screen_do_update_notifywin(scr);
    }
}

/*}}}*/


/*{{{ Infowin */


static WInfoWin *get_infowin(WScreen *scr)
{
    int stdisp_pos = get_stdisp_pos(scr);
    return do_get_popup_win(scr,
                            &scr->infowin_watch,
                            (stdisp_pos == MPLEX_STDISP_TR) ? MPLEX_STDISP_BR : MPLEX_STDISP_TR,
                            "tab-info");
}


void screen_unnotify_infowin(WScreen *scr)
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
        WInfoWin *iw=get_infowin(scr);

        if(iw!=NULL){
            int maxw=REGION_GEOM(scr).w/3;
            infowin_set_text(iw, n, maxw);

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
        screen_unnotify_infowin(scr);
    }
}



/*}}}*/


/*{{{ workspace indicator win */

// A single timer for ALL the screens
static WTimer*   workspace_indicator_remove_timer   = NULL;
static WScreen*  workspace_indicator_active_screen  = NULL;
static WGroupWS* workspace_indicator_last_workspace = NULL;

static WInfoWin *get_workspace_indicatorwin(WScreen *scr)
{
    return do_get_popup_win(scr,
                            &scr->workspace_indicatorwin_watch,
                            MPLEX_STDISP_BL,
                            "tab-info");
}


void screen_unnotify_workspace_indicatorwin(void)
{
    if( workspace_indicator_active_screen != NULL)
    {
        do_unnotify(&workspace_indicator_active_screen->workspace_indicatorwin_watch);
        workspace_indicator_active_screen    = NULL;
    }

    if( workspace_indicator_remove_timer )
        timer_reset( workspace_indicator_remove_timer );
}

static void timer_expired__workspace_indicator_remove(WTimer* UNUSED(dummy1), Obj* UNUSED(dummy2))
{
    if( workspace_indicator_active_screen != NULL )
        screen_unnotify_workspace_indicatorwin();
}

// Called when a region is focused. In response this function may either put up
// a new workspace-indicator or remove an existing one
void screen_update_workspace_indicatorwin(WRegion* reg_focused)
{
    if( ioncore_g.workspace_indicator_timeout <= 0 )
        return;

    WScreen* scr = region_screen_of(reg_focused);
    WRegion* mplex;
    if( scr == NULL ||
        (mplex = mplex_mx_current(&(scr->mplex))) == NULL ||
        !OBJ_IS(mplex, WGroupWS) ||
        mplex->ni.name == NULL)
    {
        return;
    }

    // remove the indicator from the other screen that it is on (if it IS on
    // another screen)
    if( workspace_indicator_active_screen != NULL &&
        workspace_indicator_active_screen != scr )
    {
        screen_unnotify_workspace_indicatorwin();
    }
    else if( (WGroupWS*)mplex == workspace_indicator_last_workspace )
        // If I'm already on this workspace, do nothing
        return;

    const char* name = mplex->ni.name;

    WInfoWin *iw=get_workspace_indicatorwin(scr);

    if(iw!=NULL){
        int maxw=REGION_GEOM(scr).w/3;
        infowin_set_text(iw, name, maxw);

        GrStyleSpec *spec=infowin_stylespec(iw);
        init_attr();
        gr_stylespec_unalloc(spec);

        gr_stylespec_set(spec, GR_ATTR(selected));
        gr_stylespec_set(spec, GR_ATTR(not_dragged));
        gr_stylespec_set(spec, GR_ATTR(active));
    }

    // set up the indicator on THIS screen
    workspace_indicator_active_screen  = scr;
    workspace_indicator_last_workspace = (WGroupWS*)mplex;

    // create and set the timer
    if( workspace_indicator_remove_timer == NULL )
        workspace_indicator_remove_timer = create_timer();
    if( workspace_indicator_remove_timer == NULL )
        return;

    timer_set(workspace_indicator_remove_timer, ioncore_g.workspace_indicator_timeout,
              (WTimerHandler*)timer_expired__workspace_indicator_remove, NULL);
}

void screen_unnotify_if_screen( WScreen* reg )
{
    if( reg == workspace_indicator_active_screen )
        screen_unnotify_workspace_indicatorwin();
}

void screen_unnotify_if_workspace( WGroupWS* reg)
{
    if( reg == workspace_indicator_last_workspace )
    {
        screen_unnotify_workspace_indicatorwin();
        workspace_indicator_last_workspace = NULL;
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


void screen_managed_notify(WScreen *scr, WRegion *UNUSED(reg), WRegionNotify how)
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

