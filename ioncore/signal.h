/*
 * wmcore/signal.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_SIGNAL_H
#define WMCORE_SIGNAL_H

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "common.h"

INTRSTRUCT(WTimer)

DECLSTRUCT(WTimer){
	struct timeval when;
	void (*handler)(WTimer *timer);
	WTimer *next;
};

#define INIT_TIMER(FUN) {{0, 0}, FUN, NULL}

extern void check_signals();
extern void trap_signals();
extern void set_timer(WTimer *timer, uint msecs);
extern void reset_timer(WTimer *timer);

#endif /* WMCORE_SIGNAL_H */
