/*
 * ion/ioncore/names.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */


#include <string.h>
#include <limits.h>

#include "common.h"
#include "region.h"
#include "names.h"
#include "font.h"
#include "grdata.h"


/*{{{ Name setup */


#define NAMEDNUM_TMPL "<%d>"


/* List of named regions */
static WRegion *region_list=NULL;


/*EXTL_DOC
 * Returns the instance number for \var{reg} of its name.
 */
EXTL_EXPORT
int region_name_instance(WRegion *reg)
{
	return reg->ni.instance;
}


/*EXTL_DOC
 * Returns the short name for \var{reg} without instance number.
 */
EXTL_EXPORT
const char *region_name(WRegion *reg)
{
	return reg->ni.name;
}


/*EXTL_DOC
 * Returns the full name for \var{reg} with instance number in angle
 * brackets after the short name.
 */
EXTL_EXPORT
char *region_full_name(WRegion *reg)
{
	const char *str=region_name(reg);
	char tmp[16];
	
	if(str==NULL)
		return NULL;
	
	if(reg->ni.instance!=0 || str[strlen(str)-1]=='>'){
		sprintf(tmp, NAMEDNUM_TMPL, reg->ni.instance);
		return scat(str, tmp);
	}else{
		return scopy(str);
	}
}


char *region_make_label(WRegion *reg, int maxw, WFontPtr font)
{
	char *str=region_full_name(reg);
	char *ret;
	
	if(str==NULL)
		return NULL;
	
	ret=make_label(font, str, maxw);
	
	free(str);
	
	return ret;
}


static bool region_use_name(WRegion *reg, const char *namep, int instrq)
{
	WRegion *p, *np, *minp=NULL;
	int mininst=INT_MAX;
	const char *str;
	char *name;
	
	name=scopy(namep);
	/* TODO: make this understand UTF8. Stripws in libtu currently only
	 * checks for the ASCII space character.
	 */
	stripws(name);
	
	if(name==NULL){
		warn_err();
		return FALSE;
	}
	
	region_unuse_name(reg);
	reg->ni.name=name;
	
	for(p=region_list; p!=NULL; p=p->ni.g_next){
		if(p==reg)
			continue;
		str=region_name(p);
		if(strcmp(str, name)==0)
			break;
	}
	
	if(p!=NULL && instrq>=0){
		minp=p;
		do{
			if(minp->ni.instance==instrq){
				instrq=-1;
				minp=NULL;
				break;
			}
			minp=minp->ni.n_next;
		}while(minp!=p);
		
		if(instrq!=-1)
			mininst=instrq;
	}

	if(p!=NULL && minp==NULL){
		for(; p!=minp; p=np){
			assert(p!=reg);
			np=p->ni.n_next;
			if(p->ni.instance+1==np->ni.instance){
				/* There are no free instances between p and np. */
				continue;
			}else if(p->ni.instance>=np->ni.instance && np->ni.instance!=0){
				/* p has greatest instance number and np's instance number
				 * is greater than zero so zero is free.
				 */
				mininst=0;
				minp=p;
			}else if(p->ni.instance+1<mininst){
				/* The instance number following p is free and smallest
				 * found so far.
				 */
				mininst=p->ni.instance+1;
				minp=p;
				continue;
			}
			/* else the instance number following p is free but greater
			 * than what has been found so far 
			 */
		}
	}
	
	if(p!=NULL){
		assert(minp!=NULL);
	
		np=minp->ni.n_next;
		reg->ni.n_next=np;
		np->ni.n_prev=reg;
		minp->ni.n_next=reg;
		reg->ni.n_prev=minp;
		reg->ni.instance=mininst;
	}else if(instrq>=0){
		reg->ni.instance=instrq;
	}

	LINK_ITEM(region_list, reg, ni.g_next, ni.g_prev);
	
	return TRUE;
}


void region_unuse_name(WRegion *reg)
{
	reg->ni.n_next->ni.n_prev=reg->ni.n_prev;
	reg->ni.n_prev->ni.n_next=reg->ni.n_next;
	reg->ni.n_next=reg;
	reg->ni.n_prev=reg;
	reg->ni.instance=0;
	if(reg->ni.name!=NULL){
		free(reg->ni.name);
		reg->ni.name=NULL;
	}
	
	if(reg->ni.g_prev!=NULL){
		UNLINK_ITEM(region_list, reg, ni.g_next, ni.g_prev);
	}
}


/*EXTL_DOC
 * Set the short name of \var{reg} to \var{p}. If \var{p} is nil, \var{reg}
 * is set to have no name. If the name \var{p} is already in use, the full
 * name of \var{reg} will have an instance number appended to it.
 */
EXTL_EXPORT
bool region_set_name(WRegion *reg, const char *p)
{
	return region_set_name_instrq(reg, p, -1);
}

/*EXTL_DOC
 * Similar to \fnref{region_set_name} but instance number may be requested
 * by setting \var{fnref} $\ge$ 0. Other instance number will be used if the
 * requested one is not free
 */
EXTL_EXPORT
bool region_set_name_instrq(WRegion *reg, const char *p, int instrq)
{
	if(p==NULL || *p=='\0'){
		region_unuse_name(reg);
	}else{
		if(!region_use_name(reg, p, instrq))
			return FALSE;
	}
	
	region_notify_change(reg);
	
	return TRUE;
}


/*}}}*/


/*{{{ Lookup */


/*EXTL_DOC
 * Attempt to find a region with full name \var{name} and type inheriting
 * \var{typenam}.
 */
EXTL_EXPORT
WRegion *lookup_region(const char *cname, const char *typenam)
{
	WRegion *reg;
	char *name;
	
	if(cname==NULL)
		return NULL;

	for(reg=region_list; reg!=NULL; reg=reg->ni.g_next){
		if(typenam!=NULL && !wobj_is_str((WObj*)reg, typenam))
			continue;
		
		name=(char*)region_full_name(reg);
		
		if(name==NULL)
			continue;
		
		if(strcmp(name, cname)){
			free(name);
			continue;
		}
		
		free(name);
		
		return reg;
	}
	
	return NULL;
}

/*EXTL_DOC
 * Find all regions inheriting \var{typename} and whose name begins with
 * \var{nam} or, if none are found and the length of \var{name} is at 
 * least two, all regions whose name contains \var{name}.
 */
EXTL_EXPORT
ExtlTab complete_region(const char *nam, const char *typenam)
{
	WRegion *reg;
	char *name;
	int lnum=0, l==0;
	int n=0;
	ExtlTab tab=extl_create_table();
	
	if(nam==NULL)
		nam="";
	
	l=strlen(nam);
	
again:
	
	for(reg=region_list; reg!=NULL; reg=reg->ni.g_next){
		
		if(typenam!=NULL && !wobj_is_str((WObj*)reg, typenam))
			continue;

		name=(char*)region_full_name(reg);
		
		if(name==NULL)
			continue;
		
		if(l==0 ||
		   (lnum==0 && strncmp(name, nam, l)==0) ||
		   (lnum==1 && strstr(name, nam)!=NULL)){
			if(extl_table_seti_s(tab, n+1, name))
				n++;
		}
		
		free(name);
	}
	
	if(n==0 && lnum==0 && l>1){
		lnum=1;
		goto again;
	}
	
	return tab;
}


/*}}}*/

