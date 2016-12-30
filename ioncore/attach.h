/*
 * ion/ioncore/attach.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
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

extern WRegion *region_attach_load_helper(WRegion *mgr,
                                          WWindow *par, const WFitParams *fp,
                                          WRegionDoAttachFn *fn, void *fn_param,
                                          ExtlTab tab, WPHolder **sm_ph);

extern bool region_ancestor_check(WRegion *dst, WRegion *reg);

extern void region_postdetach_dispose(WRegion *reg, WRegion *disposeroot);


#endif /* ION_IONCORE_ATTACH_H */
