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
#include "ionframe.h"

extern void ionframe_begin_resize(WIonFrame *frame);
extern void ionframe_grow_vert(WIonFrame *frame);
extern void ionframe_shrink_vert(WIonFrame *frame);
extern void ionframe_grow_horiz(WIonFrame *frame);
extern void ionframe_shrink_horiz(WIonFrame *frame);
extern void ionframe_end_resize(WIonFrame *frame);
extern void ionframe_cancel_resize(WIonFrame *frame);

#endif /* ION_IONWS_RESIZE_H */
