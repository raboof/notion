/*
 * ion/ionws/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONWS_PLACEMENT_H
#define ION_IONWS_PLACEMENT_H

#include <ioncore/common.h>
#include <ioncore/clientwin.h>
#include <ioncore/manage.h>
#include "ionws.h"


extern bool ionws_manage_clientwin(WIonWS *ws, WClientWin *cwin,
								   const WManageParams *param);

#endif /* ION_IONWS_PLACEMENT_H */
