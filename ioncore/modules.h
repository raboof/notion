/*
 * ion/ioncore/modules.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_MODULES_H
#define ION_IONCORE_MODULES_H

#include "common.h"

extern bool load_module(const char *name);
extern void unload_modules();
extern bool init_module_support();

#endif /* ION_IONCORE_MODULES_H */
