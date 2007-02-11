/*
 * ion/ioncore/bindmaps.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/map.h>
#include "binding.h"

#ifndef ION_IONCORE_BINDMAP_H
#define ION_IONCORE_BINDMAP_H

extern WBindmap *ioncore_screen_bindmap;
extern WBindmap *ioncore_mplex_bindmap;
extern WBindmap *ioncore_mplex_toplevel_bindmap;
extern WBindmap *ioncore_frame_bindmap;
extern WBindmap *ioncore_frame_toplevel_bindmap;
extern WBindmap *ioncore_frame_floating_bindmap;
extern WBindmap *ioncore_frame_tiled_bindmap;
extern WBindmap *ioncore_frame_transient_bindmap;
extern WBindmap *ioncore_moveres_bindmap;
extern WBindmap *ioncore_group_bindmap;
extern WBindmap *ioncore_groupcw_bindmap;
extern WBindmap *ioncore_groupws_bindmap;
extern WBindmap *ioncore_clientwin_bindmap;

extern void ioncore_deinit_bindmaps();
extern bool ioncore_init_bindmaps();
extern void ioncore_refresh_bindmaps();

extern WBindmap *ioncore_alloc_bindmap(const char *name, 
                                       const StringIntMap *areas);
extern WBindmap *ioncore_alloc_bindmap_frame(const char *name);
extern void ioncore_free_bindmap(const char *name, WBindmap *bm);
extern WBindmap *ioncore_lookup_bindmap(const char *name);

extern bool ioncore_do_defbindings(const char *name, ExtlTab tab);
extern ExtlTab ioncore_do_getbindings();

extern WBindmap *ioncore_create_cycle_bindmap(uint kcb, uint state, 
                                              ExtlFn cycle, ExtlFn bcycle);
extern WBindmap *region_add_cycle_bindmap(WRegion *reg, 
                                          uint kcb, uint state, 
                                          ExtlFn cycle, ExtlFn bcycle);

#endif /* ION_IONCORE_BINDMAP_H */

