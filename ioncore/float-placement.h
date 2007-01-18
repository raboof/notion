/*
 * ion/ioncore/float-placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FLOAT_PLACEMENT_H
#define ION_IONCORE_FLOAT_PLACEMENT_H

#include "common.h"
#include "group.h"


typedef enum{
    PLACEMENT_LRUD, PLACEMENT_UDLR, PLACEMENT_RANDOM
} WFloatPlacement;

extern WFloatPlacement ioncore_placement_method;

extern void group_calc_placement(WGroup *ws, WRectangle *geom);

#endif /* ION_IONCORE_FLOAT_PLACEMENT_H */
