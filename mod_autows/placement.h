/*
 * ion/mod_autows/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_AUTOWS_PLACEMENT_H
#define ION_MOD_AUTOWS_PLACEMENT_H

#include <ioncore/common.h>
#include <ioncore/clientwin.h>
#include <ioncore/manage.h>
#include <ioncore/hooks.h>
#include "autows.h"
#include "splitext.h"


extern WHook *autows_init_layout_alt;
extern WHook *autows_make_placement_alt;


extern bool autows_manage_clientwin(WAutoWS *ws, WClientWin *cwin,
                                    const WManageParams *param,
                                    int redir);
extern bool autows_handle_unused_drop(WAutoWS *ws, WSplitUnused *specifier, 
                                      WRegion *reg);

#endif /* ION_MOD_AUTOWS_PLACEMENT_H */
