/*
 * ion/ioncore/defer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_DEFER_H
#define ION_IONCORE_DEFER_H

#include "common.h"

extern bool defer_action(WObj *obj, void (*action)(WObj*));
extern bool defer_destroy(WObj *obj);
extern void execute_deferred();

#endif /* ION_IONCORE_DEFER_H */
