/*
 * ion/ioncore/saveload.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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

extern bool region_supports_save(WRegion *reg);
DYNFUN bool region_save_to_file(WRegion *reg, FILE *file, int lvl);
extern WRegion *load_create_region(WWindow *par, WRectangle geom, ExtlTab tab);

extern void write_escaped_string(FILE *file, const char *str);
extern void begin_saved_region(WRegion *reg, FILE *file, int lvl);
/*extern void end_saved_region(WRegion *reg, FILE *file, int lvl);*/
extern void save_indent_line(FILE *file, int lvl);

extern bool load_workspaces(WScreen *vp);
extern bool save_workspaces(WScreen *vp);

extern WRegion *region_add_managed_load(WRegion *mgr, ExtlTab tab);

extern void save_geom(WRectangle geom, FILE *file, int lvl);

extern void enable_workspace_saves(bool enable);

#endif /* ION_IONCORE_SAVELOAD_H */

