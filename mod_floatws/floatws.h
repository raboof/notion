/*
 * ion/mod_floatws/floatws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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

#include "classes.h"

INTRSTRUCT(WFloatStacking);

DECLSTRUCT(WFloatStacking){
    WRegion *reg;
    WRegion *above;
    WFloatStacking *next, *prev;
    bool sticky;
};


DECLCLASS(WFloatWS){
    WGenWS genws;
    WRegion *managed_stdisp;
    int stdisp_corner;
    WRegion *current_managed;
};


extern WFloatWS *create_floatws(WWindow *parent, const WFitParams *fp);

extern WRegion *floatws_circulate(WFloatWS *ws);
extern WRegion *floatws_backcirculate(WFloatWS *ws);

extern WRegion *floatws_load(WWindow *par, const WFitParams *fp, 
                             ExtlTab tab);

extern WRegion* floatws_current(WFloatWS *floatws);

extern WFloatFrame *floatws_create_frame(WFloatWS *ws, 
                                         const WRectangle *geom, int gravity, 
                                         bool inner_geom, bool respect_pos);

extern bool floatws_manage_clientwin(WFloatWS *ws, WClientWin *cwin,
                                     const WManageParams *param, int redir);

extern bool floatws_add_managed(WFloatWS *ws, WRegion *reg);

extern bool floatws_rescue_clientwins(WFloatWS *ws);

extern bool floatws_rqclose(WFloatWS *ws);
extern bool floatws_rqclose_relocate(WFloatWS *ws);

extern void floatws_raise(WFloatWS *ws, WRegion *reg);
extern void floatws_lower(WFloatWS *ws, WRegion *reg);

/* */

extern bool mod_floatws_clientwin_do_manage(WClientWin *cwin, 
                                            const WManageParams *param);

extern WFloatStacking *mod_floatws_find_stacking(WRegion *r);

/* */

typedef struct{
    WFloatWS *ws;
    WFloatStacking *st;
} WFloatWSIterTmp;

extern void floatws_iter_init(WFloatWSIterTmp *tmp, WFloatWS *ws);
extern WRegion *floatws_iter(WFloatWSIterTmp *tmp);

#define FOR_ALL_MANAGED_BY_FLOATWS(WS, VAR, TMP) \
    for(floatws_iter_init(&(TMP), WS),           \
         VAR=floatws_iter(&(TMP));               \
        VAR!=NULL;                               \
        VAR=floatws_iter(&(TMP)))
    
extern WFloatWSIterTmp floatws_iter_default_tmp;

#define FOR_ALL_MANAGED_BY_FLOATWS_UNSAFE(WS, VAR) \
    FOR_ALL_MANAGED_BY_FLOATWS(WS, VAR, floatws_iter_default_tmp)


#endif /* ION_MOD_FLOATWS_FLOATWS_H */
