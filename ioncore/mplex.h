/*
 * ion/ioncore/mplex.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_MPLEX_H
#define ION_IONCORE_MPLEX_H

#include "common.h"
#include "window.h"
#include "attach.h"
#include "manage.h"
#include "extl.h"
#include "rectangle.h"

#define MPLEX_ADD_TO_END 0x0001
#define MPLEX_MANAGED_UNVIEWABLE 0x0002

#define MPLEX_ATTACH_SWITCHTO 0x01
#define MPLEX_ATTACH_L2 0x02

enum{
    MPLEX_CHANGE_SWITCHONLY=0,
    MPLEX_CHANGE_REORDER=1,
    MPLEX_CHANGE_ADD=2,
    MPLEX_CHANGE_REMOVE=3
};


DECLCLASS(WMPlex){
    WWindow win;
    int flags;
    
    int l1_count;
    WRegion *l1_list;
    WRegion *l1_current;
    
    int l2_count;
    WRegion *l2_list;
    WRegion *l2_current;
};


/* Create/destroy */
extern bool mplex_init(WMPlex *mplex, WWindow *parent, Window win,
                       const WFitParams *fp);
extern bool mplex_init_new(WMPlex *mplex, WWindow *parent, 
                           const WFitParams *fp);
extern void mplex_deinit(WMPlex *mplex);

/* Resize and reparent */
extern bool mplex_fitrep(WMPlex *mplex, WWindow *par, const WFitParams *fp);
extern void mplex_fit_managed(WMPlex *mplex);

/* Mapping */
extern void mplex_map(WMPlex *mplex);
extern void mplex_unmap(WMPlex *mplex);

/* Attach */
extern WRegion *mplex_attach_simple(WMPlex *mplex, WRegion *reg, 
                                    int flags);
extern WRegion *mplex_attach_hnd(WMPlex *mplex, WRegionAttachHandler *hnd,
                                 void *hnd_param, int flags);

extern WRegion *mplex_attach(WMPlex *mplex, WRegion *reg, ExtlTab param);
extern WRegion *mplex_attach_new(WMPlex *mplex, ExtlTab param);

extern void mplex_attach_tagged(WMPlex *mplex);

extern void mplex_managed_remove(WMPlex *mplex, WRegion *reg);

extern bool mplex_rescue_clientwins(WMPlex *mplex);

extern bool mplex_manage_clientwin(WMPlex *mplex, WClientWin *cwin,
                                   const WManageParams *param, int redir);
extern bool mplex_manage_rescue(WMPlex *mplex, WClientWin *cwin,
                                WRegion *from);

/* Switch */
extern bool mplex_managed_display(WMPlex *mplex, WRegion *sub);
extern void mplex_switch_nth(WMPlex *mplex, uint n);
extern void mplex_switch_next(WMPlex *mplex);
extern void mplex_switch_prev(WMPlex *mplex);
extern bool mplex_l2_hide(WMPlex *mplex, WRegion *reg);
extern bool mplex_l2_show(WMPlex *mplex, WRegion *reg);

/* Focus */
extern void mplex_do_set_focus(WMPlex *mplex, bool warp);

/* Misc */
extern WRegion *mplex_current(WMPlex *mplex);

extern int mplex_l1_count(WMPlex *mplex);
extern int mplex_l2_count(WMPlex *mplex);
extern WRegion *mplex_l1_nth(WMPlex *mplex, uint n);
extern WRegion *mplex_l2_nth(WMPlex *mplex, uint n);
extern ExtlTab mplex_l1_list(WMPlex *mplex);
extern ExtlTab mplex_l2_list(WMPlex *mplex);
extern WRegion *mplex_l1_current(WMPlex *mplex);
extern WRegion *mplex_l2_current(WMPlex *mplex);

/* Dynfuns */
DYNFUN void mplex_managed_geom(const WMPlex *mplex, WRectangle *geom);
DYNFUN void mplex_size_changed(WMPlex *mplex, bool wchg, bool hchg);
DYNFUN void mplex_managed_changed(WMPlex *mplex, int what, bool sw,
                                  WRegion *mgd);

/* Save/load */

extern ExtlTab mplex_get_base_configuration(WMPlex *mplex);
extern void mplex_load_contents(WMPlex *frame, ExtlTab tab);

#endif /* ION_IONCORE_MPLEX_H */
