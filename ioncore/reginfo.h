/*
 * ion/ioncore/reginfo.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_REGINFO_H
#define ION_IONCORE_REGINFO_H

#include "common.h"
#include "obj.h"
#include "region.h"
#include "window.h"
#include "extl.h"

typedef WRegion *WRegionLoadCreateFn(WWindow *par, WRectangle geom,
									 ExtlTab tab);
typedef WRegion *WRegionSimpleCreateFn(WWindow *par, WRectangle geom);


extern bool register_region_class(WObjDescr *descr,
								  WRegionSimpleCreateFn *sc_fn,
								  WRegionLoadCreateFn *lc_fn);
extern void unregister_region_class(WObjDescr *descr);

extern WRegionLoadCreateFn *lookup_region_load_create_fn(const char *name);
extern WRegionSimpleCreateFn *lookup_region_simple_create_fn(const char *name);
extern WRegionSimpleCreateFn *lookup_region_simple_create_fn_inh(const char *name);

#endif /* ION_IONCORE_REGINFO_H */

