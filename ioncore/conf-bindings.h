/*
 * ion/ioncore/conf-bindings.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_CONF_BINDINGS_H
#define ION_IONCORE_CONF_BINDINGS_H

#include <libtu/parser.h>
#include <libtu/map.h>

#include "binding.h"
#include "function.h"

extern ConfOpt ioncore_binding_opts[];

extern bool ioncore_begin_bindings(WBindmap *bindmap,
								  const StringIntMap *areas);

#endif /* ION_IONCORE_CONF_BINDINGS_H */
