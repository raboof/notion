/*
 * ion/mod_floatws/floatws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_FLOATWS_FLOATWS_H
#define ION_MOD_FLOATWS_FLOATWS_H

#include <libtu/objlist.h>
#include <ioncore/common.h>
#include <ioncore/region.h>
#include <ioncore/screen.h>
#include <ioncore/genws.h>
#include <ioncore/manage.h>
#include <ioncore/extl.h>
#include <ioncore/rectangle.h>

INTRCLASS(WFloatWS);
INTRSTRUCT(WFloatStacking);

DECLSTRUCT(WFloatStacking){
    WRegion *reg;
    WRegion *above;
    WFloatStacking *next, *prev;
};


DECLCLASS(WFloatWS){
    WGenWS genws;
    WRegion *managed_list;
    WRegion *managed_stdisp;
    int stdisp_corner;
    WRegion *current_managed;
    WFloatStacking *stacking;
};


extern WFloatWS *create_floatws(WWindow *parent, const WFitParams *fp);

extern WRegion *floatws_circulate(WFloatWS *ws);
extern WRegion *floatws_backcirculate(WFloatWS *ws);

extern WRegion *floatws_load(WWindow *par, const WFitParams *fp, 
                             ExtlTab tab);

extern WRegion* floatws_current(WFloatWS *floatws);

extern bool floatws_manage_clientwin(WFloatWS *ws, WClientWin *cwin,
                                     const WManageParams *param, int redir);

extern bool floatws_manage_rescue(WFloatWS *ws, WClientWin *cwin,
                                  WRegion *from);

extern bool floatws_rescue_clientwins(WFloatWS *ws);

extern bool floatws_rqclose(WFloatWS *ws);
extern bool floatws_rqclose_relocate(WFloatWS *ws);

extern void floatws_raise(WFloatWS *ws, WRegion *reg);
extern void floatws_lower(WFloatWS *ws, WRegion *reg);

/* */

extern bool mod_floatws_clientwin_do_manage(WClientWin *cwin, 
                                            const WManageParams *param);

/* */

extern ObjList *floatws_sticky_list;

#endif /* ION_MOD_FLOATWS_FLOATWS_H */
