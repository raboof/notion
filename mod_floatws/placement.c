/*
 * ion/mod_floatws/placement.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <ioncore/common.h>
#include <ioncore/region.h>
#include <ioncore/region-iter.h>

#include "placement.h"
#include "floatws.h"
#include "floatframe.h"


static void random_placement(WRectangle box, WRectangle *g)
{
    box.w-=g->w;
    box.h-=g->h;
    g->x=box.x+(box.w<=0 ? 0 : rand()%box.w);
    g->y=box.y+(box.h<=0 ? 0 : rand()%box.h);
}


static void ggeom(WRegion *reg, WRectangle *geom)
{
    *geom=REGION_GEOM(reg);
}


static WRegion* is_occupied(WFloatWS *ws, const WRectangle *r)
{
    WRegion *reg;
    WRectangle p;
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
        ggeom(reg, &p);
        
        if(r->x>=p.x+p.w)
            continue;
        if(r->y>=p.y+p.h)
            continue;
        if(r->x+r->w<=p.x)
            continue;
        if(r->y+r->h<=p.y)
            continue;
        return reg;
    }
    
    return NULL;
}


static int next_least_x(WFloatWS *ws, int x)
{
    WRegion *reg;
    WRectangle p;
    int retx=REGION_GEOM(ws).x+REGION_GEOM(ws).w;
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
        ggeom(reg, &p);
        
        if(p.x+p.w>x && p.x+p.w<retx)
            retx=p.x+p.w;
    }
    
    return retx+1;
}


static int next_lowest_y(WFloatWS *ws, int y)
{
    WRegion *reg;
    WRectangle p;
    int rety=REGION_GEOM(ws).y+REGION_GEOM(ws).h;
    
    FOR_ALL_MANAGED_ON_LIST(ws->managed_list, reg){
        ggeom(reg, &p);
        
        if(p.y+p.h>y && p.y+p.h<rety)
            rety=p.y+p.h;
    }
    
    return rety+1;
}


enum{
    PLACEMENT_LRUD, PLACEMENT_UDLR, PLACEMENT_RANDOM
} placement_method=PLACEMENT_LRUD;


/*EXTL_DOC
 * Set module basic settings. Currently only the \code{placement_method} 
 * parameter is supported.
 *
 * The method can be one  of ''udlr'', ''lrud'' (default) and ''random''. 
 * The ''udlr'' method looks for free space starting from top the top left
 * corner of the workspace moving first down keeping the x coordinate fixed.
 * If it find no free space, it start looking similarly at next x coordinate
 * unoccupied by other objects and so on. ''lrud' is the same but with the 
 * role of coordinates changed and both fall back to ''random'' placement 
 * if no free area was found.
 */
EXTL_EXPORT
void mod_floatws_set(ExtlTab tab)
{
    char *method=NULL;
    
    if(extl_table_gets_s(tab, "placement_method", &method)){
        if(strcmp(method, "udlr")==0)
            placement_method=PLACEMENT_UDLR;
        else if(strcmp(method, "lrud")==0)
            placement_method=PLACEMENT_LRUD;
        else if(strcmp(method, "random")==0)
            placement_method=PLACEMENT_RANDOM;
        else
            warn(TR("Unknown placement method \"%s\"."), method);
        free(method);
    }
}

/*EXTL_DOC
 * Get module basic settings. See \fnref{mod_floatws.set} for more 
 * information.
 */
EXTL_EXPORT
ExtlTab mod_floatws_get()
{
    ExtlTab t=extl_create_table();
    extl_table_sets_s(t, "placement_method", 
                      (placement_method==PLACEMENT_UDLR
                       ? "udlr" 
                       : (placement_method==PLACEMENT_LRUD
                          ? "lrud" 
                          : "random")));
    return t;
}

static bool tiling_placement(WFloatWS *ws, WRectangle *g)
{
    WRegion *p;
    WRectangle r, r2;
    int maxx, maxy;
    
    r=REGION_GEOM(ws);
    r.w=g->w;
    r.h=g->h;

    maxx=REGION_GEOM(ws).x+REGION_GEOM(ws).w;
    maxy=REGION_GEOM(ws).y+REGION_GEOM(ws).h;
    
    if(placement_method==PLACEMENT_UDLR){
        while(r.x<maxx){
            p=is_occupied(ws, &r);
            while(p!=NULL && r.y+r.h<maxy){
                ggeom(p, &r2);
                r.y=r2.y+r2.h+1;
                p=is_occupied(ws, &r);
            }
            if(r.y+r.h<maxy && r.x+r.w<maxx){
                g->x=r.x;
                g->y=r.y;
                return TRUE;
            }else{
                r.x=next_least_x(ws, r.x);
                r.y=0;
            }
        }
    }else{
        while(r.y<maxy){
            p=is_occupied(ws, &r);
            while(p!=NULL && r.x+r.w<maxx){
                ggeom(p, &r2);
                r.x=r2.x+r2.w+1;
                p=is_occupied(ws, &r);
            }
            if(r.y+r.h<maxy && r.x+r.w<maxx){
                g->x=r.x;
                g->y=r.y;
                return TRUE;
            }else{
                r.y=next_lowest_y(ws, r.y);
                r.x=0;
            }
        }
    }

    return FALSE;

}


void floatws_calc_placement(WFloatWS *ws, WRectangle *geom)
{
    if(placement_method!=PLACEMENT_RANDOM){
        if(tiling_placement(ws, geom))
            return;
    }
    random_placement(REGION_GEOM(ws), geom);
}

