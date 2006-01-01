/*
 * ion/ioncore/exec.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
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
#include <libextl/extl.h>

enum{
    IONCORE_SM_RESIGN,
    IONCORE_SM_SHUTDOWN,
    IONCORE_SM_RESTART,
    IONCORE_SM_RESTART_OTHER,
    IONCORE_SM_SNAPSHOT
};

extern void ioncore_do_exec(const char *cmd);
extern bool ioncore_exec_on_rootwin(WRootWin *rootwin, const char *cmd);
extern bool ioncore_exec(const char *cmd);
extern void ioncore_setup_environ(int scr);
extern bool ioncore_popen_bgread(const char *cmd, ExtlFn h, ExtlFn errh);

extern bool ioncore_set_smhook(void (*fn)(int what));

extern void ioncore_restart_other(const char *cmd);
extern void ioncore_restart();
extern void ioncore_shutdown();
extern void ioncore_resign();
extern void ioncore_snapshot();

extern void ioncore_do_exit();
extern void ioncore_do_restart();
extern bool ioncore_do_snapshot();

extern void ioncore_emergency_snapshot();

#endif /* ION_IONCORE_EXEC_H */
