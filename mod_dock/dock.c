/*
 * Ion dock module
 * Copyright (C) 2003 Tom Payne
 * Copyright (C) 2003 Per Olofsson
 * Copyright (C) 2004 Tuomo Valkonen
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


/*{{{ Includes */

#include <limits.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xext.h>

#include <libtu/objp.h>
#include <libtu/map.h>
#include <ioncore/common.h>
#include <ioncore/clientwin.h>
#include <ioncore/eventh.h>
#include <ioncore/extl.h>
#include <ioncore/global.h>
#include <ioncore/manage.h>
#include <ioncore/names.h>
#include <ioncore/property.h>
#include <ioncore/readconfig.h>
#include <ioncore/resize.h>
#include <ioncore/stacking.h>
#include <ioncore/window.h>
#include <ioncore/region-iter.h>
#include <ioncore/mplex.h>
#include <ioncore/saveload.h>

/*}}}*/


/*{{{ Macros */

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

/*}}}*/


/*{{{ Global variables */

#include "../version.h"

static const char *modname="dock";
const char mod_dock_ion_api_version[]=ION_API_VERSION;
bool shape_extension=FALSE;
int shape_event_basep=0;
int shape_error_basep=0;

/*}}}*/


/*{{{ Classes */

INTRSTRUCT(WDockParam);
INTRSTRUCT(WDockApp);
INTRCLASS(WDock);

DECLSTRUCT(WDockParam){
    const char *key;
    const char *desc;
    const StringIntMap *map;
    int dflt;
};

DECLSTRUCT(WDockApp){
    WDockApp *next, *prev;
    WClientWin *cwin;
    int pos;
    bool draw_border;
    bool tile;
    WRectangle geom;
    WRectangle tile_geom;
    WRectangle border_geom;
};

DECLCLASS(WDock){
    WWindow win;
    WDock *dock_next, *dock_prev;
    WRegion *managed_list;
    int vpos, hpos;
    int grow;
    bool is_auto;
    GrBrush *brush;
    WDockApp *dockapps;
    
    int min_w, min_h;
    int max_w, max_h;
    
    bool arrange_called;
};

static WDock *docks=NULL;

/*}}}*/


/*{{{ Parameter conversion */

static void dock_param_extl_table_get(const WDockParam *param, 
                                      ExtlTab conftab, int value)
{
    const char *s;

    s=stringintmap_key(param->map, value, NULL);
    if(s){
        extl_table_sets_s(conftab, param->key, s);
    }

}


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


static bool dock_param_extl_table_set(const WDockParam *param, ExtlTab conftab,
                                      int *ret)
{
    char *s;

    if(extl_table_gets_s(conftab, param->key, &s))
        return dock_param_do_set(param, s, ret);

    return FALSE;

}


static bool dock_param_brush_set(const WDockParam *param, GrBrush *brush,
                                 int *ret)
{
    char *s;

    if(grbrush_get_extra(brush, param->key, 's', &s))
        return dock_param_do_set(param, s, ret);

    return FALSE;

}

/*}}}*/


/*{{{ Parameter descriptions */

static const WDockParam dock_param_name={
    "name",
    "name",
    NULL,
    0
};


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


static const WDockParam dock_param_is_auto={
    "is_auto",
    "is automatic",
    NULL,
    TRUE
};


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


static const WDockParam dock_param_tile_width={
    "width",
    "tile width",
    NULL,
    64
};

static const WDockParam dock_param_tile_height={
    "height",
    "tile height",
    NULL,
    64
};


/*}}}*/


/*{{{ Misc. */

#define CLIENTWIN_WINPROP_POSITION "dockposition"
#define CLIENTWIN_WINPROP_BORDER "dockborder"

static WDockApp *dock_find_dockapp(WDock *dock, WRegion *reg)
{
    WDockApp *dockapp;

    for(dockapp=dock->dockapps; dockapp!=NULL; dockapp=dockapp->next){
        if((WRegion*)dockapp->cwin==reg){
            return dockapp;
        }
    }

    return NULL;

}


static void dock_get_outline_style(WDock *dock, int *ret)
{

    *ret=dock_param_outline_style.dflt;
    if(dock->brush!=NULL)
        dock_param_brush_set(&dock_param_outline_style, dock->brush, ret);

}

/*}}}*/


/*{{{ Size calculation */


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


static void dock_get_hpos_vpos_grow(WDock *dock, int *hpos, int *vpos,
                                    int *grow)
{
    WMPlex *mplex=OBJ_CAST(REGION_PARENT(dock), WMPlex);
    WRegion *mplex_stdisp;
    int orientation, corner;
    
    if(mplex!=NULL){
        mplex_get_stdisp(mplex, &mplex_stdisp, &corner, &orientation);
        if(mplex_stdisp==(WRegion*)dock){
            bool br_grow=(dock->grow==DOCK_GROW_DOWN ||
                          dock->grow==DOCK_GROW_RIGHT);
            /* Ok, we're assigned as a status display for mplex, so
             * get parameters from there.
             */
            *hpos=((corner==MPLEX_STDISP_TL || corner==MPLEX_STDISP_BL)
                   ? DOCK_HPOS_LEFT
                   : DOCK_HPOS_RIGHT);
            *vpos=((corner==MPLEX_STDISP_TL || corner==MPLEX_STDISP_TR)
                   ? DOCK_VPOS_TOP
                   : DOCK_VPOS_BOTTOM);
            *grow=(orientation==REGION_ORIENTATION_HORIZONTAL
                   ? (br_grow ? DOCK_GROW_RIGHT : DOCK_GROW_LEFT)
                   : (br_grow ? DOCK_GROW_DOWN : DOCK_GROW_UP));
            return;
        }
    }
    
    *grow=dock->grow;
    *hpos=dock->hpos;
    *vpos=dock->vpos;
}



static void dock_reshape(WDock *dock)
{
    int outline_style;
    
    if(!shape_extension){
        return;
    }
    
    dock_get_outline_style(dock, &outline_style);
    
    switch(outline_style){
    case DOCK_OUTLINE_STYLE_NONE:
    case DOCK_OUTLINE_STYLE_EACH:
        {
            WDockApp *dockapp;
            
            /* Start with an empty set */
            XShapeCombineRectangles(ioncore_g.dpy, ((WWindow*)dock)->win,
                                    ShapeBounding, 0, 0, NULL, 0, ShapeSet, 0);
            
            /* Union with dockapp shapes */
            for(dockapp=dock->dockapps; dockapp!=NULL; dockapp=dockapp->next){
                if(outline_style==DOCK_OUTLINE_STYLE_EACH
                   && dockapp->draw_border){
                    /* Union with border shape */
                    XRectangle tile_rect;
                    
                    tile_rect.x=dockapp->border_geom.x;
                    tile_rect.y=dockapp->border_geom.y;
                    tile_rect.width=dockapp->border_geom.w;
                    tile_rect.height=dockapp->border_geom.h;
                    XShapeCombineRectangles(ioncore_g.dpy, ((WWindow*)dock)->win,
                                            ShapeBounding, 0, 0, &tile_rect, 1,
                                            ShapeUnion, 0);
                }else{
                    /* Union with dockapp shape */
                    int count;
                    int ordering;
                    
                    XRectangle *rects=XShapeGetRectangles(ioncore_g.dpy,
                                                          dockapp->cwin->win,
                                                          ShapeBounding, &count,
                                                          &ordering);
                    if(rects!=NULL){
                        WRectangle dockapp_geom=REGION_GEOM(dockapp->cwin);
                        XShapeCombineRectangles(ioncore_g.dpy, ((WWindow*)dock)->win,
                                                ShapeBounding,
                                                dockapp_geom.x, dockapp_geom.y,
                                                rects, count, ShapeUnion, ordering);
                        XFree(rects);
                    }
                }
            }
        }
        break;
        
    case DOCK_OUTLINE_STYLE_ALL:
        {
            WRectangle geom;
            XRectangle rect;
            
            geom=REGION_GEOM(dock);
            rect.x=0;
            rect.y=0;
            rect.width=geom.w;
            rect.height=geom.h;
            XShapeCombineRectangles(ioncore_g.dpy, ((WWindow*)dock)->win,
                                    ShapeBounding, 0, 0, &rect, 1, ShapeSet, 0);
        }
        break;
    }
    
}


static void dock_arrange_dockapps(WDock *dock, const WRectangle *bd_dockg, 
                                  const WDockApp *replace_this, 
                                  WDockApp *with_this)
{
    GrBorderWidths dock_bdw, dockapp_bdw;
    WDockApp dummy_copy, *dockapp;
    int hpos, vpos, grow, cur_coord=0;
    WRectangle dock_geom;

    dock->arrange_called=TRUE;
    
    dock_get_hpos_vpos_grow(dock, &hpos, &vpos, &grow);

    /* Determine dock and dockapp border widths */
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
    
    dock_geom.w=bd_dockg->w-dock_bdw.left-dock_bdw.right;
    dock_geom.h=bd_dockg->h-dock_bdw.top-dock_bdw.bottom;

    /* Calculate initial co-ordinate for layout algorithm */
    switch(grow){
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

    /* Arrange dockapps */
    for(dockapp=dock->dockapps; dockapp!=NULL; dockapp=dockapp->next){
        WDockApp *da=dockapp;
        
        if(replace_this!=NULL){
            if(replace_this==dockapp){
                da=with_this;
            }else{
                dummy_copy=*dockapp;
                da=&dummy_copy;
            }
        }
            
        /* Calculate first co-ordinate */
        switch(grow){
        case DOCK_GROW_UP:
        case DOCK_GROW_DOWN:
            switch(hpos){
            case DOCK_HPOS_LEFT:
                da->border_geom.x=0;
                break;
            case DOCK_HPOS_CENTER:
                da->border_geom.x=(dock_geom.w-da->border_geom.w)/2;
                break;
            case DOCK_HPOS_RIGHT:
                da->border_geom.x=dock_geom.w-da->border_geom.w;
                break;
            }
            da->border_geom.x+=dock_bdw.left;
            break;
        case DOCK_GROW_LEFT:
        case DOCK_GROW_RIGHT:
            switch(vpos){
            case DOCK_VPOS_TOP:
                da->border_geom.y=0;
                break;
            case DOCK_VPOS_MIDDLE:
                da->border_geom.y=(dock_geom.h-da->border_geom.h)/2;
                break;
            case DOCK_VPOS_BOTTOM:
                da->border_geom.y=dock_geom.h-da->border_geom.h;
                break;
            }
            da->border_geom.y+=dock_bdw.top;
            break;
        }

        /* Calculate second co-ordinate */
        switch(grow){
        case DOCK_GROW_UP:
            cur_coord-=da->border_geom.h;
            da->border_geom.y=cur_coord;
            cur_coord-=dockapp_bdw.spacing;
            break;
        case DOCK_GROW_DOWN:
            da->border_geom.y=cur_coord;
            cur_coord+=da->border_geom.h+dockapp_bdw.spacing;
            break;
        case DOCK_GROW_LEFT:
            cur_coord-=da->border_geom.w;
            da->border_geom.x=cur_coord;
            cur_coord-=dockapp_bdw.spacing;
            break;
        case DOCK_GROW_RIGHT:
            da->border_geom.x=cur_coord;
            cur_coord+=da->border_geom.w+dockapp_bdw.spacing;
            break;
        }

        /* Calculate tile geom */
        da->tile_geom.x=da->border_geom.x+dockapp_bdw.left;
        da->tile_geom.y=da->border_geom.y+dockapp_bdw.top;

        /* Calculate dockapp geom */
        if(da->tile){
            da->geom.x=da->tile_geom.x+(da->tile_geom.w-da->geom.w)/2;
            da->geom.y=da->tile_geom.y+(da->tile_geom.h-da->geom.h)/2;
        }else{
            da->geom.x=da->tile_geom.x;
            da->geom.y=da->tile_geom.y;
        }
        
        if(replace_this==NULL)
            region_fit((WRegion*)da->cwin, &(da->geom), REGION_FIT_BOUNDS);
    }
}


static void dock_managed_rqgeom(WDock *dock, WRegion *reg, int flags,
                                const WRectangle *geom, WRectangle *geomret)
{
    WDockApp *dockapp=NULL, *thisdockapp=NULL, thisdockapp_copy;
    WRectangle parent_geom, dock_geom, border_dock_geom;
    GrBorderWidths dock_bdw, dockapp_bdw;
    int n_dockapps=0, max_w=1, max_h=1, total_w=0, total_h=0;
    int hpos, vpos, grow;
    WRectangle tile_size;
    
    /* dock_resize calls with NULL parameters. */
    assert(reg!=NULL || (geomret==NULL && !(flags&REGION_RQGEOM_TRYONLY)));
    
    dock_get_hpos_vpos_grow(dock, &hpos, &vpos, &grow);

    /* Determine parent and tile geoms */
    parent_geom=((WRegion*)dock)->parent->geom;
    dock_get_tile_size(dock, &tile_size);

    /* Determine dock and dockapp border widths */
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

    /* Calculate widths and heights */
    for(dockapp=dock->dockapps; dockapp!=NULL; dockapp=dockapp->next){
        WDockApp *da=dockapp;
        if((WRegion*)(dockapp->cwin)==reg){
            thisdockapp=dockapp;
            if(flags&REGION_RQGEOM_TRYONLY){
                thisdockapp_copy=*dockapp;
                thisdockapp_copy.geom=*geom;
                da=&thisdockapp_copy;
            }
            
            da->geom=*geom;
        }
            /* Determine whether dockapp should be placed on a tile */
            da->tile=da->geom.w<=tile_size.w && da->geom.h<=tile_size.h;
            
            /* Calculate width and height */
            if(da->tile){
                da->tile_geom.w=tile_size.w;
                da->tile_geom.h=tile_size.h;
            }else{
                da->tile_geom.w=da->geom.w;
                da->tile_geom.h=da->geom.h;
            }
            
            /* Calculate border width and height */
            da->border_geom.w=dockapp_bdw.left+da->tile_geom.w+dockapp_bdw.right;
            da->border_geom.h=dockapp_bdw.top+da->tile_geom.h+dockapp_bdw.right;
        
        
        /* Calculate maximum and accumulated widths and heights */
        if(da->border_geom.w>max_w)
            max_w=da->border_geom.w;
        total_w+=da->border_geom.w+(n_dockapps ? dockapp_bdw.spacing : 0);
        
        if(da->border_geom.h>max_h)
            max_h=da->border_geom.h;
        total_h+=da->border_geom.h+(n_dockapps ? dockapp_bdw.spacing : 0);
        
        /* Count dockapps */
        ++n_dockapps;
    }
    
    if(thisdockapp==NULL && reg!=NULL){
        WARN_FUNC("Requesting dockapp not found.");
        if(geomret)
            *geomret=REGION_GEOM(reg);
        return;
    }
    
    /* Calculate width and height of dock */
    if(n_dockapps){
        switch(grow){
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
    
    switch(hpos){
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
    
    switch(vpos){
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
    
    
    /* Fit dock to new geom if required */
    if(!(flags&REGION_RQGEOM_TRYONLY)){
        WRectangle *g=&border_dock_geom;
        dock->min_w=g->w;
        dock->min_h=g->h;
        if(grow==DOCK_GROW_UP || grow==DOCK_GROW_DOWN){
            dock->max_w=g->w;
            dock->max_h=INT_MAX;
        }else{
            dock->max_w=INT_MAX;
            dock->max_h=g->h;
        }
        
        dock->arrange_called=FALSE;
        region_rqgeom((WRegion*)dock, REGION_RQGEOM_NORMAL, g, NULL);
        if(!dock->arrange_called)
            dock_arrange_dockapps(dock, &REGION_GEOM(dock), NULL, NULL);
        if(geomret!=NULL)
            *geomret=thisdockapp->geom;
    }else{
        dock_arrange_dockapps(dock, &border_dock_geom, thisdockapp,
                              &thisdockapp_copy);
        if(geomret!=NULL)
            *geomret=thisdockapp_copy.geom;
    }
}



void dock_size_hints(WDock *dock, XSizeHints *hints, int *relw, int *relh)
{
    *relw=REGION_GEOM(dock).w;
    *relh=REGION_GEOM(dock).h;
    hints->flags=PMinSize|PMaxSize;
    hints->min_width=dock->min_w;
    hints->min_height=dock->min_h;
    hints->max_width=dock->max_w;
    hints->max_height=dock->max_h;
}


static bool dock_fitrep(WDock *dock, WWindow *parent, const WFitParams *fp)
{
    if(!window_fitrep(&(dock->win), parent, fp))
        return FALSE;
    
    dock_arrange_dockapps(dock, &(fp->g), NULL, NULL);
    
    if(shape_extension)
        dock_reshape(dock);
    
    return TRUE;
}

/*}}}*/


/*{{{ Drawing */
static void dock_draw(WDock *dock, bool complete UNUSED)
{
    
    XClearWindow(ioncore_g.dpy, ((WWindow*)dock)->win);
    
    if(dock->brush){
        int outline_style;
        
        dock_get_outline_style(dock, &outline_style);
        switch(outline_style){
        case DOCK_OUTLINE_STYLE_NONE:
            break;
        case DOCK_OUTLINE_STYLE_ALL:
            {
                WRectangle geom=REGION_GEOM(dock);
                geom.x=geom.y=0;
                grbrush_draw_border(dock->brush, ((WWindow*)dock)->win, &geom,
                                    "dock");
            }
            break;
        case DOCK_OUTLINE_STYLE_EACH:
            {
                WDockApp *dockapp;
                for(dockapp=dock->dockapps; dockapp!=NULL;
                    dockapp=dockapp->next){
                    grbrush_draw_border(dock->brush, ((WWindow*)dock)->win,
                                        &dockapp->tile_geom, "dock");
                }
            }
            break;
        }
    }
    
}


/*EXTL_DOC
 * Resizes and refreshes \var{dock}.
 */
EXTL_EXPORT_MEMBER
void dock_resize(WDock *dock)
{
    dock_managed_rqgeom(dock, NULL, 0, NULL, NULL);
    dock_draw(dock, TRUE);
    
}

static void dock_brush_release(WDock *dock)
{
    
    if(dock->brush){
        grbrush_release(dock->brush, ((WWindow*)dock)->win);
        dock->brush=NULL;
    }

}


static void dock_brush_get(WDock *dock)
{

    dock_brush_release(dock);
    dock->brush=gr_get_brush(region_rootwin_of((WRegion*)dock), 
                             ((WWindow*)dock)->win, "dock");
}


static void dock_updategr(WDock *dock)
{
    dock_brush_get(dock);
    dock_resize(dock);
}

/*}}}*/


/*{{{ Set/get */


/*EXTL_DOC
 * Configure \var{dock}. \var{conftab} is a table of key/value pairs:
 *
 * \begin{tabularx}{\linewidth}{llX}
 *  \hline
 *  Key & Values & Description \\
 *  \hline
 *  \var{name} & string & Name of dock \\
 *  %\var{hpos} & left/center/right & Horizontal position \\
 *  %\var{vpos} & top/middle/bottom & Vertical position \\
 *  \var{grow} & up/down/left/right & 
 *       Growth direction where new dockapps are added) \\
 *  \var{is_auto} & bool & 
 *        Should \var{dock} automatically manage new dockapps? \\
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

    if(extl_table_gets_s(conftab, dock_param_name.key, &s)){
        if(!region_set_name((WRegion*)dock, s)){
            warn_obj(modname, "Can't set name to \"%s\"", s);
        }
        free(s);
    }

    if(dock_param_extl_table_set(&dock_param_hpos, conftab, &dock->hpos)){
        resize=TRUE;
    }
    if(dock_param_extl_table_set(&dock_param_vpos, conftab, &dock->vpos)){
        resize=TRUE;
    }
    if(dock_param_extl_table_set(&dock_param_grow, conftab, &dock->grow)){
        resize=TRUE;
    }
    
    if(extl_table_gets_b(conftab, dock_param_is_auto.key, &b)){
        dock->is_auto=b;
    }

    if(resize)
        dock_resize(dock);
}


static void dock_do_get(WDock *dock, ExtlTab conftab)
{
    extl_table_sets_s(conftab, dock_param_name.key,
                      region_name((WRegion*)dock));
    dock_param_extl_table_get(&dock_param_hpos, conftab, dock->hpos);
    dock_param_extl_table_get(&dock_param_vpos, conftab, dock->vpos);
    dock_param_extl_table_get(&dock_param_grow, conftab, dock->grow);
    extl_table_sets_b(conftab, dock_param_is_auto.key, dock->is_auto);
}


/*EXTL_DOC
 * Get \var{dock}'s configuration table. See \fnref{WDock.set} for a
 * description of the table.
 */
EXTL_EXPORT_MEMBER
ExtlTab dock_get(WDock *dock)
{
    ExtlTab conftab;

    conftab=extl_create_table();
    dock_do_get(dock, conftab);
    return conftab;
}


/*}}}*/


/*{{{ Init/deinit */

static bool dock_init(WDock *dock, WWindow *parent, const WFitParams *fp)
{
    dock->vpos=dock_param_vpos.dflt;
    dock->hpos=dock_param_hpos.dflt;
    dock->grow=dock_param_grow.dflt;
    dock->is_auto=dock_param_is_auto.dflt;
    dock->brush=NULL;
    dock->dockapps=NULL;
    dock->min_w=1;
    dock->min_h=1;
    dock->max_w=1;
    dock->max_h=1;
    dock->arrange_called=FALSE;

    if(!window_init((WWindow*)dock, parent, fp))
        return FALSE;
    
    ((WRegion*)dock)->flags|=REGION_SKIP_FOCUS;

    XSelectInput(ioncore_g.dpy, ((WWindow*)dock)->win,
                 EnterWindowMask|ExposureMask|FocusChangeMask|KeyPressMask
                 |SubstructureRedirectMask);

    dock_brush_get(dock);

    LINK_ITEM(docks, dock, dock_next, dock_prev);

    return TRUE;

}

static void dock_deinit(WDock *dock)
{

    UNLINK_ITEM(docks, dock, dock_next, dock_prev);

    dock_brush_release(dock);

    window_deinit((WWindow*) dock);

}

static WDock *create_dock(WWindow *parent, const WFitParams *fp)
{

    CREATEOBJ_IMPL(WDock, dock, (p, parent, fp));

}


/* EXTL_DOC
 * Create an unmanaged dock. \var{screen} is the screen number on which the 
 * dock should appear. \var{conftab} is the initial configuration table passed
 * to \fnref{WDock.set}.
 */
/*EXTL_EXPORT_AS(mod_dock, create_dock)
static WDock *create_dock_unmanaged(int screen, ExtlTab conftab)
{
    WScreen *scr=ioncore_find_screen_id(scr);
    WFitParams fp={{-1, -1, 1, 1}, REGION_FIT_EXACT};
    WDock *dock;
    
    if(!scr){
        warn_obj(modname, "Unknown screen %d", screen);
        return NULL;
    } 
    
    dock=create_dock((WWindow*)scr, &fp);

    if(dock==NULL){
        warn("Unable to create dock.");
        return NULL;
    }
    
    dock_set(dock, conftab);
    
    region_keep_on_top((WRegion*)dock);
    
    return dock;
}*/


/*EXTL_DOC
 * Destroys \var{dock} if it is empty.
 */
EXTL_EXPORT_MEMBER
void dock_destroy(WDock *dock)
{

    if(dock->managed_list!=NULL){
        warn_obj(modname, "Dock \"%s\" is still managing other objects "
                " -- refusing to close.", region_name((WRegion*)dock));
        return;
    }

    destroy_obj((Obj*)dock);

}


/*}}}*/


/*{{{ Save/load */


ExtlTab dock_get_configuration(WDock *dock)
{
    ExtlTab tab;
    
    tab=region_get_base_configuration((WRegion*)dock);
    dock_do_get(dock, tab);
    
    return tab;
}


WRegion *dock_load(WWindow *par, const WFitParams *fp, ExtlTab tab)
{
    WDock *dock=create_dock(par, fp);
    if(dock!=NULL)
        dock_set(dock, tab);
    
    return (WRegion*)dock;
}


/*}}}*/


/*{{{ Client window management setup */


static bool dock_manage_clientwin(WDock *dock, WClientWin *cwin,
                                  const WManageParams *param UNUSED,
                                  int redir)
{
    WDockApp *dockapp, *before_dockapp;
    WRectangle geom;

    if(redir==MANAGE_REDIR_STRICT_YES)
        return FALSE;
    
    /* Create and initialise a new WDockApp struct */
    dockapp=ALLOC(WDockApp);
    dockapp->cwin=cwin;
    dockapp->draw_border=TRUE;
    extl_table_gets_b(cwin->proptab, CLIENTWIN_WINPROP_BORDER,
                      &dockapp->draw_border);

    /* Insert the dockapp at the correct relative position */
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

    region_set_manager((WRegion*)cwin, (WRegion*)dock,
                       &(dock->managed_list));
    
    geom.x=0;
    geom.y=0;
    geom.w=REGION_GEOM(cwin).w;
    geom.h=REGION_GEOM(cwin).h;
    
    region_reparent((WRegion*)cwin, (WWindow*)dock, &geom, REGION_FIT_BOUNDS);
    
    dock_managed_rqgeom(dock, (WRegion*)cwin, 
                        REGION_RQGEOM_WEAK_X|REGION_RQGEOM_WEAK_Y,
                        &geom, NULL);
    
    region_map((WRegion*)cwin);

    return TRUE;
}


static void dock_managed_remove(WDock *dock, WRegion *reg)
{

    WDockApp *dockapp=dock_find_dockapp(dock, reg);
    if(dockapp!=NULL){
        UNLINK_ITEM(dock->dockapps, dockapp, next, prev);
        free(dockapp);
    }else{
        WARN_FUNC("Dockapp not found.");
    }

    region_unset_manager(reg, (WRegion*)dock, &(dock->managed_list));

    dock_resize(dock);

}


static bool dock_clientwin_is_dockapp(WClientWin *cwin,
                                      const WManageParams *param)
{
    bool is_dockapp=FALSE;

    /* First, inspect the WManageParams.dockapp parameter */
    if(param->dockapp){
        is_dockapp=TRUE;
    }

    /* Second, inspect the _NET_WM_WINDOW_TYPE property */
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
        if(XGetWindowProperty(ioncore_g.dpy, cwin->win, atom__net_wm_window_type,
                              0, sizeof(Atom), False, XA_ATOM, &actual_type,
                              &actual_format, &nitems, &bytes_after, &prop)
           ==Success){
            if(actual_type==XA_ATOM && nitems>=1
               && *(Atom*)prop==atom__net_wm_window_type_dock){
                is_dockapp=TRUE;
            }
            XFree(prop);
        }
    }

    /* Third, inspect the WM_CLASS property */
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

    /* Fourth, inspect the _KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR property */
    if(!is_dockapp){
        static Atom atom__kde_net_wm_system_tray_window_for=None;
        Atom actual_type=None;
        int actual_format;
        unsigned long nitems;
        unsigned long bytes_after;
        unsigned char *prop;

        if(atom__kde_net_wm_system_tray_window_for==None){
            atom__kde_net_wm_system_tray_window_for=XInternAtom(ioncore_g.dpy,
        							"_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR",
        							False);
        }
        if(XGetWindowProperty(ioncore_g.dpy, cwin->win,
                              atom__kde_net_wm_system_tray_window_for, 0,
                              sizeof(Atom), False, AnyPropertyType, 
                              &actual_type, &actual_format, &nitems,
                              &bytes_after, &prop)==Success){
            if(actual_type!=None){
                is_dockapp=TRUE;
            }
            XFree(prop);
        }
    }

    return is_dockapp;

}


static WDock *dock_find_suitable_dock(WClientWin *cwin,
                                      const WManageParams *param)
{
    WDock *dock;
    WScreen *cwin_scr;

    cwin_scr=clientwin_find_suitable_screen(cwin, param);
    for(dock=docks; dock; dock=dock->dock_next)
    {
        if(dock->is_auto && region_screen_of((WRegion*)dock)==cwin_scr){
            break;
        }
    }

    return dock;

}


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

    return region_manage_clientwin((WRegion*)dock, cwin, param,
                                   MANAGE_REDIR_PREFER_NO);
}

/*}}}*/


/*{{{ Module init/deinit */


bool mod_dock_init()
{
    extern bool mod_dock_register_exports();

    if(XShapeQueryExtension(ioncore_g.dpy, &shape_event_basep,
                            &shape_error_basep)){
        shape_extension=TRUE;
    }else{
        XMissingExtension(ioncore_g.dpy, "SHAPE");
    }

    if(!ioncore_register_regclass(&CLASSDESCR(WDock), 
                                  (WRegionLoadCreateFn*)dock_load)){
        return FALSE;
    }

    if(!mod_dock_register_exports()){
        ioncore_unregister_regclass(&CLASSDESCR(WDock));
        return FALSE;
    }

    ioncore_read_config(modname, NULL, TRUE);

    hook_add(clientwin_do_manage_alt, 
             (WHookDummy*)clientwin_do_manage_hook);

    return TRUE;

}


void mod_dock_deinit()
{
    WDock *dock;
    extern void mod_dock_unregister_exports();

    ioncore_unregister_regclass(&CLASSDESCR(WDock));

    hook_remove(clientwin_do_manage_alt, 
                (WHookDummy*)clientwin_do_manage_hook);

    dock=docks;
    while(dock!=NULL){
        WDock *next=dock->dock_next;
        destroy_obj((Obj*)dock);
        dock=next;
    }

    mod_dock_unregister_exports();

}


/*}}}*/


/*{{{ WDock class description and dynfun list */


static DynFunTab dock_dynfuntab[]={
    {window_draw, dock_draw},
    {region_updategr, dock_updategr},
    {region_managed_rqgeom, dock_managed_rqgeom},
    {(DynFun*)region_manage_clientwin, (DynFun*)dock_manage_clientwin},
    {region_managed_remove, dock_managed_remove},
    {(DynFun*)region_get_configuration, (DynFun*)dock_get_configuration},
    {region_size_hints, dock_size_hints},
    {(DynFun*)region_fitrep, (DynFun*)dock_fitrep},
    END_DYNFUNTAB
};

IMPLCLASS(WDock, WWindow, dock_deinit, dock_dynfuntab);


/*}}}*/

