/*
 * wmcore/defer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_DEFER_H
#define WMCORE_DEFER_H

#include "common.h"

extern bool defer_action(WThing *thing, void (*action)(WThing*));
extern bool defer_destroy(WThing *thing);
extern void execute_deferred();

#endif /* WMCORE_DEFER_H */
