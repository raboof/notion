/*
 * ion/resize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef ION_RESIZE_H
#define ION_RESIZE_H

#include <wmcore/common.h>
#include <wmcore/window.h>
#include <wmcore/resize.h>
#include "frame.h"
#include "split.h"

extern void resize_vert(WRegion *reg);
extern void resize_horiz(WRegion *reg);
extern void grow(WRegion *reg);
extern void shrink(WRegion *reg);
extern void maximize_vert(WFrame *frame);
extern void maximize_horiz(WFrame *frame);

#endif /* ION_RESIZE_H */
