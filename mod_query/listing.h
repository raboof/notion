/*
 * ion/mod_query/listing.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_QUERY_LISTING_H
#define ION_MOD_QUERY_LISTING_H

#include <ioncore/common.h>
#include <ioncore/gr.h>
#include <ioncore/rectangle.h>

INTRSTRUCT(WListing);

DECLSTRUCT(WListing){
    char **strs;
    int nstrs;
    int *itemrows;
    int ncol, nrow, nitemcol, visrow;
    int firstitem, firstoff;
    int itemw, itemh, toth;
    bool onecol;
};

extern void init_listing(WListing *l);
extern void setup_listing(WListing *l, char **strs, int nstrs, bool onecol);
extern void deinit_listing(WListing *l);
extern void fit_listing(GrBrush *brush, const WRectangle *geom, WListing *l);
extern void draw_listing(GrBrush *brush, Window win, const WRectangle *geom,
                         WListing *l, bool complete, const char *style);
extern bool scrollup_listing(WListing *l);
extern bool scrolldown_listing(WListing *l);

#endif /* ION_MOD_QUERY_LISTING_H */
