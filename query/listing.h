/*
 * ion/query/listing.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_QUERY_LISTING_H
#define ION_QUERY_LISTING_H

#include <ioncore/common.h>
#include <ioncore/drawp.h>

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
extern void deinit_listing(WListing *l);
void setup_listing(WListing *l, WFontPtr font, char **strs, int nstrs,
				   bool onecol);
extern void fit_listing(DrawInfo *dinfo, WListing *l);
extern void draw_listing(DrawInfo *dinfo, WListing *l, bool complete);
extern bool scrollup_listing(WListing *l);
extern bool scrolldown_listing(WListing *l);
extern void listing_set_font(WListing *l, WFontPtr font);

#endif /* ION_QUERY_LISTING_H */
