/*
 * ion/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef ION_PLACEMENT_H
#define ION_PLACEMENT_H

#include <wmcore/common.h>
#include <wmcore/clientwin.h>
#include "winprops.h"
#include "frame.h"

extern bool add_clientwin(WClientWin *cwin, const XWindowAttributes *attr,
						  int init_state, bool dock);
extern bool finish_add_clientwin(WRegion *reg, WClientWin *cwin,
								 bool init_state, const WWinProp *props);
extern WWinProp *setup_get_winprops(WClientWin *cwin);

#endif /* ION_PLACEMENT_H */
