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
#include <ioncore/binding.h>
#include <ioncore/conf-bindings.h>
#include <ioncore/framep.h>
#include <ioncore/extl.h>
#include "bindmaps.h"


static StringIntMap frame_areas[]={
    {"border",         FRAME_AREA_BORDER},
    {"tab",         FRAME_AREA_TAB},
    {"empty_tab",     FRAME_AREA_TAB},
    {"client",         FRAME_AREA_CLIENT},
    END_STRINGINTMAP
};


EXTL_EXPORT_AS(global, __defbindings_WIonWS)
bool mod_ionws_defbindings_WIonWS(ExtlTab tab)
{
    return bindmap_do_table(&ionws_bindmap, NULL, tab);
}


EXTL_EXPORT_AS(global, __defbindings_WIonFrame)
bool mod_ionws_defbindings_WIonFrame(ExtlTab tab)
{
    return bindmap_do_table(&ionframe_bindmap, frame_areas, tab);
}


