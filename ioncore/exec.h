/*
 * ion/ioncore/exec.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_EXEC_H
#define ION_IONCORE_EXEC_H

#include <libextl/extl.h>
#include <libmainloop/hooks.h>
#include "common.h"
#include "rootwin.h"

enum{
    IONCORE_SM_RESIGN,
    IONCORE_SM_SHUTDOWN,
    IONCORE_SM_RESTART,
    IONCORE_SM_RESTART_OTHER,
    IONCORE_SM_SNAPSHOT
};


INTRSTRUCT(WExecP);

DECLSTRUCT(WExecP){
    WRegion *target;
    const char *cmd;
    const char *wd;
};


extern bool ioncore_exec(const char *cmd);
extern int ioncore_do_exec_on(WRegion *reg, const char *cmd, const char *wd,
                              ExtlFn errh);
extern bool ioncore_popen_bgread(const char *cmd, ExtlFn h, ExtlFn errh);
extern void ioncore_setup_environ(const WExecP *p);
extern void ioncore_setup_display(int xscr);

extern bool ioncore_set_smhook(void (*fn)(int what));

extern void ioncore_restart_other(const char *cmd);
extern void ioncore_restart();
extern void ioncore_shutdown();
extern void ioncore_resign();
extern void ioncore_snapshot();

extern void ioncore_do_exit();
extern void ioncore_do_restart();
extern bool ioncore_do_snapshot(bool save_layout);

extern void ioncore_emergency_snapshot();

/* const WExecP* parameter */
extern WHook *ioncore_exec_environ_hook;

#endif /* ION_IONCORE_EXEC_H */
