/*
 * ion/ioncore/funtabs.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_FUNTABS_H
#define ION_IONCORE_FUNTABS_H

#include "function.h"
#include "binding.h"

extern WFunclist ioncore_screen_funclist;
extern WFunclist ioncore_clientwin_funclist;
extern WFunclist ioncore_viewport_funclist;
extern WFunclist ioncore_genframe_funclist;
/*extern WFunclist ioncore_moveres_funclist;*/

extern void ioncore_init_funclists();

extern WBindmap ioncore_screen_bindmap;

#endif /* ION_IONCORE_FUNTABS_H */
