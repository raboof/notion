/*
 * query/wmessage.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef QUERY_WMESSAGE_H
#define QUERY_WMESSAGE_H

#include <wmcore/common.h>
#include <wmcore/thing.h>
#include <wmcore/window.h>
#include "listing.h"
#include "input.h"

INTROBJ(WMessage)

DECLOBJ(WMessage){
	WInput input;
	WListing listing;
};

extern WMessage *create_wmsg(WRegion *par, WRectangle geom,
							 const char *msg);
extern void deinit_wmsg(WMessage *msg);

#endif /* QUERY_WMESSAGE_H */
