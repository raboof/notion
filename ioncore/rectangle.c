/*
 * ion/ioncore/rectangle.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/minmax.h>
#include "common.h"
#include <libextl/extl.h>
#include "rectangle.h"


void rectangle_constrain(WRectangle *g, const WRectangle *bounds)
{
    const WRectangle tmpg=*g;
    
    g->x=minof(maxof(tmpg.x, bounds->x), tmpg.x+tmpg.w-1);
    g->y=minof(maxof(tmpg.y, bounds->y), tmpg.y+tmpg.h-1);
    g->w=maxof(1, minof(bounds->x+bounds->w, tmpg.x+tmpg.w)-g->x);
    g->h=maxof(1, minof(bounds->y+bounds->h, tmpg.y+tmpg.h)-g->y);
}


bool rectangle_contains(const WRectangle *g, int x, int y)
{
    return (x>=g->x && x<g->x+g->w && y>=g->y && y<g->y+g->h);
}


void rectangle_debugprint(const WRectangle *g, const char *n)
{
    fprintf(stderr, "%s %d, %d; %d, %d\n", n, g->x, g->y, g->w, g->h);
}


