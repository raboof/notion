/*
 * ion/libmainloop/exec.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
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
