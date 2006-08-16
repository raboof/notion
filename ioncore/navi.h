/*
 * ion/ioncore/navi.h
 *
 * Copyright (c) Tuomo Valkonen 2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_NAVI_H
#define ION_IONCORE_NAVI_H

#include "common.h"
#include "region.h"


typedef enum{
    REGION_NAVI_ANY,
    REGION_NAVI_BEG, /* FIRST, PREV */
    REGION_NAVI_END, /* LAST, NEXT */
    REGION_NAVI_LEFT,
    REGION_NAVI_RIGHT, 
    REGION_NAVI_TOP,
    REGION_NAVI_BOTTOM
} WRegionNavi;

INTRSTRUCT(WRegionNaviData);

DYNFUN WRegion *region_navi_next(WRegion *reg, WRegion *rel, WRegionNavi nh,
                                 WRegionNaviData *data);
DYNFUN WRegion *region_navi_first(WRegion *reg, WRegionNavi nh,
                                  WRegionNaviData *data);

extern WRegion *region_navi_cont(WRegion *reg, WRegion *res, 
                                 WRegionNaviData *data);

extern bool ioncore_string_to_navi(const char *str, WRegionNavi *nv);

extern WRegionNavi ioncore_navi_reverse(WRegionNavi nh);

#endif /* ION_IONCORE_NAVI_H */
