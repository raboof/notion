/*
 * ion/ioncore/framep.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_FRAMEP_H
#define ION_IONCORE_FRAMEP_H

#include "frame.h"

#define WFRAME_WIN(FRAME) (((WFrame*)(FRAME))->mplex.win.win)
#define WFRAME_DRAW(FRAME) (((WFrame*)(FRAME))->mplex.win.draw)
#define WFRAME_MCOUNT(FRAME) (((WFrame*)(FRAME))->mplex.managed_count)
#define WFRAME_MLIST(FRAME) (((WFrame*)(FRAME))->mplex.managed_list)
#define WFRAME_CURRENT(FRAME) (((WFrame*)(FRAME))->mplex.current_sub)

enum{
	WFRAME_AREA_NONE=0,
	WFRAME_AREA_BORDER=1,
	WFRAME_AREA_TAB=2,
	WFRAME_AREA_CLIENT=3
};

#endif /* ION_IONCORE_FRAMEP_H */
