/*
 * wmcore/readconfig.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_READCONFIG_H
#define WMCORE_READCONFIG_H

#include <libtu/parser.h>

extern bool wmcore_set_cfgpath(const char *appname, const char *etcdir);
	
extern char *get_core_cfgfile_for_scr(const char *module, int xscr);
extern char *get_core_cfgfile_for(const char *module);
extern char *get_cfgfile_for_scr(const char *module, int xscr);
extern char *get_cfgfile_for(const char *module);
extern char *get_savefile_for_scr(const char *module, int xscr);
extern char *get_savefile_for(const char *module);

extern bool read_config(const char *cfgfile, const ConfOpt *opts);
extern bool read_config_for(const char *module, const ConfOpt *opts);
extern bool read_config_for_scr(const char *module, int xscr,
								const ConfOpt *opts);
extern bool read_core_config_for(const char *module, const ConfOpt *opts);
extern bool read_core_config_for_scr(const char *module, int xscr,
									 const ConfOpt *opts);

#endif /* WMCORE_READCONFIG_H */
