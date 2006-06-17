/*
 * ion/mod_floatws/floatws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_FLOATWS_FLOATWS_H
#define ION_MOD_FLOATWS_FLOATWS_H

#include <libextl/extl.h>
#include <ioncore/common.h>
#include <ioncore/region.h>
#include <ioncore/screen.h>
#include <ioncore/genws.h>
#include <ioncore/manage.h>
#include <ioncore/rectangle.h>
#include <ioncore/pholder.h>
#include <ioncore/stacking.h>

#include "classes.h"


INTRSTRUCT(WFloatWSPHAttachParams);


DECLSTRUCT(WFloatWSPHAttachParams){
    WFrame *frame;
    WRectangle geom;
    bool inner_geom;
    bool pos_ok;
    int gravity;
    int aflags;
    WRegion *stack_above;
};



DECLCLASS(WFloatWS){
    WGenWS genws;
    WMPlexSTDispInfo stdispi;
    WStacking *managed_list;
    WStacking *managed_stdisp;
    WStacking *current_managed;
    WStacking *bottom;
};


extern WFloatWS *create_floatws(WWindow *parent, const WFitParams *fp);

extern WRegion *floatws_circulate(WFloatWS *ws);
extern WRegion *floatws_backcirculate(WFloatWS *ws);

extern WRegion *floatws_load(WWindow *par, const WFitParams *fp, 
                             ExtlTab tab);

extern WRegion* floatws_current(WFloatWS *floatws);

extern WFloatFrame *floatws_create_frame(WFloatWS *ws, const WRectangle *geom, 
                                         bool inner_geom, bool respect_pos,
                                         int gravity);
extern bool floatws_phattach(WFloatWS *ws, 
                             WRegionAttachHandler *hnd, void *hnd_param,
                             WFloatWSPHAttachParams *param);

extern WPHolder *floatws_prepare_manage(WFloatWS *ws, const WClientWin *cwin,
                                        const WManageParams *param, int redir);

extern bool floatws_add_managed(WFloatWS *ws, WRegion *reg);

extern bool floatws_rescue_clientwins(WFloatWS *ws, WPHolder *ph);

extern bool floatws_rqclose(WFloatWS *ws);
extern bool floatws_rqclose_relocate(WFloatWS *ws);

extern void floatws_raise(WFloatWS *ws, WRegion *reg);
extern void floatws_lower(WFloatWS *ws, WRegion *reg);

extern WStacking *floatws_find_stacking(WFloatWS *ws, WRegion *r);

typedef WStackingFilter WFloatWSIterFilter;
typedef WStackingIterTmp WFloatWSIterTmp;

extern void floatws_iter_init(WFloatWSIterTmp *tmp, WFloatWS *ws);
extern WRegion *floatws_iter(WFloatWSIterTmp *tmp);
extern WStacking *floatws_iter_nodes(WFloatWSIterTmp *tmp);

#define FOR_ALL_MANAGED_BY_FLOATWS(WS, VAR, TMP) \
    for(floatws_iter_init(&(TMP), WS),           \
         VAR=floatws_iter(&(TMP));               \
        VAR!=NULL;                               \
        VAR=floatws_iter(&(TMP)))

#define FOR_ALL_NODES_ON_FLOATWS(WS, VAR, TMP) \
    for(floatws_iter_init(&(TMP), WS),         \
         VAR=floatws_iter_nodes(&(TMP));       \
        VAR!=NULL;                             \
        VAR=floatws_iter_nodes(&(TMP)))

extern WFloatWSIterTmp floatws_iter_default_tmp;


#endif /* ION_MOD_FLOATWS_FLOATWS_H */
