/*
 * wmcore/region.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_REGION_H
#define WMCORE_REGION_H

#include "common.h"
#include "thing.h"

INTROBJ(WRegion)
INTRSTRUCT(WWinGeomParams)
INTRSTRUCT(WSubmapState)


DECLSTRUCT(WSubmapState){
	uint key, state;
	WSubmapState *next;
};


DECLOBJ(WRegion){
	WThing thing;
	WRectangle geom;
	void *screen;
	bool flags;
	void *uldata;
	
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
};


DECLSTRUCT(WWinGeomParams){
	Window win;
	int win_x, win_y;
	WRectangle geom;
};

		
#define REGION_MAPPED		0x0001
#define REGION_ACTIVE		0x0002
#define REGION_HAS_GRABS	0x0004

#define MARK_REGION_MAPPED(R)	(((WRegion*)(R))->flags|=REGION_MAPPED)
#define MARK_REGION_UNMAPPED(R)	(((WRegion*)(R))->flags&=~REGION_MAPPED)

/* Use region_is_fully_mapped instead */
#define REGION_IS_MAPPED(R)		(((WRegion*)(R))->flags&REGION_MAPPED)
#define REGION_IS_ACTIVE(R)		(((WRegion*)(R))->flags&REGION_ACTIVE)

#define REGION_GEOM(R)  (((WRegion*)(R))->geom)
#define REGION_SCREEN(R)  ((WScreen*)((WRegion*)(R))->screen)
#define REGION_ACTIVE_SUB(R)  (((WRegion*)(R))->active_sub)

DYNFUN void fit_region(WRegion *reg, WRectangle geom);
DYNFUN bool reparent_region(WRegion *reg, WWinGeomParams params);
DYNFUN void map_region(WRegion *reg);
DYNFUN void unmap_region(WRegion *reg);
/* Use set_focus instead except in focus_region handler itself 
 * when focusing subregions.
 */
DYNFUN void focus_region(WRegion *reg, bool warp);
DYNFUN bool switch_subregion(WRegion *reg, WRegion *sub);
DYNFUN void region_rect_params(const WRegion *reg, WRectangle geom,
							   WWinGeomParams *ret);
DYNFUN void region_notify_rootpos(WRegion *reg, int x, int y);
/* mode==Above, return topmost; mode==Below, return bottomost */
DYNFUN Window region_restack(WRegion *reg, Window other, int mode);
DYNFUN Window region_lowest_win(WRegion *reg);
DYNFUN void region_activated(WRegion *reg);
DYNFUN void region_inactivated(WRegion *reg);
DYNFUN void region_sub_activated(WRegion *reg, WRegion *sub);
DYNFUN void region_notify_subname(WRegion *reg, WRegion *sub);
DYNFUN void region_request_sub_geom(WRegion *reg, WRegion *sub,
									WRectangle geom, WRectangle *geomret,
									bool tryonly);
DYNFUN void region_remove_sub(WRegion *reg, WRegion *sub);
DYNFUN WRegion *region_selected_sub(WRegion *reg);

extern void region_params(const WRegion *reg, WWinGeomParams *ret);
extern void region_params2(const WRegion *reg, WRectangle geom,
						   WWinGeomParams *ret);


/* Implementation for regions that do not allow subregions to resize
 * themselves; default is to give the size the region wants.
 */
extern void region_request_sub_geom_unallow(WRegion *reg, WRegion *sub,
											WRectangle geom, WRectangle *geomret,
											bool tryonly);
extern void region_request_sub_geom_constrain(WRegion *reg, WRegion *sub,
											  WRectangle geom, WRectangle *geomret,
											  bool tryonly);

extern void detach_region(WRegion *reg);
extern bool detach_reparent_region(WRegion *reg, WWinGeomParams params);

extern void region_rootpos(WRegion *reg, int *xret, int *yret);
extern void notify_subregions_rootpos(WRegion *reg, int x, int y);
extern void notify_subregions_move(WRegion *reg);

extern bool display_region(WRegion *reg);
extern bool switch_region(WRegion *reg);
extern bool goto_region(WRegion *reg);

extern void map_all_subregions(WRegion *reg);
extern void unmap_all_subregions(WRegion *reg);

extern void init_region(WRegion *reg, void *scr, WRectangle geom);
extern void deinit_region(WRegion *reg);

extern void region_got_focus(WRegion *reg, WRegion *sub);
extern void region_lost_focus(WRegion *reg);

extern bool region_is_fully_mapped(WRegion *reg);

extern bool set_region_name(WRegion *reg, const char *name);
extern const char *region_name(WRegion *reg);
extern uint region_name_instance(WRegion *reg);

/* Returns a newly allocated copy of the name with the instance
 * number appended. */
extern char *region_full_name(WRegion *reg);
extern char *region_make_label(WRegion *reg, int maxw, XFontStruct *font);

extern WRegion *do_lookup_region(const char *cname, WObjDescr *descr);
extern int do_complete_region(char *nam, char ***cp_ret, char **beg,
							 WObjDescr *descr);
extern WRegion *lookup_region(const char *cname);
extern int complete_region(char *nam, char ***cp_ret, char **beg);

#endif /* WMCORE_REGION_H */
