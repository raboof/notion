/*
 * ion/mod_query/wmessage.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_QUERY_WMESSAGE_H
#define ION_MOD_QUERY_WMESSAGE_H

#include <ioncore/common.h>
#include <libtu/obj.h>
#include <ioncore/window.h>
#include <ioncore/rectangle.h>
#include "listing.h"
#include "input.h"

INTRCLASS(WMessage);

DECLCLASS(WMessage){
    WInput input;
    WListing listing;
};

extern WMessage *create_wmsg(WWindow *par, const WFitParams *fp,
                             const char *msg);

#endif /* ION_MOD_QUERY_WMESSAGE_H */
