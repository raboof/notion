/*
 * ion/ioncore/tags.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_TAGS_H
#define ION_IONCORE_TAGS_H

#include <libtu/setparam.h>
#include "region.h"

extern bool region_set_tagged(WRegion *reg, int sp);
extern bool region_is_tagged(WRegion *reg);

extern void ioncore_tagged_clear();
extern WRegion *ioncore_tagged_first(bool untag);
extern bool ioncore_tagged_i(ExtlFn iterfn);

#endif /* ION_IONCORE_TAGS_H */
