/*
 * ion/ioncore/attach.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_ATTACH_H
#define ION_IONCORE_ATTACH_H

#include "region.h"
#include "reginfo.h"
#include "window.h"
#include "clientwin.h"

#define REGION_ATTACH_SWITCHTO	0x0001
#define REGION_ATTACH_POSRQ		0x0002
#define REGION_ATTACH_SIZERQ	0x0004
#define REGION_ATTACH_INITSTATE	0x0010 /* only set by add_clientwin */
#define REGION_ATTACH_DOCKAPP	0x0020 /* only set by add_clientwin */
#define REGION_ATTACH_TFOR		0x0040 /* only set by add_clientwin */
#define REGION_ATTACH_MAPRQ 	0x0080 /* only setd by add_clientwin;
										  implies POSRQ|SIZERQ */
#define REGION_ATTACH_SIZE_HINTS 0x0100

#define REGION_ATTACH_IS_GEOMRQ(FLAGS) \
 (((FLAGS)&(REGION_ATTACH_POSRQ|REGION_ATTACH_SIZERQ)) \
	  ==(REGION_ATTACH_POSRQ|REGION_ATTACH_SIZERQ))

typedef struct{
	int flags;
	int init_state;
	WRectangle geomrq;
	WClientWin *tfor;
	XSizeHints *size_hints;
} WAttachParams;


typedef WRegion *WRegionAddFn(WWindow *parent, WRectangle geom, void *param);


DYNFUN WRegion *region_do_add_managed(WRegion *reg, WRegionAddFn *fn,
									  void *fnpar, const WAttachParams *par);

extern bool region_supports_add_managed(WRegion *reg);

extern WRegion *region_add_managed_new_simple(WRegion *reg,
											  WRegionSimpleCreateFn *fn,
											  int flags);
extern bool region_add_managed_simple(WRegion *reg, WRegion *sub, int flags);
extern bool region_add_managed(WRegion *reg, WRegion *sub,
							   const WAttachParams *par);

/* */

DYNFUN WRegion *region_find_rescue_manager_for(WRegion *reg, WRegion *todst);
extern WRegion *default_find_rescue_manager_for(WRegion *reg, WRegion *todst);
extern WRegion *region_find_rescue_manager(WRegion *reg);
extern bool region_can_manage_clientwins(WRegion *reg);

extern bool rescue_clientwins_on_list(WRegion *reg, WRegion *list);
extern bool move_clientwins_on_list(WRegion *dest, WRegion *src, WRegion *list);

#endif /* ION_IONCORE_ATTACH_H */
