/*
 * ion/ionws/conf.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/readconfig.h>
#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <ioncore/framep.h>
#include <ioncore/extl.h>
#include "ionframe.h"
#include "bindmaps.h"


static StringIntMap frame_areas[]={
	{"border", 		WFRAME_AREA_BORDER},
	{"tab", 		WFRAME_AREA_TAB},
	{"empty_tab", 	WFRAME_AREA_TAB},
	{"client", 		WFRAME_AREA_CLIENT},
	END_STRINGINTMAP
};


/*EXTL_DOC
 * Add a set of bindings to the bindings available in every object
 * on WIonWS:s.
 */
EXTL_EXPORT
bool ionws_bindings(ExtlTab tab)
{
	return process_bindings(&ionws_bindmap, NULL, tab);
}


/*EXTL_DOC
 * Add a set of bindings to the bindings available in WIonFrames.
 */
EXTL_EXPORT
bool ionframe_bindings(ExtlTab tab)
{
	return process_bindings(&ionframe_bindmap, frame_areas, tab);
}


/*EXTL_DOC
 * Describe WIonFrame resize mode bindings.
 */
EXTL_EXPORT
bool ionframe_moveres_bindings(ExtlTab tab)
{
	return process_bindings(&ionframe_moveres_bindmap, NULL, tab);
}


bool ionws_module_read_config()
{
	return read_config("ionws");
}
