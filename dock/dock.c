/*
 * Ion dock module
 * Copyright (C) 2003 Tom Payne
 * Copyright (C) 2003 Per Olofsson
 *
 * by Tom Payne <ion@tompayne.org>
 * based on code by Per Olofsson <pelle@dsv.su.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Header: /home/twp/cvsroot/twp/ion/ion-devel-dock/dock.c,v 1.17 2003/12/21 11:59:48 twp Exp $
 *
 */


/* Macros {{{ */
#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif
/* }}} */


/* Includes {{{ */

#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xext.h>
#include <ioncore/clientwin.h>
#include <ioncore/common.h>
#include <ioncore/eventh.h>
#include <ioncore/extl.h>
#include <ioncore/global.h>
#include <ioncore/manage.h>
#include <ioncore/names.h>
#include <libtu/objp.h>
#include <ioncore/property.h>
#include <ioncore/readconfig.h>
#include <ioncore/resize.h>
#include <ioncore/stacking.h>
#include <ioncore/window.h>
#include <libtu/map.h>
#include <version.h>

/* }}} */


/* Global variables {{{ */
static const char *modname="dock";
const char mod_dock_ion_api_version[]=ION_API_VERSION;
bool shape_extension=FALSE;
int shape_event_basep=0;
int shape_error_basep=0;
/* }}} */


/* StringIntMap {{{ */

/* strintintmap_reverse_value {{{ */
static const char *stringintmap_reverse_value(const StringIntMap *map,
                                              int value,
                                              const char *dflt)
{
    int i;

    for(i=0; map[i].string!=NULL; ++i){
        if(map[i].value==value){
            return map[i].string;
        }
    }

    return dflt;

}
/* }}} */

/* }}} */


/* WDockParam {{{ */

INTRCLASS(WDockParam);

DECLCLASS(WDockParam){ /* {{{ */
    const char *key;
    const char *desc;
    const StringIntMap *map;
    int dflt;
};
/* }}} */

/* dock_param_extl_table_get {{{ */
static void dock_param_extl_table_get(const WDockParam *param, ExtlTab conftab,
                                      int value)
{
    const char *s;

    s=stringintmap_reverse_value(param->map, value, NULL);
    if(s){
        extl_table_sets_s(conftab, param->key, s);
    }

}
/* }}} */

/* dock_param_do_set {{{ */
static bool dock_param_do_set(const WDockParam *param, char *s,
                                 int *ret)
{
    bool changed=FALSE;
    int i=stringintmap_value(param->map, s, -1);
    if(i<0){
        warn_obj(modname, "Invalid %s \"%s\"", param->desc, s);
    }else{
        if(*ret!=i){
            changed=TRUE;
        }
        *ret=i;
    }
    free(s);

    return changed;

}
/* }}} */

/* dock_param_extl_table_set {{{ */
static bool dock_param_extl_table_set(const WDockParam *param, ExtlTab conftab,
                                      int *ret)
{
    char *s;

    if(extl_table_gets_s(conftab, param->key, &s))
        return dock_param_do_set(param, s, ret);

    return FALSE;

}
/* }}} */

/* dock_param_brush_set {{{ */
static bool dock_param_brush_set(const WDockParam *param, GrBrush *brush,
                                 int *ret)
{
    char *s;

    if(grbrush_get_extra(brush, param->key, 's', &s))
        return dock_param_do_set(param, s, ret);

    return FALSE;

}
/* }}} */

/* }}} */


/* WDockApp {{{ */

INTRCLASS(WDockApp);

DECLCLASS(WDockApp){ /* {{{ */
    WDockApp *next, *prev;
    WClientWin *cwin;
    int pos;
    bool draw_border;
    bool tile;
    WRectangle geom;
    WRectangle tile_geom;
    WRectangle border_geom;
};
/* }}} */

/* }}} */


/* WDock {{{ */

INTRCLASS(WDock);

DECLCLASS(WDock){ /* {{{ */
    WWindow win;
    WDock *dock_next, *dock_prev;
    WRegion *managed_list;
    int vpos, hpos, grow;
    bool is_auto;
    GrBrush *brush;
    WDockApp *dockapps;
};
/* }}} */

static WDock *docks=NULL;

/* Parameter descriptions {{{ */

static const WDockParam dock_param_name={ /* {{{ */
    "name",
    "name",
    NULL,
    0
};
/* }}} */

/* Horizontal position {{{ */

enum WDockHPos{
    DOCK_HPOS_LEFT,
    DOCK_HPOS_CENTER,
    DOCK_HPOS_RIGHT,
    DOCK_HPOS_LAST
};

static StringIntMap dock_hpos_map[]={
    {"left", DOCK_HPOS_LEFT},
    {"center", DOCK_HPOS_CENTER},
    {"right", DOCK_HPOS_RIGHT},
    END_STRINGINTMAP
};

static WDockParam dock_param_hpos={
    "hpos",
    "horizontal position",
    dock_hpos_map,
    DOCK_HPOS_RIGHT
};

/* }}} */

/* Vertical position {{{ */

enum WDockVPos{
    DOCK_VPOS_TOP,
    DOCK_VPOS_MIDDLE,
    DOCK_VPOS_BOTTOM
};

static StringIntMap dock_vpos_map[]={
    {"top", DOCK_VPOS_TOP},
    {"middle", DOCK_VPOS_MIDDLE},
    {"bottom", DOCK_VPOS_BOTTOM},
    END_STRINGINTMAP
};

static WDockParam dock_param_vpos={
    "vpos",
    "vertical position",
    dock_vpos_map,
    DOCK_VPOS_MIDDLE
};

/* }}} */

/* Growth direction {{{ */

enum WDockGrow{
    DOCK_GROW_UP,
    DOCK_GROW_DOWN,
    DOCK_GROW_LEFT,
    DOCK_GROW_RIGHT
};

static StringIntMap dock_grow_map[]={
    {"up", DOCK_GROW_UP},
    {"down", DOCK_GROW_DOWN},
    {"left", DOCK_GROW_LEFT},
    {"right", DOCK_GROW_RIGHT},
    END_STRINGINTMAP
};

WDockParam dock_param_grow={
    "grow",
    "growth direction",
    dock_grow_map,
    DOCK_GROW_DOWN
};

/* }}} */

static const WDockParam dock_param_is_auto={ /* {{{ */
    "is_auto",
    "is automatic",
    NULL,
    TRUE
};
/* }}} */

static const WDockParam dock_param_is_mapped={ /* {{{ */
    "is_mapped",
    "is mapped",
    NULL,
    TRUE
};
/* }}} */

/* Outline style {{{ */

enum WDockOutlineStyle{
    DOCK_OUTLINE_STYLE_NONE,
    DOCK_OUTLINE_STYLE_ALL,
    DOCK_OUTLINE_STYLE_EACH
};

static StringIntMap dock_outline_style_map[]={
    {"none", DOCK_OUTLINE_STYLE_NONE},
    {"all", DOCK_OUTLINE_STYLE_ALL},
    {"each", DOCK_OUTLINE_STYLE_EACH},
    END_STRINGINTMAP
};

WDockParam dock_param_outline_style={
    "outline_style",
    "outline style",
    dock_outline_style_map,
    DOCK_OUTLINE_STYLE_ALL
};

/* }}} */

static const WDockParam dock_param_tile_width={ /* {{{ */
    "width",
    "tile width",
    NULL,
    64
};
/* }}} */

static const WDockParam dock_param_tile_height={ /* {{{ */
    "height",
    "tile height",
    NULL,
    64
};
/* }}} */

/* }}} */

#define CLIENTWIN_WINPROP_POSITION "dockposition"
#define CLIENTWIN_WINPROP_BORDER "dockborder"

/* dock_find_dockapp {{{ */
static WDockApp *dock_find_dockapp(WDock *dock, WRegion *reg)
{
    WDockApp *dockapp;

    for(dockapp=dock->dockapps; dockapp!=NULL; dockapp=dockapp->next){
        if((WRegion *)dockapp->cwin==reg){
            return dockapp;
        }
    }

    return NULL;

}
/* }}} */

/* dock_get_outline_style {{{ */
static void dock_get_outline_style(WDock *dock, int *ret)
{

    *ret=dock_param_outline_style.dflt;
    if(dock->brush!=NULL)
        dock_param_brush_set(&dock_param_outline_style, dock->brush, ret);

}
/* }}} */

/* dock_get_tile_size {{{ */
static void dock_get_tile_size(WDock *dock, WRectangle *ret)
{
    ExtlTab tile_size_table;

    ret->x=0;
    ret->y=0;
    ret->w=dock_param_tile_width.dflt;
    ret->h=dock_param_tile_height.dflt;
    if(dock->brush==NULL)
        return;
    if(grbrush_get_extra(dock->brush, "tile_size", 't', &tile_size_table)){
        extl_table_gets_i(tile_size_table, dock_param_tile_width.key, &ret->w);
        extl_table_gets_i(tile_size_table, dock_param_tile_height.key, &ret->h);
        extl_unref_table(tile_size_table);
    }

}
/* }}} */

/* dock_reshape {{{ */
static void dock_reshape(WDock *dock)
{
    int outline_style;

    if(!shape_extension){
        return;
    }

    dock_get_outline_style(dock, &outline_style);
    switch(outline_style){

    case DOCK_OUTLINE_STYLE_NONE:
    case DOCK_OUTLINE_STYLE_EACH: /* {{{ */
        {
            WDockApp *dockapp;

            /* Start with an empty set {{{ */
            XShapeCombineRectangles(ioncore_g.dpy, ((WWindow *)dock)->win,
                                    ShapeBounding, 0, 0, NULL, 0, ShapeSet, 0);
            /* }}} */

            /* Union with dockapp shapes {{{ */
            for(dockapp=dock->dockapps; dockapp!=NULL; dockapp=dockapp->next){
                if(outline_style==DOCK_OUTLINE_STYLE_EACH
                   && dockapp->draw_border){
                    /* Union with border shape {{{ */
                    XRectangle tile_rect;

                    tile_rect.x=dockapp->border_geom.x;
                    tile_rect.y=dockapp->border_geom.y;
                    tile_rect.width=dockapp->border_geom.w;
                    tile_rect.height=dockapp->border_geom.h;
                    XShapeCombineRectangles(ioncore_g.dpy, ((WWindow *)dock)->win,
                                            ShapeBounding, 0, 0, &tile_rect, 1,
                                            ShapeUnion, 0);
                    /* }}} */
                }else{
                    /* Union with dockapp shape {{{ */
                    int count;
                    int ordering;

                    XRectangle *rects=XShapeGetRectangles(ioncore_g.dpy,
                                                          dockapp->cwin->win,
                                                          ShapeBounding, &count,
                                                          &ordering);
                    if(rects!=NULL){
                        WRectangle dockapp_geom=REGION_GEOM(dockapp->cwin);
                        XShapeCombineRectangles(ioncore_g.dpy, ((WWindow *)dock)->win,
                                                ShapeBounding,
                                                dockapp_geom.x, dockapp_geom.y,
                                                rects, count, ShapeUnion, ordering);
                        XFree(rects);
                    }
                    /* }}} */
                }
            }
            /* }}} */

        }
        break;
        /* }}} */

    case DOCK_OUTLINE_STYLE_ALL: /* {{{ */
        {
            WRectangle geom;
            XRectangle rect;

            geom=REGION_GEOM(dock);
            rect.x=0;
            rect.y=0;
            rect.width=geom.w;
            rect.height=geom.h;
            XShapeCombineRectangles(ioncore_g.dpy, ((WWindow *)dock)->win,
                                    ShapeBounding, 0, 0, &rect, 1, ShapeSet, 0);
        }
        break;
        /* }}} */

    }

}
/* }}} */

/* dock_request_managed_geom {{{ */
static void dock_request_managed_geom(WDock *dock, WRegion *reg, int flags,
                                      const WRectangle *geom,
                                      WRectangle *geomret)
{
    WDockApp *dockapp;
    WRectangle parent_geom, dock_geom, border_dock_geom;
    GrBorderWidths dock_bdw, dockapp_bdw;
    WRectangle tile_size;
    int n_dockapps=0, max_w=1, max_h=1, total_w=0, total_h=0, cur_coord=0;

    /* Determine parent and tile geoms {{{ */
    parent_geom=((WRegion *)dock)->parent->geom;
    dock_get_tile_size(dock, &tile_size);
    /* }}} */

    /* Determine dock and dockapp border widths {{{ */
    memset(&dock_bdw, 0, sizeof(GrBorderWidths));
    memset(&dockapp_bdw, 0, sizeof(GrBorderWidths));
    if(dock->brush){
        int outline_style;

        dock_get_outline_style(dock, &outline_style);
        switch(outline_style){
        case DOCK_OUTLINE_STYLE_NONE:
            break;
        case DOCK_OUTLINE_STYLE_ALL:
            grbrush_get_border_widths(dock->brush, &dock_bdw);
            dockapp_bdw.spacing=dock_bdw.spacing;
            break;
        case DOCK_OUTLINE_STYLE_EACH:
            grbrush_get_border_widths(dock->brush, &dockapp_bdw);
            break;
        }
    }
    /* }}} */

    /* Calculate widths and heights {{{ */
    for(dockapp=dock->dockapps; dockapp!=NULL; dockapp=dockapp->next){

        /* Use requested geom if available, otherwise use existing geom {{{ */
        if((WRegion *)dockapp->cwin==reg && geom){
            dockapp->geom=*geom;
        }else{
            dockapp->geom=REGION_GEOM(dockapp->cwin);
        }
        /* }}} */

        /* Determine whether dockapp should be placed on a tile {{{ */
        dockapp->tile=dockapp->geom.w<=tile_size.w && dockapp->geom.h<=tile_size.h;
        /* }}} */

        /* Calculate width and height {{{ */
        if(dockapp->tile){
            dockapp->tile_geom.w=tile_size.w;
            dockapp->tile_geom.h=tile_size.h;
        }else{
            dockapp->tile_geom.w=dockapp->geom.w;
            dockapp->tile_geom.h=dockapp->geom.h;
        }
        /* }}} */

        /* Calculate border width and height {{{ */
        dockapp->border_geom.w=dockapp_bdw.left+dockapp->tile_geom.w+dockapp_bdw.right;
        dockapp->border_geom.h=dockapp_bdw.top+dockapp->tile_geom.h+dockapp_bdw.right;
        /* }}} */

        /* Calculate maximum and accumulated widths and heights {{{ */
        if(dockapp->border_geom.w>max_w){
            max_w=dockapp->border_geom.w;
        }
        total_w+=dockapp->border_geom.w+(n_dockapps ? dockapp_bdw.spacing : 0);
        if(dockapp->border_geom.h>max_h){
            max_h=dockapp->border_geom.h;
        }
        total_h+=dockapp->border_geom.h+(n_dockapps ? dockapp_bdw.spacing : 0);
        /* }}} */

        /* Count dockapps {{{ */
        ++n_dockapps;
        /* }}} */

    }
    /* }}} */

    /* Calculate width and height of dock {{{ */
    if(n_dockapps){
        switch(dock->grow){
        case DOCK_GROW_LEFT:
        case DOCK_GROW_RIGHT:
            dock_geom.w=total_w;
            dock_geom.h=max_h;
            break;
        case DOCK_GROW_UP:
        case DOCK_GROW_DOWN:
            dock_geom.w=max_w;
            dock_geom.h=total_h;
            break;
        }
    }else{
        dock_geom.w=1;
        dock_geom.h=1;
    }
    border_dock_geom.w=dock_bdw.left+dock_geom.w+dock_bdw.right;
    border_dock_geom.h=dock_bdw.top+dock_geom.h+dock_bdw.bottom;
    /* }}} */

    /* Position dock horizontally {{{ */
    switch(dock->hpos){
    case DOCK_HPOS_LEFT:
        border_dock_geom.x=0;
        break;
    case DOCK_HPOS_CENTER:
        border_dock_geom.x=(parent_geom.w-border_dock_geom.w)/2;
        break;
    case DOCK_HPOS_RIGHT:
        border_dock_geom.x=parent_geom.w-border_dock_geom.w;
        break;
    }
    /* }}} */

    /* Position dock vertically {{{ */
    switch(dock->vpos){
    case DOCK_VPOS_TOP:
        border_dock_geom.y=0;
        break;
    case DOCK_VPOS_MIDDLE:
        border_dock_geom.y=(parent_geom.h-border_dock_geom.h)/2;
        break;
    case DOCK_VPOS_BOTTOM:
        border_dock_geom.y=parent_geom.h-border_dock_geom.h;
        break;
    }
    /* }}} */

    /* Fit dock to new geom if required {{{ */
    if(!(flags&REGION_RQGEOM_TRYONLY)){
        region_fit((WRegion *)dock, &border_dock_geom, REGION_FIT_EXACT);
    }
    /* }}} */

    /* Calculate initial co-ordinate for layout algorithm {{{ */
    switch(dock->grow){
    case DOCK_GROW_UP:
        cur_coord=dock_bdw.top+dock_geom.h;
        break;
    case DOCK_GROW_DOWN:
        cur_coord=dock_bdw.top;
        break;
    case DOCK_GROW_LEFT:
        cur_coord=dock_bdw.left+dock_geom.w;
        break;
    case DOCK_GROW_RIGHT:
        cur_coord=dock_bdw.left;
        break;
    }
    /* }}} */

    /* Arrange dockapps {{{ */
    for(dockapp=dock->dockapps; dockapp!=NULL; dockapp=dockapp->next){

        /* Calculate first co-ordinate {{{ */
        switch(dock->grow){
        case DOCK_GROW_UP:
        case DOCK_GROW_DOWN:
            switch(dock->hpos){
            case DOCK_HPOS_LEFT:
                dockapp->border_geom.x=0;
                break;
            case DOCK_HPOS_CENTER:
                dockapp->border_geom.x=(dock_geom.w-dockapp->border_geom.w)/2;
                break;
            case DOCK_HPOS_RIGHT:
                dockapp->border_geom.x=dock_geom.w-dockapp->border_geom.w;
                break;
            }
            dockapp->border_geom.x+=dock_bdw.left;
            break;
        case DOCK_GROW_LEFT:
        case DOCK_GROW_RIGHT:
            switch(dock->vpos){
            case DOCK_VPOS_TOP:
                dockapp->border_geom.y=0;
                break;
            case DOCK_VPOS_MIDDLE:
                dockapp->border_geom.y=(dock_geom.h-dockapp->border_geom.h)/2;
                break;
            case DOCK_VPOS_BOTTOM:
                dockapp->border_geom.y=dock_geom.h-dockapp->border_geom.h;
                break;
            }
            dockapp->border_geom.y+=dock_bdw.top;
            break;
        }
        /* }}} */

        /* Calculate second co-ordinate {{{ */
        switch(dock->grow){
        case DOCK_GROW_UP:
            cur_coord-=dockapp->border_geom.h;
            dockapp->border_geom.y=cur_coord;
            cur_coord-=dockapp_bdw.spacing;
            break;
        case DOCK_GROW_DOWN:
            dockapp->border_geom.y=cur_coord;
            cur_coord+=dockapp->border_geom.h+dockapp_bdw.spacing;
            break;
        case DOCK_GROW_LEFT:
            cur_coord-=dockapp->border_geom.w;
            dockapp->border_geom.x=cur_coord;
            cur_coord-=dockapp_bdw.spacing;
            break;
        case DOCK_GROW_RIGHT:
            dockapp->border_geom.x=cur_coord;
            cur_coord+=dockapp->border_geom.w+dockapp_bdw.spacing;
            break;
        }
        /* }}} */

        /* Calculate tile geom {{{ */
        dockapp->tile_geom.x=dockapp->border_geom.x+dockapp_bdw.left;
        dockapp->tile_geom.y=dockapp->border_geom.y+dockapp_bdw.top;
        /* }}} */

        /* Calculate dockapp geom {{{ */
        if(dockapp->tile){
            dockapp->geom.x=dockapp->tile_geom.x+(dockapp->tile_geom.w-dockapp->geom.w)/2;
            dockapp->geom.y=dockapp->tile_geom.y+(dockapp->tile_geom.h-dockapp->geom.h)/2;
        }else{
            dockapp->geom.x=dockapp->tile_geom.x;
            dockapp->geom.y=dockapp->tile_geom.y;
        }
        /* }}} */

        /* Return geom if required {{{ */
        if((WRegion *)dockapp->cwin==reg && geomret){
            *geomret=dockapp->geom;
        }
        /* }}} */

        /* Fit dockapp to new geom if required {{{ */
        if(!(flags&REGION_RQGEOM_TRYONLY)){
            region_fit((WRegion *)dockapp->cwin, &dockapp->geom, 
                       REGION_FIT_BOUNDS);
        }
        /* }}} */

    }
    /* }}} */

    /* Reshape the dock if required {{{ */
    if(shape_extension && !(flags&REGION_RQGEOM_TRYONLY)){
        dock_reshape(dock);
    }
    /* }}} */

}
/* }}} */

/* dock_is_mapped {{{ */
/*EXTL_DOC
 * Is \var{dock} mapped? This is much faster than calling
 * \fnref{WDock.get}\code{(dock).is_mapped}.
 */
EXTL_EXPORT_MEMBER
bool dock_is_mapped(WDock *dock)
{

    return REGION_IS_MAPPED(dock);

}
/* }}} */

/* dock_map {{{ */
/*EXTL_DOC
 * Map (show) \var{dock}. This is much faster than calling
 * \fnref{WDock.set}\code{(dock, \{is_mapped=true\})}.
 */
EXTL_EXPORT_MEMBER
void dock_map(WDock *dock)
{

    window_map((WWindow *)dock);

}
/* }}} */

/* dock_unmap {{{ */
/*EXTL_DOC
 * Unmap (hide) \var{dock}. This is much faster than calling
 * \fnref{WDock.set}\code{(dock, \{is_mapped=false\})}.
 */
EXTL_EXPORT_MEMBER
void dock_unmap(WDock *dock)
{

    window_unmap((WWindow *)dock);

}
/* }}} */

/* dock_toggle {{{ */
/*EXTL_DOC
 * Toggle \var{dock}'s mapped (shown/hidden) state.
 */
EXTL_EXPORT_MEMBER
void dock_toggle(WDock *dock)
{

    if(dock_is_mapped(dock)){
        dock_unmap(dock);
    }else{
        dock_map(dock);
    }

}
/* }}} */

/* dock_draw {{{ */
static void dock_draw(WDock *dock, bool complete UNUSED)
{
    
    if(!dock_is_mapped(dock)){
        return;
    }

    XClearWindow(ioncore_g.dpy, ((WWindow *)dock)->win);

    /* Draw border(s) {{{ */
    if(dock->brush){
        int outline_style;

        dock_get_outline_style(dock, &outline_style);
        switch(outline_style){
        case DOCK_OUTLINE_STYLE_NONE:
            break;
        case DOCK_OUTLINE_STYLE_ALL: /* {{{ */
            {
                WRectangle geom=REGION_GEOM(dock);
                geom.x=geom.y=0;
                grbrush_draw_border(dock->brush, ((WWindow *)dock)->win, &geom,
                                    "dock");
            }
            break;
            /* }}} */
        case DOCK_OUTLINE_STYLE_EACH: /* {{{ */
            {
                WDockApp *dockapp;
                for(dockapp=dock->dockapps; dockapp!=NULL;
                    dockapp=dockapp->next){
                    grbrush_draw_border(dock->brush, ((WWindow *)dock)->win,
                                        &dockapp->tile_geom, "dock");
                }
            }
            break;
            /* }}} */
        }
    }
    /* }}} */

}
/* }}} */

/* dock_resize {{{ */
/*EXTL_DOC
 * Resizes and refreshes \var{dock}.
 */
EXTL_EXPORT_MEMBER
void dock_resize(WDock *dock)
{

    dock_request_managed_geom(dock, NULL, 0, NULL, NULL);
    dock_draw(dock, TRUE);

}
/* }}} */

/* dock_brush_release {{{ */
static void dock_brush_release(WDock *dock)
{

    if(dock->brush){
        grbrush_release(dock->brush, ((WWindow *)dock)->win);
        dock->brush=NULL;
    }

}
/* }}} */

/* dock_brush_get {{{ */
static void dock_brush_get(WDock *dock)
{

    dock_brush_release(dock);
    dock->brush=gr_get_brush(region_rootwin_of((WRegion*)dock), 
                             ((WWindow *)dock)->win, "dock");
}
/* }}} */

/* dock_draw_config_updated {{{ */
static void dock_draw_config_updated(WDock *dock)
{

    dock_brush_get(dock);
    dock_resize(dock);

}
/* }}} */

/* dock_destroy {{{ */
/*EXTL_DOC
 * Destroys \var{dock} if it is empty.
 */
EXTL_EXPORT_MEMBER
void dock_destroy(WDock *dock)
{

    if(dock->managed_list!=NULL){
        warn_obj(modname, "Dock \"%s\" is still managing other objects "
                " -- refusing to close.", region_name((WRegion *)dock));
        return;
    }

    destroy_obj((Obj *)dock);

}
/* }}} */

/* dock_set {{{ */
/*EXTL_DOC
 * Configure \var{dock}. \var{conftab} is a table of key/value pairs:
 *
 * \begin{tabularx}{\linewidth}{llX}
 *  \hline
 *  Key & Values & Description \\
 *  \hline
 *  \var{name} & string & Name of dock \\
 *  \var{hpos} & left/center/right & Horizontal position \\
 *  \var{vpos} & top/middle/bottom & Vertical position \\
 *  \var{grow} & up/down/left/right & 
 *       Growth direction where new dockapps are added) \\
 *  \var{is_auto} & bool & 
 *        Should \var{dock} automatically manage new dockapps? \\
 *  \var{is_mapped} & bool & Is \var{dock} mapped? \\
 * \end{tabularx}
 *
 * Any parameters not explicitly set in \var{conftab} will be left unchanged.
 */
EXTL_EXPORT_MEMBER
void dock_set(WDock *dock, ExtlTab conftab)
{
    char *s;
    bool b;
    bool resize=FALSE;

    /* Configure name {{{ */
    if(extl_table_gets_s(conftab, dock_param_name.key, &s)){
        if(!region_set_name((WRegion *)dock, s)){
            warn_obj(modname, "Can't set name to \"%s\"", s);
        }
        free(s);
    }
    /* }}} */

    if(dock_param_extl_table_set(&dock_param_hpos, conftab, &dock->hpos)){
        resize=TRUE;
    }
    if(dock_param_extl_table_set(&dock_param_vpos, conftab, &dock->vpos)){
        resize=TRUE;
    }
    if(dock_param_extl_table_set(&dock_param_grow, conftab, &dock->grow)){
        resize=TRUE;
    }

    /* Configure is_auto {{{ */
    if(extl_table_gets_b(conftab, dock_param_is_auto.key, &b)){
        dock->is_auto=b;
    }
    /* }}} */

    /* Configure is_mapped {{{ */
    if(extl_table_gets_b(conftab, dock_param_is_mapped.key, &b)){
        if(b){
            dock_map(dock);
        }else{
            dock_unmap(dock);
        }
    }
    /* }}} */

    if(resize){
        dock_resize(dock);
    }

}
/* }}} */

/* dock_get {{{ */
/*EXTL_DOC
 * Get \var{dock}'s configuration table. See \fnref{WDock.set} for a
 * description of the table.
 */
EXTL_EXPORT_MEMBER
ExtlTab dock_get(WDock *dock)
{
    ExtlTab conftab;

    conftab=extl_create_table();
    extl_table_sets_s(conftab, dock_param_name.key,
                      region_name((WRegion *)dock));
    dock_param_extl_table_get(&dock_param_hpos, conftab, dock->hpos);
    dock_param_extl_table_get(&dock_param_vpos, conftab, dock->vpos);
    dock_param_extl_table_get(&dock_param_grow, conftab, dock->grow);
    extl_table_sets_b(conftab, dock_param_is_auto.key, dock->is_auto);
    extl_table_sets_b(conftab, dock_param_is_mapped.key, dock_is_mapped(dock));

    return conftab;

}
/* }}} */

/* dock_init {{{ */
static bool dock_init(WDock *dock, int screen, ExtlTab conftab)
{
    WFitParams fp={{-1, -1, 1, 1}, REGION_FIT_EXACT};
    WScreen *scr;
    WWindow *parent;

    /* Find screen {{{ */
    scr=ioncore_find_screen_id(screen);
    if(!scr){
        warn_obj(modname, "Unknown screen %d", screen);
        return FALSE;
    }
    parent=(WWindow *)scr;
    /* }}} */

    /* Initialise member variables {{{ */
    dock->vpos=dock_param_vpos.dflt;
    dock->hpos=dock_param_hpos.dflt;
    dock->grow=dock_param_grow.dflt;
    dock->is_auto=dock_param_is_auto.dflt;
    dock->brush=NULL;
    dock->dockapps=NULL;
    /* }}} */

    if(!window_init_new((WWindow *)dock, parent, &fp)){
        return FALSE;
    }
    
    ((WRegion*)dock)->flags|=REGION_SKIP_FOCUS;

    region_keep_on_top((WRegion *)dock);

    XSelectInput(ioncore_g.dpy, ((WWindow *)dock)->win,
                 EnterWindowMask|ExposureMask|FocusChangeMask|KeyPressMask
                 |SubstructureRedirectMask);

    dock_brush_get(dock);

    dock_set(dock, conftab);

    /* Special case: if is_mapped is default TRUE but is not specified in
     * conftab then the dock will not have been mapped by dock_set so we need
     * to do it explicitly.
     */
    if(dock_param_is_mapped.dflt
       && !extl_table_gets_b(conftab, dock_param_is_mapped.key, NULL)){
        dock_map(dock);
    }

    LINK_ITEM(docks, dock, dock_next, dock_prev);

    return TRUE;

}
/* }}} */

/* dock_deinit {{{ */
static void dock_deinit(WDock *dock)
{

    UNLINK_ITEM(docks, dock, dock_next, dock_prev);

    dock_brush_release(dock);

    window_deinit((WWindow *) dock);

}
/* }}} */

/* create_dock {{{ */
/*EXTL_DOC
 * Create a dock. \var{screen} is the screen number on which the dock should
 * appear. \var{conftab} is the initial configuration table passed to
 * \fnref{WDock.set}.
 */
EXTL_EXPORT_AS(mod_dock, create_dock)
WDock *create_dock(int screen, ExtlTab conftab)
{

    CREATEOBJ_IMPL(WDock, dock, (p, screen, conftab));

}
/* }}} */

/* dock_manage_clientwin {{{ */
static bool dock_manage_clientwin(WDock *dock, WClientWin *cwin,
                                  const WManageParams *param UNUSED,
                                  int redir)
{
    WDockApp *dockapp, *before_dockapp;
    WRectangle geom;

    if(redir==MANAGE_REDIR_STRICT_YES)
        return FALSE;
    
    /* Create and initialise a new WDockApp struct {{{ */
    dockapp=ALLOC(WDockApp);
    dockapp->cwin=cwin;
    dockapp->draw_border=TRUE;
    extl_table_gets_b(cwin->proptab, CLIENTWIN_WINPROP_BORDER,
                      &dockapp->draw_border);
    /* }}} */

    /* Insert the dockapp at the correct relative position {{{ */
    extl_table_gets_i(cwin->proptab, CLIENTWIN_WINPROP_POSITION, &dockapp->pos);
    before_dockapp=dock->dockapps;
    for(before_dockapp=dock->dockapps;
        before_dockapp!=NULL && dockapp->pos>=before_dockapp->pos;
        before_dockapp=before_dockapp->next){
    }
    if(before_dockapp!=NULL){
        LINK_ITEM_BEFORE(dock->dockapps, before_dockapp, dockapp, next, prev);
    }else{
        LINK_ITEM(dock->dockapps, dockapp, next, prev);
    }
    /* }}} */

    region_set_manager((WRegion *)cwin, (WRegion *)dock,
                       &(dock->managed_list));
    dock_request_managed_geom(dock, (WRegion *)cwin, REGION_RQGEOM_TRYONLY,
                              NULL, &geom);
    region_reparent((WRegion *)cwin, (WWindow *)dock, &geom, 
                    REGION_FIT_BOUNDS);
    dock_resize(dock);
    region_map((WRegion *)cwin);

    return TRUE;

}
/* }}} */

/* dock_remove_managed {{{ */
static void dock_remove_managed(WDock *dock, WRegion *reg)
{

    WDockApp *dockapp=dock_find_dockapp(dock, reg);
    if(dockapp!=NULL){
        UNLINK_ITEM(dock->dockapps, dockapp, next, prev);
        free(dockapp);
    }

    region_unset_manager(reg, (WRegion *)dock, &(dock->managed_list));

    dock_resize(dock);

}
/* }}} */

static DynFunTab dock_dynfuntab[]={ /* {{{ */
    {window_draw, dock_draw},
    {region_draw_config_updated, dock_draw_config_updated},
    {region_request_managed_geom, dock_request_managed_geom},
    {(DynFun *)region_manage_clientwin, (DynFun *)dock_manage_clientwin},
    {region_remove_managed, dock_remove_managed},
    END_DYNFUNTAB
};
/* }}} */

IMPLCLASS(WDock, WWindow, dock_deinit, dock_dynfuntab);

/* }}} */


/* Module {{{ */

/* dock_clientwin_is_dockapp {{{ */
static bool dock_clientwin_is_dockapp(WClientWin *cwin,
                                      const WManageParams *param)
{
    bool is_dockapp=FALSE;

    /* First, inspect the WManageParams.dockapp parameter {{{ */
    if(param->dockapp){
        is_dockapp=TRUE;
    }
    /* }}} */

    /* Second, inspect the _NET_WM_WINDOW_TYPE property {{{ */
    if(!is_dockapp){
        static Atom atom__net_wm_window_type=None;
        static Atom atom__net_wm_window_type_dock=None;
        Atom actual_type=None;
        int actual_format;
        unsigned long nitems;
        unsigned long bytes_after;
        unsigned char *prop;

        if(atom__net_wm_window_type==None){
            atom__net_wm_window_type=XInternAtom(ioncore_g.dpy,
                                                 "_NET_WM_WINDOW_TYPE",
                                                 False);
        }
        if(atom__net_wm_window_type_dock==None){
            atom__net_wm_window_type_dock=XInternAtom(ioncore_g.dpy,
                                                     "_NET_WM_WINDOW_TYPE_DOCK",
                                                      False);
        }
        XGetWindowProperty(ioncore_g.dpy, cwin->win, atom__net_wm_window_type, 0,
                           sizeof(Atom), False, XA_ATOM, &actual_type,
                           &actual_format, &nitems, &bytes_after, &prop);
        if(actual_type==XA_ATOM && nitems>=1
           && *(Atom *)prop==atom__net_wm_window_type_dock){
            is_dockapp=TRUE;
        }
        XFree(prop);
    }
    /* }}} */

    /* Third, inspect the WM_CLASS property {{{ */
    if(!is_dockapp){
        char **p;
        int n;

        p=xwindow_get_text_property(cwin->win, XA_WM_CLASS, &n);
        if(p!=NULL){
            if(n>=2 && strcmp(p[1], "DockApp")==0){
                is_dockapp=TRUE;
            }
            XFreeStringList(p);
        }
    }
    /* }}} */

    return is_dockapp;

}
/* }}} */

/* dock_find_suitable_dock {{{ */
static WDock *dock_find_suitable_dock(WClientWin *cwin,
                                      const WManageParams *param)
{
    WDock *dock;
    WScreen *cwin_scr;

    cwin_scr=clientwin_find_suitable_screen(cwin, param);
    for(dock=docks; dock; dock=dock->dock_next)
    {
        if(dock->is_auto && region_screen_of((WRegion *)dock)==cwin_scr){
            break;
        }
    }

    return dock;

}
/* }}} */

/* clientwin_do_manage_hook {{{ */
static bool clientwin_do_manage_hook(WClientWin *cwin, const WManageParams *param)
{
    WDock *dock;

    if(!dock_clientwin_is_dockapp(cwin, param)){
        return FALSE;
    }

    dock=dock_find_suitable_dock(cwin, param);
    if(!dock){
        return FALSE;
    }

    return region_manage_clientwin((WRegion *)dock, cwin, param,
                                   MANAGE_REDIR_PREFER_NO);
}

/* }}} */

/* mod_dock_init {{{ */
bool mod_dock_init()
{
    extern bool mod_dock_register_exports();

    if(XShapeQueryExtension(ioncore_g.dpy, &shape_event_basep,
                            &shape_error_basep)){
        shape_extension=TRUE;
    }else{
        XMissingExtension(ioncore_g.dpy, "SHAPE");
    }

    if(!mod_dock_register_exports()){
        return FALSE;
    }

    ioncore_read_config(modname, NULL, TRUE);

    ADD_HOOK(clientwin_do_manage_alt, clientwin_do_manage_hook);

    return TRUE;

}
/* }}} */

/* mod_dock_deinit {{{ */
void mod_dock_deinit()
{
    WDock *dock;
    extern void mod_dock_unregister_exports();

    REMOVE_HOOK(clientwin_do_manage_alt, clientwin_do_manage_hook);

    /* Destroy all docks {{{ */
    dock=docks;
    while(dock!=NULL){
        WDock *next=dock->dock_next;
        destroy_obj((Obj *)dock);
        dock=next;
    }
    /* }}} */

    mod_dock_unregister_exports();

}
/* }}} */

/* }}} */
