/*
 * ion/de/brush.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_DE_BRUSH_H
#define ION_DE_BRUSH_H

#include <ioncore/common.h>
#include <ioncore/gr.h>
#include <ioncore/extl.h>


#define MATCHES(S, A) (gr_stylespec_score(S, A)>0)
#define MATCHES2(S, A1, A2) (gr_stylespec_score2(S, A1, A2)>0)


/*{{{ Classes and structures */


INTROBJ(DEBorder);
INTROBJ(DEBrush);
INTROBJ(DETabBrush);

#include "font.h"
#include "colour.h"


enum{
	DEBORDER_INLAID=0,    /* -\xxxxxx/- */
	DEBORDER_RIDGE,       /* /-\xxxx/-\ */
	DEBORDER_ELEVATED,    /* /-xxxxxx-\ */
	DEBORDER_GROOVE       /* \_/xxxx\_/ */
};


enum{
	DEALIGN_LEFT=0,
	DEALIGN_RIGHT=1,
	DEALIGN_CENTER=2
};


DECLSTRUCT(DEBorder){
	uint sh, hl, pad;
	uint style;
};


DECLOBJ(DEBrush){
	GrBrush grbrush;
	char *style;
	int usecount;
	bool is_fallback;
	
	WRootWin *rootwin;
	
	GC normal_gc;	
	
	DEBorder border;
	DEColourGroup cgrp;
	int n_extra_cgrps;
	DEColourGroup *extra_cgrps;
	GrTransparency transparency_mode;
	DEFontPtr font;
	int textalign;
	uint spacing;
	
	ExtlTab data_table;
	
	DEBrush *next, *prev;
};


DECLOBJ(DETabBrush){
	DEBrush debrush;
	
	GC stipple_gc;
	GC copy_gc;
	
	Pixmap tag_pixmap;
	int tag_pixmap_w;
	int tag_pixmap_h;
};


/*}}}*/


/*{{{ Initilisation/deinitialisation */


extern DEBrush *create_debrush(WRootWin *rootwin, const char *name);
extern DETabBrush *create_detabbrush(WRootWin *rootwin, const char *name);
extern DEBrush *de_find_or_create_brush(WRootWin *rootwin, const char *name);

extern DEBrush *de_get_brush(WRootWin *rootwin, Window win, 
							 const char *style);
extern bool de_get_brush_values(WRootWin *rootwin, const char *style,
								GrBorderWidths *bdw, GrFontExtents *fnte,
								ExtlTab *tab);

extern DEBrush *debrush_get_slave(DEBrush *brush, WRootWin *rootwin, 
								  Window win, const char *style);

extern void debrush_release(DEBrush *brush, Window win);

extern void debrush_deinit(DEBrush *brush);
extern void detabbrush_deinit(DETabBrush *brush);

extern void de_reset();
extern void de_deinit_brushes();


/*}}}*/


/*{{{ Drawing */


/* Borders */

extern void debrush_draw_border(DEBrush *brush, Window win, 
								const WRectangle *geom,
								const char *attrib);

extern void debrush_get_border_widths(DEBrush *brush, GrBorderWidths *bdw);


/* Textboxes */

extern void debrush_draw_textbox(DEBrush *brush, Window win, 
    	    	    	    	 const WRectangle *geom,
								 const char *text, 
								 const char *attr,
    	    	    	    	 bool needfill);

extern void detabbrush_draw_textbox(DETabBrush *brush, Window win, 
									const WRectangle *geom,
									const char *text, 
									const char *attr,
									bool needfill);

extern void debrush_draw_textboxes(DEBrush *brush, Window win, 
    	    	    	    	   const WRectangle *geom, int n,
								   const GrTextElem *elem, bool needfill,
								   const char *common_attrib);


extern void detabbrush_draw_textboxes(DETabBrush *brush, Window win, 
									  const WRectangle *geom, int n,
									  const GrTextElem *elem, bool needfill, 
									  const char *common_attrib);

/* Misc */

extern void debrush_set_clipping_rectangle(DEBrush *brush, Window win,
    	    	    	    	    	   const WRectangle *geom);
extern void debrush_clear_clipping_rectangle(DEBrush *brush, Window win);

extern void debrush_set_window_shape(DEBrush *brush, Window win, bool rough,
									 int n, const WRectangle *rects);

extern void debrush_enable_transparency(DEBrush *brush, Window win, 
										GrTransparency mode);

extern void debrush_fill_area(DEBrush *brush, Window win, 
							  const WRectangle *geom, const char *attr);
extern void debrush_clear_area(DEBrush *brush, Window win, 
							   const WRectangle *geom);


/*}}}*/


/*{{{ Misc. */


DEColourGroup *debrush_get_colour_group2(DEBrush *brush, 
										 const char *attr_p1,
										 const char *attr_p2);

DEColourGroup *debrush_get_colour_group(DEBrush *brush, const char *attr);

extern void debrush_get_extra_values(DEBrush *brush, ExtlTab *tab);

/*}}}*/


#endif /* ION_DE_BRUSH_H */
