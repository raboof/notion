/*
 * ion/mod_ionws/split-dock.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_IONWS_SPLIT_DOCK_H
#define ION_MOD_IONWS_SPLIT_DOCK_H

#include "split.h"

extern bool split_try_sink_dock(WSplit *node, bool iterate, bool force);
extern bool split_try_unsink_dock(WSplit *node, bool iterate, bool force);
extern bool split_regularise_dock(WSplit *dock);

#endif /* ION_MOD_IONWS_SPLIT_DOCK_H */
