/*
 * ion/ioncore/readconfig.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_READCONFIG_H
#define ION_IONCORE_READCONFIG_H

#include "common.h"
#include "extl.h"

typedef int WTryConfigFn(const char *file, void *param);

enum{
    IONCORE_TRYCONFIG_MEMERROR=-3,
    IONCORE_TRYCONFIG_NOTFOUND=-2,
    IONCORE_TRYCONFIG_LOAD_FAILED=-1,
    IONCORE_TRYCONFIG_CALL_FAILED=0,
    IONCORE_TRYCONFIG_OK=1
};


extern bool ioncore_set_userdirs(const char *appname);
extern bool ioncore_set_sessiondir(const char *session);
extern bool ioncore_add_scriptdir(const char *dir);
extern bool ioncore_add_moduledir(const char *dir);
extern const char* ioncore_userdir();

extern int ioncore_try_config(const char *module, WTryConfigFn *tryfn, 
                              void *tryfnparam);

extern char *ioncore_lookup_script(const char *file, const char *sp);

extern bool ioncore_read_config(const char *file, const char *sp, 
                                bool warn_nx);

extern char *ioncore_get_savefile(const char *module);
extern bool ioncore_read_savefile(const char *module, ExtlTab *tabret);
extern bool ioncore_write_savefile(const char *module, ExtlTab tab);

#endif /* ION_IONCORE_READCONFIG_H */
