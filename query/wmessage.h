/*
 * ion/query/wmessage.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_QUERY_WMESSAGE_H
#define ION_QUERY_WMESSAGE_H

#include <ioncore/common.h>
#include <ioncore/obj.h>
#include <ioncore/window.h>
#include "listing.h"
#include "input.h"

INTROBJ(WMessage)

DECLOBJ(WMessage){
	WInput input;
	WListing listing;
};

extern WMessage *create_wmsg(WWindow *par, WRectangle geom,
							 const char *msg);
extern void deinit_wmsg(WMessage *msg);

#endif /* ION_QUERY_WMESSAGE_H */
