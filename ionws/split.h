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
extern int tree_do_resize(WObj *node_, int dir, int npos, int nsize);
extern int calcresize_window(WWindow *wwin, int dir, int prim, int nsize,
							 WResizeTmp *tmp);
extern void resize_tmp(const WResizeTmp *tmp);
extern int tree_do_resize(WObj *node_, int dir, int npos, int nsize);

extern WRegion *workspace_find_new_manager(WRegion *reg);

#endif /* ION_SPLIT_H */

