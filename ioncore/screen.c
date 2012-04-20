/*
 * ion/ioncore/screen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <string.h>

#include <libtu/objp.h>
#include <libtu/minmax.h>

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
#include "event.h"
#include "bindmaps.h"
#include "regbind.h"
#include "frame-pointer.h"
#include "rectangle.h"
#include "extlconv.h"
#include "llist.h"
#include "group-ws.h"
#include "mplex.h"
#include "conf.h"
#include "activity.h"
#include "screen-notify.h"


WHook *screen_managed_changed_hook=NULL;


/*{{{ Init/deinit */


bool screen_init(WScreen *scr, WRootWin *parent,
                 const WFitParams *fp, int id, Window rootwin)
{
    Window win;
    XSetWindowAttributes attr;
    ulong attrflags=0;
    bool is_root=FALSE;
    
    scr->id=id;
    scr->atom_workspace=None;
    scr->managed_off.x=0;
    scr->managed_off.y=0;
    scr->managed_off.w=0;
    scr->managed_off.h=0;
    scr->next_scr=NULL;
    scr->prev_scr=NULL;
    
    watch_init(&(scr->notifywin_watch));
    watch_init(&(scr->infowin_watch));

    if(parent==NULL){
        win=rootwin;
        is_root=TRUE;
    }else{
        attr.background_pixmap=ParentRelative;
        attrflags=CWBackPixmap;
        
        win=XCreateWindow(ioncore_g.dpy, WROOTWIN_ROOT(parent),
                          fp->g.x, fp->g.y, fp->g.w, fp->g.h, 0, 
                          DefaultDepth(ioncore_g.dpy, parent->xscr),
                          InputOutput,
                          DefaultVisual(ioncore_g.dpy, parent->xscr),
                          attrflags, &attr);
        if(win==None)
            return FALSE;
    }

    if(!mplex_do_init((WMPlex*)scr, (WWindow*)parent, fp, win, "WScreen")){
        if(!is_root)
            XDestroyWindow(ioncore_g.dpy, win);
        return FALSE;
    }

    /*scr->mplex.win.region.rootwin=rootwin;
    region_set_parent((WRegion*)scr, (WRegion*)rootwin);*/
    scr->mplex.flags|=MPLEX_ADD_TO_END;
    scr->mplex.win.region.flags|=REGION_BINDINGS_ARE_GRABBED;
    
    if(!is_root){
        scr->mplex.win.region.flags|=REGION_MAPPED;
        window_select_input((WWindow*)scr, IONCORE_EVENTMASK_SCREEN);
    }
    
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

    /* Add all the needed bindings here; mplex does nothing so that
     * frames don't have to remove extra bindings.
     */
    region_add_bindmap((WRegion*)scr, ioncore_screen_bindmap);
    region_add_bindmap((WRegion*)scr, ioncore_mplex_bindmap);
    region_add_bindmap((WRegion*)scr, ioncore_mplex_toplevel_bindmap);

    LINK_ITEM(ioncore_g.screens, scr, next_scr, prev_scr);
    
    return TRUE;
}


WScreen *create_screen(WRootWin *parent, const WFitParams *fp, int id)
{
    CREATEOBJ_IMPL(WScreen, screen, (p, parent, fp, id, None));
}


void screen_deinit(WScreen *scr)
{
    UNLINK_ITEM(ioncore_g.screens, scr, next_scr, prev_scr);
    
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
    WRegion *curr=mplex_mx_current(&(scr->mplex));

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


static void screen_managed_changed(WScreen *scr, int mode, bool sw, 
                                   WRegion *reg_)
{
    if(ioncore_g.opmode==IONCORE_OPMODE_DEINIT)
        return;
    
    if(sw && scr->atom_workspace!=None){
        WRegion *reg=mplex_mx_current(&(scr->mplex));
        const char *n=NULL;
        
        if(reg!=NULL)
            n=region_displayname(reg);
        
        xwindow_set_string_property(region_root_of((WRegion*)scr),
                                    scr->atom_workspace, 
                                    n==NULL ? "" : n);
    }
    
    if(region_is_activity_r((WRegion*)scr))
        screen_update_notifywin(scr);
    
    screen_update_infowin(scr);
    
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

void screen_inactivated(WScreen *scr)
{
    screen_update_infowin(scr);
}


void screen_activated(WScreen *scr)
{
    screen_update_infowin(scr);
}


/*}}}*/


/*{{{ Misc. */


/*EXTL_DOC
 * Find the screen with numerical id \var{id}. 
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


static WScreen *current_screen()
{
    if(ioncore_g.focus_current==NULL)
        return ioncore_g.screens;
    else
        return region_screen_of(ioncore_g.focus_current);
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
    WScreen *scr=current_screen();
    
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
    WScreen *scr=current_screen();

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


static WRegion *screen_managed_disposeroot(WScreen *scr, WRegion *reg)
{
    bool onmxlist=FALSE, others=FALSE;
    WLListNode *lnode;
    WLListIterTmp tmp;
    
    if(OBJ_IS(reg, WGroupWS)){
        FOR_ALL_NODES_ON_LLIST(lnode, scr->mplex.mx_list, tmp){
            if(lnode->st->reg==reg){
                onmxlist=TRUE;
            }else if(OBJ_IS(lnode->st->reg, WGroupWS)){
                others=TRUE;
                break;
            }
        }

        if(onmxlist && !others){
            warn(TR("Only workspace may not be destroyed/detached."));
            return NULL;
        }
    }
    
    return reg;
}


static bool screen_may_dispose(WScreen *scr)
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


static bool create_initial_ws(WScreen *scr)
{
    WRegion *reg=NULL;
    WMPlexAttachParams par=MPLEXATTACHPARAMS_INIT;
    ExtlTab lo=ioncore_get_layout("default");
    
    if(lo==extl_table_none()){
        reg=mplex_do_attach_new(&scr->mplex, &par,
                                (WRegionCreateFn*)create_groupws, NULL);
    }else{
        reg=mplex_attach_new_(&scr->mplex, &par, 0, lo);
        extl_unref_table(lo);
    }
    
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
     
    {region_inactivated, 
     screen_inactivated},
    
    {(DynFun*)region_managed_disposeroot,
     (DynFun*)screen_managed_disposeroot},

    {(DynFun*)region_may_dispose,
     (DynFun*)screen_may_dispose},

    {mplex_managed_changed, 
     screen_managed_changed},
    
    {region_managed_notify, 
     screen_managed_notify},
    
    {mplex_managed_geom, 
     screen_managed_geom},

    {(DynFun*)region_get_configuration,
     (DynFun*)screen_get_configuration},

    {(DynFun*)region_handle_drop, 
     (DynFun*)screen_handle_drop},

    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WScreen, WMPlex, screen_deinit, screen_dynfuntab);


/*}}}*/
