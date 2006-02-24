/*
 * ion/ioncore/sizepolicy.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_SIZEPOLICY_H
#define ION_IONCORE_SIZEPOLICY_H

#include "common.h"
#include "region.h"


typedef enum{
    SIZEPOLICY_DEFAULT,
    SIZEPOLICY_FULL_EXACT,
    SIZEPOLICY_FULL_BOUNDS,
    SIZEPOLICY_FREE
} WSizePolicy;

extern void sizepolicy(WSizePolicy szplcy, const WRegion *reg,
                       const WRectangle *rq_geom, WFitParams *fp);

#endif /* ION_IONCORE_SIZEPOLICY_H */
