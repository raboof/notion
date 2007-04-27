/*
 * ion/mainloop/signal.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_LIBMAINLOOP_SIGNAL_H
#define ION_LIBMAINLOOP_SIGNAL_H

#include <sys/time.h>
#include <sys/signal.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <libtu/obj.h>
#include <libtu/types.h>
#include <libextl/extl.h>

#include "hooks.h"

INTRCLASS(WTimer);

typedef void WTimerHandler(WTimer *timer, Obj *obj);


DECLCLASS(WTimer){
    Obj obj;
    struct timeval when;
    WTimer *next;
    WTimerHandler *handler;
    Watch objwatch;
    ExtlFn extl_handler;
};

extern bool timer_init(WTimer *timer);
extern void timer_deinit(WTimer *timer);

extern WTimer *create_timer();
extern WTimer *create_timer_extl_owned();

extern void timer_set(WTimer *timer, uint msecs, WTimerHandler *handler,
                      Obj *obj);
extern void timer_set_extl(WTimer *timer, uint msecs, ExtlFn fn);

extern void timer_reset(WTimer *timer);
extern bool timer_is_set(WTimer *timer);

extern bool mainloop_check_signals();
extern void mainloop_trap_signals(const sigset_t *set);

extern WHook *mainloop_sigchld_hook;
extern WHook *mainloop_sigusr2_hook;

#endif /* ION_LIBMAINLOOP_SIGNAL_H */
