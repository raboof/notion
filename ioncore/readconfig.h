/*
 * ion/ioncore/readconfig.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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

typedef int TryConfigFn(const char *file, void *param);

enum{
	TRYCONFIG_MEMERROR=-3,
	TRYCONFIG_NOTFOUND=-2,
	TRYCONFIG_LOAD_FAILED=-1,
	TRYCONFIG_CALL_FAILED=0,
	TRYCONFIG_OK=1
};


extern bool ioncore_add_scriptdir(const char *dir);
extern bool ioncore_add_moduledir(const char *dir);
extern bool ioncore_add_userdirs(const char *appname);
extern bool ioncore_add_default_dirs();
extern bool ioncore_set_sessiondir(const char *appname, const char *session);

extern int try_config_for(const char *module, TryConfigFn *tryfn, 
						  void *tryfnparam);

extern bool read_config(const char *cfgfile);
extern bool read_config_for(const char *module);
extern bool read_config_for_args(const char *module, bool warn_nx,
								 const char *spec, const char *rspec, ...);

extern char *get_savefile_for(const char *module);

#endif /* ION_IONCORE_READCONFIG_H */
