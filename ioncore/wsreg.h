/*
 * wmcore/wsreg.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_WSREG_H
#define WMCORE_WSREG_H

#include "clientwin.h"
#include "winprops.h"
#include "obj.h"


extern bool add_clientwin_default(WClientWin *cwin,
								  const XWindowAttributes *attr,
								  int init_state, bool dock);


DYNFUN bool region_ws_add_clientwin(WRegion *reg, WClientWin *cwin,
									const XWindowAttributes *attr,
									int init_state, WWinProp *props);


DYNFUN bool region_ws_add_transient(WRegion *reg, WClientWin *tfor,
									WClientWin *cwin,
									const XWindowAttributes *attr,
									int init_state, WWinProp *props);

extern bool finish_add_clientwin(WRegion *reg, WClientWin *cwin,
								 bool init_state, const WWinProp *props);

#endif /* WMCORE_WSREG_H */
