/*
 * ion/ioncore/tags.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
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
