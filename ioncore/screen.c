/*
 * ion/ioncore/screen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/objp.h>
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
#include "region-iter.h"
#include <libtu/minmax.h>

#define SCR_MLIST(SCR) ((SCR)->mplex.l1_list)
#define SCR_MCOUNT(SCR) ((SCR)->mplex.l1_count)
#define SCR_WIN(SCR) ((SCR)->mplex.win.win)


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

    if(!mplex_init((WMPlex*)scr, (WWindow*)rootwin, win, fp))
        return FALSE;

    /*scr->mplex.win.region.rootwin=rootwin;
    region_set_parent((WRegion*)scr, (WRegion*)rootwin);*/
    scr->mplex.flags|=MPLEX_ADD_TO_END;
    scr->mplex.win.region.flags|=REGION_BINDINGS_ARE_GRABBED;
    if(useroot)
        scr->mplex.win.region.flags|=REGION_MAPPED;
    
    XSelectInput(ioncore_g.dpy, win, 
                 FocusChangeMask|EnterWindowMask|
                 KeyPressMask|KeyReleaseMask|
                 ButtonPressMask|ButtonReleaseMask|
                 (useroot ? IONCORE_EVENTMASK_ROOT : 0));

    if(id==0){
        scr->atom_workspace=XInternAtom(ioncore_g.dpy, "_ION_WORKSPACE", False);
    }else if(id>=0){
        char *str;
        libtu_asprintf(&str, "_ION_WORKSPACE%d", id);
        if(str==NULL){
            warn_err();
        }else{
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
        SCR_WIN(scr)=None;
    
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
    WRegion *curr=mplex_l1_current(&(scr->mplex));

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
    
    if(par==NULL){
        warn("Unable to reparent screens.");
        return FALSE;
    }
    
    if(scr->uses_root)
        return FALSE;

    return mplex_fitrep((WMPlex*)scr, NULL, fp);
}


static void screen_managed_changed(WScreen *scr, int mode, bool sw, 
                                   WRegion *reg_)
{
    const char *n=NULL;
    WRegion *reg;
    
    if(!sw)
        return;
    
    reg=mplex_l1_current(&(scr->mplex));

    if(scr->atom_workspace!=None && ioncore_g.opmode!=IONCORE_OPMODE_DEINIT){
        if(reg!=NULL)
            n=region_name(reg);
        
        xwindow_set_string_property(region_root_of((WRegion*)scr),
                                    scr->atom_workspace, 
                                    n==NULL ? "" : n);
    
        extl_call_named("call_hook", "soo", NULL,
                        "screen_workspace_switched", scr, reg);
    }
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


/*{{{ Misc. */


/*EXTL_DOC
 * Find the screen with numerical id \var{id}. If Xinerama is
 * not present, \var{id} corresponds to X screen numbers. Otherwise
 * the ids are some arbitrary ordering of Xinerama rootwins.
 */
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
    if(scr!=NULL)
        region_goto((WRegion*)scr);
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
    if(scr!=NULL)
        region_goto((WRegion*)scr);
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
    if(scr!=NULL)
        region_goto((WRegion*)scr);
    return scr;
}


/*EXTL_DOC
 * Return the numerical id for screen \var{scr}.
 */
EXTL_EXPORT_MEMBER
int screen_id(WScreen *scr)
{
    return scr->id;
}



static bool screen_may_destroy_managed(WScreen *scr, WRegion *reg)
{
    WRegion *r2;
    
    FOR_ALL_MANAGED_ON_LIST(SCR_MLIST(scr), r2){
        if(OBJ_IS(r2, WGenWS) && r2!=reg)
            return TRUE;
    }
    
    warn("Cannot destroy only workspace.");
    return FALSE;
}


void screen_set_managed_offset(WScreen *scr, const WRectangle *off)
{
    scr->managed_off=*off;
    mplex_fit_managed((WMPlex*)scr);
}


/*}}}*/


/*{{{ Save/load */


ExtlTab screen_get_configuration(WScreen *scr)
{
    return mplex_get_base_configuration(&scr->mplex);
}


static bool create_initial_ws(WScreen *scr)
{
    WRegionSimpleCreateFn *fn=NULL;
    WRegion *reg=NULL;
    
    /* Check default_ws_type */{
        ExtlTab tab=extl_globals();
        char *wsclass=NULL;
        if(extl_table_gets_s(tab, "DEFAULT_WS_TYPE", &wsclass)){
            WRegClassInfo *info=ioncore_lookup_regclass(wsclass,
                                                        FALSE, TRUE, FALSE);
            if(info!=NULL)
                fn=info->sc_fn;
            free(wsclass);
        }
        extl_unref_table(tab);
    }
        
    if(fn==NULL){
        WRegClassInfo *info=ioncore_lookup_regclass("WGenWS",
                                                    TRUE, TRUE, FALSE);
        if(info!=NULL)
            fn=info->sc_fn;
    }
    
    if(fn==NULL){
        warn("Could not find a complete workspace class. "
             "Please load some modules.");
        return FALSE;
    }
    
    reg=mplex_attach_hnd(&scr->mplex, (WRegionAttachHandler*)fn, NULL, 0);
    
    if(reg==NULL){
        warn("Unable to create a workspace on screen %d\n", scr->id);
        return FALSE;
    }
    
    region_set_name(reg, "main");
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
    {region_map, screen_map},
    {region_unmap, screen_unmap},
    {region_activated, screen_activated},
    
    {(DynFun*)region_may_destroy_managed,
     (DynFun*)screen_may_destroy_managed},

    {mplex_managed_changed, screen_managed_changed},
    
    {mplex_managed_geom, screen_managed_geom},

    {(DynFun*)region_get_configuration,
     (DynFun*)screen_get_configuration},

    {(DynFun*)region_handle_drop, 
     (DynFun*)screen_handle_drop},

    {(DynFun*)region_fitrep,
     (DynFun*)screen_fitrep},
    
    END_DYNFUNTAB
};


IMPLCLASS(WScreen, WMPlex, screen_deinit, screen_dynfuntab);


/*}}}*/
