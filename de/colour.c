/*
 * ion/de/colour.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <ioncore/common.h>
#include "colour.h"


bool de_alloc_colour(WRootWin *rootwin, DEColour *ret, const char *name)
{
#ifdef HAVE_X11_XFT
    if(name==NULL)
        return FALSE;
    return XftColorAllocName(
        ioncore_g.dpy,
        XftDEDefaultVisual(),
        rootwin->default_cmap,
        name,
        ret);
#else /* HAVE_X11_XFT */
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
#endif /* HAVE_X11_XFT */
}


bool de_duplicate_colour(WRootWin *rootwin, DEColour in, DEColour *out)
{
#ifdef HAVE_X11_XFT
    return XftColorAllocValue(
        ioncore_g.dpy,
        XftDEDefaultVisual(),
        rootwin->default_cmap,
        &(in.color),
        out);
#else /* HAVE_X11_XFT */
    XColor c;
    c.pixel=in;
    XQueryColor(ioncore_g.dpy, rootwin->default_cmap, &c);
    if(XAllocColor(ioncore_g.dpy, rootwin->default_cmap, &c)){
        *out=c.pixel;
        return TRUE;
    }
    return FALSE;
#endif /* HAVE_X11_XFT */
}


void de_free_colour_group(WRootWin *rootwin, DEColourGroup *cg)
{
#ifdef HAVE_X11_XFT
    de_free_colour(rootwin, cg->bg);
    de_free_colour(rootwin, cg->fg);
    de_free_colour(rootwin, cg->hl);
    de_free_colour(rootwin, cg->sh);
    de_free_colour(rootwin, cg->pad);
#else /* HAVE_X11_XFT */
    DEColour pixels[5];
    
    pixels[0]=cg->bg;
    pixels[1]=cg->fg;
    pixels[2]=cg->hl;
    pixels[3]=cg->sh;
    pixels[4]=cg->pad;
    
    XFreeColors(ioncore_g.dpy, rootwin->default_cmap, pixels, 5, 0);
    
    gr_stylespec_unalloc(&cg->spec);
#endif /* HAVE_X11_XFT */
}


void de_free_colour(WRootWin *rootwin, DEColour col)
{
#ifdef HAVE_X11_XFT
    XftColorFree(ioncore_g.dpy, XftDEDefaultVisual(), rootwin->default_cmap, &col);
#else /* HAVE_X11_XFT */
    DEColour pixels[1];
    
    pixels[0]=col;
    
    XFreeColors(ioncore_g.dpy, rootwin->default_cmap, pixels, 1, 0);
#endif /* HAVE_X11_XFT */
}

