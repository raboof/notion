/*
 * wmcore/commandsq.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_COMMANDSQ_H
#define WMCORE_COMMANDSQ_H

#include "thing.h"

extern bool execute_command_sequence(WThing *thing, char *fn,
									 WFunclist *funclist);

#endif /* WMCORE_COMMANDSQ_H */
