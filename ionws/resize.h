/*
 * ion/ionws/resize.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONWS_RESIZE_H
#define ION_IONWS_RESIZE_H

#include <ioncore/common.h>
#include <ioncore/window.h>
#include <ioncore/resize.h>
#include <ioncore/genframe.h>
#include "split.h"

extern void resize_vert(WRegion *reg);
extern void resize_horiz(WRegion *reg);
extern void grow(WRegion *reg);
extern void shrink(WRegion *reg);

#endif /* ION_IONWS_RESIZE_H */
