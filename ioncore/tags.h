/*
 * ion/ioncore/tag.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_TAG_H
#define ION_IONCORE_TAG_H

#include "region.h"

extern void tag_region(WRegion *reg);
extern void untag_region(WRegion *reg);
extern void toggle_region_tag(WRegion *reg);
extern void clear_tags(WRegion *reg);
extern void clear_sub_tags(WRegion *reg);

extern WRegion *tag_first();
extern WRegion *tag_take_first();
extern WRegion *tag_next(WRegion *reg);

#endif /* ION_IONCORE_WSREG_H */
