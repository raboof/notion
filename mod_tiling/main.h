/*
 * ion/mod_tiling/main.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_TILING_MAIN_H
#define ION_MOD_TILING_MAIN_H

#include <ioncore/binding.h>
#include <ioncore/regbind.h>

extern bool mod_tiling_init();
extern void mod_tiling_deinit();

extern WBindmap *mod_tiling_tiling_bindmap;
extern WBindmap *mod_tiling_frame_bindmap;

extern int mod_tiling_raise_delay;

#endif /* ION_MOD_TILING_MAIN_H */
