/*
 * ion/mod_floatws/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_FLOATWS__PLACEMENT_H
#define ION_MOD_FLOATWS__PLACEMENT_H

#include <ioncore/rectangle.h>
#include "floatws.h"

extern void floatws_calc_placement(WFloatWS *ws, WRectangle *geom);
extern void mod_floatws_set_placement_method(const char *method);

#endif /* ION_MOD_FLOATWS__PLACEMENT_H */
