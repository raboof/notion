/*
 * ion/workspace.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef ION_WORKSPACE_H
#define ION_WORKSPACE_H

#include <wmcore/common.h>

INTROBJ(WWorkspace)

#include <wmcore/region.h>
#include <wmcore/window.h>
#include <wmcore/screen.h>
#include <wmcore/viewport.h>

#include "split.h"


DECLOBJ(WWorkspace){
	WRegion region;
	WObj *splitree;
};


extern WWorkspace *create_workspace(WScreen *scr, WWinGeomParams params,
									const char *name, bool ci);

extern void workspace_add_sub(WWorkspace *ws, WRegion *reg);
extern void workspace_remove_sub(WWorkspace *ws, WRegion *reg);

extern bool remove_split(WWorkspace *ws, WWsSplit *split);
extern WRegion *workspace_find_current(WWorkspace *ws);

extern void init_workspaces(WViewport *vp);
extern void setup_screens();

extern WWorkspace *lookup_workspace(const char *name);
extern int complete_workspace(char *nam, char ***cp_ret, char **beg,
							  void *unused);

extern bool switch_workspace_name(const char *str);

extern WWorkspace *create_new_workspace_on_vp(WViewport *vp, const char *name);

extern void workspace_goto_above(WWorkspace *ws);
extern void workspace_goto_below(WWorkspace *ws);
extern void workspace_goto_left(WWorkspace *ws);
extern void workspace_goto_right(WWorkspace *ws);

#endif /* ION_WORKSPACEGEN_H */
