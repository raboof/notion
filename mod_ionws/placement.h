/*
 * ion/mod_ionws/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_IONWS_PLACEMENT_H
#define ION_MOD_IONWS_PLACEMENT_H

#include <ioncore/common.h>
#include <ioncore/clientwin.h>
#include <ioncore/manage.h>
#include <ioncore/hooks.h>
#include "ionws.h"

/* Handlers of this hook should take (WClientWin*, WIonWS*, WManageParams*) 
 * as parameter. 
 */
extern WHook *ionws_placement_alt;

extern bool ionws_manage_clientwin(WIonWS *ws, WClientWin *cwin,
                                   const WManageParams *param,
                                   int redir);

#endif /* ION_MOD_IONWS_PLACEMENT_H */
