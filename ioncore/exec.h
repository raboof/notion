/*
 * wmcore/exec.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_EXEC_H
#define WMCORE_EXEC_H

#include "common.h"
#include "screen.h"

extern void wm_do_exec(const char *cmd);
extern void wm_exec(WScreen *scr, const char *cmd);
extern void do_open_with(WScreen *scr, const char *cmd,
						 const char *filename);
extern void wm_restart_other(const char *cmd);
extern void wm_restart();
extern void wm_exit();
extern void wm_exitret();
extern void setup_environ(int scr);

#endif /* WMCORE_EXEC_H */
