/*
 * ion/ioncore/mplex.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 * 
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_MPLEX_H
#define ION_IONCORE_MPLEX_H

#include <libextl/extl.h>
#include <libmainloop/hooks.h>
#include "common.h"
#include "window.h"
#include "attach.h"
#include "manage.h"
#include "rectangle.h"
#include "pholder.h"

#define MPLEX_ADD_TO_END 0x0001
#define MPLEX_MANAGED_UNVIEWABLE 0x0002

#define MPLEX_MGD_UNVIEWABLE(MPLEX) \
    ((MPLEX)->flags&MPLEX_MANAGED_UNVIEWABLE)

#define MPLEX_ATTACH_SWITCHTO 0x01
#define MPLEX_ATTACH_L2 0x02
#define MPLEX_ATTACH_L2_PASSIVE 0x04
#define MPLEX_ATTACH_L2_GEOM 0x08

enum{
    MPLEX_CHANGE_SWITCHONLY=0,
    MPLEX_CHANGE_REORDER=1,
    MPLEX_CHANGE_ADD=2,
    MPLEX_CHANGE_REMOVE=3
};


enum{
    MPLEX_STDISP_TL,
    MPLEX_STDISP_TR,
    MPLEX_STDISP_BL,
    MPLEX_STDISP_BR
};


INTRSTRUCT(WMPlexSTDispInfo);
INTRSTRUCT(WMPlexChangedParams);
INTRSTRUCT(WMPlexAttachParams);
INTRSTRUCT(WLListNode); /* in mplexp.h */


DECLSTRUCT(WMPlexSTDispInfo){
    Watch regwatch;
    int pos;
};


DECLSTRUCT(WMPlexChangedParams){
    WMPlex *reg;
    int mode;
    bool sw;
    WRegion *sub;
};


DECLSTRUCT(WMPlexAttachParams){
    int flags;
    int index;
    WRectangle l2geom;
};


DECLCLASS(WMPlex){
    WWindow win;
    int flags;
    
    int l1_count;
    WLListNode *l1_list;
    WLListNode *l1_current;
    WMPlexPHolder *l1_phs;
    
    int l2_count;
    WLListNode *l2_list;
    WLListNode *l2_current;
    WMPlexPHolder *l2_phs;
    
    WMPlexSTDispInfo stdispinfo;
};


/* Create/destroy */

extern WMPlex *create_mplex(WWindow *parent, const WFitParams *fp);
extern bool mplex_init(WMPlex *mplex, WWindow *parent,
                       const WFitParams *fp);
extern bool mplex_do_init(WMPlex *mplex, WWindow *parent, Window win,
                          const WFitParams *fp, bool create);
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

extern WRegion *mplex_do_attach(WMPlex *mplex, WRegionAttachHandler *hnd,
                                void *hnd_param, WMPlexAttachParams *param);
extern WLListNode *mplex_do_attach_after(WMPlex *mplex, 
                                            WLListNode *after, 
                                            WMPlexAttachParams *param,
                                            WRegionAttachHandler *hnd,
                                            void *hnd_param);

extern void mplex_attach_tagged(WMPlex *mplex);

extern void mplex_managed_remove(WMPlex *mplex, WRegion *reg);
extern void mplex_child_removed(WMPlex *mplex, WRegion *sub);

extern bool mplex_rescue_clientwins(WMPlex *mplex, WPHolder *ph);

extern bool mplex_manage_clientwin(WMPlex *mplex, WClientWin *cwin,
                                   const WManageParams *param, int redir);

/* Switch */

extern bool mplex_managed_goto(WMPlex *mplex, WRegion *sub, bool cfocus);
extern void mplex_switch_nth(WMPlex *mplex, uint n);
extern void mplex_switch_next(WMPlex *mplex);
extern void mplex_switch_prev(WMPlex *mplex);
extern bool mplex_l2_hidden(WMPlex *mplex, WRegion *reg);
extern bool mplex_l2_set_hidden(WMPlex *mplex, WRegion *reg, bool sp);

/* Focus */

extern void mplex_do_set_focus(WMPlex *mplex, bool warp);
extern void mplex_managed_activated(WMPlex *mplex, WRegion *reg);

/* Misc */

extern WRegion *mplex_current(WMPlex *mplex);
extern bool mplex_may_destroy(WMPlex *mplex);

extern int mplex_layer(WMPlex *mplex, WRegion *reg);
extern int mplex_lcount(WMPlex *mplex, uint l);
extern WRegion *mplex_lnth(WMPlex *mplex, uint l, uint n);
extern ExtlTab mplex_llist(WMPlex *mplex, uint l);
extern WRegion *mplex_lcurrent(WMPlex *mplex, uint l);

extern void mplex_call_changed_hook(WMPlex *mplex, WHook *hook, 
                                    int mode, bool sw, WRegion *reg);

extern WLListNode *mplex_find_node(WMPlex *mplex, WRegion *reg);

/* Dynfuns */

DYNFUN void mplex_managed_geom(const WMPlex *mplex, WRectangle *geom);
DYNFUN void mplex_size_changed(WMPlex *mplex, bool wchg, bool hchg);
DYNFUN void mplex_managed_changed(WMPlex *mplex, int what, bool sw,
                                  WRegion *mgd);

/* Save/load */

extern ExtlTab mplex_get_configuration(WMPlex *mplex);
extern WRegion *mplex_load(WWindow *par, const WFitParams *fp, ExtlTab tab);
extern void mplex_load_contents(WMPlex *frame, ExtlTab tab);


/* Sticky status display support */

extern bool mplex_set_stdisp(WMPlex *mplex, WRegion *stdisp, int pos);
extern void mplex_get_stdisp(WMPlex *mplex, WRegion **stdisp, int *pos);

extern WRegion *mplex_set_stdisp_extl(WMPlex *mplex, ExtlTab t);
extern ExtlTab mplex_get_stdisp_extl(WMPlex *mplex);


#endif /* ION_IONCORE_MPLEX_H */
