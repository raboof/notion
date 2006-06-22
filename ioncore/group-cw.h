/*
 * ion/ioncore/group-cw.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GROUPCW_H
#define ION_IONCORE_GROUPCW_H

#include "common.h"
#include "group.h"
#include "clientwin.h"


DECLCLASS(WGroupCW){
    WGroup grp;
    /*WPHolder *fs_pholder;*/
    WSizePolicy transient_szplcy; /* default transient size policy */
};

extern bool groupcw_init(WGroupCW *cwg, WWindow *parent, const WFitParams *fp);
extern WGroupCW *create_groupcw(WWindow *parent, const WFitParams *fp);
extern void groupcw_deinit(WGroupCW *cwg);

extern bool groupcw_attach_transient(WGroupCW *cwg, WRegion *transient);

extern WPHolder *groupcw_prepare_manage(WGroupCW *cwg, 
                                        const WClientWin *cwin2,
                                        const WManageParams *param, 
                                        int redir);

extern WPHolder *groupcw_prepare_manage_transient(WGroupCW *cwg, 
                                                  const WClientWin *transient,
                                                  const WManageParams *param,
                                                  int unused);

extern WRegion *groupcw_load(WWindow *par, const WFitParams *fp, ExtlTab tab);

#endif /* ION_IONCORE_GROUPCW_H */
