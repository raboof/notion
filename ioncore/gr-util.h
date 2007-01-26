/*
 * ion/ioncore/gr-util.h
 *
 * Copyright (c) Tuomo Valkonen 2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GR_UTIL_H
#define ION_IONCORE_GR_UTIL_H

#include "gr.h"

#define GR_ATTR(X) grattr_##X
#define GR_DEFATTR(X) static GrAttr GR_ATTR(X) = STRINGID_NONE
#define GR_ALLOCATTR_BEGIN static bool alloced=FALSE; if(alloced) return
#define GR_ALLOCATTR_END alloced=TRUE
#define GR_ALLOCATTR(X) GR_ATTR(X) = stringstore_alloc(#X)

#endif /* ION_IONCORE_GR_UTIL_H */

