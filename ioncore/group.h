/*
 * ion/ioncore/group.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GROUP_H
#define ION_IONCORE_GROUP_H

#include <libextl/extl.h>
#include <ioncore/common.h>
#include <ioncore/region.h>
#include <ioncore/screen.h>
#include <ioncore/genws.h>
#include <ioncore/manage.h>
#include <ioncore/rectangle.h>
#include <ioncore/pholder.h>
#include <ioncore/stacking.h>


INTRSTRUCT(WGroupAttachParams);

DECLSTRUCT(WGroupAttachParams){
    uint level_set:1;
    uint szplcy_set:1;
    uint geom_set:1;
    uint switchto_set:1;
    uint switchto:1;
    
    uint modal:1;
    uint sticky:1;
    uint bottom:1;
    
    uint level;
    WRectangle geom;
    WSizePolicy szplcy;
};


DECLCLASS(WGroup){
    WGenWS genws;
    WStacking *managed_list;
    WStacking *managed_stdisp;
    WStacking *current_managed;
    WStacking *bottom;
};


extern bool group_init(WGroup *grp, WWindow *parent, const WFitParams *fp);
extern WGroup *create_group(WWindow *parent, const WFitParams *fp);
extern void group_deinit(WGroup *grp);

extern WRegion *group_circulate(WGroup *ws);
extern WRegion *group_backcirculate(WGroup *ws);

extern WRegion *group_load(WWindow *par, const WFitParams *fp, 
                           ExtlTab tab);

extern WRegion* group_current(WGroup *group);

DYNFUN WStacking *group_do_add_managed(WGroup *ws, WRegion *reg, 
                                       int level, WSizePolicy szplcy);
extern WStacking *group_do_add_managed_default(WGroup *ws, WRegion *reg, 
                                               int level, WSizePolicy szplcy);

extern WRegion *group_do_attach(WGroup *ws, 
                                WRegionAttachHandler *fn, void *fnparams, 
                                const WGroupAttachParams *param);

extern WRegion *group_attach(WGroup *ws, WRegion *reg, ExtlTab param);
extern WRegion *group_attach_new(WGroup *ws, ExtlTab param);

extern void group_managed_remove(WGroup *ws, WRegion *reg);

extern bool group_rescue_clientwins(WGroup *ws, WPHolder *ph);

extern bool group_rqclose(WGroup *ws);
extern bool group_rqclose_relocate(WGroup *ws);

extern void group_raise(WGroup *ws, WRegion *reg);
extern void group_lower(WGroup *ws, WRegion *reg);

extern WStacking *group_find_stacking(WGroup *ws, WRegion *r);

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
