/*
 * ion/ioncore/conf-bindings.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_CONF_BINDINGS_H
#define ION_IONCORE_CONF_BINDINGS_H

#include <libtu/map.h>

#include "binding.h"
#include <libextl/extl.h>

extern bool bindmap_defbindings(WBindmap *bindmap, ExtlTab tab, bool submap);
extern ExtlTab bindmap_getbindings(WBindmap *bindmap);

#endif /* ION_IONCORE_CONF_BINDINGS_H */
