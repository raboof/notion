/*
 * ion/ioncore/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_IONCORE_H
#define ION_IONCORE_IONCORE_H

#include <libmainloop/hooks.h>
#include "common.h"

#define IONCORE_STARTUP_ONEROOT    0x0001

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

extern void ioncore_warn_nolog(const char *str);

#endif /* ION_IONCORE_IONCORE_H */
