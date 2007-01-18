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

extern void ioncore_clear_tags();
extern WRegion *ioncore_tagged_first();
extern WRegion *ioncore_tagged_take_first();

#endif /* ION_IONCORE_TAGS_H */
