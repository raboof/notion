/*
 * ion/mod_tiling/split-stdisp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_TILING_SPLIT_STDISP_H
#define ION_MOD_TILING_SPLIT_STDISP_H

#include "split.h"

extern bool split_try_sink_stdisp(WSplitSplit *node, bool iterate, bool force);
extern bool split_try_unsink_stdisp(WSplitSplit *node, bool iterate, bool force);
extern bool split_regularise_stdisp(WSplitST *stdisp);

extern int stdisp_recommended_w(WSplitST *stdisp);
extern int stdisp_recommended_h(WSplitST *stdisp);

#endif /* ION_MOD_TILING_SPLIT_STDISP_H */
