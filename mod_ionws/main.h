/*
 * ion/mod_ionws/main.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_IONWS_MAIN_H
#define ION_MOD_IONWS_MAIN_H

#include <ioncore/binding.h>
#include <ioncore/regbind.h>

extern bool mod_ionws_init();
extern void mod_ionws_deinit();

extern WBindmap *mod_ionws_ionws_bindmap;
extern WBindmap *mod_ionws_ionframe_bindmap;

#endif /* ION_MOD_IONWS_MAIN_H */
