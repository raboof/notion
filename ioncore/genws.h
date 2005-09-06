/*
 * ion/ioncore/genws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GENWS_H
#define ION_IONCORE_GENWS_H

#include <libextl/extl.h>
#include "region.h"
#include "window.h"
#include "rectangle.h"
#include "mplex.h"

DECLCLASS(WGenWS){
    WRegion reg;
    Window dummywin;
};

extern bool genws_init(WGenWS *ws, WWindow *par, const WFitParams *fp);
extern void genws_deinit(WGenWS *ws);

extern void genws_do_reparent(WGenWS *ws, WWindow *par, const WFitParams *fp);
extern void genws_fallback_focus(WGenWS *ws, bool warp);
extern Window genws_xwindow(const WGenWS *ws);
extern void genws_do_map(WGenWS *ws);
extern void genws_do_unmap(WGenWS *ws);

DYNFUN void genws_manage_stdisp(WGenWS *ws, WRegion *stdisp, 
                                const WMPlexSTDispInfo *info);
DYNFUN void genws_unmanage_stdisp(WGenWS *ws, bool permanent, bool nofocus);

#endif /* ION_IONCORE_GENWS_H */
