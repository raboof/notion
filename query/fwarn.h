/*
 * ion/query/fwarn.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_QUERY_FWARN_H
#define ION_QUERY_FWARN_H

#include <ioncore/mplex.h>
#include "wmessage.h"

extern WMessage *query_message(WMPlex *mplex, const char *p);
extern WMessage *query_fwarn(WMPlex *mplex, const char *p);

#endif /* ION_QUERY_FWARN_H */
