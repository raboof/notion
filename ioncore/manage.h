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

#include "common.h"
#include "clientwin.h"
#include "genws.h"
#include "attach.h"


#define INIT_WMANAGEPARAMS \
  {FALSE, FALSE, FALSE, FALSE, ForgetGravity, {0, 0, 0, 0}, NULL}


typedef struct{
	bool switchto;
	bool userpos;
	bool dockapp;
	bool maprq;
	int gravity;
	WRectangle geom;
	WClientWin *tfor;
} WManageParams;



extern bool add_clientwin_default(WClientWin *cwin,
								  const WManageParams *param);

DYNFUN bool region_manage_clientwin(WRegion *reg, WClientWin *cwin,
									const WManageParams *par);

extern bool region_has_manage_clientwin(WRegion *reg);


#endif /* ION_IONCORE_MANAGE_H */
