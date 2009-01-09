/*
 * ion/ioncore/navi.h
 *
 * Copyright (c) Tuomo Valkonen 2006-2009. 
 *
 * See the included file LICENSE for details.
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

extern WRegion *ioncore_goto_next(WRegion *reg, const char *dirstr, 
                                  ExtlTab param);
extern WRegion *ioncore_goto_first(WRegion *reg, const char *dirstr, 
                                   ExtlTab param);
extern WRegion *ioncore_navi_next(WRegion *reg, const char *dirstr, 
                                  ExtlTab param);
extern WRegion *ioncore_navi_first(WRegion *reg, const char *dirstr, 
                                   ExtlTab param);

#endif /* ION_IONCORE_NAVI_H */
