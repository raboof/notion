/*
 * ion/ioncore/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_WMCORE_H
#define ION_IONCORE_WMCORE_H

#include "common.h"

/* argc and argv are needed for restart_wm */
bool ioncore_startup(int argc, char *argv[], const char *display,
					 bool oneroot, const char *cfgfile);
void ioncore_deinit();

#endif /* ION_IONCORE_WMCORE_H */
