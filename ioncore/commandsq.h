/*
 * wmcore/commandsq.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_COMMANDSQ_H
#define WMCORE_COMMANDSQ_H

#include "thing.h"
#include "function.h"

extern bool execute_command_sequence(WThing *thing, char *fn);
extern bool execute_command_sequence_restricted(WThing *thing, char *fn,
												WFunclist *funclist);
extern void commands_at_leaf();

#endif /* WMCORE_COMMANDSQ_H */
