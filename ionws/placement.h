/*
 * ion/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_PLACEMENT_H
#define ION_PLACEMENT_H

#include <wmcore/common.h>
#include <wmcore/clientwin.h>
#include "winprops.h"
#include "ionws.h"


/* These implement dynfuns introduced in wmcore/wsreg.h */

extern bool ionws_add_clientwin(WIonWS *ws, WClientWin *cwin,
								const XWindowAttributes *attr,
								int init_state, WWinProp *props);

extern bool ionws_add_transient(WIonWS *ws, WClientWin *tfor,
								WClientWin *cwin,
								const XWindowAttributes *attr,
								int init_state, WWinProp *props);

#endif /* ION_PLACEMENT_H */
