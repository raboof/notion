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

extern bool ioncore_add_scriptdir(const char *dir);
extern bool ioncore_add_moduledir(const char *dir);
extern bool ioncore_add_userdirs(const char *appname);
extern bool ioncore_add_default_dirs();

extern char *get_cfgfile_for_scr(const char *module, int xscr);
extern char *get_cfgfile_for(const char *module);
extern char *get_savefile_for_scr(const char *module, int xscr);
extern char *get_savefile_for(const char *module);

extern bool read_config(const char *cfgfile);
extern bool read_config_for(const char *module);
extern bool read_config_for_scr(const char *module, int xscr);

/* The library path stuff is in readconfig.c */
extern char *find_module(const char *fname);

#endif /* ION_IONCORE_READCONFIG_H */
