/*
 * wmcore/names.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_NAMES_H
#define WMCORE_NAMES_H

#include "region.h"
#include "font.h"


extern bool region_set_name(WRegion *reg, const char *name);
extern const char *region_name(WRegion *reg);
extern uint region_name_instance(WRegion *reg);

/* Returns a newly allocated copy of the name with the instance
 * number appended. */
extern char *region_full_name(WRegion *reg);
extern char *region_make_label(WRegion *reg, int maxw, WFont *font);

extern WRegion *do_lookup_region(const char *cname, WObjDescr *descr);
extern int do_complete_region(char *nam, char ***cp_ret, char **beg,
							  WObjDescr *descr);
extern WRegion *lookup_region(const char *cname);
extern int complete_region(char *nam, char ***cp_ret, char **beg,
						   void *unused);

extern void goto_named_region(char *name);
extern void	region_unuse_name(WRegion *reg);

#endif /* WMCORE_NAMES_H */
