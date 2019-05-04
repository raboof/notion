/*
 * ion/ioncore/group.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GROUP_H
#define ION_IONCORE_GROUP_H

#include <libextl/extl.h>
#include <ioncore/common.h>
#include <ioncore/region.h>
#include <ioncore/screen.h>
#include <ioncore/manage.h>
#include <ioncore/rectangle.h>
#include <ioncore/pholder.h>
#include <ioncore/stacking.h>
#include <ioncore/framedpholder.h>


INTRSTRUCT(WGroupAttachParams);

typedef WRegionSimpleCreateFn WGroupMkFrameFn;


DECLSTRUCT(WGroupAttachParams){
    uint level_set:1;
    uint szplcy_set:1;
    uint geom_set:1;
    uint geom_weak_set:1;
    uint switchto_set:1;
    
    uint switchto:1;
    uint bottom:1;
    
    int geom_weak;
    WRectangle geom;
    
    uint level;
    WSizePolicy szplcy;
    WRegion *stack_above;
};

#define GROUPATTACHPARAMS_INIT \
    {0, 0, 0, 0, 0,  0, 0,   0, {0, 0, 0, 0},  0, 0, NULL}


DECLCLASS(WGroup){
    WRegion reg;
    WStacking *managed_list;
    WStacking *managed_stdisp;
    WStacking *current_managed;
    WStacking *bottom;
    Window dummywin;
};


extern bool group_init(WGroup *grp, WWindow *parent, const WFitParams *fp, const char *name);
extern WGroup *create_group(WWindow *parent, const WFitParams *fp, const char *name);
extern void group_deinit(WGroup *grp);

extern WRegion *group_load(WWindow *par, const WFitParams *fp, 
                           ExtlTab tab);
extern void group_do_load(WGroup *ws, ExtlTab tab);

extern WRegion* group_current(WGroup *group);

DYNFUN WStacking *group_do_add_managed(WGroup *ws, WRegion *reg, 
                                       int level, WSizePolicy szplcy);
extern WStacking *group_do_add_managed_default(WGroup *ws, WRegion *reg, 
                                               int level, WSizePolicy szplcy);

extern WRegion *group_do_attach(WGroup *ws, 
                                WGroupAttachParams *param,
                                WRegionAttachData *data);
extern bool group_do_attach_final(WGroup *ws, 
                                  WRegion *reg,
                                  const WGroupAttachParams *param);

extern WRegion *group_attach(WGroup *ws, WRegion *reg, ExtlTab param);
extern WRegion *group_attach_new(WGroup *ws, ExtlTab param);

extern void group_manage_stdisp(WGroup *ws, WRegion *stdisp, 
                                const WMPlexSTDispInfo *di);

extern void group_managed_remove(WGroup *ws, WRegion *reg);

extern void group_managed_notify(WGroup *ws, WRegion *reg, WRegionNotify how);

extern WRegion *group_bottom(WGroup *ws);
extern bool group_set_bottom(WGroup *ws, WRegion *reg);

extern bool group_rescue_clientwins(WGroup *ws, WRescueInfo *info);

extern bool group_rqclose(WGroup *ws);
extern bool group_rqclose_relocate(WGroup *ws);

extern bool group_managed_rqorder(WGroup *grp, WRegion *sub, 
                                  WRegionOrder order);

extern WStacking *group_find_stacking(WGroup *ws, WRegion *r);
extern WStacking *group_find_to_focus(WGroup *ws, WStacking *to_try);

extern WRegion *region_groupleader_of(WRegion *reg);
/*extern WRegion *region_group_of(WRegion *reg);*/

extern ExtlTab group_get_configuration(WGroup *ws);


typedef WStackingFilter WGroupIterFilter;
typedef WStackingIterTmp WGroupIterTmp;

extern void group_iter_init(WGroupIterTmp *tmp, WGroup *ws);
extern WRegion *group_iter(WGroupIterTmp *tmp);
extern WStacking *group_iter_nodes(WGroupIterTmp *tmp);

extern WStacking *group_get_stacking(WGroup *ws);
extern WStacking **group_get_stackingp(WGroup *ws);

#define FOR_ALL_MANAGED_BY_GROUP(WS, VAR, TMP) \
    for(group_iter_init(&(TMP), WS),           \
         VAR=group_iter(&(TMP));               \
        VAR!=NULL;                             \
        VAR=group_iter(&(TMP)))

#define FOR_ALL_NODES_IN_GROUP(WS, VAR, TMP) \
    for(group_iter_init(&(TMP), WS),         \
         VAR=group_iter_nodes(&(TMP));       \
        VAR!=NULL;                           \
        VAR=group_iter_nodes(&(TMP)))

extern WGroupIterTmp group_iter_default_tmp;


#endif /* ION_IONCORE_GROUP_H */
