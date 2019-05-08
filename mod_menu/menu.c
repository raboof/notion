/*
 * ion/mod_menu/menu.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <limits.h>

#include <libtu/minmax.h>
#include <libtu/objp.h>
#include <libtu/obj.h>
#include <libmainloop/defer.h>
#include <libmainloop/signal.h>

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/global.h>
#include <ioncore/regbind.h>
#include <ioncore/strings.h>
#include <ioncore/pointer.h>
#include <ioncore/focus.h>
#include <ioncore/event.h>
#include <ioncore/xwindow.h>
#include <ioncore/names.h>
#include <ioncore/gr.h>
#include <ioncore/gr-util.h>
#include <ioncore/sizehint.h>
#include <ioncore/resize.h>
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


/*{{{ Drawing routines */


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
        geom->w=MAXOF(0, geom->w);
        geom->h=MAXOF(0, geom->h);
    }
}


GR_DEFATTR(active);
GR_DEFATTR(inactive);
GR_DEFATTR(selected);
GR_DEFATTR(unselected);
GR_DEFATTR(normal);
GR_DEFATTR(submenu);


static void init_attr()
{
    GR_ALLOCATTR_BEGIN;
    GR_ALLOCATTR(active);
    GR_ALLOCATTR(inactive);
    GR_ALLOCATTR(selected);
    GR_ALLOCATTR(unselected);
    GR_ALLOCATTR(normal);
    GR_ALLOCATTR(submenu);
    GR_ALLOCATTR_END;
}


static void menu_draw_entry(WMenu *menu, int i, const WRectangle *igeom,
                            bool complete)
{
    WRectangle geom;
    GrAttr sa, aa;

    aa=(REGION_IS_ACTIVE(menu) ? GR_ATTR(active) : GR_ATTR(inactive));
    sa=(menu->selected_entry==i ? GR_ATTR(selected) : GR_ATTR(unselected));

    if(menu->entry_brush==NULL)
        return;

    geom=*igeom;
    geom.h=menu->entry_h;
    geom.y+=(i-menu->first_entry)*(menu->entry_h+menu->entry_spacing);

    grbrush_begin(menu->entry_brush, &geom, GRBRUSH_AMEND|GRBRUSH_KEEP_ATTR);

    grbrush_init_attr(menu->entry_brush, &menu->entries[i].attr);

    grbrush_set_attr(menu->entry_brush, aa);
    grbrush_set_attr(menu->entry_brush, sa);

    grbrush_draw_textbox(menu->entry_brush, &geom, menu->entries[i].title,
                         complete);

    grbrush_end(menu->entry_brush);
}


void menu_draw_entries(WMenu *menu, bool complete)
{
    WRectangle igeom;
    int i, mx;

    if(menu->entry_brush==NULL)
        return;

    get_inner_geom(menu, &igeom);

    mx=menu->first_entry+menu->vis_entries;
    mx=(mx < menu->n_entries ? mx : menu->n_entries);

    for(i=menu->first_entry; i<mx; i++)
        menu_draw_entry(menu, i, &igeom, complete);
}


void menu_draw(WMenu *menu, bool complete)
{
    GrAttr aa=(REGION_IS_ACTIVE(menu) ? GR_ATTR(active) : GR_ATTR(inactive));
    WRectangle geom;

    if(menu->brush==NULL)
        return;

    get_outer_geom(menu, &geom);

    grbrush_begin(menu->brush, &geom,
                  (complete ? 0 : GRBRUSH_NO_CLEAR_OK));

    grbrush_set_attr(menu->brush, aa);

    grbrush_draw_border(menu->brush, &geom);

    menu_draw_entries(menu, FALSE);

    grbrush_end(menu->brush);
}


/*}}}*/


/*{{{ Resize */


static void menu_calc_size(WMenu *menu, bool maxexact,
                           int maxw, int maxh,
                           int *w_ret, int *h_ret)
{
    GrBorderWidths bdw, e_bdw;
    char *str;
    int i;
    int bdh, maxew=menu->max_entry_w;

    grbrush_get_border_widths(menu->brush, &bdw);
    grbrush_get_border_widths(menu->entry_brush, &e_bdw);

    if(maxexact || maxew>maxw-(int)bdw.left-(int)bdw.right){
        maxew=maxw-bdw.left-bdw.right;
        *w_ret=maxw;
    }else{
        *w_ret=maxew+bdw.left+bdw.right;
    }

    bdh=bdw.top+bdw.bottom;

    if(menu->n_entries==0){
        *h_ret=(maxexact ? maxh : bdh);
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
        if(maxexact)
            *h_ret=maxh;
        else
            *h_ret=vis*menu->entry_h+(vis-1)*e_bdw.spacing+bdh;
    }

    /* Calculate new shortened entry names */
    maxew-=e_bdw.left+e_bdw.right;

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
    if(menu->pmenu_mode){
        menu_calc_size(menu, FALSE, INT_MAX, INT_MAX, w, h);
    }else{
        menu_calc_size(menu, !(menu->last_fp.mode&REGION_FIT_BOUNDS),
                       menu->last_fp.g.w, menu->last_fp.g.h, w, h);
    }
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


#define MINIMUM_Y_VISIBILITY 20
#define POINTER_OFFSET 5

static void menu_firstfit(WMenu *menu, bool submenu, const WRectangle *refg)
{
    WRectangle geom;

    calc_size(menu, &(geom.w), &(geom.h));

    if(!(menu->last_fp.mode&REGION_FIT_BOUNDS)){
        geom.x=menu->last_fp.g.x;
        geom.y=menu->last_fp.g.y;
    }else if(menu->pmenu_mode){
        geom.x=refg->x;
        geom.y=refg->y;

        if(!submenu){
            const WRectangle *maxg =
                &REGION_GEOM(REGION_PARENT((WRegion*)menu));

            geom.x-=geom.w/2;
            geom.y+=POINTER_OFFSET;

            if(geom.y+MINIMUM_Y_VISIBILITY>maxg->y+maxg->h){
                geom.y=maxg->y+maxg->h-MINIMUM_Y_VISIBILITY;
                geom.x=refg->x+POINTER_OFFSET;
                if(geom.x+geom.w>maxg->x+maxg->w)
                    geom.x=refg->x-geom.w-POINTER_OFFSET;
            }else{
                if(geom.x<0)
                    geom.x=0;
                else if(geom.x+geom.w>maxg->x+maxg->w)
                    geom.x=maxg->x+maxg->w-geom.w;
            }
        }
    }else{
        const WRectangle *maxg=&(menu->last_fp.g);
        if(submenu){
            int l, r, t, b, xoff, yoff;

            get_placement_offs(menu, &xoff, &yoff);
            l=refg->x+xoff;
            r=refg->x+refg->w+xoff;
            t=refg->y-yoff;
            b=refg->y+refg->h-yoff;

            geom.x=MAXOF(l, r-geom.w);
            if(geom.x+geom.w>maxg->x+maxg->w)
                geom.x=maxg->x;

            geom.y=MINOF(b-geom.h, t);
            if(geom.y<maxg->y)
                geom.y=maxg->y;
        }else{
            geom.x=maxg->x;
            geom.y=maxg->y+maxg->h-geom.h;
        }
    }

    window_do_fitrep(&menu->win, NULL, &geom);
}


static void menu_do_refit(WMenu *menu, WWindow *par, const WFitParams *oldfp)
{
    WRectangle geom;

    calc_size(menu, &(geom.w), &(geom.h));

    if(!(menu->last_fp.mode&REGION_FIT_BOUNDS)){
        geom.x=menu->last_fp.g.x;
        geom.y=menu->last_fp.g.y;
    }else if(menu->pmenu_mode){
        geom.x=REGION_GEOM(menu).x;
        geom.y=REGION_GEOM(menu).y;
    }else{
        const WRectangle *maxg=&(menu->last_fp.g);
        int xdiff=REGION_GEOM(menu).x-oldfp->g.x;
        int ydiff=(REGION_GEOM(menu).y+REGION_GEOM(menu).h
                   -(oldfp->g.y+oldfp->g.h));
        geom.x=MAXOF(0, MINOF(maxg->x+xdiff, maxg->x+maxg->w-geom.w));
        geom.y=MAXOF(0, MINOF(maxg->y+maxg->h+ydiff, maxg->y+maxg->h)-geom.h);
    }

    window_do_fitrep(&menu->win, par, &geom);
}


bool menu_fitrep(WMenu *menu, WWindow *par, const WFitParams *fp)
{
    WFitParams oldfp;

    if(par!=NULL && !region_same_rootwin((WRegion*)par, (WRegion*)menu))
        return FALSE;

    oldfp=menu->last_fp;
    menu->last_fp=*fp;
    menu_do_refit(menu, par, &oldfp);

    if(menu->submenu!=NULL && !menu->pmenu_mode)
        region_fitrep((WRegion*)(menu->submenu), par, fp);

    return TRUE;
}


void menu_size_hints(WMenu *menu, WSizeHints *hints_ret)
{
    int n=menu->n_entries;
    int w=menu->max_entry_w;
    int h=menu->entry_h*n + menu->entry_spacing*MAXOF(0, n-1);

    if(menu->brush!=NULL){
        GrBorderWidths bdw;
        grbrush_get_border_widths(menu->brush, &bdw);

        w+=bdw.left+bdw.right;
        h+=bdw.top+bdw.bottom;
    }

    hints_ret->min_set=TRUE;
    hints_ret->min_width=w;
    hints_ret->min_height=h;
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
    const char *style=(menu->big_mode
                       ? "input-menu-big"
                       : (menu->pmenu_mode
                          ? "input-menu-pmenu"
                          : "input-menu-normal"));

    const char *entry_style=(menu->big_mode
                                   ? "tab-menuentry-big"
                                   : (menu->pmenu_mode
                                      ? "tab-menuentry-pmenu"
                                      : "tab-menuentry-normal"));

    brush=gr_get_brush(win, rootwin, style);

    if(brush==NULL)
        return FALSE;

    entry_brush=grbrush_get_slave(brush, rootwin, entry_style);

    if(entry_brush==NULL){
        grbrush_release(brush);
        return FALSE;
    }

    if(menu->entry_brush!=NULL)
        grbrush_release(menu->entry_brush);
    if(menu->brush!=NULL)
        grbrush_release(menu->brush);

    menu->brush=brush;
    menu->entry_brush=entry_brush;

    calc_entry_dimens(menu);

    return TRUE;
}


void menu_updategr(WMenu *menu)
{
    if(!menu_init_gr(menu, region_rootwin_of((WRegion*)menu),
                     MENU_WIN(menu))){
        return;
    }

    menu_do_refit(menu, NULL, &(menu->last_fp));

    region_updategr_default((WRegion*)menu);

    window_draw((WWindow*)menu, TRUE);
}


static void menu_release_gr(WMenu *menu)
{
    if(menu->entry_brush!=NULL){
        grbrush_release(menu->entry_brush);
        menu->entry_brush=NULL;
    }
    if(menu->brush!=NULL){
        grbrush_release(menu->brush);
        menu->brush=NULL;
    }
}


/*}}}*/


/*{{{ Init/deinit */


static WMenuEntry *preprocess_menu(ExtlTab tab, int *n_entries)
{
    WMenuEntry *entries;
    ExtlTab entry;
    int i, n;

    n=extl_table_get_n(tab);
    *n_entries=n;

    if(n<=0)
        return NULL;

    entries=ALLOC_N(WMenuEntry, n);

    if(entries==NULL)
        return NULL;

    init_attr();

    /* Initialise entries and check submenus */
    for(i=1; i<=n; i++){
        WMenuEntry *ent=&entries[i-1];

        ent->title=NULL;
        ent->flags=0;

        gr_stylespec_init(&ent->attr);

        if(extl_table_geti_t(tab, i, &entry)){
            char *attr;
            ExtlTab sub;
            ExtlFn fn;

            if(extl_table_gets_s(entry, "attr", &attr)){
                gr_stylespec_load_(&ent->attr, attr, TRUE);
                free(attr);
            }

            if(extl_table_gets_f(entry, "submenu_fn", &fn)){
                ent->flags|=WMENUENTRY_SUBMENU;
                extl_unref_fn(fn);
            }else if(extl_table_gets_t(entry, "submenu", &sub)){
                ent->flags|=WMENUENTRY_SUBMENU;
                extl_unref_table(sub);
            }

            if(ent->flags&WMENUENTRY_SUBMENU)
                gr_stylespec_set(&ent->attr, GR_ATTR(submenu));

            extl_unref_table(entry);
        }
    }

    return entries;
}


static void deinit_entries(WMenu *menu);


bool menu_init(WMenu *menu, WWindow *par, const WFitParams *fp,
               const WMenuCreateParams *params)
{
    Window win;

    menu->entries=preprocess_menu(params->tab, &(menu->n_entries));

    if(menu->entries==NULL){
        warn(TR("Empty menu."));
        return FALSE;
    }

    menu->tab=extl_ref_table(params->tab);
    menu->handler=extl_ref_fn(params->handler);
    menu->pmenu_mode=params->pmenu_mode;
    menu->big_mode=params->big_mode;
    /*menu->cycle_bindmap=NULL;*/

    menu->last_fp=*fp;

    if(params->pmenu_mode){
        menu->selected_entry=-1;
    }else{
        menu->selected_entry=params->initial-1;
        if(menu->selected_entry<0)
           menu->selected_entry=0;
        if(params->initial > menu->n_entries)
           menu->selected_entry=0;
    }

    menu->max_entry_w=0;
    menu->entry_h=0;
    menu->brush=NULL;
    menu->entry_brush=NULL;
    menu->entry_spacing=0;
    menu->vis_entries=menu->n_entries;
    menu->first_entry=0;
    menu->submenu=NULL;
    menu->typeahead=NULL;

    menu->gm_kcb=0;
    menu->gm_state=0;

    if(!window_init((WWindow*)menu, par, fp, "WMenu"))
        goto fail;

    win=menu->win.win;

    if(!menu_init_gr(menu, region_rootwin_of((WRegion*)par), win))
        goto fail2;

    init_attr();

    menu_firstfit(menu, params->submenu_mode, &(params->refg));

    window_select_input(&(menu->win), IONCORE_EVENTMASK_NORMAL);

    region_add_bindmap((WRegion*)menu, mod_menu_menu_bindmap);

    region_register((WRegion*)menu);

    return TRUE;

fail2:
    window_deinit((WWindow*)menu);
fail:
    extl_unref_table(menu->tab);
    extl_unref_fn(menu->handler);
    deinit_entries(menu);
    return FALSE;
}


WMenu *create_menu(WWindow *par, const WFitParams *fp,
                   const WMenuCreateParams *params)
{
    CREATEOBJ_IMPL(WMenu, menu, (p, par, fp, params));
}


static void deinit_entries(WMenu *menu)
{
    int i;

    for(i=0; i<menu->n_entries; i++){
        gr_stylespec_unalloc(&menu->entries[i].attr);
        if(menu->entries[i].title!=NULL)
            free(menu->entries[i].title);
    }

    free(menu->entries);
}


void menu_deinit(WMenu *menu)
{
    menu_typeahead_clear(menu);

    if(menu->submenu!=NULL)
        destroy_obj((Obj*)menu->submenu);

    /*if(menu->cycle_bindmap!=NULL)
        bindmap_destroy(menu->cycle_bindmap);*/

    extl_unref_table(menu->tab);
    extl_unref_fn(menu->handler);

    deinit_entries(menu);

    menu_release_gr(menu);

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


static void menu_managed_remove(WMenu *menu, WRegion *sub)
{
    bool mcf=region_may_control_focus((WRegion*)menu);

    if(sub!=(WRegion*)menu->submenu)
        return;

    menu->submenu=NULL;

    region_unset_manager(sub, (WRegion*)menu);

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
    WFitParams fp;
    WMenuCreateParams fnp;
    WMenu *submenu;
    WWindow *par;

    par=REGION_PARENT(menu);

    if(par==NULL)
        return;

    fp=menu->last_fp;

    fnp.pmenu_mode=menu->pmenu_mode;
    fnp.big_mode=menu->big_mode;
    fnp.submenu_mode=TRUE;

    if(menu->pmenu_mode){
        fnp.refg.x=REGION_GEOM(menu).x+REGION_GEOM(menu).w;
        fnp.refg.y=REGION_GEOM(menu).y+get_sub_y_off(menu, n);
        fnp.refg.w=0;
        fnp.refg.h=0;
    }else{
        fnp.refg=REGION_GEOM(menu);
    }

    fnp.tab=extl_table_none();
    {
        ExtlFn fn;
        if(extl_table_getis(menu->tab, n+1, "submenu_fn", 'f', &fn)){
            extl_protect(NULL);
            extl_call(fn, NULL, "t", &(fnp.tab));
            extl_unprotect(NULL);
            extl_unref_fn(fn);
        }else{
            extl_table_getis(menu->tab, n+1, "submenu", 't', &(fnp.tab));
        }
        if(fnp.tab==extl_table_none())
            return;
    }

    fnp.handler=extl_ref_fn(menu->handler);

    fnp.initial=0;
    {
        ExtlFn fn;
        if(extl_table_getis(menu->tab, n+1, "initial", 'f', &fn)){
            extl_protect(NULL);
            extl_call(fn, NULL, "i", &(fnp.initial));
            extl_unprotect(NULL);
            extl_unref_fn(fn);
        }else{
            extl_table_getis(menu->tab, n+1, "initial", 'i', &(fnp.initial));
        }
    }

    submenu=create_menu(par, &fp, &fnp);

    if(submenu==NULL)
        return;

    menu->submenu=submenu;
    region_set_manager((WRegion*)submenu, (WRegion*)menu);

    region_restack((WRegion*)submenu, MENU_WIN(menu), Above);
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


void menu_restack(WMenu *menu, Window other, int mode)
{
    xwindow_restack(MENU_WIN(menu), other, mode);
    if(menu->submenu!=NULL)
        region_restack((WRegion*)(menu->submenu), MENU_WIN(menu), Above);
}


void menu_stacking(WMenu *menu, Window *bottomret, Window *topret)
{
    *topret=None;

    if(menu->submenu!=NULL)
        region_stacking((WRegion*)(menu->submenu), bottomret, topret);

    *bottomret=MENU_WIN(menu);
    if(*topret==None)
        *topret=MENU_WIN(menu);

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

        /* !!!BEGIN!!! */
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
    WMenu *head=menu_head(menu);

    handler=menu->handler;
    menu->handler=extl_fn_none();

    ok=extl_table_geti_t(menu->tab, menu->selected_entry+1, &tab);

    if(!region_rqdispose((WRegion*)head)){
        if(head->submenu!=NULL)
            destroy_obj((Obj*)head->submenu);
    }

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

    mainloop_defer_action((Obj*)menu, (WDeferredAction*)menu_do_finish);
}



/*EXTL_DOC
 * Close \var{menu} not calling any possible finish handlers.
 */
EXTL_EXPORT_MEMBER
void menu_cancel(WMenu *menu)
{
    region_defer_rqdispose((WRegion*)menu);
}


/*}}}*/


/*{{{ Scroll */

static int scroll_time=20;
static int scroll_amount=3;
static WTimer *scroll_timer=NULL;


static void reset_scroll_timer()
{
    if(scroll_timer!=NULL){
        destroy_obj((Obj*)scroll_timer);
        scroll_timer=NULL;
    }
}


/*EXTL_DOC
 * Set module basic settings. The parameter table may contain the
 * following fields:
 *
 * \begin{tabularx}{\linewidth}{lX}
 *  \tabhead{Field & Description}
 *  \var{scroll_amount} & Number of pixels to scroll at a time
 *                        pointer-controlled menus when one extends
 *                        beyond a border of the screen and the pointer
 *                        touches that border. \\
 *  \var{scroll_delay}  & Time between such scrolling events in
 *                        milliseconds.
 * \end{tabularx}
 */
EXTL_EXPORT
void mod_menu_set(ExtlTab tab)
{
    int a, t;

    if(extl_table_gets_i(tab, "scroll_amount", &a))
        scroll_amount=MAXOF(0, a);
    if(extl_table_gets_i(tab, "scroll_delay", &t))
        scroll_time=MAXOF(0, t);
}


/*EXTL_DOC
 * Get module basic settings. For details, see \fnref{mod_menu.set}.
 */
EXTL_SAFE
EXTL_EXPORT
ExtlTab mod_menu_get()
{
    ExtlTab tab=extl_create_table();
    extl_table_sets_i(tab, "scroll_amount", scroll_amount);
    extl_table_sets_i(tab, "scroll_delay", scroll_time);
    return tab;
}


enum{
    D_LEFT,
    D_RIGHT,
    D_DOWN,
    D_UP
};


static int calc_diff(const WRectangle *mg, const WRectangle *pg, int d)
{
    switch(d){
    case D_LEFT:
        return mg->x+mg->w-pg->w;
    case D_UP:
        return mg->y+mg->h-pg->h;
    case D_RIGHT:
        return -mg->x;
    case D_DOWN:
        return -mg->y;
    }
    return 0;
}


static int scrolld_subs(WMenu *menu, int d)
{
    int diff=0;
    WRegion *p=REGION_PARENT_REG(menu);
    const WRectangle *pg;

    if(p==NULL)
        return 0;

    pg=&REGION_GEOM(p);

    while(menu!=NULL){
        int new_diff = calc_diff(&REGION_GEOM(menu), pg, d);
        diff=MAXOF(diff, new_diff);
        menu=menu->submenu;
    }

    return MINOF(MAXOF(0, diff), scroll_amount);
}


static void menu_select_entry_at(WMenu *menu, int px, int py);


static void do_scroll(WMenu *menu, int xd, int yd)
{
    WRectangle g;
    int px=-1, py=-1;

    xwindow_pointer_pos(region_root_of((WRegion*)menu), &px, &py);

    while(menu!=NULL){
        g=REGION_GEOM(menu);
        g.x+=xd;
        g.y+=yd;

        window_do_fitrep((WWindow*)menu, NULL, &g);

        menu_select_entry_at(menu, px, py);

        menu=menu->submenu;
    }
}


static void scroll_left(WTimer *timer, WMenu *menu)
{
    if(menu!=NULL){
        do_scroll(menu, -scrolld_subs(menu, D_LEFT), 0);
        if(scrolld_subs(menu, D_LEFT)>0){
            timer_set(timer, scroll_time, (WTimerHandler*)scroll_left,
                      (Obj*)menu);
            return;
        }
    }
}


static void scroll_up(WTimer *timer, WMenu *menu)
{
    if(menu!=NULL){
        do_scroll(menu, 0, -scrolld_subs(menu, D_UP));
        if(scrolld_subs(menu, D_UP)>0){
            timer_set(timer, scroll_time, (WTimerHandler*)scroll_up,
                      (Obj*)menu);

            return;
        }
    }
}


static void scroll_right(WTimer *timer, WMenu *menu)
{
    if(menu!=NULL){
        do_scroll(menu, scrolld_subs(menu, D_RIGHT), 0);
        if(scrolld_subs(menu, D_RIGHT)>0){
            timer_set(timer, scroll_time, (WTimerHandler*)scroll_right,
                      (Obj*)menu);
            return;
        }
    }
}


static void scroll_down(WTimer *timer, WMenu *menu)
{
    if(menu!=NULL){
        do_scroll(menu, 0, scrolld_subs(menu, D_DOWN));
        if(scrolld_subs(menu, D_DOWN)>0){
            timer_set(timer, scroll_time, (WTimerHandler*)scroll_down,
                      (Obj*)menu);
            return;
        }
    }
}


static void end_scroll(WMenu *UNUSED(menu))
{
    reset_scroll_timer();
}

#define SCROLL_OFFSET 10

static void check_scroll(WMenu *menu, int x, int y)
{
    WRegion *parent=REGION_PARENT_REG(menu);
    int rx, ry;
    WTimerHandler *fn=NULL;

    if(!menu->pmenu_mode)
        return;

    if(parent==NULL){
        end_scroll(menu);
        return;
    }

    region_rootpos(parent, &rx, &ry);
    x-=rx;
    y-=ry;

    if(x<=SCROLL_OFFSET){
        fn=(WTimerHandler*)scroll_right;
    }else if(y<=SCROLL_OFFSET){
        fn=(WTimerHandler*)scroll_down;
    }else if(x>=REGION_GEOM(parent).w-SCROLL_OFFSET){
        fn=(WTimerHandler*)scroll_left;
    }else if(y>=REGION_GEOM(parent).h-SCROLL_OFFSET){
        fn=(WTimerHandler*)scroll_up;
    }else{
        end_scroll(menu);
        return;
    }

    assert(fn!=NULL);

    if(scroll_timer!=NULL){
        if(scroll_timer->handler==(WTimerHandler*)fn &&
           timer_is_set(scroll_timer)){
            return;
        }
    }else{
        scroll_timer=create_timer();
        if(scroll_timer==NULL)
            return;
    }

    fn(scroll_timer, (Obj*)menu_head(menu));
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


static void menu_select_entry_at(WMenu *menu, int px, int py)
{
    int entry=menu_entry_at_root_tree(menu, px, py, &menu);
    if(entry>=0)
        menu_do_select_nth(menu, entry);
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


void menu_motion(WMenu *menu, XMotionEvent *ev, int UNUSED(dx), int UNUSED(dy))
{
    menu_select_entry_at(menu, ev->x_root, ev->y_root);
    check_scroll(menu, ev->x_root, ev->y_root);
}


void menu_button(WMenu *menu, XButtonEvent *ev)
{
    int entry=menu_entry_at_root_tree(menu, ev->x_root, ev->y_root, &menu);
    if(entry>=0)
        menu_select_nth(menu, entry);
}


int menu_press(WMenu *menu, XButtonEvent *ev, WRegion **UNUSED(reg_ret))
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
            free(newta_orig);
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
    {(DynFun*)region_fitrep, (DynFun*)menu_fitrep},
    {region_updategr, menu_updategr},
    {window_draw, menu_draw},
    {(DynFun*)window_press, (DynFun*)menu_press},
    {region_managed_remove, menu_managed_remove},
    {region_do_set_focus, menu_do_set_focus},
    {region_activated, menu_activated},
    {region_inactivated, menu_inactivated},
    {window_insstr, menu_insstr},
    {region_restack, menu_restack},
    {region_stacking, menu_stacking},
    {region_size_hints, menu_size_hints},
    END_DYNFUNTAB
};


EXTL_EXPORT
IMPLCLASS(WMenu, WWindow, menu_deinit, menu_dynfuntab);


/*}}}*/


