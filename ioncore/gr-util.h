/*
 * ion/ioncore/gr-util.h
 *
 * Copyright (c) Tuomo Valkonen 2007-2008. 
 *
 * See the included file LICENSE for details.
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

