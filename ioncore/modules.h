/*
 * wmcore/modules.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_MODULES_H
#define WMCORE_MODULES_H

#include "common.h"

extern bool load_module(const char *name);
extern void unload_modules();

#endif /* WMCORE_MODULES_H */
