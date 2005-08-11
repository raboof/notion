/*
 * ion/ioncore/screen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
#include <libtu/minmax.h>
#include <libmainloop/defer.h>
#include "common.h"
#include "global.h"
#include "screen.h"
#include "region.h"
#include "attach.h"
#include "manage.h"
#include "focus.h"
#include "property.h"
#include "names.h"
#include "reginfo.h"
#include "saveload.h"
#include "resize.h"
#include "genws.h"
#include "event.h"
#include "bindmaps.h"
#include "regbind.h"
#include "frame-pointer.h"
#include "rectangle.h"
#include "infowin.h"
#include "activity.h"
#include "extlconv.h"
#include "llist.h"


WHook *screen_managed_changed_hook=NULL;


/*{{{ Init/deinit */


static bool screen_init(WScreen *scr, WRootWin *rootwin,
                        int id, const WFitParams *fp, bool useroot)
{
    Window win;
    XSetWindowAttributes attr;
    ulong attrflags=0;
    
    scr->id=id;
    scr->atom_workspace=None;
    scr->uses_root=useroot;
    scr->managed_off.x=0;
    scr->managed_off.y=0;
    scr->managed_off.w=0;
    scr->managed_off.h=0;
    scr->next_scr=NULL;
    scr->prev_scr=NULL;
    scr->pivoted=FALSE;
    
    watch_init(&(scr->notifywin_watch));

    if(useroot){
        win=WROOTWIN_ROOT(rootwin);
    }else{
        attr.background_pixmap=ParentRelative;
        attrflags=CWBackPixmap;
        
        win=XCreateWindow(ioncore_g.dpy, WROOTWIN_ROOT(rootwin),
                          fp->g.x, fp->g.y, fp->g.w, fp->g.h, 0, 
                          DefaultDepth(ioncore_g.dpy, rootwin->xscr),
                          InputOutput,
                          DefaultVisual(ioncore_g.dpy, rootwin->xscr),
                          attrflags, &attr);
        if(win==None)
            return FALSE;
    }

    if(!mplex_do_init((WMPlex*)scr, (WWindow*)rootwin, win, fp, FALSE))
        return FALSE;

    /*scr->mplex.win.region.rootwin=rootwin;
    region_set_parent((WRegion*)scr, (WRegion*)rootwin);*/
    scr->mplex.flags|=MPLEX_ADD_TO_END;
    scr->mplex.win.region.flags|=REGION_BINDINGS_ARE_GRABBED;
    if(useroot)
        scr->mplex.win.region.flags|=REGION_MAPPED;
    
    window_select_input(&(scr->mplex.win),
                        FocusChangeMask|EnterWindowMask|
                        KeyPressMask|KeyReleaseMask|
                        ButtonPressMask|ButtonReleaseMask|
                        (useroot ? IONCORE_EVENTMASK_ROOT : 0));

    if(id==0){
        scr->atom_workspace=XInternAtom(ioncore_g.dpy, 
                                        "_ION_WORKSPACE", False);
    }else if(id>=0){
        char *str;
        libtu_asprintf(&str, "_ION_WORKSPACE%d", id);
        if(str!=NULL){
            scr->atom_workspace=XInternAtom(ioncore_g.dpy, str, False);
            free(str);
        }
    }

    /* Add rootwin's bindings to screens (ungrabbed) so that bindings
     * are called with the proper region.
     */
    region_add_bindmap((WRegion*)scr, ioncore_rootwin_bindmap);

    LINK_ITEM(ioncore_g.screens, scr, next_scr, prev_scr);
    
    if(ioncore_g.active_screen==NULL)
        ioncore_g.active_screen=scr;

    return TRUE;
}


WScreen *create_screen(WRootWin *rootwin, int id, const WFitParams *fp,
                       bool useroot)
{
    CREATEOBJ_IMPL(WScreen, screen, (p, rootwin, id, fp, useroot));
}


void screen_deinit(WScreen *scr)
{
    UNLINK_ITEM(ioncore_g.screens, scr, next_scr, prev_scr);
    
    if(scr->uses_root)
        scr->mplex.win.win=None;
    
    mplex_deinit((WMPlex*)scr);
}


/*}}}*/


/*{{{ Attach/detach */


void screen_managed_geom(WScreen *scr, WRectangle *geom)
{
    geom->x=scr->managed_off.x;
    geom->y=scr->managed_off.y;
    geom->w=REGION_GEOM(scr).w+scr->managed_off.w;
    geom->h=REGION_GEOM(scr).h+scr->managed_off.h;
    geom->w=maxof(geom->w, 0);
    geom->h=maxof(geom->h, 0);
}


static bool screen_handle_drop(WScreen *scr, int x, int y, WRegion *dropped)
{
    WRegion *curr=mplex_lcurrent(&(scr->mplex), 1);

    /* This code should handle dropping tabs on floating workspaces. */
    if(curr && HAS_DYN(curr, region_handle_drop)){
        int rx, ry;
        region_rootpos(curr, &rx, &ry);
        if(rectangle_contains(&REGION_GEOM(curr), x-rx, y-ry)){
            if(region_handle_drop(curr, x, y, dropped))
                return TRUE;
        }
    }
    
    /* Do not attach to ourselves unlike generic WMPlex. */
    return FALSE;
}


/*}}}*/


/*{{{ Region dynfun implementations */


static bool screen_fitrep(WScreen *scr, WWindow *par, const WFitParams *fp)
{
    WRegion *sub;
    
    if(par==NULL)
        return FALSE;
    
    if(scr->uses_root)
        return FALSE;

    return mplex_fitrep((WMPlex*)scr, NULL, fp);
}


static void screen_managed_changed(WScreen *scr, int mode, bool sw, 
                                   WRegion *reg_)
{
    const char *n=NULL;
    WRegion *reg;

    if(ioncore_g.opmode==IONCORE_OPMODE_DEINIT)
        return;
    
    reg=mplex_lcurrent(&(scr->mplex), 1);

    if(sw && scr->atom_workspace!=None){
        if(reg!=NULL)
            n=region_name(reg);
        
        xwindow_set_string_property(region_root_of((WRegion*)scr),
                                    scr->atom_workspace, 
                                    n==NULL ? "" : n);
    }

    mplex_call_changed_hook((WMPlex*)scr,
                            screen_managed_changed_hook,
                            mode, sw, reg_);
}


static void screen_map(WScreen *scr)
{
    if(scr->uses_root)
        return;
    mplex_map((WMPlex*)scr);
}


static void screen_unmap(WScreen *scr)
{
    if(scr->uses_root)
        return;
    mplex_unmap((WMPlex*)scr);
}


static void screen_activated(WScreen *scr)
{
    ioncore_g.active_screen=scr;
}


/*}}}*/


/*{{{ Notifications */


void screen_notify(WScreen *scr, const char *str)
{
    WInfoWin *iw=(WInfoWin*)(scr->notifywin_watch.obj);
    WFitParams fp;
    
    if(iw!=NULL){
        infowin_set_text(iw, str);
        return;
    }

    fp.mode=REGION_FIT_EXACT;
    fp.g.x=0;
    fp.g.y=0;
    fp.g.w=1;
    fp.g.h=1;
    
    iw=create_infowin((WWindow*)scr, &fp, "actnotify");
    
    if(iw==NULL)
        return;
    
    watch_setup(&(scr->notifywin_watch), (Obj*)iw, NULL);

    infowin_set_text(iw, str);
    region_raise((WRegion*)iw);
    region_map((WRegion*)iw);
}


void screen_unnotify(WScreen *scr)
{
    Obj *iw=scr->notifywin_watch.obj;
    if(iw!=NULL){
        mainloop_defer_destroy(iw);
        watch_reset(&(scr->notifywin_watch));
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
    WLListIterTmp tmp;
    char *notstr=NULL;
    WRegion *reg;
    
    FOR_ALL_REGIONS_ON_LLIST(reg, scr->mplex.l1_list, tmp){
        if(region_is_activity_r(reg) && !REGION_IS_MAPPED(reg))
            notstr=addnot(notstr, reg);
    }

    FOR_ALL_REGIONS_ON_LLIST(reg, scr->mplex.l2_list, tmp){
        if(region_is_activity_r(reg) && !REGION_IS_MAPPED(reg))
            notstr=addnot(notstr, reg);
    }
    
    return notstr;
}


static void screen_managed_notify(WScreen *scr, WRegion *sub)
{
    char *notstr;
    
    if(ioncore_g.opmode!=IONCORE_OPMODE_NORMAL)
        return;
    
    if(ioncore_g.screen_notify){
        notstr=screen_managed_activity(scr);
        if(notstr!=NULL){
            screen_notify(scr, notstr);
            free(notstr);
            return;
        }
    }

    screen_unnotify(scr);
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Find the screen with numerical id \var{id}. If Xinerama is
 * not present, \var{id} corresponds to X screen numbers. Otherwise
 * the ids are some arbitrary ordering of Xinerama rootwins.
 */
EXTL_SAFE
EXTL_EXPORT
WScreen *ioncore_find_screen_id(int id)
{
    WScreen *scr, *maxscr=NULL;
    
    FOR_ALL_SCREENS(scr){
        if(id==-1){
            if(maxscr==NULL || scr->id>maxscr->id)
                maxscr=scr;
        }
        if(scr->id==id)
            return scr;
    }
    
    return maxscr;
}


/*EXTL_DOC
 * Switch focus to the screen with id \var{id} and return it.
 * 
 * Note that this function is asynchronous; the screen will not
 * actually have received the focus when this function returns.
 */
EXTL_EXPORT
WScreen *ioncore_goto_nth_screen(int id)
{
    WScreen *scr=ioncore_find_screen_id(id);
    if(scr!=NULL){
        if(!region_goto((WRegion*)scr))
            return NULL;
    }
    return scr;
}


/*EXTL_DOC
 * Switch focus to the next screen and return it.
 * 
 * Note that this function is asynchronous; the screen will not
 * actually have received the focus when this function returns.
 */
EXTL_EXPORT
WScreen *ioncore_goto_next_screen()
{
    WScreen *scr=ioncore_g.active_screen;
    
    if(scr!=NULL)
        scr=scr->next_scr;
    if(scr==NULL)
        scr=ioncore_g.screens;
    if(scr!=NULL){
        if(!region_goto((WRegion*)scr))
            return NULL;
    }
    return scr;
}


/*EXTL_DOC
 * Switch focus to the previous screen and return it.
 * 
 * Note that this function is asynchronous; the screen will not
 * actually have received the focus when this function returns.
 */
EXTL_EXPORT
WScreen *ioncore_goto_prev_screen()
{
    WScreen *scr=ioncore_g.active_screen;

    if(scr!=NULL)
        scr=scr->prev_scr;
    else
        scr=ioncore_g.screens;
    if(scr!=NULL){
        if(!region_goto((WRegion*)scr))
            return NULL;
    }
    return scr;
}


/*EXTL_DOC
 * Return the numerical id for screen \var{scr}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
int screen_id(WScreen *scr)
{
    return scr->id;
}


static bool screen_managed_may_destroy(WScreen *scr, WRegion *reg)
{
    WLListNode *node;
    bool onl1list=FALSE;

    if(OBJ_IS(reg, WClientWin))
        return TRUE;
    
    FOR_ALL_NODES_ON_LLIST(node, scr->mplex.l1_list){
        if(node->reg==reg)
            onl1list=TRUE;
        else if(OBJ_IS(node->reg, WGenWS))
            return TRUE;
    }
    
    if(!onl1list)
        return TRUE;
    
    warn(TR("Only workspace may not be destroyed."));
    
    return FALSE;
}


static bool screen_may_destroy(WScreen *scr)
{
    warn(TR("Screens may not be destroyed."));
    return FALSE;
}



void screen_set_managed_offset(WScreen *scr, const WRectangle *off)
{
    scr->managed_off=*off;
    mplex_fit_managed((WMPlex*)scr);
}


/*EXTL_DOC
 * Set offset of objects managed by the screen from actual screen geometry.
 * The table \var{offset} should contain the entries \code{x}, \code{y}, 
 * \code{w} and \code{h} indicating offsets of that component of screen 
 * geometry.
 */
EXTL_EXPORT_AS(WScreen, set_managed_offset)
bool screen_set_managed_offset_extl(WScreen *scr, ExtlTab offset)
{
    WRectangle g;
    
    if(!extl_table_to_rectangle(offset, &g))
        goto err;
    
    if(-g.w>=REGION_GEOM(scr).w)
        goto err;
    if(-g.h>=REGION_GEOM(scr).h)
        goto err;
    
    screen_set_managed_offset(scr, &g);
    
    return TRUE;
err:
    warn(TR("Invalid offset."));
    return FALSE;
}


WPHolder *screen_get_rescue_pholder_for(WScreen *scr, WRegion *mgd)
{
    WPHolder *ph;
    WLListNode *node, *node2;

    node=mplex_find_node(&(scr->mplex), mgd);

    if(node==NULL){
        node=scr->mplex.l1_current;
        if(node==NULL)
            node=scr->mplex.l1_list;
        if(node!=NULL){
            ph=region_get_rescue_pholder_for(node->reg, mgd);
            if(ph!=NULL)
                return ph;
        }
    }
            
    if(node!=NULL){
        node2=node->prev;
        for(node2=node->prev; node2->next!=NULL; node2=node2->prev){
            ph=region_get_rescue_pholder_for(node2->reg, mgd);
            if(ph!=NULL)
                return ph;
        }
        for(node2=node->next; node2!=NULL; node2=node->next){
            ph=region_get_rescue_pholder_for(node2->reg, mgd);
            if(ph!=NULL)
                return ph;
        }
    }
    
    return (WPHolder*)mplex_get_rescue_pholder_for(&(scr->mplex), mgd);
}

/*}}}*/


/*{{{ Save/load */


ExtlTab screen_get_configuration(WScreen *scr)
{
    return mplex_get_configuration(&scr->mplex);
}


static WRegion *do_create_initial(WWindow *parent, const WFitParams *fp, 
                                  WRegionLoadCreateFn *fn)
{
    return fn(parent, fp, extl_table_none());
}


extern char *ioncore_default_ws_type;


static bool create_initial_ws(WScreen *scr)
{
    WRegionLoadCreateFn *fn=NULL;
    WRegion *reg=NULL;
    
    if(ioncore_default_ws_type!=NULL){
        WRegClassInfo *info;
        info=ioncore_lookup_regclass(ioncore_default_ws_type, FALSE);
        if(info!=NULL)
            fn=info->lc_fn;
    }
        
    if(fn==NULL){
        WRegClassInfo *info=ioncore_lookup_regclass("WGenWS", TRUE);
        if(info!=NULL)
            fn=info->lc_fn;
    }
    
    if(fn==NULL){
        warn(TR("Could not find a complete workspace class. "
                "Please load some modules."));
        return FALSE;
    }
    
    reg=mplex_attach_hnd(&scr->mplex,
                         (WRegionAttachHandler*)do_create_initial, 
                         (void*)fn, 0);
    
    if(reg==NULL){
        warn(TR("Unable to create a workspace on screen %d."), scr->id);
        return FALSE;
    }
    
    return TRUE;
}


bool screen_init_layout(WScreen *scr, ExtlTab tab)
{
    char *name;
    ExtlTab substab, subtab;
    int n, i;

    if(tab==extl_table_none())
        return create_initial_ws(scr);
    
    mplex_load_contents(&scr->mplex, tab);
    
    return TRUE;
}

/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab screen_dynfuntab[]={
    {region_map, 
     screen_map},
    
    {region_unmap, 
     screen_unmap},
    
    {region_activated, 
     screen_activated},
    
    {(DynFun*)region_managed_may_destroy,
     (DynFun*)screen_managed_may_destroy},

    {(DynFun*)region_may_destroy,
     (DynFun*)screen_may_destroy},

    {mplex_managed_changed, 
     screen_managed_changed},
    
    {mplex_managed_geom, 
     screen_managed_geom},

    {region_managed_notify, 
     screen_managed_notify},

    {(DynFun*)region_get_configuration,
     (DynFun*)screen_get_configuration},

    {(DynFun*)region_handle_drop, 
     (DynFun*)screen_handle_drop},

    {(DynFun*)region_fitrep,
     (DynFun*)screen_fitrep},

    {(DynFun*)region_get_rescue_pholder_for,
     (DynFun*)screen_get_rescue_pholder_for},
    
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WScreen, WMPlex, screen_deinit, screen_dynfuntab);


/*}}}*/
