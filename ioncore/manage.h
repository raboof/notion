/*
 * ion/ioncore/manage.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
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
