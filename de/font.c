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
#include "fontset.h"
#include "brush.h"


/*{{{ Load/free */


static DEFont *fonts=NULL;


DEFont *de_load_font(const char *fontname)
{
	DEFont *fnt;
	XFontSet fontset=NULL;
	XFontStruct *fontstruct=NULL;
	
	if(fontname==NULL){
		warn("Attempt to load NULL as font");
		return NULL;
	}
	
	/* There shouldn't be that many fonts... */
	for(fnt=fonts; fnt!=NULL; fnt=fnt->next){
		if(strcmp(fnt->pattern, fontname)==0){
			fnt->refcount++;
			return fnt;
		}
	}

	if(wglobal.utf8_mode){
		fontset=de_create_font_set(fontname);
		if(fontset!=NULL){
			if(XContextDependentDrawing(fontset)){
				warn("Fontset for font pattern '%s' implements context "
					 "dependent drawing, which is unsupported. Expect "
					 "clutter.", fontname);
			}
		}
	}else{
		fontstruct=XLoadQueryFont(wglobal.dpy, fontname);
	}

	if(fontstruct==NULL && fontset==NULL){
		if(strcmp(fontname, CF_FALLBACK_FONT_NAME)!=0){
			warn("Could not load font \"%s\", trying \"%s\"",
				 fontname, CF_FALLBACK_FONT_NAME);
			return de_load_font(CF_FALLBACK_FONT_NAME);
		}
		return NULL;
	}
	
	fnt=ALLOC(DEFont);
	
	if(fnt==NULL){
		warn_err();
		return NULL;
	}
	
	fnt->fontset=fontset;
	fnt->fontstruct=fontstruct;
	fnt->pattern=scopy(fontname);
	fnt->next=NULL;
	fnt->prev=NULL;
	fnt->refcount=1;

	LINK_ITEM(fonts, fnt, next, prev);
	
	return fnt;
}


void de_free_font(DEFont *font)
{
	if(--font->refcount!=0)
		return;
	
	if(font->fontset!=NULL)
		XFreeFontSet(wglobal.dpy, font->fontset);
	if(font->fontstruct!=NULL)
		XFreeFont(wglobal.dpy, font->fontstruct);
	if(font->pattern!=NULL)
		free(font->pattern);
	
	UNLINK_ITEM(fonts, font, next, prev);
	free(font);
}


/*}}}*/


/*{{{ Lengths */


void debrush_get_font_extents(DEBrush *brush, GrFontExtents *fnte)
{
	if(brush->font==NULL)
		return;
	
	if(brush->font->fontset!=NULL){
		XFontSetExtents *ext=XExtentsOfFontSet(brush->font->fontset);
		if(ext==NULL)
			goto fail;
		fnte->max_height=ext->max_logical_extent.height;
		fnte->max_width=ext->max_logical_extent.width;
		fnte->baseline=-ext->max_logical_extent.y;
		return;
	}else if(brush->font->fontstruct!=NULL){
		XFontStruct *fnt=brush->font->fontstruct;
        fnte->max_height=fnt->ascent+fnt->descent;
		fnte->max_width=fnt->max_bounds.width;
		fnte->baseline=fnt->ascent;
		return;
	}

fail:	
	fnte->max_height=0;
	fnte->max_width=0;
	fnte->baseline=0;
}


uint debrush_get_text_width(DEBrush *brush, const char *text, uint len)
{
	if(brush->font==NULL)
		return 0;
	
	if(brush->font->fontset!=NULL){
		XRectangle lext;
		Xutf8TextExtents(brush->font->fontset, text, len, NULL, &lext);
		return lext.width;
	}else if(brush->font->fontstruct!=NULL){
		return XTextWidth(brush->font->fontstruct, text, len);
	}else{
		return 0;
	}
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
		if(brush->font->fontset!=NULL){
			Xutf8DrawString(wglobal.dpy, win, brush->font->fontset,
							gc, x, y, str, len);
		}else 
#endif
		if(brush->font->fontstruct!=NULL){
			XDrawString(wglobal.dpy, win, gc, x, y, str, len);
		}
	}else{
		XSetBackground(wglobal.dpy, gc, colours->bg);
#ifdef CF_UTF8		
		if(brush->font->fontset!=NULL){
			Xutf8DrawImageString(wglobal.dpy, win, brush->font->fontset,
								 gc, x, y, str, len);
		}else 
#endif			
		if(brush->font->fontstruct!=NULL){
			XDrawImageString(wglobal.dpy, win, gc, x, y, str, len);
		}
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

