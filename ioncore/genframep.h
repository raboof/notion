/*
 * ion/ioncore/genframep.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_IONCORE_GENFRAMEP_H
#define ION_IONCORE_GENFRAMEP_H

#include "genframe.h"

#define WGENFRAME_WIN(FRAME) (((WGenFrame*)(FRAME))->win.win)
#define WGENFRAME_DRAW(FRAME) (((WGenFrame*)(FRAME))->win.draw)

#define REGION_LABEL(REG) ((REG)->mgr_data.p)

enum{
	WGENFRAME_AREA_NONE=0,
	WGENFRAME_AREA_BORDER=1,
	WGENFRAME_AREA_TAB=2,
	WGENFRAME_AREA_EMPTY_TAB=3,
	WGENFRAME_AREA_CLIENT=4
};

#endif /* ION_IONCORE_GENFRAMEP_H */
