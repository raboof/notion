/*
 * ion/ioncore/tag.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
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
