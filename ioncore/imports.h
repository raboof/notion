/*
 * wmcore/imports.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_IMPORTS_H
#define WMCORE_IMPORTS_H

#include "screen.h"
#include "viewport.h"

extern void calc_grdata(WScreen *scr);
extern void read_workspaces(WViewport *vp);
extern void write_workspaces(WViewport *vp);

#endif /* WMCORE_IMPORTS_H */
