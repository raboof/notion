/*
 * ion/mod_floatws/main.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_FLOATWS_MAIN_H
#define ION_MOD_FLOATWS_MAIN_H

#include <ioncore/binding.h>

extern bool mod_floatws_init();
extern void mod_floatws_deinit();

extern WBindmap *mod_floatws_floatws_bindmap;
extern WBindmap *mod_floatws_floatframe_bindmap;

#endif /* ION_MOD_FLOATWS_MAIN_H */
