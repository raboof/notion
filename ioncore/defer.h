/*
 * ion/ioncore/defer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_DEFER_H
#define ION_IONCORE_DEFER_H

#include "common.h"

extern bool defer_action(WThing *thing, void (*action)(WThing*));
extern bool defer_destroy(WThing *thing);
extern void execute_deferred();

#endif /* ION_IONCORE_DEFER_H */
