/*
 * ion/ioncore/reginfo.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_REGINFO_H
#define ION_IONCORE_REGINFO_H

#include "common.h"
#include <libtu/obj.h>
#include "region.h"
#include "window.h"
#include "extl.h"
#include "rectangle.h"

typedef WRegion *WRegionLoadCreateFn(WWindow *par, const WRectangle *geom,
									 ExtlTab tab);
typedef WRegion *WRegionSimpleCreateFn(WWindow *par, const WRectangle *geom);

INTRSTRUCT(WRegClassInfo);
	
DECLSTRUCT(WRegClassInfo){
	ClassDescr *descr;
	WRegionSimpleCreateFn *sc_fn;
	WRegionLoadCreateFn *lc_fn;
	WRegClassInfo *next, *prev;
};


extern bool ioncore_register_regclass(ClassDescr *descr,
                                      WRegionSimpleCreateFn *sc_fn,
                                      WRegionLoadCreateFn *lc_fn);
extern void ioncore_unregister_regclass(ClassDescr *descr);

extern WRegClassInfo *ioncore_lookup_regclass(const char *name, 
                                              bool inheriting_ok,
                                              bool need_simplefn,
                                              bool need_loadfn);

#endif /* ION_IONCORE_REGINFO_H */

