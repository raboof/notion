/*
 * ion/de/fontset.c
 * 
 * Copyright (c) 2003 Tuomo Valkonen
 * 
 * Based on:
 *
 * Screen.cc for Blackbox - an X11 Window manager
 * Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
 * Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <ctype.h>
#include <locale.h>

#include <ioncore/common.h>
#include <ioncore/global.h>

#ifndef CF_FONT_ELEMENT_SIZE
#define CF_FONT_ELEMENT_SIZE 50
#endif


static const char *mystrcasestr(const char *str, const char *ptn) 
{
	const char *s2, *p2;
	for(; *str; str++) {
		for(s2=str,p2=ptn; ; s2++,p2++) {
			if (!*p2) return str;
			if (toupper(*s2) != toupper(*p2)) break;
		}
	}
	return NULL;
}


static const char *get_font_element(const char *pattern, char *buf,
									int bufsiz, ...) 
{
	const char *p, *v;
	char *p2;
	va_list va;
	
	va_start(va, bufsiz);
	buf[bufsiz-1] = 0;
	buf[bufsiz-2] = '*';
	while((v = va_arg(va, char *)) != NULL) {
		p = mystrcasestr(pattern, v);
		if (p) {
			strncpy(buf, p+1, bufsiz-2);
			p2 = strchr(buf, '-');
			if (p2) *p2=0;
			va_end(va);
			return p;
		}
	}
	va_end(va);
	strncpy(buf, "*", bufsiz);
	return NULL;
}


static const char *get_font_size(const char *pattern, int *size) 
{
	const char *p;
	const char *p2=NULL;
	int n=0;
	
	for (p=pattern; 1; p++) {
		if (!*p) {
			if (p2!=NULL && n>1 && n<72) {
				*size = n; return p2+1;
			} else {
				*size = 16; return NULL;
			}
		} else if (*p=='-') {
			if (n>1 && n<72 && p2!=NULL) {
				*size = n;
				return p2+1;
			}
			p2=p; n=0;
		} else if (*p>='0' && *p<='9' && p2!=NULL) {
			n *= 10;
			n += *p-'0';
		} else {
			p2=NULL; n=0;
		}
	}
}


XFontSet de_create_font_set(const char *fontname) 
{
	XFontSet fs;
	char **missing, *def = "-";
	int nmissing, pixel_size = 0;
	char weight[CF_FONT_ELEMENT_SIZE], slant[CF_FONT_ELEMENT_SIZE];
	const char *nfontname = fontname;
	char *pattern2 = NULL;

	fs = XCreateFontSet(wglobal.dpy, fontname, &missing, &nmissing, &def);
	
	if (fs && (! nmissing))
		return fs;
	
	if (! fs) {
		char *lcc=NULL;
		const char *lc;
		if (nmissing) 
			XFreeStringList(missing);
		
		lc=setlocale(LC_CTYPE, NULL);
		if(lc!=NULL && strcmp(lc, "POSIX")!=0 && strcmp(lc, "C")!=0)
			lcc=scopy(lc);
		
		setlocale(LC_CTYPE, "C");
		
		fs = XCreateFontSet(wglobal.dpy, fontname, &missing, &nmissing, &def);
		
		if(lcc!=NULL){
			setlocale(LC_CTYPE, lcc);
			free(lcc);
		}
	}
	
	if (fs) {
		XFontStruct **fontstructs;
		char **fontnames;
		XFontsOfFontSet(fs, &fontstructs, &fontnames);
		nfontname = fontnames[0];
	}
	
	get_font_element(nfontname, weight, CF_FONT_ELEMENT_SIZE,
					 "-medium-", "-bold-", "-demibold-", "-regular-", NULL);
	get_font_element(nfontname, slant, CF_FONT_ELEMENT_SIZE,
					 "-r-", "-i-", "-o-", "-ri-", "-ro-", NULL);
	get_font_size(nfontname, &pixel_size);
	
	if (! strcmp(weight, "*"))
		strncpy(weight, "medium", CF_FONT_ELEMENT_SIZE);
	if (! strcmp(slant, "*"))
		strncpy(slant, "r", CF_FONT_ELEMENT_SIZE);
	if (pixel_size < 3)
		pixel_size = 3;
	else if (pixel_size > 97)
		pixel_size = 97;
	
	libtu_asprintf(&pattern2,
				   "%s,"
				   "-*-*-%s-%s-*-*-%d-*-*-*-*-*-*-*,"
				   "-*-*-*-*-*-*-%d-*-*-*-*-*-*-*,*",
				   nfontname, weight, slant, pixel_size, pixel_size);
	
	if(pattern2==NULL)
		return NULL;
	
	nfontname = pattern2;
	
	if (nmissing)
		XFreeStringList(missing);
	if (fs)
		XFreeFontSet(wglobal.dpy, fs);
	
	fs = XCreateFontSet(wglobal.dpy, nfontname, &missing, &nmissing, &def);
	
	free(pattern2);
	
	return fs;
}
