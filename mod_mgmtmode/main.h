/*
 * ion/mod_mgmtmode/main.h
 *
 * Copyright (c) Tuomo Valkonen 2004-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_MGMTMODE_MAIN_H
#define ION_MOD_MGMTMODE_MAIN_H

#include <ioncore/binding.h>

extern bool mod_mgmtmode_init();
extern void mod_mgmtmode_deinit();

extern WBindmap *mod_mgmtmode_bindmap;

#endif /* ION_MOD_MGMTMODE_MAIN_H */
