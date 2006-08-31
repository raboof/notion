/*
 * ion/ioncore/float-placement.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include "common.h"
#include "group.h"
#include "float-placement.h"


WFloatPlacement ioncore_placement_method=PLACEMENT_LRUD;


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


static WRegion* is_occupied(WGroup *ws, const WRectangle *r)
{
    WRegion *reg;
    WRectangle p;
    WGroupIterTmp tmp;
    
    FOR_ALL_MANAGED_BY_GROUP(ws, reg, tmp){
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


static int next_least_x(WGroup *ws, int x)
{
    WRegion *reg;
    WRectangle p;
    int retx=REGION_GEOM(ws).x+REGION_GEOM(ws).w;
    WGroupIterTmp tmp;
    
    FOR_ALL_MANAGED_BY_GROUP(ws, reg, tmp){
        ggeom(reg, &p);
        
        if(p.x+p.w>x && p.x+p.w<retx)
            retx=p.x+p.w;
    }
    
    return retx+1;
}


static int next_lowest_y(WGroup *ws, int y)
{
    WRegion *reg;
    WRectangle p;
    int rety=REGION_GEOM(ws).y+REGION_GEOM(ws).h;
    WGroupIterTmp tmp;
    
    FOR_ALL_MANAGED_BY_GROUP(ws, reg, tmp){
        ggeom(reg, &p);
        
        if(p.y+p.h>y && p.y+p.h<rety)
            rety=p.y+p.h;
    }
    
    return rety+1;
}


static bool tiling_placement(WGroup *ws, WRectangle *g)
{
    WRegion *p;
    WRectangle r, r2;
    int maxx, maxy;
    
    r=REGION_GEOM(ws);
    r.w=g->w;
    r.h=g->h;

    maxx=REGION_GEOM(ws).x+REGION_GEOM(ws).w;
    maxy=REGION_GEOM(ws).y+REGION_GEOM(ws).h;
    
    if(ioncore_placement_method==PLACEMENT_UDLR){
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


void group_calc_placement(WGroup *ws, WRectangle *geom)
{
    if(ioncore_placement_method!=PLACEMENT_RANDOM){
        if(tiling_placement(ws, geom))
            return;
    }
    random_placement(REGION_GEOM(ws), geom);
}

