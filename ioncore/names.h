/*
 * ion/ioncore/names.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_NAMES_H
#define ION_IONCORE_NAMES_H

#include "region.h"
#include "font.h"
#include "extl.h"


extern bool region_set_name(WRegion *reg, const char *name);
extern const char *region_name(WRegion *reg);
extern uint region_name_instance(WRegion *reg);

/* Returns a newly allocated copy of the name with the instance
 * number appended. */
extern char *region_full_name(WRegion *reg);
extern char *region_make_label(WRegion *reg, int maxw, WFontPtr font);

extern WRegion *do_lookup_region(const char *cname, WObjDescr *descr);
extern ExtlTab do_complete_region(const char *nam, WObjDescr *descr);
extern WRegion *lookup_region(const char *cname);
extern ExtlTab complete_region(const char *nam);

extern void	region_unuse_name(WRegion *reg);

#endif /* ION_IONCORE_NAMES_H */
