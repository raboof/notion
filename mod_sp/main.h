/*
 * ion/mod_sp/main.h
 *
 * Copyright (c) Tuomo Valkonen 2004-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_SP_MAIN_H
#define ION_MOD_SP_MAIN_H

#include <ioncore/binding.h>

extern bool mod_sp_init();
extern void mod_sp_deinit();

extern WBindmap *mod_sp_scratchpad_bindmap;

#define CF_SCRATCHPAD_DEFAULT_W 800
#define CF_SCRATCHPAD_DEFAULT_H 600

#endif /* ION_MOD_SP_MAIN_H */
