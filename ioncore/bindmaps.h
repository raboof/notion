/*
 * ion/ioncore/bindmaps.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "binding.h"

#ifndef ION_IONCORE_BINDMAP_H
#define ION_IONCORE_BINDMAP_H

extern WBindmap ioncore_rootwin_bindmap;
extern WBindmap ioncore_mplex_bindmap;
extern WBindmap ioncore_frame_bindmap;
extern WBindmap ioncore_kbresize_bindmap;

extern void ioncore_deinit_bindmaps();

#endif /* ION_IONCORE_BINDMAP_H */

