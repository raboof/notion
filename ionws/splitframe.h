/*
 * ion/splitframe.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_SPLITFRAME_H
#define ION_SPLITFRAME_H

#include <wmcore/common.h>
#include "split.h"
#include "frame.h"
#include "workspace.h"

extern WFrame *find_frame_at(WWorkspace *ws, int x, int y);

extern void split(WRegion *reg, const char *str);
extern void split_empty(WRegion *reg, const char *str);
extern void split_top(WWorkspace *ws, const char *str);
extern void frame_close(WFrame *frame);
extern void frame_close_if_empty(WFrame *frame);

#endif /* ION_SPLITFRAME_H */

