/*
 * ion/ioncore/float-placement.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
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


static bool st_filt(WStacking *st, void *lvl)
{
    uint level=*(uint*)lvl;
    
    return (st->reg!=NULL && 
            REGION_IS_MAPPED(st->reg) && 
            st->level==level);
}


#define FOR_ALL_STACKING_NODES(VAR, WS, LVL, TMP)          \
    for(stacking_iter_init(&(TMP), group_get_stacking(ws), \
                          st_filt, &LVL),                  \
        VAR=stacking_iter_nodes(&(TMP));                   \
        VAR!=NULL;                                         \
        VAR=stacking_iter_nodes(&(TMP)))


#define IGNORE_ST(ST, WS) ((ST)->reg==NULL || (ST)==(WS)->bottom)


static WRegion* is_occupied(WGroup *ws, uint level, const WRectangle *r)
{
    WStackingIterTmp tmp;
    WStacking *st;
    WRectangle p;
    
    FOR_ALL_STACKING_NODES(st, ws, level, tmp){
        ggeom(st->reg, &p);
        
        if(r->x>=p.x+p.w)
            continue;
        if(r->y>=p.y+p.h)
            continue;
        if(r->x+r->w<=p.x)
            continue;
        if(r->y+r->h<=p.y)
            continue;
        return st->reg;
    }
    
    return NULL;
}


static int next_least_x(WGroup *ws, uint level, int x)
{
    WRectangle p;
    int retx=REGION_GEOM(ws).x+REGION_GEOM(ws).w;
    WStackingIterTmp tmp;
    WStacking *st;
    
    FOR_ALL_STACKING_NODES(st, ws, level, tmp){
        ggeom(st->reg, &p);
        
        if(p.x+p.w>x && p.x+p.w<retx)
            retx=p.x+p.w;
    }
    
    return retx+1;
}



static int next_lowest_y(WGroup *ws, uint level, int y)
{
    WRectangle p;
    int rety=REGION_GEOM(ws).y+REGION_GEOM(ws).h;
    WStackingIterTmp tmp;
    WStacking *st;
    
    FOR_ALL_STACKING_NODES(st, ws, level, tmp){
        ggeom(st->reg, &p);
        
        if(p.y+p.h>y && p.y+p.h<rety)
            rety=p.y+p.h;
    }
    
    return rety+1;
}


static bool tiling_placement(WGroup *ws, uint level, WRectangle *g)
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
            p=is_occupied(ws, level, &r);
            while(p!=NULL && r.y+r.h<maxy){
                ggeom(p, &r2);
                r.y=r2.y+r2.h+1;
                p=is_occupied(ws, level, &r);
            }
            if(r.y+r.h<maxy && r.x+r.w<maxx){
                g->x=r.x;
                g->y=r.y;
                return TRUE;
            }else{
                r.x=next_least_x(ws, level, r.x);
                r.y=0;
            }
        }
    }else{
        while(r.y<maxy){
            p=is_occupied(ws, level, &r);
            while(p!=NULL && r.x+r.w<maxx){
                ggeom(p, &r2);
                r.x=r2.x+r2.w+1;
                p=is_occupied(ws, level, &r);
            }
            if(r.y+r.h<maxy && r.x+r.w<maxx){
                g->x=r.x;
                g->y=r.y;
                return TRUE;
            }else{
                r.y=next_lowest_y(ws, level, r.y);
                r.x=0;
            }
        }
    }

    return FALSE;

}


void group_calc_placement(WGroup *ws, uint level, WRectangle *geom)
{
    if(ioncore_placement_method!=PLACEMENT_RANDOM){
        if(tiling_placement(ws, level, geom))
            return;
    }
    random_placement(REGION_GEOM(ws), geom);
}

