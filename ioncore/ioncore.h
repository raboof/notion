/*
 * ion/ioncore/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_WMCORE_H
#define ION_IONCORE_WMCORE_H

#include "common.h"

bool ioncore_init(/*const char *name, const char *etcdir, const char *libdir,*/
				 const char *display, bool onescreen);
void ioncore_deinit();

#endif /* ION_IONCORE_WMCORE_H */
