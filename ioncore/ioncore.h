/*
 * ion/ioncore/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_IONCORE_H
#define ION_IONCORE_IONCORE_H

#include "common.h"
#include "hooks.h"

#define IONCORE_STARTUP_ONEROOT    0x0001
#define IONCORE_STARTUP_NOXINERAMA 0x0002

extern bool ioncore_init(const char *prog, int argc, char *argv[],
                         const char *localedir);
extern bool ioncore_startup(const char *display, const char *cfgfile, 
                            int flags);
extern void ioncore_deinit();

extern const char *ioncore_aboutmsg();
extern const char *ioncore_version();

/* These hooks have no parameters. */
extern WHook *ioncore_post_layout_setup_hook;
extern WHook *ioncore_snapshot_hook;
extern WHook *ioncore_deinit_hook;

#endif /* ION_IONCORE_IONCORE_H */
