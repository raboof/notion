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

#ifndef ION_IONCORE_GENFRAMEP_H
#define ION_IONCORE_GENFRAMEP_H

#include "frame.h"

#define WGENFRAME_WIN(FRAME) (((WFrame*)(FRAME))->mplex.win.win)
#define WGENFRAME_DRAW(FRAME) (((WFrame*)(FRAME))->mplex.win.draw)
#define WGENFRAME_MCOUNT(FRAME) (((WFrame*)(FRAME))->mplex.managed_count)
#define WGENFRAME_MLIST(FRAME) (((WFrame*)(FRAME))->mplex.managed_list)
#define WGENFRAME_CURRENT(FRAME) (((WFrame*)(FRAME))->mplex.current_sub)

enum{
	WGENFRAME_AREA_NONE=0,
	WGENFRAME_AREA_BORDER=1,
	WGENFRAME_AREA_TAB=2,
	WGENFRAME_AREA_CLIENT=3
};

#endif /* ION_IONCORE_GENFRAMEP_H */
