/*
 * ion/ionws/conf.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/readconfig.h>
#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <ioncore/genframep.h>
#include <ioncore/extl.h>
#include "ionframe.h"
#include "bindmaps.h"


static StringIntMap frame_areas[]={
	{"border", 		WGENFRAME_AREA_BORDER},
	{"tab", 		WGENFRAME_AREA_TAB},
	{"empty_tab", 	WGENFRAME_AREA_EMPTY_TAB},
	{"client", 		WGENFRAME_AREA_CLIENT},
	END_STRINGINTMAP
};


EXTL_EXPORT
void ionws_bindings(ExtlTab tab)
{
	process_bindings(&ionws_bindmap, NULL, tab);
}


EXTL_EXPORT
void ionframe_bindings(ExtlTab tab)
{
	process_bindings(&ionframe_bindmap, frame_areas, tab);
}


EXTL_EXPORT
void ionframe_moveres_bindings(ExtlTab tab)
{
	process_bindings(&ionframe_moveres_bindmap, NULL, tab);
}


bool ionws_module_read_config()
{
	return read_config_for("ionws");
}
