/*
 * ion/ioncore/attach.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include "clientwin.h"
#include "genws.h"
#include "rectangle.h"


/* Attach helpers */


typedef WRegion *WRegionAttachHandler(WWindow *parent, 
									  const WRectangle *geom, 
									  void *param);
	
typedef WRegion *WRegionDoAttachFn(WRegion *reg, 
								   WRegionAttachHandler *handler,
								   void *handlerparams,
								   void *param);


extern bool region__attach_reparent(WRegion *mgr, WRegion *reg, 
                                    WRegionDoAttachFn *fn, void *param);

extern WRegion *region__attach_new(WRegion *mgr, WRegionSimpleCreateFn *cfn,
                                   WRegionDoAttachFn *fn, void *param);

extern WRegion *region__attach_load(WRegion *mgr, ExtlTab tab,
                                    WRegionDoAttachFn *fn, void *param);


/* Rescue */

extern WRegion *region_find_rescue_manager(WRegion *reg);
DYNFUN WRegion *region_find_rescue_manager_for(WRegion *reg, WRegion *todst);
extern WRegion *region_find_rescue_manager_for_default(WRegion *reg, WRegion *todst);

extern bool region_rescue_clientwins(WRegion *reg);
extern bool region_do_rescue_managed_clientwins(WRegion *reg, WRegion *dest, WRegion *list);
extern bool region_do_rescue_child_clientwins(WRegion *reg, WRegion *dest);
/* dest may be NULL */
DYNFUN bool region_do_rescue_clientwins(WRegion *reg, WRegion *dest);

#endif /* ION_IONCORE_ATTACH_H */
