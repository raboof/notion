/*
 * ion/mod_panews/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_PANEWS_PLACEMENT_H
#define ION_MOD_PANEWS_PLACEMENT_H

#include <libextl/extl.h>
#include <libmainloop/hooks.h>
#include <ioncore/common.h>
#include <ioncore/clientwin.h>
#include <ioncore/manage.h>
#include <mod_ionws/split.h>
#include "panews.h"
#include "splitext.h"


typedef struct{
    WPaneWS *ws;
    ExtlTab layout;
} WPaneWSInitParams;


typedef struct{
    WPaneWS *ws;
    WFrame *frame;
    WRegion *reg;
    WSplitUnused *specifier;
    
    WSplit *res_node;
    ExtlTab res_config;
    int res_w, res_h;
} WPaneWSPlacementParams;


/* Handlers of this hook should accept WPaneWSInitParams as parameter. */
extern WHook *panews_init_layout_alt;
/* Handlers of this hook should accept WPaneWSPlacementParams as parameter. */
extern WHook *panews_make_placement_alt;


extern bool panews_manage_clientwin(WPaneWS *ws, WClientWin *cwin,
                                    const WManageParams *param,
                                    int redir);
extern bool panews_handle_unused_drop(WPaneWS *ws, WSplitUnused *specifier, 
                                      WRegion *reg);

#endif /* ION_MOD_PANEWS_PLACEMENT_H */
