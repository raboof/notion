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

#include <ioncore/genframe.h>

extern void query_message(WGenFrame *frame, const char *p);
extern void query_fwarn(WGenFrame *frame, const char *p);

#endif /* ION_QUERY_FWARN_H */
