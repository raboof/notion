/*
 * ion/floatws/main.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_FLOATWS_MAIN_H
#define ION_FLOATWS_MAIN_H

extern bool floatws_module_init();
extern void floatws_module_deinit();

extern WBindmap floatws_bindmap;
extern WBindmap floatframe_bindmap;
extern WFunclist floatws_funclist;
extern WFunclist floatframe_funclist;

#endif /* ION_FLOATWS_MAIN_H */
