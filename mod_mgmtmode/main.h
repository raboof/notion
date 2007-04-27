/*
 * ion/mod_mgmtmode/main.h
 *
 * Copyright (c) Tuomo Valkonen 2004-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_MGMTMODE_MAIN_H
#define ION_MOD_MGMTMODE_MAIN_H

#include <ioncore/binding.h>

extern bool mod_mgmtmode_init();
extern void mod_mgmtmode_deinit();

extern WBindmap *mod_mgmtmode_bindmap;

#endif /* ION_MOD_MGMTMODE_MAIN_H */
