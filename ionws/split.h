/*
 * ion/split.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_SPLIT_H
#define ION_SPLIT_H

#include <wmcore/common.h>
#include <wmcore/window.h>

INTROBJ(WWsSplit)
INTRSTRUCT(WResizeTmp)

#define SPLIT_OF(X) ((X)->mgr_data)

enum WSplitDir{
	HORIZONTAL,
	VERTICAL
};


enum PrimaryNode{
	ANY,
	TOP_OR_LEFT,
	BOTTOM_OR_RIGHT
};


DECLOBJ(WWsSplit){
	WObj obj;
	int dir;
	WRectangle geom;
	int tmpsize, knowsize;
	int res;
	int current;
	WObj *tl, *br;
	WWsSplit *parent;
};


DECLSTRUCT(WResizeTmp){
	WObj *startnode;
	int postmp, sizetmp;
	int winpostmp, winsizetmp;
	int dir;
};


extern WWsSplit *create_split(int dir, WObj *tl, WObj *br, WRectangle geom);
extern int split_tree_do_resize(WObj *node_, int dir, int npos, int nsize);

extern int split_tree_size(WObj *obj, int dir);
extern int split_tree_pos(WObj *obj, int dir);
extern int split_tree_other_size(WObj *obj, int dir);

#include "workspace.h"

extern void workspace_add_managed(WWorkspace *ws, WRegion *reg);
extern WRegion *workspace_do_find_new_manager(WRegion *reg);
extern WRegion *workspace_find_new_manager(WRegion *reg);
extern void workspace_request_managed_geom(WWorkspace *ws, WRegion *reg,
										   WRectangle geom,
										   WRectangle *geomret, bool tryonly);
extern void workspace_remove_managed(WWorkspace *ws, WRegion *reg);
extern bool remove_split(WWorkspace *ws, WWsSplit *split);
extern WRegion *workspace_find_current(WWorkspace *ws);

#endif /* ION_SPLIT_H */

