/*
 * ion/ioncore/commandsq.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_COMMANDSQ_H
#define ION_IONCORE_COMMANDSQ_H

#include "common.h"
#include "region.h"
#include "function.h"

extern bool execute_command_sequence(WRegion *reg, char *fn);
extern bool execute_command_sequence_restricted(WRegion *reg, char *fn,
												WFunclist *funclist);
extern void commands_at_leaf();

#endif /* ION_IONCORE_COMMANDSQ_H */
