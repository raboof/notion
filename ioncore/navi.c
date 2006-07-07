/*
 * ion/ioncore/navi.c
 *
 * Copyright (c) Tuomo Valkonen 2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <libtu/objp.h>

#include "common.h"
#include "region.h"
#include "navi.h"


WRegion *region_navi_first(WRegion *reg, WRegionNavi nh)
{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, region_navi_first, reg, (reg, nh));
    return ret;
}


WRegion *region_navi_next(WRegion *reg, WRegion *mgd, WRegionNavi nh)
{
    WRegion *ret=NULL;
    CALL_DYN_RET(ret, WRegion*, region_navi_next, reg, (reg, mgd, nh));
    return ret;
}


bool ioncore_string_to_navi(const char *str, WRegionNavi *nh)
{
    if(str==NULL){
        warn(TR("Invalid parameter."));
        return FALSE;
    }
    
    if(!strcmp(str, "any")){
        *nh=REGION_NAVI_ANY;
    }else if (!strcmp(str, "first")){
        *nh=REGION_NAVI_FIRST;
    }else if (!strcmp(str, "last")){
        *nh=REGION_NAVI_LAST;
    }else if(!strcmp(str, "left")){
        *nh=REGION_NAVI_LEFT;
    }else if(!strcmp(str, "right")){
        *nh=REGION_NAVI_RIGHT;
    }else if(!strcmp(str, "top") || 
             !strcmp(str, "above") || 
             !strcmp(str, "up")){
        *nh=REGION_NAVI_TOP;
    }else if(!strcmp(str, "bottom") || 
             !strcmp(str, "below") ||
             !strcmp(str, "down")){
        *nh=REGION_NAVI_BOTTOM;
    }else{
        warn(TR("Invalid parameter."));
        return FALSE;
    }
    
    return TRUE;
}
