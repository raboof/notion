/*
 * pwm/genericws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_GENERICWS_H
#define WMCORE_GENERICWS_H

#include "region.h"

INTROBJ(WGenericWS)

DECLOBJ(WGenericWS){
	WRegion reg;
};

extern void init_genericws(WGenericWS *ws, WRegion *parent, WRectangle geom);
extern void deinit_genericws(WGenericWS *ws);

extern WGenericWS *lookup_workspace(const char *name);
extern int complete_workspace(char *nam, char ***cp_ret, char **beg,
							  void *unused);

#endif /* WMCORE_GENERICWS_H */
