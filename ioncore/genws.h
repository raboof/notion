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

INTROBJ(WGenWS)

DECLOBJ(WGenWS){
	WRegion reg;
};

extern void init_genws(WGenWS *ws, WWindow *parent, WRectangle geom);
extern void deinit_genws(WGenWS *ws);

extern WGenWS *lookup_workspace(const char *name);
extern int complete_workspace(char *nam, char ***cp_ret, char **beg,
							  void *unused);

#endif /* ION_IONCORE_GENWS_H */
