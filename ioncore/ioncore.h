/*
 * ion/ioncore/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_IONCORE_H
#define ION_IONCORE_IONCORE_H

#include "common.h"

#define IONCORE_STARTUP_ONEROOT    0x0001
#define IONCORE_STARTUP_NOXINERAMA 0x0002

/* argc and argv are needed for restart_wm */
bool ioncore_init(const char *appname, int argc, char *argv[]);
bool ioncore_init_i18n();
bool ioncore_startup(const char *display, const char *cfgfile, int flags);
void ioncore_deinit();

bool ioncore_is_i18n();

#endif /* ION_IONCORE_IONCORE_H */
