/*
 * ion/ioncore/readconfig.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_READCONFIG_H
#define ION_IONCORE_READCONFIG_H

/*extern bool ioncore_set_paths(const char *appname, const char *etcdir,
							 const char *libdir);*/
extern bool ioncore_add_confdir(const char *dir);
extern bool ioncore_add_libdir(const char *dir);

extern char *get_core_cfgfile_for_scr(const char *module, int xscr);
extern char *get_core_cfgfile_for(const char *module);
extern char *get_cfgfile_for_scr(const char *module, int xscr);
extern char *get_cfgfile_for(const char *module);
extern char *get_savefile_for_scr(const char *module, int xscr);
extern char *get_savefile_for(const char *module);

extern bool read_config(const char *cfgfile);
extern bool read_config_for(const char *module);
extern bool read_config_for_scr(const char *module, int xscr);
extern bool read_core_config_for(const char *module);
extern bool read_core_config_for_scr(const char *module, int xscr);

/* The library path stuff is in readconfig.c */
extern char *find_module(const char *fname);

#endif /* ION_IONCORE_READCONFIG_H */
