/*
 * ion/de/colour.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#include <ioncore/common.h>
#include "colour.h"


bool de_alloc_colour(WRootWin *rootwin, DEColour *ret, const char *name)
{
    XColor c;
    bool ok=FALSE;

    if(name==NULL)
        return FALSE;

    if(XParseColor(ioncore_g.dpy, rootwin->default_cmap, name, &c)){
        ok=XAllocColor(ioncore_g.dpy, rootwin->default_cmap, &c);
        if(ok)
            *ret=c.pixel;
    }

    return ok;
}


bool de_duplicate_colour(WRootWin *rootwin, DEColour in, DEColour *out)
{
    XColor c;
    c.pixel=in;
    XQueryColor(ioncore_g.dpy, rootwin->default_cmap, &c);
    if(XAllocColor(ioncore_g.dpy, rootwin->default_cmap, &c)){
        *out=c.pixel;
        return TRUE;
    }
    return FALSE;
}


void de_free_colour_group(WRootWin *rootwin, DEColourGroup *cg)
{
    DEColour pixels[5];

    pixels[0]=cg->bg;
    pixels[1]=cg->fg;
    pixels[2]=cg->hl;
    pixels[3]=cg->sh;
    pixels[4]=cg->pad;

    XFreeColors(ioncore_g.dpy, rootwin->default_cmap, pixels, 5, 0);

    gr_stylespec_unalloc(&cg->spec);
}


void de_free_colour(WRootWin *rootwin, DEColour col)
{
    DEColour pixels[1];

    pixels[0]=col;

    XFreeColors(ioncore_g.dpy, rootwin->default_cmap, pixels, 1, 0);
}

