/*
 * ion/ioncore/reginfo.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_REGINFO_H
#define ION_IONCORE_REGINFO_H

#include <libtu/tokenizer.h>

#include "common.h"
#include "obj.h"
#include "region.h"
#include "window.h"

typedef WRegion *WRegionLoadCreateFn(WWindow *par, WRectangle geom,
									 Tokenizer *tokz);
typedef WRegion *WRegionSimpleCreateFn(WWindow *par, WRectangle geom);


extern bool register_region_class(WObjDescr *descr,
								  WRegionSimpleCreateFn *sc_fn,
								  WRegionLoadCreateFn *lc_fn);
extern void unregister_region_class(WObjDescr *descr);

extern WRegionLoadCreateFn *lookup_region_load_create_fn(const char *name);
extern WRegionSimpleCreateFn *lookup_region_simple_create_fn(const char *name);
extern WRegionSimpleCreateFn *lookup_region_simple_create_fn_inh(const char *name);

#endif /* ION_IONCORE_REGINFO_H */

