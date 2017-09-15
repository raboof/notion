/*
 * ion/ioncore/float-placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_FLOAT_PLACEMENT_H
#define ION_IONCORE_FLOAT_PLACEMENT_H

#include "common.h"
#include "group.h"

typedef enum{
    PLACEMENT_LRUD,
    PLACEMENT_UDLR,
    PLACEMENT_POINTER,
    PLACEMENT_RANDOM,
} WFloatPlacement;

extern WFloatPlacement ioncore_placement_method;
extern int ioncore_placement_padding;

extern void group_calc_placement(WGroup *ws, uint level,
                                 WRectangle *geom);

#endif /* ION_IONCORE_FLOAT_PLACEMENT_H */
