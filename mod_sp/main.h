/*
 * ion/mod_sp/main.h
 *
 * Copyright (c) Tuomo Valkonen 2004-2007.
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_SP_MAIN_H
#define ION_MOD_SP_MAIN_H

#include <ioncore/binding.h>

extern bool mod_sp_init();
extern void mod_sp_deinit();

extern WBindmap *mod_sp_scratchpad_bindmap;

#define CF_SCRATCHPAD_DEFAULT_W 640
#define CF_SCRATCHPAD_DEFAULT_H 480

#endif /* ION_MOD_SP_MAIN_H */
