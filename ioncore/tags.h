/*
 * ion/ioncore/tag.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_TAG_H
#define ION_IONCORE_TAG_H

#include "region.h"

extern void region_tag(WRegion *reg);
extern void region_untag(WRegion *reg);
extern void region_toggle_tag(WRegion *reg);
extern void clear_tags();

extern WRegion *tag_first();
extern WRegion *tag_take_first();
extern WRegion *tag_next(WRegion *reg);

#endif /* ION_IONCORE_WSREG_H */
