/*
 * ion/ioncore/wsreg.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_WSREG_H
#define ION_IONCORE_WSREG_H

#include "clientwin.h"
#include "obj.h"


extern bool add_clientwin_default(WClientWin *cwin,
								  const XWindowAttributes *attr,
								  int init_state, bool dock);


DYNFUN bool region_ws_add_clientwin(WRegion *reg, WClientWin *cwin,
									const XWindowAttributes *attr,
									int init_state);


DYNFUN bool region_ws_add_transient(WRegion *reg, WClientWin *tfor,
									WClientWin *cwin,
									const XWindowAttributes *attr,
									int init_state);

extern bool finish_add_clientwin(WRegion *reg, WClientWin *cwin,
								 bool init_state);

#endif /* ION_IONCORE_WSREG_H */
