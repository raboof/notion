/*
 * ion/ioncore/signal.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_SIGNAL_H
#define ION_IONCORE_SIGNAL_H

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "common.h"

INTRSTRUCT(WTimer);

DECLSTRUCT(WTimer){
    struct timeval when;
    void (*handler)();
    WTimer *next;
    Watch paramwatch;
};

#define TIMER_INIT(FUN) {{0, 0}, FUN, NULL, WWATCH_INIT}

extern bool ioncore_check_signals();
extern void ioncore_trap_signals();

extern void timer_set(WTimer *timer, uint msecs);
extern void timer_set_param(WTimer *timer, uint msecs, Obj *param);
extern void timer_reset(WTimer *timer);
extern bool timer_is_set(WTimer *timer);

#endif /* ION_IONCORE_SIGNAL_H */
