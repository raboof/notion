/*
 * ion/ioncore/conf-bindings.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_CONF_BINDINGS_H
#define ION_IONCORE_CONF_BINDINGS_H

#include <libtu/map.h>

#include "binding.h"
#include "extl.h"

extern bool process_bindings(WBindmap *bindmap, StringIntMap *areamap,
							 ExtlTab tab);

#endif /* ION_IONCORE_CONF_BINDINGS_H */
