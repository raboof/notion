/*
 * ion/de/colour.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
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
    
    if(cg->spec!=NULL){
        free(cg->spec);
        cg->spec=NULL;
    }
}


void de_free_colour(WRootWin *rootwin, DEColour col)
{
    DEColour pixels[1];
    
    pixels[0]=col;
    
    XFreeColors(ioncore_g.dpy, rootwin->default_cmap, pixels, 1, 0);
}

