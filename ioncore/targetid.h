/*
 * wmcore/targetid.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_TARGETID_H
#define WMCORE_TARGETID_H

#include "common.h"
#include "region.h"

extern WRegion *find_target_by_id(int id);
extern int use_target_id(WRegion *reg, int id);
extern int alloc_target_id(WRegion *reg);
extern void free_target_id(int id);
extern void	set_target_id(WRegion *reg, int id);

#endif /* WMCORE_TARGETID_H */
