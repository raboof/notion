/*
 * wmcore/font.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <libtu/output.h>
#include <libtu/misc.h>
#include <string.h>
#include <regex.h>
#include "common.h"
#include "font.h"
#ifdef CF_XFT
#include "global.h"
#endif

#ifdef CF_UTF8
#include <unicode.h>
#endif


int str_prevoff(const char *p, int pos)
{
#ifdef CF_UTF8
	const char *prev;

	while(pos>0){
		/* unicode_previous_utf8 returns NULL if there's no valid
		 * utf8 character at p+len... lame.
		 */
		prev=unicode_previous_utf8(p, p+pos);
		if(prev!=NULL)
			return p+pos-prev;
		pos--;
	}
	return 0;
#else
	return (pos>0 ? 1 : 0);
#endif
}


int str_nextoff(const char *p)
{
#ifdef CF_UTF8
	char *next;
	next=unicode_next_utf8(p);
	if(next==NULL)
		return 0;
	return next-p;
#else
	return (*p=='\0' ? 0 : 1);
#endif
}


#ifdef CF_XFT

static WFontPtr xft_load_pattern(Display *dpy, XftPattern *pattern)
{
	WFontPtr xfnt;
	XftPattern *match;
	XftResult result;
	
	match=XftFontMatch(dpy, DefaultScreen(dpy), pattern, &result);
	
	xfnt=XftFontOpenPattern(dpy, match);
	
	if(!xfnt)
		XftPatternDestroy(match);
	
	return xfnt;
}

static WFontPtr xft_load_font(Display *dpy, const char *fontname)
{
	WFontPtr xfnt=NULL;
	XftPattern *pattern;
	bool use_xft=FALSE;
	
	if(strlen(fontname)>4 && !strncmp("xft:", fontname, 4)){
		use_xft=TRUE;
		fontname+=4;
	}
	
	if((pattern=XftXlfdParse(fontname, False, False))){
		if(!use_xft)
			XftPatternAddBool(pattern, XFT_CORE, True);
		xfnt=xft_load_pattern(dpy, pattern);
		XftPatternDestroy(pattern);
	}
	
	if(!xfnt){
		if((pattern=XftNameParse(fontname))){
			if(!use_xft)
				XftPatternAddBool(pattern, XFT_CORE, True);
			xfnt=xft_load_pattern(dpy, pattern);
			XftPatternDestroy(pattern);
		}
	}
	
	return xfnt;
}

#endif

WFontPtr load_font(Display *dpy, const char *fontname)
{
	WFontPtr xfnt;
#ifdef CF_XFT
	xfnt=xft_load_font(dpy, fontname);
#else
#ifdef CF_UTF8
	char **dummy_missing;
	int dummy_missing_n;
	char *dummy_def;
	/*if(!XSupportsLocale())
		warn("Locale unsupported\n");*/
	
	xfnt=XCreateFontSet(dpy, fontname, &dummy_missing,
						&dummy_missing_n, &dummy_def);
#else
	xfnt=XLoadQueryFont(dpy, fontname);
#endif
#endif
	
	if(xfnt==NULL){
		warn("Could not load font \"%s\", trying \"%s\"",
			 fontname, CF_FALLBACK_FONT_NAME);
#ifdef CF_XFT
		xfnt=xft_load_font(dpy, CF_FALLBACK_FONT_NAME);
#else
#ifdef CF_UTF8
		xfnt=XCreateFontSet(dpy, CF_FALLBACK_FONT_NAME, &dummy_missing,
							&dummy_missing_n, &dummy_def);
#else
		xfnt=XLoadQueryFont(dpy, CF_FALLBACK_FONT_NAME);
#endif
#endif
		if(xfnt==NULL){
			warn("Failed loading fallback font.");
			return FALSE;
		}
	}
	
	return xfnt;
}


#ifdef CF_XFT


int text_width(WFontPtr font, const char *str, int len)
{
    XGlyphInfo extents;
#ifdef CF_UTF8
    XftTextExtentsUtf8(wglobal.dpy, font, (XftChar8 *) str, len, &extents);
#else
    XftTextExtents8(wglobal.dpy, font, (XftChar8 *) str, len, &extents);
#endif
    return extents.xOff;
}

#else

#ifdef CF_UTF8

int fontset_height(XFontSet fntset)
{
	XFontSetExtents *ext=XExtentsOfFontSet(fntset);
	if(ext==NULL)
		return 1;
	return ext->max_logical_extent.height;
}

int fontset_max_width(XFontSet fntset)
{
	XFontSetExtents *ext=XExtentsOfFontSet(fntset);
	if(ext==NULL)
		return 1;
	return ext->max_logical_extent.width;
}

int fontset_baseline(XFontSet fntset)
{
	XFontSetExtents *ext=XExtentsOfFontSet(fntset);
	if(ext==NULL)
		return 1;
	return -ext->max_logical_extent.y;
}

int fontset_text_width(XFontSet fntset, const char *str, int len)
{
	XRectangle lext;
	Xutf8TextExtents(fntset, str, len, NULL, &lext);
	return lext.width;
}

#endif

#endif


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



/*{{{ Title shortening */

INTRSTRUCT(SR)
	
	DECLSTRUCT(SR){
		regex_t re;
		char *rule;
		SR *next, *prev;
	};


static SR *shortenrules=NULL;


bool add_shortenrule(const char *rx, const char *rule)
{
	SR *si;
	int ret;
	
	si=ALLOC(SR);
	
	if(si==NULL)
		return FALSE;
	
	ret=regcomp(&(si->re), rx, REG_EXTENDED);
	
	if(ret!=0){
		warn("Error compiling regular_expression");
		goto fail2;
	}
	
	si->rule=scopy(rule);
	
	if(si->rule==NULL)
		goto fail;
	
	LINK_ITEM(shortenrules, si, next, prev);
	
	return TRUE;
	
	fail:
	warn_err();
	regfree(&(si->re));
	fail2:
	free(si);
	return FALSE;
}


static char *shorten(WFontPtr fnt, const char *str, int maxw,
					 const char *rule, int nmatch, regmatch_t *pmatch)
{
	char *s;
	int rl, l, i, j, k, ll;
	int strippt=0;
	int stripdir=-1;
	bool more=FALSE;
	
	/* Stupid alloc rule that wastes space */
	
	rl=strlen(rule);
	l=rl;
	
	for(i=0; i<nmatch; i++){
		if(pmatch[i].rm_so==-1)
			continue;
		l+=(pmatch[i].rm_eo-pmatch[i].rm_so);
	}
	
	s=ALLOC_N(char, l);
	
	if(s==NULL){
		warn_err();
		return NULL;
	}
	
	do{
		more=FALSE;
		j=0;
		
		for(i=0; i<rl; i++){
			if(rule[i]!='$'){
				s[j++]=rule[i];
				continue;
			}
			
			i++;
			
			if(rule[i]=='|'){
				rule=rule+i+1;
				more=TRUE;
				break;
			}
			
			if(rule[i]=='$'){
				s[j++]='$';
				continue;
			}
			
			if(rule[i]=='<'){
				strippt=j;
				stripdir=-1;
				continue;
			}
			
			if(rule[i]=='>'){
				strippt=j;
				stripdir=1;
				continue;
			}
			
			if(rule[i]>='0' && rule[i]<='9'){
				k=(int)(rule[i]-'0');
				if(k>=nmatch)
					continue;
				if(pmatch[k].rm_so==-1)
					continue;
				ll=(pmatch[k].rm_eo-pmatch[k].rm_so);
				strncpy(s+j, str+pmatch[k].rm_so, ll);
				j+=ll;
			}
		}
		
		s[j]='\0';
		
		while(1){
			if(text_width(fnt, s, strlen(s))<=maxw)
				return s;
			if(stripdir==-1){
				if(strippt==0)
					break;
				ll=str_prevoff(s, strippt);
				if(ll!=0){
					strcpy(s+strippt-ll, s+strippt);
					strippt-=ll;
				}
			}else{
				if(s[strippt]=='\0')
					break;
				ll=str_nextoff(s+strippt);
				strcpy(s+strippt, s+strippt+ll);
			}
		}
	}while(more);
		
	free(s);
	warn("Failed to shorten title");
	return NULL;
}


char *make_label(WFontPtr fnt, const char *str, int maxw)
{
	size_t nmatch=10;
	regmatch_t pmatch[10];
	SR *rule;
	int ret;
	char *retstr;
	
	if(text_width(fnt, str, strlen(str))<=maxw)
		return scopy(str);
	
	for(rule=shortenrules; rule!=NULL; rule=rule->next){
		ret=regexec(&(rule->re), str, nmatch, pmatch, 0);
		if(ret!=0)
			continue;
		retstr=shorten(fnt, str, maxw, rule->rule, nmatch, pmatch);
		goto rettest;
	}

	pmatch[0].rm_so=0;
	pmatch[0].rm_eo=strlen(str)-1;
	retstr=shorten(fnt, str, maxw, "$1$<...", 1, pmatch);
	
rettest:
	if(retstr!=NULL)
		return retstr;
	return scopy("");
}


/*}}}*/
