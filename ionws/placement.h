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
#include "workspace.h"


/* These implement dynfuns introduced in wmcore/wsreg.h */

extern bool workspace_add_clientwin(WWorkspace *ws, WClientWin *cwin,
									const XWindowAttributes *attr,
									int init_state, WWinProp *props);

extern bool workspace_add_transient(WRegion *reg, WClientWin *tfor,
									WClientWin *cwin,
									const XWindowAttributes *attr,
									int init_state, WWinProp *props);

#endif /* ION_PLACEMENT_H */
