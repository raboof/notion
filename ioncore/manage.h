/*
 * ion/ioncore/manage.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_MANAGE_H
#define ION_IONCORE_MANAGE_H

#include "clientwin.h"
#include "genws.h"
#include "attach.h"


extern bool finish_add_clientwin(WRegion *reg, WClientWin *cwin,
								 const WAttachParams *par);

extern bool do_add_clientwin(WRegion *reg, WClientWin *cwin,
							 const WAttachParams *param);

extern bool add_clientwin_default(WClientWin *cwin,
								  const WAttachParams *param);

DYNFUN bool genws_add_clientwin(WGenWS *genws, WClientWin *cwin,
								const WAttachParams *par);

#endif /* ION_IONCORE_MANAGE_H */
