/*
 * ion/ioncore/detach.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_DETACH_H
#define ION_IONCORE_DETACH_H

#include "region.h"

extern bool ioncore_detach(WRegion *reg, int sp);

extern void ioncore_unsqueeze(WRegion *reg, bool override);

#endif /* ION_IONCORE_DETACH_H */

