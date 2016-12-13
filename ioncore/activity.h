/*
 * ion/ioncore/activity.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_ACTIVITY_H
#define ION_IONCORE_ACTIVITY_H

#include <libtu/setparam.h>
#include <libmainloop/hooks.h>
#include <libextl/extl.h>
#include "region.h"

/**
 * Manipulate the 'activity' flag of this region. If the region is already
 * active the 'activity'-flag will remain off.
 *
 * @param sp SET, UNSET or TOGGLE
 * @returns the new value of the 'activity' flag
 */
extern bool region_set_activity(WRegion *reg, int sp);
extern bool region_is_activity(WRegion* re);
extern bool region_is_activity_r(WRegion *reg);

extern void region_mark_mgd_activity(WRegion *mgr);
extern void region_clear_mgd_activity(WRegion *mgr);

extern bool ioncore_activity_i(ExtlFn fn);
extern WRegion *ioncore_activity_first();
extern bool ioncore_goto_activity();
extern ObjList *ioncore_activity_list();

#endif /* ION_IONCORE_ACTIVITY_H */
