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

extern WRegion *create_region_load(WWindow *par, const WFitParams *fp, 
                                   ExtlTab tab);

extern bool region_supports_save(WRegion *reg);
DYNFUN ExtlTab region_get_configuration(WRegion *reg);
extern ExtlTab region_get_base_configuration(WRegion *reg);

extern bool ioncore_init_layout();
extern bool ioncore_save_layout();

#endif /* ION_IONCORE_SAVELOAD_H */

