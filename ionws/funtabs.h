/*
 * ion/ionws/funtab.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONWS_FUNTAB_H
#define ION_IONWS_FUNTAB_H

#include <ioncore/function.h>

extern WFunclist ionws_funclist;
extern WFunclist ionframe_funclist;
extern WFunclist ionframe_moveres_funclist;

extern bool ionws_module_init_funclists();
extern void ionws_module_clear_funclists();

#endif /* ION_IONWS_FUNTAB_H */
