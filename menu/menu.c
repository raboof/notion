/*
 * ion/menu/menu.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <limits.h>

#include <libtu/minmax.h>
#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/global.h>
#include <ioncore/regbind.h>
#include <ioncore/defer.h>
#include <ioncore/strings.h>
#include <ioncore/pointer.h>
#include <ioncore/stacking.h>
#include <ioncore/signal.h>
#include <ioncore/focus.h>
#include <ioncore/event.h>
#include <libtu/objp.h>
#include <ioncore/region-iter.h>
#include "menu.h"
#include "main.h"

#define MENU_WIN(MENU) ((MENU)->win.win)

/*{{{ Helpers */


static bool extl_table_getis(ExtlTab tab, int i, const char *s, char c,
                             void *p)
{
    ExtlTab sub;
    bool ret;
    
    if(!extl_table_geti_t(tab, i, &sub))
        return FALSE;
    ret=extl_table_get(sub, 's', c, s, p);
    extl_unref_table(sub);
    return ret;
}


/*}}}*/


/*{{{ Drawing routines */


static void get_outer_geom(WMenu *menu, WRectangle *geom)
{
    geom->x=0;
    geom->y=0;
    geom->w=REGION_GEOM(menu).w;
    geom->h=REGION_GEOM(menu).h;
}


static void get_inner_geom(WMenu *menu, WRectangle *geom)
{
    GrBorderWidths bdw;
    
    get_outer_geom(menu, geom);
    
    if(menu->brush!=NULL){
        grbrush_get_border_widths(menu->brush, &bdw);
        geom->x+=bdw.left;
        geom->y+=bdw.top;
        geom->w-=bdw.left+bdw.right;
        geom->h-=bdw.top+bdw.bottom;
        geom->w=maxof(0, geom->w);
        geom->h=maxof(0, geom->h);
    }
}


void menu_draw_entry(WMenu *menu, int i, const WRectangle *igeom,
                     bool complete)
{
    WRectangle geom;
    int a;

    static const char *attrs[]={
        "active-selected-normal",
        "active-selected-submenu",
        "active-unselected-normal",
        "active-unselected-submenu",
        "inactive-selected-normal",
        "inactive-selected-submenu",
        "inactive-unselected-normal",
        "inactive-unselected-submenu",
    };
    
    if(menu->entry_brush==NULL)
        return;
    
    geom=*igeom;
    geom.h=menu->entry_h;
    geom.y+=(i-menu->first_entry)*(menu->entry_h+menu->entry_spacing);
    
    a=((REGION_IS_ACTIVE(menu) ? 0 : 4)
       |(menu->selected_entry==i ? 0 : 2)
       |(menu->entries[i].flags&WMENUENTRY_SUBMENU ? 1 : 0));

    grbrush_draw_textbox(menu->entry_brush, MENU_WIN(menu), &geom,
                         menu->entries[i].title, attrs[a], complete);
}

    
void menu_draw_entries(WMenu *menu, bool complete)
{
    WRectangle igeom;
    int i, mx;
    
    get_inner_geom(menu, &igeom);
    
    mx=menu->first_entry+menu->vis_entries;
    mx=(mx < menu->n_entries ? mx : menu->n_entries);
    
    for(i=menu->first_entry; i<mx; i++)
        menu_draw_entry(menu, i, &igeom, complete);
}


void menu_draw(WMenu *menu, bool complete)
{
    WRectangle geom;
    const char *substyle=(REGION_IS_ACTIVE(menu) ? "active" : "inactive");
    
    if(menu->brush==NULL)
        return;
    
    get_outer_geom(menu, &geom);
    
    grbrush_draw_border(menu->brush, MENU_WIN(menu), &geom, substyle);
    
    menu_draw_entries(menu, FALSE);
}


/*}}}*/


/*{{{ Resize */


static void menu_calc_size(WMenu *menu, int maxw, int maxh, 
                           int *w_ret, int *h_ret)
{
    GrBorderWidths bdw, e_bdw;
    char *str;
    int i;
    int nath, bdh, maxew=menu->max_entry_w;
    
    grbrush_get_border_widths(menu->brush, &bdw);
    grbrush_get_border_widths(menu->entry_brush, &e_bdw);
    
    if(maxew>maxw-(int)bdw.left-(int)bdw.right){
        maxew=maxw-bdw.left-bdw.right;
        *w_ret=maxw;
    }else{
        *w_ret=maxew+bdw.left+bdw.right;
    }
    
    bdh=bdw.top+bdw.bottom;
    
    if(menu->n_entries==0){
        *h_ret=bdh;
        menu->first_entry=0;
        menu->vis_entries=0;
    }else{
        int vis=(maxh-bdh+e_bdw.spacing)/(e_bdw.spacing+menu->entry_h);
        if(vis>menu->n_entries){
            vis=menu->n_entries;
            menu->first_entry=0;
        }else if(menu->selected_entry>=0){
            if(menu->selected_entry<menu->first_entry)
                menu->first_entry=menu->selected_entry;
            else if(menu->selected_entry>=menu->first_entry+vis)
                menu->first_entry=menu->selected_entry-vis+1;
        }
        if(vis<=0)
            vis=1;
        menu->vis_entries=vis;
        *h_ret=vis*menu->entry_h+(vis-1)*e_bdw.spacing+bdh;
    }

    /* Calculate new shortened entry names */
    maxew-=e_bdw.left+e_bdw.right;
#if 0
    if(menu->title!=NULL){
        free(menu->title);
        menu->title=NULL;
    }
    
    if(extl_table_gets_s(tab, title, &str)){
        menu->title=grbrush_make_label(menu->title_brush, str, maxew);
        free(str);
    }
#endif
    
    for(i=0; i<menu->n_entries; i++){
        if(menu->entries[i].title){
            free(menu->entries[i].title);
            menu->entries[i].title=NULL;
        }
        if(maxew<=0)
            continue;
        
        if(extl_table_getis(menu->tab, i+1, "name", 's', &str)){
            menu->entries[i].title=grbrush_make_label(menu->entry_brush, 
                                                      str, maxew);
            free(str);
        }
    }
}


void calc_size(WMenu *menu, int *w, int *h)
{
    if(menu->pmenu_mode)
        menu_calc_size(menu, INT_MAX, INT_MAX, w, h);
    else
        menu_calc_size(menu, menu->max_geom.w, menu->max_geom.h, w, h);
}
    

/* Return offset from bottom-left corner of containing mplex or top-right
 * corner of  parent menu for the respective corner of menu.
 */
static void get_placement_offs(WMenu *menu, int *xoff, int *yoff)
{
    GrBorderWidths bdw;
    
    *xoff=0; 
    *yoff=0;
    
    if(menu->brush!=NULL){
        grbrush_get_border_widths(menu->brush, &bdw);
        *xoff+=bdw.right;
        *yoff+=bdw.top;
    }
    
    if(menu->entry_brush!=NULL){
        grbrush_get_border_widths(menu->entry_brush, &bdw);
        *xoff+=bdw.right;
        *yoff+=bdw.top;
    }
}
    
    
static void menu_firstfit(WMenu *menu, bool submenu, int ref_x, int ref_y)
{
    WRectangle geom;
    
    calc_size(menu, &(geom.w), &(geom.h));
    
    if(menu->pmenu_mode){
        geom.x=ref_x;
        geom.y=ref_y;
        if(!submenu){
            geom.x-=geom.w/2;
            geom.y+=5;
        }
    }else{
        if(submenu){
            int xoff, yoff, x2, y2;
            get_placement_offs(menu, &xoff, &yoff);
            x2=minof(ref_x+xoff, menu->max_geom.x+menu->max_geom.w);
            y2=maxof(ref_y-yoff, menu->max_geom.y);
            geom.x=menu->max_geom.x+xoff;
            if(geom.x+geom.w<x2)
                geom.x=x2-geom.w;
            geom.y=menu->max_geom.y+menu->max_geom.h-yoff-geom.h;
            if(geom.y>y2)
                geom.y=y2;
            
        }else{
            geom.x=menu->max_geom.x;
            geom.y=menu->max_geom.y+menu->max_geom.h-geom.h;
        }
    }
    
    window_fit(&menu->win, &geom);
}


static void menu_refit(WMenu *menu)
{
    WRectangle geom;
    
    calc_size(menu, &(geom.w), &(geom.h));
    
    geom.x=REGION_GEOM(menu).x;
    geom.y=REGION_GEOM(menu).y;
    
    if(!menu->pmenu_mode){
        geom.x=maxof(geom.x, menu->max_geom.x);
        geom.x=minof(geom.x, menu->max_geom.x+menu->max_geom.w-geom.w);
        geom.y=maxof(geom.y, menu->max_geom.y);
        geom.y=minof(geom.y, menu->max_geom.y+menu->max_geom.h-geom.h);
    }
    window_fit(&menu->win, &geom);
}


void menu_fit(WMenu *menu, const WRectangle *geom)
{
    menu->max_geom=*geom;
    menu_refit(menu);
}


/*}}}*/


/*{{{ Brush update */


static void calc_entry_dimens(WMenu *menu)
{
    int i, n=extl_table_get_n(menu->tab);
    GrFontExtents fnte;
    GrBorderWidths bdw;
    int maxw=0;
    char *str;
    
#if 0    
    if(extl_table_gets_s(menu->tab, title, &str)){
        maxw=grbrush_get_text_width(title_brush, str, strlen(str));
        free(str);
    }
#endif
    
    for(i=1; i<=n; i++){
        if(extl_table_getis(menu->tab, i, "name", 's', &str)){
            int w=grbrush_get_text_width(menu->entry_brush, 
                                         str, strlen(str));
            if(w>maxw)
                maxw=w;
            free(str);
        }
    }
    
    grbrush_get_border_widths(menu->entry_brush, &bdw);
    grbrush_get_font_extents(menu->entry_brush, &fnte);
    
    menu->max_entry_w=maxw+bdw.left+bdw.right;
    menu->entry_h=fnte.max_height+bdw.top+bdw.bottom;
    menu->entry_spacing=bdw.spacing;
}


static bool menu_init_gr(WMenu *menu, WRootWin *rootwin, Window win)
{
    GrBrush *brush, *entry_brush;
    char *st;
    
    brush=gr_get_brush(rootwin, win,  (menu->big_mode
                                       ? "input-menu-big"
                                       : "input-menu-normal"));
    
    if(brush==NULL)
        return FALSE;

    entry_brush=grbrush_get_slave(brush, rootwin, win, 
                                  (menu->big_mode
                                   ? "tab-menuentry-big"
                                   : "tab-menuentry-normal"));
    
    if(entry_brush==NULL){
        grbrush_release(brush, win);
        return FALSE;
    }
    
    if(menu->entry_brush!=NULL)
        grbrush_release(menu->entry_brush, win);
    if(menu->brush!=NULL)
        grbrush_release(menu->brush, win);

    menu->brush=brush;
    menu->entry_brush=entry_brush;
    
    calc_entry_dimens(menu);
    
    return TRUE;
}


void menu_draw_config_updated(WMenu *menu)
{
    if(!menu_init_gr(menu, region_rootwin_of((WRegion*)menu),
                     MENU_WIN(menu)))
        return;
    
    menu_refit(menu);
    
    region_draw_config_updated_default((WRegion*)menu);
    
    window_draw((WWindow*)menu, TRUE);
}


static void menu_release_gr(WMenu *menu, Window win)
{
    if(menu->entry_brush!=NULL)
        grbrush_release(menu->entry_brush, win);
    if(menu->brush!=NULL)
        grbrush_release(menu->brush, win);
}


/*}}}*/


/*{{{ Init/deinit */


static WMenuEntry *preprocess_menu(ExtlTab tab, int *n_entries)
{
    ExtlTab entry, sub;
    ExtlFn fn;
    WMenuEntry *entries;
    int i, n;
    
    n=extl_table_get_n(tab);
    *n_entries=n;
    
    if(n<=0)
        return NULL;

    entries=ALLOC_N(WMenuEntry, n);  
    
    if(entries==NULL){
        warn_err();
        return NULL;
    }

    /* Initialise entries and check submenus */
    for(i=1; i<=n; i++){
        entries[i-1].title=NULL;
        entries[i-1].flags=0;
        if(extl_table_getis(tab, i, "submenu_fn", 'f', &fn)){
            entries[i-1].flags|=WMENUENTRY_SUBMENU;
            extl_unref_fn(fn);
        }else if(extl_table_getis(tab, i, "submenu", 't', &sub)){
            entries[i-1].flags|=WMENUENTRY_SUBMENU;
            extl_unref_table(sub);
        }
    }
    
    return entries;
}



bool menu_init(WMenu *menu, WWindow *par, const WRectangle *geom,
               const WMenuCreateParams *params)
{
    Window win;
    int i;

    menu->entries=preprocess_menu(params->tab, &(menu->n_entries));
    
    if(menu->entries==NULL){
        warn("Empty menu");
        return FALSE;
    }

    menu->tab=extl_ref_table(params->tab);
    menu->handler=extl_ref_fn(params->handler);
    menu->pmenu_mode=params->pmenu_mode;
    menu->big_mode=params->big_mode;

    menu->max_geom=*geom;
    menu->selected_entry=(params->pmenu_mode ? -1 : 0);
    menu->max_entry_w=0;
    menu->entry_h=0;
    menu->brush=NULL;
    menu->entry_brush=NULL;
    menu->entry_spacing=0;
    menu->vis_entries=menu->n_entries;
    menu->first_entry=0;
    menu->submenu=NULL;
    menu->typeahead=NULL;
    
    if(!window_init_new((WWindow*)menu, par, geom))
        goto fail;

    win=menu->win.win;
    
    if(!menu_init_gr(menu, region_rootwin_of((WRegion*)par), win))
        goto fail2;

    menu_firstfit(menu, params->submenu_mode, params->ref_x, params->ref_y);
    
    XSelectInput(ioncore_g.dpy, win, IONCORE_EVENTMASK_INPUT);
    region_add_bindmap((WRegion*)menu, &mod_menu_menu_bindmap);
    
    return TRUE;

fail2:
    window_deinit((WWindow*)menu);
fail:
    extl_unref_table(menu->tab);
    extl_unref_fn(menu->handler);
    free(menu->entries);
    return FALSE;
}


WMenu *create_menu(WWindow *par, const WRectangle *geom, 
                   const WMenuCreateParams *params)
{
    CREATEOBJ_IMPL(WMenu, menu, (p, par, geom, params));
}



void menu_deinit(WMenu *menu)
{
    int i;
    
    menu_typeahead_clear(menu);
    
    if(menu->submenu!=NULL)
        destroy_obj((Obj*)menu->submenu);
    
    extl_unref_table(menu->tab);
    extl_unref_fn(menu->handler);
    
    for(i=0; i<menu->n_entries; i++)
        free(menu->entries[i].title);
    free(menu->entries);
    
    menu_release_gr(menu, MENU_WIN(menu));
    window_deinit((WWindow*)menu);
}


/*}}}*/


/*{{{ Focus  */


static void menu_inactivated(WMenu *menu)
{
    window_draw((WWindow*)menu, FALSE);
}


static void menu_activated(WMenu *menu)
{
    window_draw((WWindow*)menu, FALSE);
}


/*}}}*/


/*{{{ Submenus */


static WMenu *menu_head(WMenu *menu)
{
    WMenu *m=REGION_MANAGER_CHK(menu, WMenu);
    return (m==NULL ? menu : menu_head(m));
}


static WMenu *menu_tail(WMenu *menu)
{
    return (menu->submenu==NULL ? menu : menu_tail(menu->submenu));
}


static void menu_remove_managed(WMenu *menu, WRegion *sub)
{
    bool mcf=region_may_control_focus((WRegion*)menu);
    
    if(sub!=(WRegion*)menu->submenu)
        return;

    region_unset_manager(sub, (WRegion*)menu, (WRegion**)&(menu->submenu));

    if(mcf)
        region_do_set_focus((WRegion*)menu, FALSE);
}


int get_sub_y_off(WMenu *menu, int n)
{
    /* top + sum of above entries and spacings - top_of_sub */
    return (menu->entry_h+menu->entry_spacing)*(n-menu->first_entry);
}


static void show_sub(WMenu *menu, int n)
{
    WRectangle g;
    WMenuCreateParams fnp;
    WMenu *submenu;
    WWindow *par;
    
    par=REGION_PARENT_CHK(menu, WWindow);
    
    if(par==NULL)
        return;
    
    g=menu->max_geom;
    
    if(menu->pmenu_mode){
        fnp.ref_x=REGION_GEOM(menu).x+REGION_GEOM(menu).w;
        fnp.ref_y=REGION_GEOM(menu).y+get_sub_y_off(menu, n);
    }else{
        fnp.ref_x=REGION_GEOM(menu).x+REGION_GEOM(menu).w;
        fnp.ref_y=REGION_GEOM(menu).y;
    }

    fnp.tab=extl_table_none();
    {
        ExtlFn fn;
        if(extl_table_getis(menu->tab, n+1, "submenu_fn", 'f', &fn)){
            extl_call(fn, NULL, "t", &(fnp.tab));
            extl_unref_fn(fn);
        }else{
            extl_table_getis(menu->tab, n+1, "submenu", 't', &(fnp.tab));
        }
        if(fnp.tab==extl_table_none())
            return;
    }
    
    fnp.handler=extl_ref_fn(menu->handler);
    fnp.pmenu_mode=menu->pmenu_mode;
    fnp.big_mode=menu->big_mode;
    fnp.submenu_mode=TRUE;
    
    submenu=create_menu(par, &g, &fnp);
    
    if(submenu==NULL)
        return;
    
    region_set_manager((WRegion*)submenu, (WRegion*)menu,
                       (WRegion**)&(menu->submenu));
    region_stack_above((WRegion*)submenu, (WRegion*)menu);
    region_map((WRegion*)submenu);
    
    if(!menu->pmenu_mode && region_may_control_focus((WRegion*)menu))
        region_do_set_focus((WRegion*)submenu, FALSE);
}


static void menu_do_set_focus(WMenu *menu, bool warp)
{
    if(menu->submenu!=NULL)
        region_do_set_focus((WRegion*)menu->submenu, warp);
    else
        window_do_set_focus((WWindow*)menu, warp);
}


/*}}}*/


/*{{{ Exports */


static void menu_do_select_nth(WMenu *menu, int n)
{
    int oldn=menu->selected_entry;
    bool drawfull=FALSE;
    
    if(oldn==n)
        return;
    
    if(menu->submenu!=NULL)
        destroy_obj((Obj*)menu->submenu);
    
    assert(menu->submenu==NULL);

    menu->selected_entry=n;

    if(n>=0){
        if(n<menu->first_entry){
            menu->first_entry=n;
            drawfull=TRUE;
        }else if(n>=menu->first_entry+menu->vis_entries){
            menu->first_entry=n-menu->vis_entries+1;
            drawfull=TRUE;
        }
        
        if(menu->entries[n].flags&WMENUENTRY_SUBMENU &&
           menu->pmenu_mode){
            show_sub(menu, n);
        }
    }
    
    if(drawfull){
        menu_draw_entries(menu, TRUE);
    }else{
        /* redraw new and old selected entry */ 
        WRectangle igeom;
        get_inner_geom(menu, &igeom);

        if(oldn!=-1)
            menu_draw_entry(menu, oldn, &igeom, TRUE);
        if(n!=-1)
            menu_draw_entry(menu, n, &igeom, TRUE);
    }
}


/*EXTL_DOC
 * Select \var{n}:th entry in menu.
 */
EXTL_EXPORT_MEMBER
void menu_select_nth(WMenu *menu, int n)
{
    if(n<0)
        n=0;
    if(n>=menu->n_entries)
        n=menu->n_entries-1;

    menu_typeahead_clear(menu);
    menu_do_select_nth(menu, n);
}


/*EXTL_DOC
 * Select previous entry in menu.
 */
EXTL_EXPORT_MEMBER
void menu_select_prev(WMenu *menu)
{
    menu_select_nth(menu, (menu->selected_entry<=0 
                           ? menu->n_entries-1
                           : menu->selected_entry-1));
}


/*EXTL_DOC
 * Select next entry in menu.
 */
EXTL_EXPORT_MEMBER
void menu_select_next(WMenu *menu)
{
    menu_select_nth(menu, (menu->selected_entry+1)%menu->n_entries);
}


static void menu_do_finish(WMenu *menu)
{
    ExtlFn handler;
    ExtlTab tab;
    bool ok;
    WMenu *actmenu=menu;
    
    handler=actmenu->handler;
    actmenu->handler=extl_fn_none();
    
    ok=extl_table_geti_t(actmenu->tab, actmenu->selected_entry+1, &tab);
    
    destroy_obj((Obj*)menu_head(menu));
    
    if(ok)
        extl_call(handler, "t", NULL, tab);

    extl_unref_fn(handler);
    extl_unref_table(tab);
}


/*EXTL_DOC
 * If selected entry is a submenu, display that.
 * Otherwise destroy the menu and call handler for selected entry.
 */
EXTL_EXPORT_MEMBER
void menu_finish(WMenu *menu)
{
    menu_typeahead_clear(menu);

    if(!menu->pmenu_mode && menu->selected_entry>=0 &&
       menu->entries[menu->selected_entry].flags&WMENUENTRY_SUBMENU){
        show_sub(menu, menu->selected_entry);
        return;
    }
    
    ioncore_defer_action((Obj*)menu, (WDeferredAction*)menu_do_finish);
}



/*EXTL_DOC
 * Close \var{menu} not calling any possible finish handlers.
 */
EXTL_EXPORT_MEMBER
void menu_cancel(WMenu *menu)
{
    ioncore_defer_destroy((Obj*)menu);
}


bool menu_rqclose(WMenu *menu)
{
    menu_cancel(menu);
    return TRUE;
}


/*}}}*/


/*{{{ Scroll */

static int scroll_time=20;
static int scroll_amount=3;
static WTimer scroll_timer=TIMER_INIT(NULL);


/*EXTL_DOC
 * Set ''pmenu'' off-screen scrolling parameters: the number of pixels 
 * to scroll at each timer event, \var{amount}, and the time between
 * those events, \var{delay} (in milliseconds). The default values are 3
 * pixels every 20msec.
 */
EXTL_EXPORT
void mod_menu_set_scroll_params(int delay, int amount)
{
    scroll_amount=maxof(0, amount);
    scroll_time=maxof(1, delay);
}


static int scrolld(int d)
{
    return minof(maxof(0, d), scroll_amount);
}


static bool get_parent_g(WMenu *menu, WRectangle *geom)
{
    WRegion *p=REGION_PARENT(menu);
    if(menu==NULL)
        return FALSE;
    *geom=REGION_GEOM(p);
    return TRUE;
}


static int right_diff(WMenu *menu)
{
    WRectangle g;
    if(get_parent_g(menu, &g))
        return scrolld(REGION_GEOM(menu).x+REGION_GEOM(menu).w-g.x-g.w);
    return 0;
}


static int bottom_diff(WMenu *menu)
{
    WRectangle g;
    if(get_parent_g(menu, &g))
        return scrolld(REGION_GEOM(menu).y+REGION_GEOM(menu).h-g.y-g.h);
    return 0;
}


static int left_diff(WMenu *menu)
{
    WRectangle g;
    if(get_parent_g(menu, &g))
        return scrolld(g.x-REGION_GEOM(menu).x);
    return 0;
}


static int top_diff(WMenu *menu)
{
    WRectangle g;
    if(get_parent_g(menu, &g))
        return scrolld(g.y-REGION_GEOM(menu).y);
    return 0;
}


static void scroll_left_or_up(WMenu *menu, int xd, int yd)
{
    WRectangle g;
    
    while(menu!=NULL){
        g=REGION_GEOM(menu);
        g.x-=xd;
        g.y-=yd;
        window_fit((WWindow*)menu, &g);
        menu=REGION_MANAGER_CHK(menu, WMenu);
    }
}


static void scroll_left(WTimer *timer, WMenu *menu)
{
    menu=menu_tail(menu);
    scroll_left_or_up(menu, right_diff(menu), 0);
    if(right_diff(menu)>0)
        timer_set_param(timer, scroll_time, (Obj*)menu);
}


static void scroll_up(WTimer *timer, WMenu *menu)
{
    menu=menu_tail(menu);
    scroll_left_or_up(menu, 0, bottom_diff(menu));
    if(bottom_diff(menu)>0)
        timer_set_param(timer, scroll_time, (Obj*)menu);
}


static void scroll_right_or_down(WMenu *menu, int xd, int yd)
{
    WRectangle g;
    
    while(menu!=NULL){
        g=REGION_GEOM(menu);
        g.x+=xd;
        g.y+=yd;
        window_fit((WWindow*)menu, &g);
        menu=menu->submenu;
    }
}


static void scroll_right(WTimer *timer, WMenu *menu)
{
    menu=menu_head(menu);
    scroll_right_or_down(menu, left_diff(menu), 0);
    if(left_diff(menu)>0)
        timer_set_param(timer, scroll_time, (Obj*)menu);
}


static void scroll_down(WTimer *timer, WMenu *menu)
{
    menu=menu_head(menu);
    scroll_right_or_down(menu, 0, top_diff(menu));
    if(top_diff(menu)>0)
        timer_set_param(timer, scroll_time, (Obj*)menu);
}


static void end_scroll(WMenu *menu)
{
    timer_reset(&(scroll_timer));
}


static void check_scroll(WMenu *menu, int x, int y)
{
    WRegion *parent=REGION_PARENT(menu);
    int rx, ry;
    void (*fn)(void)=NULL;
    
    if(!menu->pmenu_mode)
        return;
    
    if(parent==NULL){
        end_scroll(menu);
        return;
    }

    region_rootpos(parent, &rx, &ry);
    x-=rx;
    y-=ry;
    
    if(x<=1){
        fn=(void(*)(void))scroll_right;
    }else if(y<=1){
        fn=(void(*)(void))scroll_down;
    }else if(x>=REGION_GEOM(parent).w-1){
        fn=(void(*)(void))scroll_left;
    }else if(y>=REGION_GEOM(parent).h-1){
        fn=(void(*)(void))scroll_up;
    }else{
        end_scroll(menu);
        return;
    }
    
    assert(fn!=NULL);
    
    menu=menu_head(menu);

    while(menu!=NULL){
        if(rectangle_contains(&REGION_GEOM(menu), x, y)){
            if(scroll_timer.handler!=fn || !timer_is_set(&scroll_timer)){
                scroll_timer.handler=fn;
                timer_set_param(&scroll_timer, scroll_time, (Obj*)menu);
            }
            return;
        }
        menu=menu->submenu;
    }
}


/*}}}*/


/*{{{ Pointer handlers */


int menu_entry_at_root(WMenu *menu, int root_x, int root_y)
{
    int rx, ry, x, y, entry;
    WRectangle ig;
    region_rootpos((WRegion*)menu, &rx, &ry);
    
    get_inner_geom(menu, &ig);
    
    x=root_x-rx-ig.x;
    y=root_y-ry-ig.y;
    
    if(x<0 || x>=ig.w || y<0  || y>=ig.h)
        return -1;
    
    entry=y/(menu->entry_h+menu->entry_spacing);
    if(entry<0 || entry>=menu->vis_entries)
        return -1;
    return entry+menu->first_entry;
}


int menu_entry_at_root_tree(WMenu *menu, int root_x, int root_y, 
                            WMenu **realmenu)
{
    int entry=-1;
    
    menu=menu_tail(menu);
    
    *realmenu=menu;
    
    if(!menu->pmenu_mode)
        return menu_entry_at_root(menu, root_x, root_y);
    
    while(menu!=NULL){
        entry=menu_entry_at_root(menu, root_x, root_y);
        if(entry>=0){
            *realmenu=menu;
            break;
        }
        menu=REGION_MANAGER_CHK(menu, WMenu);
    }
    
    return entry;
}


void menu_release(WMenu *menu, XButtonEvent *ev)
{
    int entry=menu_entry_at_root_tree(menu, ev->x_root, ev->y_root, &menu);
    end_scroll(menu);
    if(entry>=0){
        menu_select_nth(menu, entry);
        menu_finish(menu);
    }else if(menu->pmenu_mode){
        menu_cancel(menu_head(menu));
    }
}


void menu_motion(WMenu *menu, XMotionEvent *ev, int dx, int dy)
{
    int entry=menu_entry_at_root_tree(menu, ev->x_root, ev->y_root, &menu);
    if(menu->pmenu_mode || entry>=0)
        menu_do_select_nth(menu, entry);
    check_scroll(menu, ev->x_root, ev->y_root);
}


void menu_button(WMenu *menu, XButtonEvent *ev)
{
    int entry=menu_entry_at_root_tree(menu, ev->x_root, ev->y_root, &menu);
    if(entry>=0)
        menu_select_nth(menu, entry);
}


int menu_press(WMenu *menu, XButtonEvent *ev, WRegion **reg_ret)
{
    menu_button(menu, ev);
    menu=menu_head(menu);
    ioncore_set_drag_handlers((WRegion*)menu,
                        NULL,
                        (WMotionHandler*)menu_motion,
                        (WButtonHandler*)menu_release,
                        NULL, 
                        NULL);
    return 0;
}


/*}}}*/


/*{{{ Typeahead find */


static void menu_insstr(WMenu *menu, const char *buf, size_t n)
{
    size_t oldlen=(menu->typeahead==NULL ? 0 : strlen(menu->typeahead));
    char *newta=(char*)malloc(oldlen+n+1);
    char *newta_orig;
    int entry;
    
    if(newta==NULL)
        return;
    
    if(oldlen!=0)
        memcpy(newta, menu->typeahead, oldlen);
    if(n!=0)
        memcpy(newta+oldlen, buf, n);
    newta[oldlen+n]='\0';
    newta_orig=newta;
    
    while(*newta!='\0'){
        bool found=FALSE;
        entry=menu->selected_entry;
        do{
            if(menu->entries[entry].title!=NULL){
                size_t l=strlen(menu->entries[entry].title);
                if(libtu_strcasestr(menu->entries[entry].title, newta)){
                    found=TRUE;
                    break;
                }
            }
            entry=(entry+1)%menu->n_entries;
        }while(entry!=menu->selected_entry);
        if(found){
            menu_do_select_nth(menu, entry);
            break;
        }
        newta++;
    }
    
    if(newta_orig!=newta){
        if(*newta=='\0'){
            free(newta);
            newta=NULL;
        }else{
            char *p=scopy(newta);
            free(newta_orig);
            newta=p;
        }
    }
    if(menu->typeahead!=NULL)
        free(menu->typeahead);
    menu->typeahead=newta;
}


/*EXTL_DOC
 * Clear typeahead buffer.
 */
EXTL_EXPORT_MEMBER
void menu_typeahead_clear(WMenu *menu)
{
    if(menu->typeahead!=NULL){
        free(menu->typeahead);
        menu->typeahead=NULL;
    }
}


/*}}}*/


/*{{{ Dynamic function table and class implementation */


static DynFunTab menu_dynfuntab[]={
    {region_fit, menu_fit},
    {region_draw_config_updated, menu_draw_config_updated},
    {(DynFun*)region_rqclose, (DynFun*)menu_rqclose},
    {window_draw, menu_draw},
    {(DynFun*)window_press, (DynFun*)menu_press},
    {region_remove_managed, menu_remove_managed},
    {region_do_set_focus, menu_do_set_focus},
    {region_activated, menu_activated},
    {region_inactivated, menu_inactivated},
    {window_insstr, menu_insstr},
    END_DYNFUNTAB
};


IMPLCLASS(WMenu, WWindow, menu_deinit, menu_dynfuntab);

    
/*}}}*/


