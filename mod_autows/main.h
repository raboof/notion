/*
 * ion/autows/main.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_AUTOWS_MAIN_H
#define ION_AUTOWS_MAIN_H

#include <ioncore/binding.h>
#include <ioncore/regbind.h>

extern bool mod_autows_init();
extern void mod_autows_deinit();

extern WBindmap *mod_autows_autows_bindmap;
extern WBindmap *mod_autows_autoframe_bindmap;

#endif /* ION_AUTOWS_MAIN_H */
