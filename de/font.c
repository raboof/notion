/*
 * ion/de/font.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#include <ioncore/common.h>
#include "font.h"
#include "brush.h"


/*{{{ Load/free */


DEFontPtr de_load_font(const char *fontname)
{
	DEFontPtr xfnt;
	
#ifdef CF_UTF8
	char **dummy_missing;
	int dummy_missing_n;
	char *dummy_def;
	
	if(fontname==NULL){
		warn("Attempt to load NULL as font");
		return 0;
	}
	
	xfnt=XCreateFontSet(wglobal.dpy, fontname, &dummy_missing,
						&dummy_missing_n, &dummy_def);
#else
	xfnt=XLoadQueryFont(wglobal.dpy, fontname);
#endif
	
	if(xfnt==NULL && strcmp(fontname, CF_FALLBACK_FONT_NAME)!=0){
		warn("Could not load font \"%s\", trying \"%s\"",
			 fontname, CF_FALLBACK_FONT_NAME);
#ifdef CF_UTF8
		xfnt=XCreateFontSet(wglobal.dpy, CF_FALLBACK_FONT_NAME, &dummy_missing,
							&dummy_missing_n, &dummy_def);
#else
		xfnt=XLoadQueryFont(wglobal.dpy, CF_FALLBACK_FONT_NAME);
#endif
		if(xfnt==NULL){
			warn("Failed loading fallback font.");
			return 0;
		}
	}
	
	return xfnt;
}


void de_free_font(DEFontPtr font)
{
#ifdef CF_UTF8
	XFreeFontSet(wglobal.dpy, font);
#else
	XFreeFont(wglobal.dpy, font);
#endif
}


/*}}}*/


/*{{{ Lengths */


void debrush_get_font_extents(DEBrush *brush, GrFontExtents *fnte)
{
	if(brush->font==NULL)
		goto fail;

#ifdef CF_UTF8
	{
		XFontSetExtents *ext=XExtentsOfFontSet(brush->font);
		if(ext==NULL)
			goto fail;
		fnte->max_height=ext->max_logical_extent.height;
		fnte->max_width=ext->max_logical_extent.width;
		fnte->baseline=-ext->max_logical_extent.y;
	}
#else
	{
		DEFontPtr fnt=brush->font;
		fnte->max_height=fnt->ascent+fnt->descent;
		fnte->max_width=fnt->max_bounds.width;
		fnte->baseline=fnt->ascent;
	}
#endif
    return;

fail:	
	fnte->max_height=0;
	fnte->max_width=0;
	fnte->baseline=0;
}


uint debrush_get_text_width(DEBrush *brush, const char *text, uint len)
{
	if(brush->font==NULL)
		return 0;
#ifdef CF_UTF8
	{
		XRectangle lext;
		Xutf8TextExtents(brush->font, text, len, NULL, &lext);
		return lext.width;
	}
#else
	return XTextWidth(brush->font, text, len);
#endif
}


/*}}}*/


/*{{{ String drawing */


void debrush_do_draw_string(DEBrush *brush, Window win, int x, int y,
							const char *str, int len, bool needfill, 
							DEColourGroup *colours)
{
	GC gc=brush->normal_gc;
	
	if(brush->font==NULL)
		return;
	
	XSetForeground(wglobal.dpy, gc, colours->fg);
	if(!needfill){
#ifdef CF_UTF8
		Xutf8DrawString(wglobal.dpy, win, brush->font, gc, x, y, str, len);
#else
		XDrawString(wglobal.dpy, win, gc, x, y, str, len);
#endif
	}else{
		XSetBackground(wglobal.dpy, gc, colours->bg);
#ifdef CF_UTF8
		Xutf8DrawImageString(wglobal.dpy, win, brush->font, gc, 
							 x, y, str, len);
#else
		XDrawImageString(wglobal.dpy, win, gc, x, y, str, len);
#endif
	}
}


void debrush_draw_string(DEBrush *brush, Window win, int x, int y,
						 const char *str, int len, bool needfill,
						 const char *attrib)
{
	DEColourGroup *cg=debrush_get_colour_group(brush, attrib);
	if(cg!=NULL)
		debrush_do_draw_string(brush, win, x, y, str, len, needfill, cg);
}


/*}}}*/

