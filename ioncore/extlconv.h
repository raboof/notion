/*
 * ion/ioncore/extlconv.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_EXTLCONV_H
#define ION_IONCORE_EXTLCONV_H

#include "common.h"
#include "region.h"
#include "extl.h"

extern ExtlTab managed_list_to_table(WRegion *list, 
                                    bool (*filter)(WRegion *r));

extern bool extl_table_is_bool_set(ExtlTab tab, const char *entry);

extern bool extl_table_to_rectangle(ExtlTab tab, WRectangle *rect);
extern ExtlTab extl_table_from_rectangle(const WRectangle *rect);

extern bool extl_table_gets_rectangle(ExtlTab tab, const char *nam,
                                      WRectangle *rect);
extern void extl_table_sets_rectangle(ExtlTab tab, const char *nam,
                                      const WRectangle *rect);

#endif /* ION_IONCORE_EXTLCONV_H */

