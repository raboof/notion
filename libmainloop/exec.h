/*
 * ion/libmainloop/exec.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_LIBMAINLOOP_EXEC_H
#define ION_LIBMAINLOOP_EXEC_H

#include <sys/types.h>
#include <unistd.h>

#include <libextl/extl.h>

extern void mainloop_do_exec(const char *cmd);
extern pid_t mainloop_fork(void (*fn)(void *p), void *p,
                           int *infd, int *outfd);
extern pid_t mainloop_do_spawn(const char *cmd, 
                              void (*initenv)(void *p), void *p,
                              int *infd, int *outfd);
extern pid_t mainloop_spawn(const char *cmd);

extern pid_t mainloop_popen_bgread(const char *cmd, ExtlFn handler,
                                   void (*initenv)(void *p), void *p);

#endif /* ION_LIBMAINLOOP_EXEC_H */
