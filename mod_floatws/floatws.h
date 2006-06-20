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

#include <ioncore/common.h>
#include <ioncore/rectangle.h>
#include <ioncore/group.h>
#include "floatws.h"
#include "classes.h"


DECLCLASS(WFloatWS){
    WGroup grp;
};


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


extern void floatws_calc_placement(WFloatWS *ws, WRectangle *geom);
extern void mod_floatws_set_placement_method(const char *method);

extern WFloatFrame *floatws_create_frame(WFloatWS *ws, const WRectangle *geom, 
                                         bool inner_geom, bool respect_pos,
                                         int gravity);
extern bool floatws_phattach(WFloatWS *ws, 
                             WRegionAttachHandler *hnd, void *hnd_param,
                             WFloatWSPHAttachParams *param);

extern WPHolder *floatws_prepare_manage(WFloatWS *ws, 
                                        const WClientWin *cwin,
                                        const WManageParams *param, 
                                        int redir);

extern WPHolder *floatws_prepare_manage_transient(WFloatWS *ws, 
                                                  const WClientWin *cwin,
                                                  const WManageParams *param,
                                                  int unused);

extern bool floatws_handle_drop(WFloatWS *ws, int x, int y,
                                WRegion *dropped);

extern WRegion *floatws_load(WWindow *par, const WFitParams *fp, ExtlTab tab);

#endif /* ION_MOD_FLOATWS_FLOATWS_H */
