/*
 * ion/ioncore/activity.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_ACTIVITY_H
#define ION_IONCORE_ACTIVITY_H

#include "region.h"

extern void region_notify_activity(WRegion *reg);
extern void region_clear_activity(WRegion *reg);
extern bool region_activity(WRegion *reg);

#endif /* ION_IONCORE_ACTIVITY_H */
