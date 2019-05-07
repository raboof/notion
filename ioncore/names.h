/*
 * ion/ioncore/names.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_NAMES_H
#define ION_IONCORE_NAMES_H

#include "region.h"
#include "clientwin.h"
#include "gr.h"
#include <libextl/extl.h>


typedef struct{
    /** Names, keyed by WRegionNameInfo's */
    struct rb_node *rb;
    bool initialised;
} WNamespace;


extern WNamespace ioncore_internal_ns;
extern WNamespace ioncore_clientwin_ns;


/** Register a region (but not a clientwin) to the naming registry */
extern bool region_register(WRegion *reg);
extern bool region_set_name(WRegion *reg, const char *name);
extern bool region_set_name_exact(WRegion *reg, const char *name);

extern bool clientwin_register(WClientWin *cwin);
extern bool clientwin_set_name(WClientWin *cwin, const char *name);

extern void region_unregister(WRegion *reg);
extern void region_do_unuse_name(WRegion *reg, bool insert_unnamed);

extern const char *region_name(WRegion *reg);
DYNFUN const char *region_displayname(WRegion *reg);

extern char *region_make_label(WRegion *reg, int maxw, GrBrush *brush);

extern bool ioncore_region_i(ExtlFn fn, const char *typenam);
extern bool ioncore_clientwin_i(ExtlFn fn);
/**
 * Look up a region (internal windows, not client windows) by name and type
 * name.
 *
 * As region names are unique, the 'typename' parameter is only used to filter
 * out regions that do not have the expected type.
 */
extern WRegion *ioncore_lookup_region(const char *cname, const char *typenam);
extern WClientWin *ioncore_lookup_clientwin(const char *cname);

#endif /* ION_IONCORE_NAMES_H */
