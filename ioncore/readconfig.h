/*
 * ion/libextl/readconfig.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef LIBEXTL_READCONFIG_H
#define LIBEXTL_READCONFIG_H

#include <libtu/types.h>
#include "extl.h"

typedef int WTryConfigFn(const char *file, void *param);

enum{
    EXTL_TRYCONFIG_MEMERROR=-3,
    EXTL_TRYCONFIG_NOTFOUND=-2,
    EXTL_TRYCONFIG_LOAD_FAILED=-1,
    EXTL_TRYCONFIG_CALL_FAILED=0,
    EXTL_TRYCONFIG_OK=1
};


extern bool extl_set_userdirs(const char *appname);
extern bool extl_set_sessiondir(const char *session);
extern bool extl_set_searchpath(const char *path);
extern bool extl_add_searchdir(const char *dir);

extern const char *extl_userdir();
extern const char *extl_sessiondir();
extern const char *extl_searchpath();

extern int extl_try_config(const char *fname, const char *cfdir,
                           WTryConfigFn *tryfn, void *tryfnparam,
                           const char *ext1, const char *ext2);

extern char *extl_lookup_script(const char *file, const char *sp);

extern bool extl_read_config(const char *file, const char *sp, 
                             bool warn_nx);

extern char *extl_get_savefile(const char *module);
extern bool extl_read_savefile(const char *module, ExtlTab *tabret);
extern bool extl_write_savefile(const char *module, ExtlTab tab);

#endif /* LIBEXTL_READCONFIG_H */
