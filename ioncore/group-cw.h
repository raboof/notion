/*
 * ion/ioncore/group-cw.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
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
