/*
 * ion/ioncore/manage.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_MANAGE_H
#define ION_IONCORE_MANAGE_H

#include "common.h"

INTRSTRUCT(WManageParams);

#include "clientwin.h"
#include "genws.h"
#include "attach.h"
#include "rectangle.h"
#include "extl.h"


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


extern ExtlTab manageparams_to_table(WManageParams *mp);


extern WScreen *clientwin_find_suitable_screen(WClientWin *cwin,
                                               const WManageParams *param);

/* Manage */

extern bool clientwin_do_manage_default(WClientWin *cwin,
                                        const WManageParams *param);

DYNFUN bool region_manage_clientwin(WRegion *reg, WClientWin *cwin,
                                    const WManageParams *par, int redir);

extern bool region_manage_clientwin_default(WRegion *reg, WClientWin *cwin,
                                            const WManageParams *par, 
                                            int redir);

/* Rescue */

extern bool region_rescue_clientwins(WRegion *reg);
extern bool region_rescue_child_clientwins(WRegion *reg);
extern bool region_rescue_managed_clientwins(WRegion *reg, WRegion *list);

DYNFUN bool region_manage_rescue(WRegion *reg, WClientWin *cwin,
                                 WRegion *from);

extern bool region_manage_rescue_default(WRegion *reg, WClientWin *cwin,
                                         WRegion *from);



#endif /* ION_IONCORE_MANAGE_H */
