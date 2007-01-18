/*
 * ion/mod_tiling/placement.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_TILING_PLACEMENT_H
#define ION_MOD_TILING_PLACEMENT_H

#include <libmainloop/hooks.h>
#include <ioncore/common.h>
#include <ioncore/clientwin.h>
#include <ioncore/manage.h>
#include "tiling.h"


typedef struct{
    WTiling *ws;
    WRegion *reg;
    const WManageParams *mp;
    
    WFrame *res_frame;
} WTilingPlacementParams;

/* Handlers of this hook should take (WClientWin*, WTiling*, WManageParams*) 
 * as parameter. 
 */
extern WHook *tiling_placement_alt;

extern WPHolder *tiling_prepare_manage(WTiling *ws, const WClientWin *cwin,
                                      const WManageParams *param, int redir);

#endif /* ION_MOD_TILING_PLACEMENT_H */
