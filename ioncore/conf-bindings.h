/*
 * wmcore/conf-bindings.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_CONF_BINDINGS_H
#define WMCORE_CONF_BINDINGS_H

#include <libtu/parser.h>
#include <libtu/map.h>

#include "binding.h"
#include "function.h"

extern ConfOpt wmcore_binding_opts[];

extern bool wmcore_begin_bindings(WBindmap *bindmap,
								  const StringIntMap *areas);

#endif /* WMCORE_CONF_BINDINGS_H */
