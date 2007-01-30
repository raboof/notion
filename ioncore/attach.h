/*
 * ion/ioncore/attach.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_ATTACH_H
#define ION_IONCORE_ATTACH_H

#include "region.h"
#include "reginfo.h"
#include "window.h"


typedef WRegion *WRegionCreateFn(WWindow *parent, 
                                 const WFitParams *fp, 
                                 void *param);

typedef WRegion *WRegionAttachFn(WRegion *reg, 
                                 void *param, 
                                 WRegionAttachData *data);


typedef enum{
    REGION_ATTACH_REPARENT,
    REGION_ATTACH_NEW,
    REGION_ATTACH_LOAD
} WRegionAttachType;


DECLSTRUCT(WRegionAttachData){
    WRegionAttachType type;
    union{
        WRegion *reg;
        struct{
            WRegionCreateFn *fn;
            void *param;
        } n;
        ExtlTab tab;
    } u;
};


typedef bool WRegionDoAttachFn(WRegion *reg, WRegion *sub, void *param);
typedef bool WRegionDoAttachFnSimple(WRegion *reg, WRegion *sub);

extern WRegion *region_attach_helper(WRegion *mgr,
                                     WWindow *par, const WFitParams *fp,
                                     WRegionDoAttachFn *fn, void *fn_param,
                                     const WRegionAttachData *data);

extern bool region_attach_reparent_check(WRegion *mgr, WRegion *reg);


#endif /* ION_IONCORE_ATTACH_H */
