/*
 * ion/ioncore/activity.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_ACTIVITY_H
#define ION_IONCORE_ACTIVITY_H

#include <libtu/setparam.h>
#include <libmainloop/hooks.h>
#include <libextl/extl.h>
#include "region.h"

extern bool region_set_activity(WRegion *reg, int sp);
extern bool region_is_activity(WRegion* re);
extern bool region_is_activity_r(WRegion *reg);

extern ExtlTab ioncore_activity_list();
extern WRegion *ioncore_activity_first();
extern bool ioncore_goto_activity();

extern WHook *region_activity_hook;

#endif /* ION_IONCORE_ACTIVITY_H */
