/*
 * ion/ioncore/names.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_NAMES_H
#define ION_IONCORE_NAMES_H

#include "region.h"
#include "clientwin.h"
#include "font.h"
#include "extl.h"

extern bool region_set_name(WRegion *reg, const char *name);
extern bool region_set_name_exact(WRegion *reg, const char *name);
extern bool clientwin_set_name(WClientWin *cwin, const char *name);
extern void	region_unuse_name(WRegion *reg);

extern const char *region_name(WRegion *reg);

extern char *region_make_label(WRegion *reg, int maxw, WFontPtr font);

extern WRegion *lookup_region(const char *cname, const char *typenam);
extern ExtlTab complete_region(const char *nam, const char *typenam);
extern WClientWin *lookup_clientwin(const char *cname);
extern ExtlTab complete_clientwin(const char *nam);

#endif /* ION_IONCORE_NAMES_H */
