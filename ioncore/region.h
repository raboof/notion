/*
 * ion/ioncore/region.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
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
#define REGION_GRAB_ON_PARENT 0x0040
#define REGION_ACTIVITY      0x0100
#define REGION_SKIP_FOCUS    0x0200
#define REGION_CWINS_BEING_RESCUED 0x0400
#define REGION_PLEASE_WARP   0x0800
#define REGION_BINDING_UPDATE_SCHEDULED 0x1000

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

#define REGION_MANAGER(R)       (((WRegion*)(R))->manager)
#define REGION_MANAGER_CHK(R, TYPE) OBJ_CAST(REGION_MANAGER(R), TYPE)

#define REGION_PARENT(REG)      (((WRegion*)(REG))->parent)
#define REGION_PARENT_REG(REG)  ((WRegion*)REGION_PARENT(REG))

#define REGION_FIT_BOUNDS    0x0001 /* Geometry is maximum bounds */
#define REGION_FIT_ROTATE    0x0002 /* for Xrandr */
#define REGION_FIT_WHATEVER  0x0004 /* for attach routines; g is not final */
#define REGION_FIT_GRAVITY   0x0008 /* just a hint; for use with BOUNDS */
#define REGION_FIT_EXACT     0x0000 /* No flags; exact fit */


typedef int WRegionFitMode;


INTRSTRUCT(WFitParams);
DECLSTRUCT(WFitParams){
    WRectangle g;
    WRegionFitMode mode;
    int gravity;
    int rotation;
};


INTRSTRUCT(WRegionNameInfo);
DECLSTRUCT(WRegionNameInfo){
    char *name;
    int inst_off;
    void *node;
};


INTRSTRUCT(WPrepareFocusResult);
DECLSTRUCT(WPrepareFocusResult){
    WRegion *reg;
    int flags;
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
    WSubmapState *submapstat;
    
    WRegion *active_sub;
    WRegion *active_prev, *active_next;
    
    WRegionNameInfo ni;
    
    WRegion *manager;
    
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
DYNFUN bool region_rqclose(WRegion *reg, bool relocate);
DYNFUN WRegion *region_rqclose_propagate(WRegion *reg, WRegion *maybe_sub);
DYNFUN WRegion *region_current(WRegion *mgr);
DYNFUN void region_notify_rootpos(WRegion *reg, int x, int y);
DYNFUN bool region_may_destroy(WRegion *reg);
DYNFUN WRegion *region_managed_control_focus(WRegion *mgr, WRegion *reg);
DYNFUN void region_managed_remove(WRegion *reg, WRegion *sub);
DYNFUN bool region_managed_prepare_focus(WRegion *reg, WRegion *sub, 
                                         int flags, WPrepareFocusResult *res);
DYNFUN void region_managed_activated(WRegion *reg, WRegion *sub);
DYNFUN void region_managed_inactivated(WRegion *reg, WRegion *sub);
DYNFUN void region_managed_notify(WRegion *reg, WRegion *sub);
DYNFUN bool region_managed_may_destroy(WRegion *mgr, WRegion *reg);

DYNFUN void region_child_removed(WRegion *reg, WRegion *sub);

DYNFUN void region_manager_changed(WRegion *reg, WRegion *mgr_or_null);

DYNFUN void region_restack(WRegion *reg, Window other, int mode);
DYNFUN void region_stacking(WRegion *reg, Window *bottomret, Window *topret);

DYNFUN bool region_handle_drop(WRegion *reg, int x, int y, WRegion *dropped);

extern bool region_prepare_focus(WRegion *reg, int flags,
                                 WPrepareFocusResult *res);

extern void region_raise(WRegion *reg);
extern void region_lower(WRegion *reg);

extern void region_fit(WRegion *reg, const WRectangle *geom, 
                       WRegionFitMode mode);
extern bool region_reparent(WRegion *reg, WWindow *target, 
                            const WRectangle *geom, WRegionFitMode mode);

extern void region_updategr_default(WRegion *reg);

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
extern void region_set_manager(WRegion *reg, WRegion *mgr);
extern void region_unset_manager(WRegion *reg, WRegion *mgr);
extern void region_unset_parent(WRegion *reg);

extern WRootWin *region_rootwin_of(const WRegion *reg);
extern Window region_root_of(const WRegion *reg);
extern WScreen *region_screen_of(WRegion *reg);
extern bool region_same_rootwin(const WRegion *reg1, const WRegion *reg2);

extern bool region_manager_allows_destroying(WRegion *reg);

extern WRegion *region_managed_within(WRegion *reg, WRegion *mgd);

#endif /* ION_IONCORE_REGION_H */
