/*
 * ion/ionws/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONWS_PLACEMENT_H
#define ION_IONWS_PLACEMENT_H

#include <ioncore/common.h>
#include <ioncore/clientwin.h>
#include <ioncore/attach.h>
#include "ionws.h"


extern bool ionws_add_clientwin(WIonWS *ws, WClientWin *cwin,
								const WAttachParams *param);

#endif /* ION_IONWS_PLACEMENT_H */
