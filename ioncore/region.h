/*
 * ion/ioncore/region.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_REGION_H
#define ION_IONCORE_REGION_H

#include "common.h"
#include "obj.h"

INTROBJ(WRegion)
INTRSTRUCT(WSubmapState)


DECLSTRUCT(WSubmapState){
	uint key, state;
	WSubmapState *next;
};


DECLOBJ(WRegion){
	WObj obj;
	
	WRectangle geom;
	void *screen;
	bool flags;

	WRegion *parent, *children;
	WRegion *p_next, *p_prev;
	
	void *bindings;
	WSubmapState submapstat;
	
	WRegion *active_sub, *previous_act;
	
	struct{
		char *name;
		int instance;
		WRegion *n_next, *n_prev;
		WRegion *g_next, *g_prev;
	} ni;
	
	void *funclist;
	
	WRegion *tag_next, *tag_prev;
	
	WRegion *manager;
	WRegion *mgr_next, *mgr_prev;
	void *mgr_data;
};


#define REGION_MAPPED		0x0001
#define REGION_ACTIVE		0x0002
#define REGION_HAS_GRABS 	0x0004
#define REGION_TAGGED		0x0008
#define REGION_BINDINGS_ARE_GRABBED 0x0020
#define REGION_SUBDEST 0x0040

#define MARK_REGION_MAPPED(R)	(((WRegion*)(R))->flags|=REGION_MAPPED)
#define MARK_REGION_UNMAPPED(R)	(((WRegion*)(R))->flags&=~REGION_MAPPED)

/* Use region_is_fully_mapped instead */
#define REGION_IS_MAPPED(R)		(((WRegion*)(R))->flags&REGION_MAPPED)
#define REGION_IS_ACTIVE(R)		(((WRegion*)(R))->flags&REGION_ACTIVE)
#define REGION_IS_TAGGED(R)		(((WRegion*)(R))->flags&REGION_TAGGED)
#define REGION_GEOM(R)  		(((WRegion*)(R))->geom)
#define REGION_ACTIVE_SUB(R)  	(((WRegion*)(R))->active_sub)
#define REGION_MANAGER(R)  		(((WRegion*)(R))->manager)


#define FOR_ALL_MANAGED_ON_LIST(LIST, REG) \
	for((REG)=(LIST); (REG)!=NULL; (REG)=(REG)->mgr_next)

#define FOR_ALL_MANAGED_ON_LIST_W_NEXT(LIST, REG, NEXT)            \
  for((REG)=(LIST), (NEXT)=((REG)==NULL ? NULL : (REG)->mgr_next); \
      (REG)!=NULL;                                                 \
	  (REG)=(NEXT), (NEXT)=((REG)==NULL ? NULL : (REG)->mgr_next))

#define NEXT_MANAGED(LIST, REG) (((WRegion*)(REG))->mgr_next)
#define PREV_MANAGED(LIST, REG) ((((WRegion*)(REG))->mgr_prev->mgr_next) ? \
                                 (((WRegion*)(REG))->mgr_prev) : NULL)
#define NEXT_MANAGED_WRAP(LIST, REG) (((REG) && ((WRegion*)(REG))->mgr_next) \
                                      ? ((WRegion*)(REG))->mgr_next : (LIST))
#define PREV_MANAGED_WRAP(LIST, REG) ((REG) ? ((WRegion*)(REG))->mgr_prev \
                                      : (LIST))
#define FIRST_MANAGED(LIST) 	(LIST)
#define LAST_MANAGED(LIST) 	 	((LIST)==NULL ? NULL              \
								 : PREV_MANAGED_WRAP(LIST, LIST))


DYNFUN void fit_region(WRegion *reg, WRectangle geom);
DYNFUN void map_region(WRegion *reg);
DYNFUN void unmap_region(WRegion *reg);
/* Use set_focus instead except in focus_region handler itself 
 * when focusing subregions.
 */
DYNFUN void focus_region(WRegion *reg, bool warp);
DYNFUN void region_notify_rootpos(WRegion *reg, int x, int y);
/* mode==Above, return topmost; mode==Below, return bottomost */
DYNFUN Window region_restack(WRegion *reg, Window other, int mode);
DYNFUN Window region_x_window(const WRegion *reg);
DYNFUN void region_activated(WRegion *reg);
DYNFUN void region_inactivated(WRegion *reg);
DYNFUN void region_draw_config_updated(WRegion *reg);
extern void default_draw_config_updated(WRegion *reg);

extern void region_rootpos(WRegion *reg, int *xret, int *yret);
extern void notify_subregions_rootpos(WRegion *reg, int x, int y);
extern void notify_subregions_move(WRegion *reg);

extern bool display_region(WRegion *reg);
extern bool display_region_sp(WRegion *reg);
extern bool goto_region(WRegion *reg);

extern void init_region(WRegion *reg, WRegion *parent, WRectangle geom);
extern void deinit_region(WRegion *reg);

/* --> window? */
extern void region_got_focus(WRegion *reg);
extern void region_lost_focus(WRegion *reg);

extern bool region_is_fully_mapped(WRegion *reg);
extern void region_notify_change(WRegion *reg);

extern void region_detach(WRegion *reg);
extern void region_detach_parent(WRegion *reg);
extern void region_detach_manager(WRegion *reg);

extern void region_set_parent(WRegion *reg, WRegion *par);
extern void region_set_manager(WRegion *reg, WRegion *mgr, WRegion **listptr);
extern void region_unset_manager(WRegion *reg, WRegion *mgr, WRegion **listptr);
extern bool region_manages_active_reg(WRegion *reg);

DYNFUN WRegion *region_managed_enter_to_focus(WRegion *mgr, WRegion *reg);
DYNFUN void region_remove_managed(WRegion *reg, WRegion *sub);
DYNFUN bool region_display_managed(WRegion *reg, WRegion *sub);
DYNFUN void region_managed_activated(WRegion *reg, WRegion *sub);
DYNFUN void region_managed_inactivated(WRegion *reg, WRegion *sub);
DYNFUN void region_notify_managed_change(WRegion *reg, WRegion *sub);
DYNFUN void region_request_managed_geom(WRegion *reg, WRegion *sub,
										WRectangle geom, WRectangle *geomret,
										bool tryonly);

/* Implementation for regions that do not allow subregions to resize
 * themselves; default is to give the size the region wants.
 */
extern void region_request_managed_geom_unallow(WRegion *reg, WRegion *sub,
												WRectangle geom, WRectangle *geomret,
												bool tryonly);
extern void region_request_managed_geom_constrain(WRegion *reg, WRegion *sub,
												  WRectangle geom, WRectangle *geomret,
												  bool tryonly);
/* default */
extern void region_request_managed_geom_allow(WRegion *reg, WRegion *sub,
											  WRectangle geom, WRectangle *geomret,
											  bool tryonly);


/* Old WThing stuff */

#define FIRST_CHILD(NAM, TYPE) (TYPE*)first_child((WRegion*)NAM, &OBJDESCR(TYPE))
#define NEXT_CHILD(NAM, TYPE) (TYPE*)next_child((WRegion*)NAM, &OBJDESCR(TYPE))
#define NEXT_CHILD_FB(NAM, TYPE, FB) (TYPE*)next_child_fb((WRegion*)NAM, &OBJDESCR(TYPE), (WRegion*)FB)
#define LAST_CHILD(NAM, TYPE) (TYPE*)last_child((WRegion*)NAM, &OBJDESCR(TYPE))
#define PREV_CHILD(NAM, TYPE) (TYPE*)prev_child((WRegion*)NAM, &OBJDESCR(TYPE))
#define PREV_CHILD_FB(NAM, TYPE, FB) (TYPE*)prev_child_fb((WRegion*)NAM, &OBJDESCR(TYPE), (WRegion*)FB)
#define FIND_PARENT(NAM, TYPE) (TYPE*)find_parent((WRegion*)NAM, &OBJDESCR(TYPE))
#define FIND_PARENT1(NAM, TYPE) (TYPE*)find_parent1((WRegion*)NAM, &OBJDESCR(TYPE))
#define NTH_CHILD(NAM, N, TYPE) (TYPE*)nth_child((WRegion*)NAM, N, &OBJDESCR(TYPE))
#define FOR_ALL_TYPED_CHILDREN(NAM, NAM2, TYPE) \
	for(NAM2=FIRST_CHILD(NAM, TYPE); NAM2!=NULL; NAM2=NEXT_CHILD(NAM2, TYPE))

extern void link_child(WRegion *parent, WRegion *child);
extern void link_child_before(WRegion *before, WRegion *child);
extern void link_child_after(WRegion *after, WRegion *reg);
extern void unlink_from_parent(WRegion *reg);
extern void destroy_children(WRegion *reg);

extern WRegion *next_child(WRegion *first, const WObjDescr *descr);
extern WRegion *next_child_fb(WRegion *first, const WObjDescr *descr, WRegion *fb);
extern WRegion *prev_child(WRegion *first, const WObjDescr *descr);
extern WRegion *prev_child_fb(WRegion *first, const WObjDescr *descr, WRegion *fb);
extern WRegion *first_child(WRegion *parent, const WObjDescr *descr);
extern WRegion *last_child(WRegion *parent, const WObjDescr *descr);
extern WRegion *find_parent(WRegion *p, const WObjDescr *descr);
extern WRegion *find_parent1(WRegion *p, const WObjDescr *descr);
extern WRegion *nth_child(WRegion *parent, int n, const WObjDescr *descr);

extern bool region_is_ancestor(WRegion *reg, WRegion *reg2);
extern bool region_is_child(WRegion *reg, WRegion *reg2);

#endif /* ION_IONCORE_REGION_H */
