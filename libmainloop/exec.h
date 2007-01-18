/*
 * ion/libmainloop/exec.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
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
                           int *infd, int *outfd, int *errfd);
extern pid_t mainloop_do_spawn(const char *cmd, 
                              void (*initenv)(void *p), void *p,
                              int *infd, int *outfd, int *errfd);
extern pid_t mainloop_spawn(const char *cmd);

extern pid_t mainloop_popen_bgread(const char *cmd, 
                                   void (*initenv)(void *p), void *p,
                                   ExtlFn handler, ExtlFn errhandler);

extern bool mainloop_register_input_fd_extlfn(int fd, ExtlFn fn);
extern bool mainloop_process_pipe_extlfn(int fd, ExtlFn fn);

extern void cloexec_braindamage_fix(int fd);

#endif /* ION_LIBMAINLOOP_EXEC_H */
