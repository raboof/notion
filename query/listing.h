/*
 * query/listing.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef QUERY_LISTING_H
#define QUERY_LISTING_H

#include <wmcore/common.h>
#include <wmcore/drawp.h>

INTRSTRUCT(WListing)

DECLSTRUCT(WListing){
	char **strs;
	int nstrs;
	int *itemrows;
	int ncol, nrow, nitemcol, visrow;
	int firstitem, firstoff;
	int itemw, itemh, toth;
};

extern void init_listing(WListing *l);
extern void deinit_listing(WListing *l);
void setup_listing(WListing *l, WFontPtr font, char **strs, int nstrs);
extern void fit_listing(DrawInfo *dinfo, WListing *l);
extern void draw_listing(DrawInfo *dinfo, WListing *l, bool complete);
extern bool scrollup_listing(WListing *l);
extern bool scrolldown_listing(WListing *l);
extern void listing_set_font(WListing *l, WFontPtr font);

#endif /* QUERY_LISTING_H */
