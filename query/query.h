/*
 * ion/query/query.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_QUERY_QUERY_H
#define ION_QUERY_QUERY_H

#include <ioncore/common.h>
#include <ioncore/mplex.h>
#include <ioncore/extl.h>
#include "wedln.h"

extern WEdln *querymod_query(WMPlex *mplex, const char *prompt, 
                             const char *dflt, ExtlFn handler, 
                             ExtlFn completor);

#endif /* ION_QUERY_QUERY_H */
