/*
 * ion/ionws.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONWS_H
#define ION_IONWS_H

#include <wmcore/common.h>

INTROBJ(WIonWS)

#include <wmcore/region.h>
#include <wmcore/viewport.h>
#include <wmcore/genericws.h>

#include "split.h"


DECLOBJ(WIonWS){
	WGenericWS genws;
	WObj *split_tree;
	WRegion *managed_list;
};


extern WIonWS *create_ionws(WRegion *parent, WRectangle bounds,
									const char *name, bool ci);
extern WIonWS *create_new_ionws_on_vp(WViewport *vp, const char *name);

extern void ionws_goto_above(WIonWS *ws);
extern void ionws_goto_below(WIonWS *ws);
extern void ionws_goto_left(WIonWS *ws);
extern void ionws_goto_right(WIonWS *ws);

extern void init_workspaces(WViewport *vp);
extern void setup_screens();

#endif /* ION_IONWS_H */
