/*
 * ion/de/colour.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_DE_COLOUR_H
#define ION_DE_COLOUR_H

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/rootwin.h>


INTRSTRUCT(DEColourGroup);


typedef unsigned long DEColour;


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

#endif /* ION_DE_COLOUR_H */
