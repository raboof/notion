/*
 * wmcore/saveload.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_SAVELOAD_H
#define WMCORE_SAVELOAD_H

#include <libtu/tokenizer.h>

#include "common.h"
#include "region.h"
#include "viewport.h"


typedef WRegion *WRegionLoadCreateFn(WRegion *par, WRectangle geom,
									 Tokenizer *tokz);

extern bool register_region_load_create_fn(const char *name,
										   WRegionLoadCreateFn *fn);
extern void unregister_region_load_create_fn(const char *name);
extern WRegionLoadCreateFn *lookup_reg_load_create_fn(const char *name);


extern bool region_supports_save(WRegion *reg);
DYNFUN bool region_save_to_file(WRegion *reg, FILE *file, int lvl);
extern WRegion *load_create_region(WRegion *par, WRectangle geom,
								   Tokenizer *tokz, int n, Token *toks);


extern void begin_saved_region(WRegion *reg, FILE *file, int lvl);
extern void end_saved_region(WRegion *reg, FILE *file, int lvl);
extern void save_indent_line(FILE *file, int lvl);

extern bool load_workspaces(WViewport *vp);
extern bool save_workspaces(WViewport *vp);


#endif /* WMCORE_SAVELOAD_H */

