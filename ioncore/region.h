/*
 * ion/ioncore/region.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_REGION_H
#define ION_IONCORE_REGION_H

#include "common.h"
#include <libtu/obj.h>
#include "rectangle.h"

#define REGION_MAPPED		0x0001
#define REGION_ACTIVE		0x0002
#define REGION_HAS_GRABS 	0x0004
#define REGION_TAGGED		0x0008
#define REGION_BINDINGS_ARE_GRABBED 0x0020
#define REGION_KEEPONTOP 	0x0080
#define REGION_ACTIVITY		0x0100
#define REGION_SKIP_FOCUS 	0x0200

/* Use region_is_fully_mapped instead for most cases. */
#define REGION_IS_MAPPED(R)		(((WRegion*)(R))->flags&REGION_MAPPED)
#define REGION_MARK_MAPPED(R)	(((WRegion*)(R))->flags|=REGION_MAPPED)
#define REGION_MARK_UNMAPPED(R)	(((WRegion*)(R))->flags&=~REGION_MAPPED)
#define REGION_IS_ACTIVE(R)		(((WRegion*)(R))->flags&REGION_ACTIVE)
#define REGION_IS_TAGGED(R)		(((WRegion*)(R))->flags&REGION_TAGGED)
#define REGION_IS_URGENT(R)		(((WRegion*)(R))->flags&REGION_URGENT)
#define REGION_GEOM(R)  		(((WRegion*)(R))->geom)
#define REGION_ACTIVE_SUB(R)  	(((WRegion*)(R))->active_sub)

INTRSTRUCT(WSubmapState);
DECLSTRUCT(WSubmapState){
	uint key, state;
	WSubmapState *next;
};

DECLCLASS(WRegion){
	Obj obj;
	
	WRectangle geom;
	void *rootwin;
	bool flags;

	WRegion *parent, *children;
	WRegion *p_next, *p_prev;
	
	void *bindings;
	WSubmapState submapstat;
	
	WRegion *active_sub;
	
	struct{
		char *name;
		void *namespaceinfo;
		WRegion *ns_next, *ns_prev;
	} ni;
	
	WRegion *manager;
	WRegion *mgr_next, *mgr_prev;
	
	struct{
		WRegion *below_list;
		WRegion *above;
		WRegion *next, *prev;
	} stacking;
	
	int mgd_activity;
};

extern void region_init(WRegion *reg, WRegion *parent, 
						const WRectangle *geom);
extern void region_deinit(WRegion *reg);

DYNFUN void region_fit(WRegion *reg, const WRectangle *geom);
DYNFUN void region_map(WRegion *reg);
DYNFUN void region_unmap(WRegion *reg);
DYNFUN Window region_xwindow(const WRegion *reg);
DYNFUN void region_activated(WRegion *reg);
DYNFUN void region_inactivated(WRegion *reg);
DYNFUN void region_draw_config_updated(WRegion *reg);
DYNFUN void region_close(WRegion *reg);
DYNFUN WRegion *region_control_managed_focus(WRegion *mgr, WRegion *reg);
DYNFUN void region_remove_managed(WRegion *reg, WRegion *sub);
DYNFUN bool region_display_managed(WRegion *reg, WRegion *sub);
DYNFUN void region_managed_activated(WRegion *reg, WRegion *sub);
DYNFUN void region_managed_inactivated(WRegion *reg, WRegion *sub);
DYNFUN void region_notify_managed_change(WRegion *reg, WRegion *sub);
DYNFUN bool region_may_destroy_managed(WRegion *mgr, WRegion *reg);
DYNFUN WRegion *region_current(WRegion *mgr);
DYNFUN void region_notify_rootpos(WRegion *reg, int x, int y);

extern void region_draw_config_updated_default(WRegion *reg);

extern bool region_may_destroy(WRegion *reg);

extern void region_rootpos(WRegion *reg, int *xret, int *yret);
extern void region_notify_subregions_rootpos(WRegion *reg, int x, int y);
extern void region_notify_subregions_move(WRegion *reg);
extern void region_notify_change(WRegion *reg);

extern bool region_display(WRegion *reg);
extern bool region_display_sp(WRegion *reg);
extern bool region_goto(WRegion *reg);

extern bool region_is_fully_mapped(WRegion *reg);

extern void region_detach(WRegion *reg);
extern void region_detach_parent(WRegion *reg);
extern void region_detach_manager(WRegion *reg);

extern WRegion *region_active_sub(WRegion *reg);
extern WRegion *region_parent(WRegion *reg);
extern WRegion *region_manager(WRegion *reg);
extern WRegion *region_manager_or_parent(WRegion *reg);
extern void region_set_parent(WRegion *reg, WRegion *par);
extern void region_attach_parent(WRegion *reg, WRegion *par);
extern void region_set_manager(WRegion *reg, WRegion *mgr, WRegion **listptr);
extern void region_unset_manager(WRegion *reg, WRegion *mgr, WRegion **listptr);

extern WRootWin *region_rootwin_of(const WRegion *reg);
extern Window region_root_of(const WRegion *reg);
extern bool region_same_rootwin(const WRegion *reg1, const WRegion *reg2);

#endif /* ION_IONCORE_REGION_H */
