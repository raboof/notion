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


EXTL_EXPORT_AS(global, __defbindings_WScreen)
bool ioncore_defbindings_WScreen(ExtlTab tab)
{
	return bindmap_do_table(&ioncore_rootwin_bindmap, NULL, tab);
}


EXTL_EXPORT_AS(global, __defbindings_WMPlex)
bool ioncore_defbindings_WMPlex(ExtlTab tab)
{
	return bindmap_do_table(&ioncore_mplex_bindmap, NULL, tab);
}


static StringIntMap frame_areas[]={
	{"border", 		FRAME_AREA_BORDER},
	{"tab", 		FRAME_AREA_TAB},
	{"empty_tab", 	FRAME_AREA_TAB},
	{"client", 		FRAME_AREA_CLIENT},
	END_STRINGINTMAP
};


EXTL_EXPORT_AS(global, __defbindings_WFrame)
bool ioncore_defbindings_WFrame(ExtlTab tab)
{
	return bindmap_do_table(&ioncore_frame_bindmap, frame_areas, tab);
}


EXTL_EXPORT_AS(global, __defbindings_WMoveresMode)
bool ioncore_defbindings_WMoveresMode(ExtlTab tab)
{
	return bindmap_do_table(&ioncore_moveres_bindmap, NULL, tab);
}


void ioncore_deinit_bindmaps()
{
	bindmap_deinit(&ioncore_rootwin_bindmap);
	bindmap_deinit(&ioncore_mplex_bindmap);
	bindmap_deinit(&ioncore_frame_bindmap);
	bindmap_deinit(&ioncore_moveres_bindmap);
}

