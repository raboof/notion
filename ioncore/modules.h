/*
 * ion/ioncore/modules.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_MODULES_H
#define ION_IONCORE_MODULES_H

#include "common.h"

extern bool ioncore_load_module(const char *name);
extern void ioncore_unload_modules();
extern bool ioncore_init_module_support();

typedef struct{
    const char *name;
    bool (*init)();
    void (*deinit)();
    bool loaded;
} WStaticModuleInfo;

#endif /* ION_IONCORE_MODULES_H */
