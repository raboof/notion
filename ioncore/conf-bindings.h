/*
 * ion/ioncore/conf-bindings.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2009.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_CONF_BINDINGS_H
#define ION_IONCORE_CONF_BINDINGS_H

#include <libtu/map.h>

#include "binding.h"
#include <libextl/extl.h>

extern bool bindmap_defbindings(WBindmap *bindmap, ExtlTab tab, bool submap);
extern ExtlTab bindmap_getbindings(WBindmap *bindmap);

extern bool ioncore_parse_keybut(const char *str,
                                 uint *mod_ret, uint *ksb_ret,
                                 bool button, bool init_any);

#endif /* ION_IONCORE_CONF_BINDINGS_H */
