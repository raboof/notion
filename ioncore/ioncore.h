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

#ifndef ION_IONCORE_WMCORE_H
#define ION_IONCORE_WMCORE_H

#include "common.h"

/* argc and argv are needed for restart_wm */
bool ioncore_init(int argc, char *argv[]);
bool ioncore_startup(const char *display, bool oneroot, 
					 const char *cfgfile);
void ioncore_deinit();

#endif /* ION_IONCORE_WMCORE_H */
