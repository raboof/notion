/*
 * ion/ioncore/detach.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_DETACH_H
#define ION_IONCORE_DETACH_H

#include "region.h"

extern bool ioncore_detach(WRegion *reg, int sp);

extern void ioncore_unsqueeze(WRegion *reg, bool override);

#endif /* ION_IONCORE_DETACH_H */

