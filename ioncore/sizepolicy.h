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
    SIZEPOLICY_FREE,
    /* Sigh. Algebraic data types would make this so 
     * much more convenient.. 
     */
    SIZEPOLICY_GRAVITY_NORTHWEST,
    SIZEPOLICY_GRAVITY_NORTH,
    SIZEPOLICY_GRAVITY_NORTHEAST,
    SIZEPOLICY_GRAVITY_WEST,
    SIZEPOLICY_GRAVITY_CENTER,
    SIZEPOLICY_GRAVITY_EAST,
    SIZEPOLICY_GRAVITY_SOUTHWEST,
    SIZEPOLICY_GRAVITY_SOUTH,
    SIZEPOLICY_GRAVITY_SOUTHEAST,
    
    SIZEPOLICY_STRETCH_NORTH,
    SIZEPOLICY_STRETCH_WEST,
    SIZEPOLICY_STRETCH_EAST,
    SIZEPOLICY_STRETCH_SOUTH
} WSizePolicy;


extern void sizepolicy(WSizePolicy szplcy, WRegion *reg,
                       const WRectangle *rq_geom, int rq_flags,
                       WFitParams *fp);


bool string2sizepolicy(const char *szplcy, WSizePolicy *value);


#endif /* ION_IONCORE_SIZEPOLICY_H */
