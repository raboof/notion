/*
 * ion/ioncore/bindmaps.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include "common.h"
#include "conf-bindings.h"
#include "binding.h"
#include "extl.h"

WBindmap ioncore_rootwin_bindmap=BINDMAP_INIT;
WBindmap ioncore_mplex_bindmap=BINDMAP_INIT;
WBindmap ioncore_genframe_bindmap=BINDMAP_INIT;

/*EXTL_DOC
 * Add a set of bindings available everywhere. The bound functions
 * should accept \type{WScreen} as argument.
 */
EXTL_EXPORT
void global_bindings(ExtlTab tab)
{
	process_bindings(&ioncore_rootwin_bindmap, NULL, tab);
}


/*EXTL_DOC
 * Add a set of bindings available in \type{WMPlex}es (screens and all
 * types of frames).
 */
EXTL_EXPORT
void mplex_bindings(ExtlTab tab)
{
	process_bindings(&ioncore_mplex_bindmap, NULL, tab);
}


/*EXTL_DOC
 * Add a set of bindings available in \type{WGenFrame}s (all types of frames).
 */
EXTL_EXPORT
void genframe_bindings(ExtlTab tab)
{
	process_bindings(&ioncore_genframe_bindmap, NULL, tab);
}

