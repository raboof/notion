/*
 * ion/ioncore/region.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_REGION_H
#define ION_IONCORE_REGION_H

#include <libtu/obj.h>
#include "common.h"
#include "rectangle.h"

#define REGION_MAPPED        0x0001
#define REGION_ACTIVE        0x0002
#define REGION_HAS_GRABS     0x0004
#define REGION_TAGGED        0x0008
#define REGION_BINDINGS_ARE_GRABBED 0x0020
#define REGION_ACTIVITY      0x0100
#define REGION_SKIP_FOCUS    0x0200
#define REGION_CWINS_BEING_RESCUED 0x0400
#define REGION_PLEASE_WARP   0x0800

#define REGION_GOTO_FOCUS    0x0001
#define REGION_GOTO_NOWARP   0x0002
#define REGION_GOTO_ENTERWINDOW 0x0004

/* Use region_is_fully_mapped instead for most cases. */
#define REGION_IS_MAPPED(R)     (((WRegion*)(R))->flags&REGION_MAPPED)
#define REGION_MARK_MAPPED(R)   (((WRegion*)(R))->flags|=REGION_MAPPED)
#define REGION_MARK_UNMAPPED(R) (((WRegion*)(R))->flags&=~REGION_MAPPED)
#define REGION_IS_ACTIVE(R)     (((WRegion*)(R))->flags&REGION_ACTIVE)
#define REGION_IS_TAGGED(R)     (((WRegion*)(R))->flags&REGION_TAGGED)
#define REGION_IS_URGENT(R)     (((WRegion*)(R))->flags&REGION_URGENT)
#define REGION_GEOM(R)          (((WRegion*)(R))->geom)
#define REGION_ACTIVE_SUB(R)    (((WRegion*)(R))->active_sub)

typedef enum{
    REGION_FIT_EXACT,
    REGION_FIT_BOUNDS
} WRegionFitMode;

INTRSTRUCT(WFitParams);
DECLSTRUCT(WFitParams){
    WRectangle g;
    WRegionFitMode mode;
};

INTRSTRUCT(WSubmapState);
DECLSTRUCT(WSubmapState){
    uint key, state;
    WSubmapState *next;
};

INTRSTRUCT(WRegionNameInfo);
DECLSTRUCT(WRegionNameInfo){
    char *name;
    int inst_off;
    void *namespaceinfo;
};

DECLCLASS(WRegion){
    Obj obj;
    
    WRectangle geom;
    void *rootwin;
    bool flags;

    WWindow *parent;
    WRegion *children;
    WRegion *p_next, *p_prev;
    
    void *bindings;
    WSubmapState submapstat;
    
    WRegion *active_sub;
    
    WRegionNameInfo ni;
    
    WRegion *manager;
    WRegion *mgr_next, *mgr_prev;
    
    int mgd_activity;
};

extern void region_init(WRegion *reg, WWindow *par, const WFitParams *fp);
extern void region_deinit(WRegion *reg);

DYNFUN bool region_fitrep(WRegion *reg, WWindow *par, const WFitParams *fp);
DYNFUN void region_map(WRegion *reg);
DYNFUN void region_unmap(WRegion *reg);
DYNFUN Window region_xwindow(const WRegion *reg);
DYNFUN void region_activated(WRegion *reg);
DYNFUN void region_inactivated(WRegion *reg);
DYNFUN void region_updategr(WRegion *reg);
DYNFUN bool region_rqclose(WRegion *reg);
DYNFUN WRegion *region_rqclose_propagate(WRegion *reg, WRegion *maybe_sub);
DYNFUN WRegion *region_current(WRegion *mgr);
DYNFUN void region_notify_rootpos(WRegion *reg, int x, int y);

DYNFUN WRegion *region_managed_control_focus(WRegion *mgr, WRegion *reg);
DYNFUN void region_managed_remove(WRegion *reg, WRegion *sub);
DYNFUN bool region_managed_goto(WRegion *reg, WRegion *sub, int flags);
DYNFUN void region_managed_activated(WRegion *reg, WRegion *sub);
DYNFUN void region_managed_inactivated(WRegion *reg, WRegion *sub);
DYNFUN void region_managed_notify(WRegion *reg, WRegion *sub);
DYNFUN bool region_managed_may_destroy(WRegion *mgr, WRegion *reg);

DYNFUN void region_child_removed(WRegion *reg, WRegion *sub);

DYNFUN void region_manager_changed(WRegion *reg, WRegion *mgr_or_null);

DYNFUN void region_restack(WRegion *reg, Window other, int mode);
DYNFUN void region_stacking(WRegion *reg, Window *bottomret, Window *topret);

extern void region_raise(WRegion *reg);
extern void region_lower(WRegion *reg);

extern void region_fit(WRegion *reg, const WRectangle *geom, 
                       WRegionFitMode mode);
extern bool region_reparent(WRegion *reg, WWindow *target, 
                            const WRectangle *geom, WRegionFitMode mode);

extern void region_updategr_default(WRegion *reg);

extern bool region_may_destroy(WRegion *reg);

extern void region_rootpos(WRegion *reg, int *xret, int *yret);
extern void region_notify_change(WRegion *reg);

extern bool region_goto(WRegion *reg);
extern bool region_goto_flags(WRegion *reg, int flags);

extern bool region_is_fully_mapped(WRegion *reg);

extern void region_detach_manager(WRegion *reg);

extern WWindow *region_parent(WRegion *reg);
extern WRegion *region_manager(WRegion *reg);
extern WRegion *region_manager_or_parent(WRegion *reg);
extern void region_set_parent(WRegion *reg, WWindow *par);
extern void region_set_manager(WRegion *reg, WRegion *mgr, WRegion **listptr);
extern void region_unset_manager(WRegion *reg, WRegion *mgr, WRegion **listptr);
extern void region_unset_parent(WRegion *reg);

extern WRootWin *region_rootwin_of(const WRegion *reg);
extern Window region_root_of(const WRegion *reg);
extern WScreen *region_screen_of(WRegion *reg);
extern bool region_same_rootwin(const WRegion *reg1, const WRegion *reg2);

#endif /* ION_IONCORE_REGION_H */
