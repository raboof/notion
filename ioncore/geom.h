/*
 * ion/ioncore/geom.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GEOM_H
#define ION_IONCORE_GEOM_H

#include "common.h"
#include "extl.h"

extern bool extltab_to_geom(ExtlTab tab, WRectangle *geomret);
extern ExtlTab geom_to_extltab(WRectangle geom);

/* Debugging */
extern void pgeom(const char *n, WRectangle g);

#endif /* ION_IONCORE_GEOM_H */

