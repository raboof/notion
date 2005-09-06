/*
 * ion/mod_ionws/split-stdisp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_IONWS_SPLIT_STDISP_H
#define ION_MOD_IONWS_SPLIT_STDISP_H

#include "split.h"

extern bool split_try_sink_stdisp(WSplitSplit *node, bool iterate, bool force);
extern bool split_try_unsink_stdisp(WSplitSplit *node, bool iterate, bool force);
extern bool split_regularise_stdisp(WSplitST *stdisp);

extern int stdisp_recommended_w(WSplitST *stdisp);
extern int stdisp_recommended_h(WSplitST *stdisp);

#endif /* ION_MOD_IONWS_SPLIT_STDISP_H */
