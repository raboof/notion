/*
 * ion/ioncore/modules.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_MODULES_H
#define ION_IONCORE_MODULES_H

#include "common.h"

extern bool load_module(const char *name);
extern void unload_modules();
extern bool init_module_support();

#endif /* ION_IONCORE_MODULES_H */
