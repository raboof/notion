/*
 * ion/ioncore/grdata.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_GRDATA_H
#define ION_IONCORE_GRDATA_H

#include "common.h"

INTRSTRUCT(WGRData);
INTRSTRUCT(WColorGroup);
INTRSTRUCT(WBorder);

#define BORDER_TL_TOTAL(BORDER) ((BORDER)->tl+(BORDER)->ipad)
#define BORDER_BR_TOTAL(BORDER) ((BORDER)->br+(BORDER)->ipad)
#define BORDER_TOTAL(BORDER) (BORDER_TL_TOTAL(BORDER)+BORDER_BR_TOTAL(BORDER))
#define BORDER_IX(BORDER, X) (X+BORDER_TL_TOTAL(BORDER))
#define BORDER_IY(BORDER, Y) (Y+BORDER_TL_TOTAL(BORDER))
#define BORDER_IW(BORDER, W) (W-BORDER_TL_TOTAL(BORDER)-BORDER_BR_TOTAL(BORDER))
#define BORDER_IH(BORDER, H) (H-BORDER_TL_TOTAL(BORDER)-BORDER_BR_TOTAL(BORDER))

DECLSTRUCT(WColorGroup){
	WColor bg, hl, sh, fg;
};


DECLSTRUCT(WBorder){
	int tl, br, ipad;
};


DECLSTRUCT(WGRData){
	/* configurable data */
	WColorGroup act_frame_colors, frame_colors;
	WColorGroup act_tab_colors, tab_colors;
	WColorGroup act_tab_sel_colors, tab_sel_colors;
	WColorGroup input_colors;
	WColor frame_bgcolor, selection_bgcolor, selection_fgcolor;
	bool transparent_background;
	
	WBorder frame_border;
	WBorder tab_border;
	WBorder input_border;

	WFontPtr font, tab_font;

	/* Ion-specific */
	bool bar_inside_frame;
	int spacing;
	
	/* PWM-specific */
	int pwm_tab_min_width;
	float pwm_bar_max_width_q;

	/* calculated data (from configurable) */
	int bar_h, tab_spacing;
	
	/* other data */
	GC gc;
	GC tab_gc;
	GC xor_gc;
	GC stipple_gc;
	GC copy_gc;
	Pixmap stick_pixmap;
	int stick_pixmap_w;
	int stick_pixmap_h;
	WColor black, white;
	
	Window moveres_win;
	WExtraDrawInfo *moveres_draw;
	WRectangle moveres_geom;
	
	Window drag_win;
	WExtraDrawInfo *drag_draw;
	WRectangle drag_geom;
};

#endif /* ION_IONCORE_GRDATA_H */
