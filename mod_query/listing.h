/*
 * ion/mod_query/listing.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_QUERY_LISTING_H
#define ION_MOD_QUERY_LISTING_H

#include <ioncore/common.h>
#include <ioncore/gr.h>
#include <ioncore/rectangle.h>
#include <libtu/minmax.h>

INTRSTRUCT(WListing);
INTRSTRUCT(WListingItemInfo);

DECLSTRUCT(WListingItemInfo){
    int len;
    int n_parts;
    int *part_lens;
};

DECLSTRUCT(WListing){
    char **strs;
    WListingItemInfo *iteminfos;
    int nstrs;
    int selected_str;
    int ncol, nrow, nitemcol, visrow;
    int firstitem, firstoff;
    int itemw, itemh, toth;
    bool onecol;
};


#define LISTING_DRAW_COMPLETE 1
#define LISTING_DRAW_ALL 0
#define LISTING_DRAW_SELECTED(X) minof(-1, -(X)-2)
#define LISTING_DRAW_GET_SELECTED(X) (-(X)-2)

    
extern void init_listing(WListing *l);
extern void setup_listing(WListing *l, char **strs, int nstrs, bool onecol);
extern void deinit_listing(WListing *l);
extern void fit_listing(GrBrush *brush, const WRectangle *geom, WListing *l);
extern void draw_listing(GrBrush *brush, const WRectangle *geom, WListing *l, 
                         bool complete, GrAttr selattr);
extern bool scrollup_listing(WListing *l);
extern bool scrolldown_listing(WListing *l);
extern bool listing_select(WListing *l, int i);

#endif /* ION_MOD_QUERY_LISTING_H */
