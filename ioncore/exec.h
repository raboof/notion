/*
 * ion/ioncore/exec.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_EXEC_H
#define ION_IONCORE_EXEC_H

#include "common.h"
#include "screen.h"

extern void ioncore_do_exec(const char *cmd);
extern void ioncore_exec(WScreen *scr, const char *cmd);
extern void do_open_with(WScreen *scr, const char *cmd,
						 const char *filename);
extern void ioncore_restart_other(const char *cmd);
extern void ioncore_restart();
extern void ioncore_exit();
extern void setup_environ(int scr);

#endif /* ION_IONCORE_EXEC_H */
