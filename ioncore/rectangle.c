/*
 * ion/ioncore/rectangle.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "extl.h"
#include "rectangle.h"


bool rectangle_contains(const WRectangle *g, int x, int y)
{
    return (x>=g->x && x<g->x+g->w && y>=g->y && y<g->y+g->h);
}


void rectangle_debugprint(const WRectangle *g, const char *n)
{
    fprintf(stderr, "%s %d, %d; %d, %d\n", n, g->x, g->y, g->w, g->h);
}


void rectangle_writecode(const WRectangle *geom, FILE *file)
{
    fprintf(file, "geom = { x = %d, y = %d, w = %d, h = %d},\n",
            geom->x, geom->y, geom->w, geom->h);
}
