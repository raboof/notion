/*
 * ion/ioncore/bindmaps.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
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
#include "framep.h"


WBindmap ioncore_rootwin_bindmap=BINDMAP_INIT;
WBindmap ioncore_mplex_bindmap=BINDMAP_INIT;
WBindmap ioncore_frame_bindmap=BINDMAP_INIT;
WBindmap ioncore_moveres_bindmap=BINDMAP_INIT;


EXTL_EXPORT
bool __defbindings_WScreen(ExtlTab tab)
{
	return process_bindings(&ioncore_rootwin_bindmap, NULL, tab);
}


EXTL_EXPORT
bool __defbindings_WMPlex(ExtlTab tab)
{
	return process_bindings(&ioncore_mplex_bindmap, NULL, tab);
}


static StringIntMap frame_areas[]={
	{"border", 		WFRAME_AREA_BORDER},
	{"tab", 		WFRAME_AREA_TAB},
	{"empty_tab", 	WFRAME_AREA_TAB},
	{"client", 		WFRAME_AREA_CLIENT},
	END_STRINGINTMAP
};


EXTL_EXPORT
bool __defbindings_WFrame(ExtlTab tab)
{
	return process_bindings(&ioncore_frame_bindmap, frame_areas, tab);
}


EXTL_EXPORT
bool __defbindings_WMoveresMode(ExtlTab tab)
{
	return process_bindings(&ioncore_moveres_bindmap, NULL, tab);
}


void ioncore_deinit_bindmaps()
{
	deinit_bindmap(&ioncore_rootwin_bindmap);
	deinit_bindmap(&ioncore_mplex_bindmap);
	deinit_bindmap(&ioncore_frame_bindmap);
	deinit_bindmap(&ioncore_moveres_bindmap);
}

