/*
 * wmcore/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_WMCORE_H
#define WMCORE_WMCORE_H

#include "common.h"

bool wmcore_init(const char *name, const char *etcdir, const char *libdir,
				 const char *display, bool onescreen);
void wmcore_deinit();

#endif /* WMCORE_WMCORE_H */
