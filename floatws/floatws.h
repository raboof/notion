/*
 * ion/floatws/floatws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_FLOATWS_FLOATWS_H
#define ION_FLOATWS_FLOATWS_H

#include <libtu/objlist.h>
#include <ioncore/common.h>
#include <ioncore/region.h>
#include <ioncore/screen.h>
#include <ioncore/genws.h>
#include <ioncore/manage.h>
#include <ioncore/extl.h>
#include <ioncore/rectangle.h>

INTRCLASS(WFloatWS);

DECLCLASS(WFloatWS){
    WGenWS genws;
    Window dummywin;
    WRegion *managed_list;
    WRegion *current_managed;
};


extern WFloatWS *create_floatws(WWindow *parent, const WRectangle *bounds);

extern WRegion *floatws_circulate(WFloatWS *ws);
extern WRegion *floatws_backcirculate(WFloatWS *ws);

extern WRegion *floatws_load(WWindow *par, const WRectangle *geom, 
                             ExtlTab tab);

extern WRegion* floatws_current(WFloatWS *floatws);

extern bool floatws_manage_clientwin(WFloatWS *ws, WClientWin *cwin,
                                     const WManageParams *param, int redir);

extern bool floatws_manage_rescue(WFloatWS *ws, WClientWin *cwin,
                                  WRegion *from);

extern bool floatws_rescue_clientwins(WFloatWS *ws);

/* */

extern bool floatwsmod_clientwin_do_manage(WClientWin *cwin, 
                                           const WManageParams *param);

/* */

extern ObjList *floatws_sticky_list;

#endif /* ION_FLOATWS_FLOATWS_H */
