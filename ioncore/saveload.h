/*
 * ion/ioncore/saveload.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_SAVELOAD_H
#define ION_IONCORE_SAVELOAD_H

#include "common.h"
#include "region.h"
#include "screen.h"
#include "extl.h"
#include "rectangle.h"

extern WRegion *create_region_load(WWindow *par, const WRectangle *geom, 
								   ExtlTab tab);

extern bool region_supports_save(WRegion *reg);
DYNFUN bool region_save_to_file(WRegion *reg, FILE *file, int lvl);
extern void region_save_identity(WRegion *reg, FILE *file, int lvl);

extern void file_write_escaped_string(FILE *file, const char *str);
extern void file_indent(FILE *file, int lvl);

extern bool ioncore_load_workspaces();
extern bool ioncore_save_workspaces();

#endif /* ION_IONCORE_SAVELOAD_H */

