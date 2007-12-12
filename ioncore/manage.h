/*
 * ion/ioncore/manage.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
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
    MANAGE_PRIORITY_NONE,
    MANAGE_PRIORITY_LOW,
    MANAGE_PRIORITY_NORMAL,
    MANAGE_PRIORITY_GROUP,
    MANAGE_PRIORITY_NO,
    /* Special */
    MANAGE_PRIORITY_NOREDIR
};

#define MANAGE_PRIORITY_OK(PRIORITY, OUR)                        \
    ((PRIORITY) <= (OUR) || (PRIORITY)==MANAGE_PRIORITY_NOREDIR)

#define MANAGE_PRIORITY_SUB(PRIORITY, OUR)      \
    ((PRIORITY)==MANAGE_PRIORITY_NOREDIR        \
     ? MANAGE_PRIORITY_NO                       \
     : (PRIORITY) < (OUR) ? (OUR) : (PRIORITY))

#define MANAGE_PRIORITY_SUBX(PRIORITY, OUR)                    \
    ((PRIORITY)==MANAGE_PRIORITY_NOREDIR || (OUR) < (PRIORITY) \
     ? MANAGE_PRIORITY_NO                                      \
     : MANAGE_PRIORITY_NONE)


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

#define REGION_RESCUE_PHFLAGS_OK 0x01
#define REGION_RESCUE_NODEEP     0x02

INTRSTRUCT(WRescueInfo);

extern WPHolder *rescueinfo_pholder(WRescueInfo *info, bool take);

/* if ph is given, it is used, otherwise one is looked for when needed */
extern bool region_rescue(WRegion *reg, WPHolder *ph, int flags);
extern bool region_rescue_needed(WRegion *reg);
extern bool region_rescue_clientwins(WRegion *reg, WRescueInfo *info);
extern bool region_rescue_child_clientwins(WRegion *reg, WRescueInfo *info);
extern bool region_rescue_some_clientwins(WRegion *reg, WRescueInfo *info,
                                          WRegionIterator *iter, void *st);
extern bool region_do_rescue_this(WRegion *tosave, WRescueInfo *info, int ph_flags);


#endif /* ION_IONCORE_MANAGE_H */
