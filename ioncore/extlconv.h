/*
 * ion/ioncore/extlconv.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_EXTLCONV_H
#define ION_IONCORE_EXTLCONV_H

#include <libextl/extl.h>
#include <libtu/iterable.h>
#include <libtu/setparam.h>
#include "common.h"
#include "region.h"

extern ExtlTab extl_obj_iterable_to_table(ObjIterator *iter, void *st);

extern bool extl_table_is_bool_set(ExtlTab tab, const char *entry);

extern bool extl_table_to_rectangle(ExtlTab tab, WRectangle *rect);
extern ExtlTab extl_table_from_rectangle(const WRectangle *rect);

extern bool extl_table_gets_rectangle(ExtlTab tab, const char *nam,
                                      WRectangle *rect);
extern void extl_table_sets_rectangle(ExtlTab tab, const char *nam,
                                      const WRectangle *rect);

#endif /* ION_IONCORE_EXTLCONV_H */

