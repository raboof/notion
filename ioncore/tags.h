/*
 * ion/ioncore/tags.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_TAGS_H
#define ION_IONCORE_TAGS_H

#include "region.h"

extern void region_tag(WRegion *reg);
extern void region_untag(WRegion *reg);
extern void region_toggle_tag(WRegion *reg);

extern void ioncore_clear_tags();
extern WRegion *ioncore_tags_first();
extern WRegion *ioncore_tags_take_first();
extern WRegion *ioncore_tags_next(WRegion *reg);

#endif /* ION_IONCORE_TAGS_H */
