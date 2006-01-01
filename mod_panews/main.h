/*
 * ion/panews/main.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_PANEWS_MAIN_H
#define ION_PANEWS_MAIN_H

#include <ioncore/binding.h>
#include <ioncore/regbind.h>

extern bool mod_panews_init();
extern void mod_panews_deinit();

extern WBindmap *mod_panews_panews_bindmap;
extern WBindmap *mod_panews_unusedwin_bindmap;
extern WBindmap *mod_panews_frame_bindmap;

#endif /* ION_PANEWS_MAIN_H */
