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
#include "gr.h"
#include "extl.h"


typedef struct{
    struct rb_node *rb;
    struct rb_node *rb_unnamed;
    WRegion *list;
    bool initialised;
} WNamespace;


extern WNamespace ioncore_internal_ns;
extern WNamespace ioncore_clientwin_ns;


extern bool region_init_name(WRegion *reg);

extern bool region_set_name(WRegion *reg, const char *name);
extern bool region_set_name_exact(WRegion *reg, const char *name);
extern bool clientwin_set_name(WClientWin *cwin, const char *name);

extern void region_unuse_name(WRegion *reg);
extern void region_do_unuse_name(WRegion *reg, bool insert_unnamed);

extern const char *region_name(WRegion *reg);

extern char *region_make_label(WRegion *reg, int maxw, GrBrush *brush);

extern ExtlTab ioncore_region_list(const char *typenam);
extern ExtlTab ioncore_clientwin_list();
extern WRegion *ioncore_lookup_region(const char *cname, const char *typenam);
extern WClientWin *ioncore_lookup_clientwin(const char *cname);

#endif /* ION_IONCORE_NAMES_H */
