/*
 * ion/mod_panews/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_PANEWS_PLACEMENT_H
#define ION_MOD_PANEWS_PLACEMENT_H

#include <ioncore/common.h>
#include <ioncore/clientwin.h>
#include <ioncore/manage.h>
#include <ioncore/hooks.h>
#include "panews.h"
#include "splitext.h"


extern WHook *panews_init_layout_alt;
extern WHook *panews_make_placement_alt;


extern bool panews_manage_clientwin(WPaneWS *ws, WClientWin *cwin,
                                    const WManageParams *param,
                                    int redir);
extern bool panews_handle_unused_drop(WPaneWS *ws, WSplitUnused *specifier, 
                                      WRegion *reg);

#endif /* ION_MOD_PANEWS_PLACEMENT_H */
