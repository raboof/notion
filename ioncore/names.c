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


EXTL_EXPORT
uint region_name_instance(WRegion *reg)
{
	return reg->ni.instance;
}


EXTL_EXPORT
const char *region_name(WRegion *reg)
{
	return reg->ni.name;
}


EXTL_EXPORT
char *region_full_name(WRegion *reg)
{
	const char *str=region_name(reg);
	char tmp[16];
	
	if(str==NULL)
		return NULL;
	
	if(reg->ni.instance!=0){
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


static bool region_use_name(WRegion *reg, const char *namep)
{
	WRegion *p, *np, *minp=NULL;
	int mininst=INT_MAX;
	const char *str;
	char *name;
	
	name=scopy(namep);
	
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
	
	if(p!=NULL){
		for(; ; p=np){
			assert(p!=reg);
			np=p->ni.n_next;
			if(p->ni.instance+1==np->ni.instance){
				continue;
			}else if(p->ni.instance>=np->ni.instance && np->ni.instance!=0){
				mininst=0;
				minp=p;
			}else if(p->ni.instance+1<mininst){
				mininst=p->ni.instance+1;
				minp=p;
				continue;
			}
			break;
		}
	
		assert(minp!=NULL);
	
		np=minp->ni.n_next;
		reg->ni.n_next=np;
		np->ni.n_prev=reg;
		minp->ni.n_next=reg;
		reg->ni.n_prev=minp;
		reg->ni.instance=mininst;
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


EXTL_EXPORT
bool region_set_name(WRegion *reg, const char *p)
{
	if(p==NULL || *p=='\0'){
		region_unuse_name(reg);
	}else{
		if(!region_use_name(reg, p))
			return FALSE;
	}
	
	region_notify_change(reg);
	
	return TRUE;
}


/*}}}*/


/*{{{ Lookup */


WRegion *do_lookup_region(const char *cname, WObjDescr *descr)
{
	WRegion *reg;
	char *name;
	
	if(cname==NULL)
		return NULL;

	for(reg=region_list; reg!=NULL; reg=reg->ni.g_next){
		if(!wobj_is((WObj*)reg, descr))
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


ExtlTab do_complete_region(const char *nam, WObjDescr *descr)
{
	WRegion *reg;
	char *name;
	int lnum=0, l=strlen(nam);
	int n=0;
	ExtlTab tab=extl_create_table();
	
	if(nam==NULL)
		nam="";
	
again:
	
	for(reg=region_list; reg!=NULL; reg=reg->ni.g_next){
		
		if(!wobj_is((WObj*)reg, descr))
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


EXTL_EXPORT
WRegion *lookup_region(const char *name)
{
	return do_lookup_region(name, &OBJDESCR(WObj));
}


EXTL_EXPORT
ExtlTab complete_region(const char *nam)
{
	return do_complete_region(nam, &OBJDESCR(WObj));
}

