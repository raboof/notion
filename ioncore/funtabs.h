/*
 * wmcore/funtabs.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_FUNTABS_H
#define WMCORE_FUNTABS_H

#include "function.h"
#include "binding.h"

extern WFunclist wmcore_screen_funclist;
extern WFunclist wmcore_clientwin_funclist;
extern WFunclist wmcore_viewport_funclist;

extern void wmcore_init_funclists();

extern WBindmap wmcore_screen_bindmap;
extern WBindmap wmcore_clientwin_bindmap;
extern WBindmap wmcore_viewport_bindmap;

#endif /* WMCORE_FUNTABS_H */
