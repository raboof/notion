/*
 * ion/ioncore/rectangle.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <libtu/minmax.h>
#include <libextl/extl.h>

#include "common.h"
#include "rectangle.h"


void rectangle_constrain(WRectangle *g, const WRectangle *bounds)
{
    const WRectangle tmpg=*g;

    g->x=MINOF(MAXOF(tmpg.x, bounds->x), tmpg.x+tmpg.w-1);
    g->y=MINOF(MAXOF(tmpg.y, bounds->y), tmpg.y+tmpg.h-1);
    g->w=MAXOF(1, MINOF(bounds->x+bounds->w, tmpg.x+tmpg.w)-g->x);
    g->h=MAXOF(1, MINOF(bounds->y+bounds->h, tmpg.y+tmpg.h)-g->y);
}


bool rectangle_contains(const WRectangle *g, int x, int y)
{
    return (x>=g->x && x<g->x+g->w && y>=g->y && y<g->y+g->h);
}


void rectangle_debugprint(const WRectangle *g, const char *n)
{
    fprintf(stderr, "%s %d, %d; %d, %d\n", n, g->x, g->y, g->w, g->h);
}


int rectangle_compare(const WRectangle *g, const WRectangle *h)
{
    return ((g->x!=h->x ? RECTANGLE_X_DIFF : 0) |
            (g->y!=h->y ? RECTANGLE_Y_DIFF : 0) |
            (g->w!=h->w ? RECTANGLE_W_DIFF : 0) |
            (g->h!=h->h ? RECTANGLE_H_DIFF : 0));
}
