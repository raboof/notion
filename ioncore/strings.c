/*
 * ion/ioncore/strings.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/output.h>
#include <libtu/misc.h>
#include <string.h>
#include <regex.h>

#ifdef CF_UTF8
#include <errno.h>
#include <iconv.h>
#ifdef CF_LIBUTF8
#include <libutf8.h>
#endif
#endif

#include "common.h"
#include "global.h"
#include "strings.h"


/*{{{ String scanning */


#ifdef CF_UTF8
static wchar_t str_wchar_at_utf8(char *p, int max)
{
	static iconv_t ic=(iconv_t)(-1);
	static bool iconv_tried=FALSE;
	size_t il, ol, n;
	wchar_t wc, *wcp=&wc;

	if(max<=0)
		return 0;
	
	if(!iconv_tried){
		iconv_tried=TRUE;
		ic=iconv_open(CF_ICONV_TARGET, CF_ICONV_SOURCE);
		if(ic==(iconv_t)(-1)){
			warn_err_obj("iconv_open(\"" CF_ICONV_TARGET "\", \""
						 CF_ICONV_SOURCE "\")");
		}
	}
	
	if(ic==(iconv_t)(-1))
		return ((*p&0x80)==0 ? (wchar_t)*p : 0);
	
	ol=sizeof(wchar_t);
	il=max;
	
	n=iconv(ic, &p, &il, (char**)&wcp, &ol);
	if(n==(size_t)(-1) && errno!=E2BIG){
		/*iconv_close(ic);
		iconv_tried=FALSE;*/
		warn_err_obj("iconv");
		return 0;
	}
	return wc;
}
#endif


wchar_t str_wchar_at(char *p, int max)
{
#ifdef CF_UTF8
	if(wglobal.utf8_mode)
		return str_wchar_at_utf8(p, max);
#endif
	return *p;
}

int str_prevoff(const char *p, int pos)
{
#ifdef CF_UTF8
	if(wglobal.utf8_mode){
		int opos=pos;
		
		while(pos>0){
			pos--;
			if((p[pos]&0xC0)!=0x80)
				break;
		}
		return opos-pos;
	}
#endif
	return (pos>0 ? 1 : 0);
}


int str_nextoff(const char *p)
{
#ifdef CF_UTF8
	if(wglobal.utf8_mode){
		int pos=0;
		
		while(p[pos]){
			pos++;
			if((p[pos]&0xC0)!=0x80)
				break;
		}
		return pos;
	}
#endif
	return (*p=='\0' ? 0 : 1);
}



/*}}}*/



/*{{{ Title shortening */


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

INTRSTRUCT(SR);
	
DECLSTRUCT(SR){
	regex_t re;
	char *rule;
	SR *next, *prev;
};


static SR *shortenrules=NULL;


/*EXTL_DOC
 * Add a rule describing how too long titles should be shortened to fit in tabs.
 * The regular expression \var{rx} (POSIX, not Lua!) is used to match titles
 * and when \var{rx} matches, \var{rule} is attempted to use as a replacement
 * for title. 
 *
 * Similarly to sed's 's' command, \var{rule} may contain characters that are
 * inserted in the resulting string and specials as follows:
 * 
 * \begin{tabularx}{\linewidth}{lX}
 *  \hline
 *  Special & Description \\
 *  \hline
 *  \$0 & 		 Place the original string here. \\
 *  \$1 to \$9 & Insert n:th capture here (as usual,captures are surrounded
 *				 by parentheses in the regex). \\
 *  \$| & 		 Alternative shortening separator. The shortening described
 *				 before the first this kind of separator is tried first and
 *				 if it fails to make the string short enough, the next is 
 *			 	 tried, and so on. \\
 *  \$< &		 Remove characters on the left of this marker to shorten the
 *				 string. \\
 *  \$> &		 Remove characters on the right of this marker to shorten the
 *				 string. Only the first \$< or \$> within an alternative 
 *				 shortening is used. \\
 * \end{tabularx}
 */
EXTL_EXPORT
bool add_shortenrule(const char *rx, const char *rule)
{
	SR *si;
	int ret;
	
	if(rx==NULL || rule==NULL)
		return FALSE;
	
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


static char *shorten(GrBrush *brush, const char *str, uint maxw,
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
	
	assert(rule!=NULL);
	
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
				rl=rl-i-1;
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
			if(grbrush_get_text_width(brush, s, strlen(s))<=maxw)
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
	/*warn("Failed to shorten title");*/
	return NULL;
}


char *make_label(GrBrush *brush, const char *str, uint maxw)
{
	size_t nmatch=10;
	regmatch_t pmatch[10];
	SR *rule;
	int ret;
	char *retstr;
	
	if(grbrush_get_text_width(brush, str, strlen(str))<=maxw)
		return scopy(str);
	
	for(rule=shortenrules; rule!=NULL; rule=rule->next){
		ret=regexec(&(rule->re), str, nmatch, pmatch, 0);
		if(ret!=0)
			continue;
		retstr=shorten(brush, str, maxw, rule->rule, nmatch, pmatch);
		goto rettest;
	}

	pmatch[0].rm_so=0;
	pmatch[0].rm_eo=strlen(str)-1;
	retstr=shorten(brush, str, maxw, "$1$<...", 1, pmatch);
	
rettest:
	if(retstr!=NULL)
		return retstr;
	return scopy("");
}


/*}}}*/
