/*
 * ion/floatws/main.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_FLOATWS_MAIN_H
#define ION_FLOATWS_MAIN_H

#include <ioncore/binding.h>

extern bool floatws_module_init();
extern void floatws_module_deinit();

extern WBindmap floatws_bindmap;
extern WBindmap floatframe_bindmap;

#endif /* ION_FLOATWS_MAIN_H */
