/*
 * ion/ioncore/genframep.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GENFRAMEP_H
#define ION_IONCORE_GENFRAMEP_H

#include "genframe.h"

#define WGENFRAME_MIN_W(SCR) (CF_MIN_WIDTH*(SCR)->w_unit)
#define WGENFRAME_MIN_H(SCR) (CF_MIN_HEIGHT*(SCR)->h_unit)
#define WGENFRAME_WIN(FRAME) (((WGenFrame*)(FRAME))->win.win)
#define WGENFRAME_DRAW(FRAME) (((WGenFrame*)(FRAME))->win.draw)

#define REGION_LABEL(REG) ((REG)->mgr_data)

enum{
	WGENFRAME_AREA_NONE,
	WGENFRAME_AREA_BORDER,
	WGENFRAME_AREA_TAB,
	WGENFRAME_AREA_EMPTY_TAB,
	WGENFRAME_AREA_CLIENT
};

#endif /* ION_IONCORE_GENFRAMEP_H */
