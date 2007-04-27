/*
 * ion/ioncore/groupws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GROUPWS_H
#define ION_IONCORE_GROUPWS_H

#include <ioncore/common.h>
#include <ioncore/rectangle.h>
#include <ioncore/group.h>
#include "classes.h"


DECLCLASS(WGroupWS){
    WGroup grp;
};


extern WPHolder *groupws_prepare_manage(WGroupWS *ws, 
                                        const WClientWin *cwin,
                                        const WManageParams *param, 
                                        int redir);

extern WPHolder *groupws_prepare_manage_transient(WGroupWS *ws, 
                                                  const WClientWin *cwin,
                                                  const WManageParams *param,
                                                  int unused);

extern bool groupws_handle_drop(WGroupWS *ws, int x, int y,
                                WRegion *dropped);

extern WGroupWS *create_groupws(WWindow *parent, const WFitParams *fp);
extern bool groupws_init(WGroupWS *ws, WWindow *parent, const WFitParams *fp);
extern void groupws_deinit(WGroupWS *ws);

extern WRegion *groupws_load(WWindow *par, const WFitParams *fp, ExtlTab tab);

extern void ioncore_groupws_set(ExtlTab tab);
extern void ioncore_groupws_get(ExtlTab t);

#endif /* ION_IONCORE_GROUPWS_H */
