/*
 * ion/de/fontset.c
 * 
 * This file contains routines to attempt to add fonts to a font pattern
 * so that XCreateFontSet will not fail because the given font(s) do not
 * contain all the characters required by the locale. The original code
 * was apparently written by Tomohiro Kubota; see
 * <http://www.debian.org/doc/manuals/intro-i18n/ch-examples.en.html#s13.4.5>.
 * 
 */

#include <string.h>
#include <ctype.h>
#include <locale.h>

#include <ioncore/common.h>
#include <ioncore/global.h>

#ifndef CF_FONT_ELEMENT_SIZE
#define CF_FONT_ELEMENT_SIZE 50
#endif

#define FNT_D(X) /*X*/

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
	
    FNT_D(fprintf(stderr, "FNTRQ: %s\n", fontname));
	
	fs = XCreateFontSet(ioncore_g.dpy, fontname, &missing, &nmissing, &def);
	
	if (fs && (! nmissing))
		return fs;
	
	/* Not a warning, nothing serious */
	FNT_D(fprintf(stderr, "Failed to load fonset.\n"));
	
	if (! fs) {
		char *lcc=NULL;
		const char *lc;
		if (nmissing) 
			XFreeStringList(missing);
		
		lc=setlocale(LC_CTYPE, NULL);
		if(lc!=NULL && strcmp(lc, "POSIX")!=0 && strcmp(lc, "C")!=0)
			lcc=scopy(lc);
		
		setlocale(LC_CTYPE, "C");
		
		fs = XCreateFontSet(ioncore_g.dpy, fontname, &missing, &nmissing, &def);
		
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

    /*if(fs){
		XFontStruct **fontstructs;
		char **fontnames;
		int i, n=XFontsOfFontSet(fs, &fontstructs, &fontnames);
		for(i=0; fontnames[i] && i<n; i++)
			fprintf(stderr, "%s\n", fontnames[i]);
	}*/
	
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
				   fontname, weight, slant, pixel_size, pixel_size);
	
	if(pattern2==NULL)
		return NULL;
	
	FNT_D(fprintf(stderr, "NRQ: %s\n", pattern2));
	
	nfontname = pattern2;
	
	if (nmissing)
		XFreeStringList(missing);
	if (fs)
		XFreeFontSet(ioncore_g.dpy, fs);

	FNT_D(if(fs) fprintf(stderr, "Trying '%s'.\n", nfontname));
	
	fs = XCreateFontSet(ioncore_g.dpy, nfontname, &missing, &nmissing, &def);
	
	free(pattern2);
	
    /*if(fs){
		XFontStruct **fontstructs;
		char **fontnames;
		int i, n=XFontsOfFontSet(fs, &fontstructs, &fontnames);
		for(i=0; fontnames[i] && i<n; i++)
			fprintf(stderr, "%s\n", fontnames[i]);
	}*/


	return fs;
}
