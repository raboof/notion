/*
 * wmcore/font.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <libtu/output.h>
#include <libtu/misc.h>
#include <string.h>
#include "font.h"
#include "common.h"


XFontStruct *load_font(Display *dpy, const char *fontname)
{
	XFontStruct *xfnt;
	
	xfnt=XLoadQueryFont(dpy, fontname);
	
	if(xfnt==NULL){
		warn("Could not load font \"%s\", trying \"%s\"",
			 fontname, CF_FALLBACK_FONT_NAME);
		xfnt=XLoadQueryFont(dpy, CF_FALLBACK_FONT_NAME);
		if(xfnt==NULL){
			warn("Failed loading fallback font.");
			return FALSE;
		}
	}
	
	return xfnt;
}


static char *scatn3(const char *p1, int l1,
					const char *p2, int l2,
					const char *p3, int l3)
{
	char *p=ALLOC_N(char, l1+l2+l3+1);
	
	if(p==NULL){
		warn_err();
	}else{
		strncat(p, p1, l1);
		strncat(p, p2, l2);
		strncat(p, p3, l3);
	}
	return p;
}


char *make_label(XFontStruct *fnt, const char *str, const char *trailer,
				 int maxw, int *wret)
{
	static const char *dots="...";
	const char *b;
	size_t len, tlen;
	int w, bw, dw,  tw;
	
	len=strlen(str);
	tlen=strlen(trailer);
	
	w=XTextWidth(fnt, str, len);
	tw=XTextWidth(fnt, trailer, tlen);
	
	if(tw+w<=maxw){
		if(wret!=NULL)
			*wret=tw+w;
		return scat(str, trailer);
	}
	
	b=strchr(str, ':');
	
	if(b!=NULL){
		b++;
		b+=strspn(b, " ");
		bw=XTextWidth(fnt, str, b-str);
		
		if(tw+w-bw<=maxw){
			if(wret!=NULL)
				*wret=tw+w-bw;
			return scat(b, trailer);
		}
		
		len-=(b-str);
		str=b;
		w=bw;
	}

	dw=XTextWidth(fnt, dots, 3);
	
	while(--len){
		w=XTextWidth(fnt, str, len);
		if(tw+w+dw<=maxw)
			break;
	}
	
	if(wret!=NULL)
		*wret=tw+w+dw;
	
	
	return scatn3(str, len, dots, 3, trailer, tlen);
}

