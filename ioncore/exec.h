/*
 * ion/ioncore/exec.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_EXEC_H
#define ION_IONCORE_EXEC_H

#include "common.h"
#include "rootwin.h"
#include "extl.h"

extern void ioncore_do_exec(const char *cmd);
extern bool ioncore_exec_on_rootwin(WRootWin *rootwin, const char *cmd);
extern bool ioncore_exec(const char *cmd);
extern void ioncore_restart_other(const char *cmd);
extern void ioncore_restart();
extern void ioncore_exit();
extern void ioncore_setup_environ(int scr);
extern bool ioncore_popen_bgread(const char *cmd, ExtlFn handler);

#endif /* ION_IONCORE_EXEC_H */
