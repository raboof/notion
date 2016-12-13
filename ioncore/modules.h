/*
 * ion/ioncore/modules.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
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
