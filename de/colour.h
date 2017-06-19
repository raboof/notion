/*
 * ion/de/colour.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_DE_COLOUR_H
#define ION_DE_COLOUR_H

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/rootwin.h>
#ifdef HAVE_X11_XFT
#include <X11/Xft/Xft.h>
#endif /* HAVE_X11_XFT */


INTRSTRUCT(DEColourGroup);


#ifdef HAVE_X11_XFT
typedef XftColor DEColour;
#else /* HAVE_X11_XFT */
typedef unsigned long DEColour;
#endif /* HAVE_X11_XFT */


DECLSTRUCT(DEColourGroup){
    GrStyleSpec spec;
    DEColour bg, hl, sh, fg, pad;
};


#define DE_BLACK(rootwin) BlackPixel(ioncore_g.dpy, rootwin->xscr)
#define DE_WHITE(rootwin) WhitePixel(ioncore_g.dpy, rootwin->xscr)

bool de_init_colour_group(WRootWin *rootwin, DEColourGroup *cg);
bool de_alloc_colour(WRootWin *rootwin, DEColour *ret, const char *name);
bool de_duplicate_colour(WRootWin *rootwin, DEColour in, DEColour *out);
void de_free_colour_group(WRootWin *rootwin, DEColourGroup *cg);
void de_free_colour(WRootWin *rootwin, DEColour col);
#define XftDEDefaultVisual()    DefaultVisual(ioncore_g.dpy, 0)

#endif /* ION_DE_COLOUR_H */
