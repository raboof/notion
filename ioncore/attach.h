/*
 * ion/ioncore/attach.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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


typedef WRegion *WRegionAttachHandler(WWindow *parent, 
                                      const WFitParams *fp, 
                                      void *param);

typedef WRegion *WRegionDoAttachFn(WRegion *reg, 
                                   WRegionAttachHandler *handler,
                                   void *handlerparams,
                                   void *param);


extern WRegion *region__attach_reparent(WRegion *mgr, WRegion *reg, 
                                        WRegionDoAttachFn *fn, void *param);

extern WRegion *region__attach_new(WRegion *mgr, WRegionSimpleCreateFn *cfn,
                                   WRegionDoAttachFn *fn, void *param);

extern WRegion *region__attach_load(WRegion *mgr, ExtlTab tab,
                                    WRegionDoAttachFn *fn, void *param);


#endif /* ION_IONCORE_ATTACH_H */
