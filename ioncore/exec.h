/*
 * ion/ioncore/exec.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_EXEC_H
#define ION_IONCORE_EXEC_H

#include "common.h"
#include "rootwin.h"

extern void do_exec(const char *cmd);
extern bool exec_on_rootwin(WRootWin *rootwin, const char *cmd);
extern bool exec_on_wm_display(const char *cmd);
extern void restart_other_wm(const char *cmd);
extern void restart_wm();
extern void exit_wm();
extern void setup_environ(int scr);

#endif /* ION_IONCORE_EXEC_H */
