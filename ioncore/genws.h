/*
 * pwm/genws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GENWS_H
#define ION_IONCORE_GENWS_H

#include "region.h"
#include "window.h"
#include "extl.h"

INTROBJ(WGenWS);

DECLOBJ(WGenWS){
	WRegion reg;
};

extern void genws_init(WGenWS *ws, WWindow *parent, WRectangle geom);
extern void genws_deinit(WGenWS *ws);

extern WGenWS *lookup_workspace(const char *name);
extern ExtlTab complete_workspace(const char *nam);

#endif /* ION_IONCORE_GENWS_H */
