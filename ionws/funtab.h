/*
 * ion/funtab.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef ION_FUNTAB_H
#define ION_FUNTAB_H

#include <wmcore/function.h>
#include <wmcore/thing.h>

extern WFunclist ion_main_funclist;
extern WFunclist ion_moveres_funclist;

extern void init_funclists();

extern bool command_sequence(WThing *thing, char *fn);

#endif /* ION_FUNTAB_H */
