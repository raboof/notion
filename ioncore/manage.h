/*
 * ion/ioncore/manage.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_MANAGE_H
#define ION_IONCORE_MANAGE_H

#include <libextl/extl.h>
#include "common.h"

INTRSTRUCT(WManageParams);

#include "clientwin.h"
#include "attach.h"
#include "rectangle.h"
#include "extlconv.h"
#include "pholder.h"


#define MANAGEPARAMS_INIT                                                \
  {FALSE, FALSE, FALSE, FALSE, FALSE, ForgetGravity, {0, 0, 0, 0}, NULL}

enum{
    MANAGE_REDIR_PREFER_YES,
    MANAGE_REDIR_PREFER_NO,
    MANAGE_REDIR_STRICT_YES,
    MANAGE_REDIR_STRICT_NO
};

DECLSTRUCT(WManageParams){
    bool switchto;
    bool jumpto;
    bool userpos;
    bool dockapp;
    bool maprq;
    int gravity;
    WRectangle geom;
    WClientWin *tfor;
};


typedef WRegion *WRegionIterator(void *st);


extern ExtlTab manageparams_to_table(const WManageParams *mp);


extern WScreen *clientwin_find_suitable_screen(WClientWin *cwin,
                                               const WManageParams *param);

/* Manage */

extern bool clientwin_do_manage_default(WClientWin *cwin,
                                        const WManageParams *param);

extern bool region_manage_clientwin(WRegion *reg, WClientWin *cwin,
                                    const WManageParams *par, int redir);

DYNFUN WPHolder *region_prepare_manage(WRegion *reg, const WClientWin *cwin,
                                       const WManageParams *par, int redir);

extern WPHolder *region_prepare_manage_default(WRegion *reg, 
                                               const WClientWin *cwin,
                                               const WManageParams *par, 
                                               int redir);


extern WPHolder *region_prepare_manage_transient(WRegion *reg, 
                                                 const WClientWin *cwin,
                                                 const WManageParams *param,
                                                 int unused);

extern WPHolder *region_prepare_manage_transient_default(WRegion *reg, 
                                                         const WClientWin *cwin,
                                                         const WManageParams *param,
                                                         int unused);

/* Rescue */

extern bool region_rescue_clientwins(WRegion *reg, WPHolder *ph);
extern bool region_rescue_child_clientwins(WRegion *reg, WPHolder *ph);
extern bool region_rescue_some_clientwins(WRegion *reg, WPHolder *ph,
                                          WRegionIterator *iter, void *st);


#endif /* ION_IONCORE_MANAGE_H */
