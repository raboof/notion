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
#include "clientwin.h"
#include "names.h"
#include "font.h"
#include "grdata.h"
#include "extl.h"


/*{{{ Implementation */


/* To keep the amount of unnecessary bloat down and to keep things simple, 
 * Lua's tables are used as our hash implementation.
 */

typedef struct{
	ExtlTab alloc_tab;
	WRegion *list;
	bool initialised;
} Namespace;


Namespace internal_ns={0, NULL, FALSE};
Namespace clientwin_ns={0, NULL, FALSE};


static bool initialise_ns(Namespace *ns)
{
	if(ns->initialised)
		return TRUE;
	ns->alloc_tab=extl_create_table();
	if(ns->alloc_tab!=extl_table_none())
		ns->initialised=TRUE;
	return ns->initialised;
}


static int parseinst(const char *name, const char **startinst)
{
	const char *p2, *p3=NULL;
	int inst;
	int l=strlen(name);
	
	if(name[l-1]!='>')
		return -1;
	
	p2=strrchr(name, '<');
	if(p2==NULL)
		return -1;
	
	*startinst=p2;
	
	inst=strtoul(p2+1, (char**)&p3, 10);
	
	if(inst<0 || p3!=name+l-1)
		return -1;
	
	return inst;
}


void region_unuse_name(WRegion *reg)
{
	int inst;
	char *startinst;
	ExtlTab nrtab;
	Namespace *ns=reg->ni.namespaceinfo;
	bool needwarn=TRUE;
	
	if(ns==NULL || reg->ni.name==NULL)
		return;
	
	inst=parseinst(reg->ni.name, (const char**)&startinst);
	if(inst>=0)
		*startinst='\0';
	else
		inst=0;
	
	if(extl_table_gets_t(ns->alloc_tab, reg->ni.name, &nrtab)){
		int start=0, n=0;
		extl_table_cleari(nrtab, inst);
		if(extl_table_gets_i(nrtab, "lookup_start", &start)){
			if(inst<start)
				extl_table_sets_i(nrtab, "lookup_start", inst);
		}

		if(extl_table_gets_i(nrtab, "n_names", &n)){
			n--;
			if(n<=0)
				needwarn=!extl_table_clears(ns->alloc_tab, reg->ni.name);
			else
				needwarn=!extl_table_sets_i(nrtab, "n_names", n);
		}
		
		extl_unref_table(nrtab);
	}
	
	if(needwarn)
		warn("There was an error unreserving region name instance.");

	free(reg->ni.name);
	reg->ni.name=NULL;
	reg->ni.namespaceinfo=NULL;
	UNLINK_ITEM(ns->list, reg, ni.ns_next, ni.ns_prev);
}


static int try_inst(WRegion *reg, Namespace *ns, ExtlTab nrtab,
					 int inst, bool append_always, const char *name)
{
	char *newname=NULL;
	bool needwarn=TRUE;
	bool b;
	int n=0;

	if(extl_table_geti_b(nrtab, inst, &b)){
		if(b)
			return 0;
	}
	
	if(!extl_table_seti_b(nrtab, inst, TRUE)){
		warn("Unable to reserve name instance");
		return -1;
	}
	
	if(inst==0 && !append_always)
		newname=scopy(name);
	else
		libtu_asprintf(&newname, "%s<%d>", name, inst);
	
	if(newname==NULL){
		warn_err();
		return -1;
	}

	region_unuse_name(reg);
	
	reg->ni.name=newname;
	reg->ni.namespaceinfo=ns;
	LINK_ITEM(ns->list, reg, ni.ns_next, ni.ns_prev);

	extl_table_gets_i(nrtab, "n_names", &n);
	if(extl_table_sets_i(nrtab, "n_names", n+1))
		needwarn=FALSE;
	
	if(needwarn)
		warn("There was an error reserving name instance");

	return 1;
}


static bool do_use_name(WRegion *reg, Namespace *ns, const char *name,
						int instrq, bool failchange)
{
	int parsed_inst=-1;
	const char *dummy;
	ExtlTab nrtab;
	int i=0, ret=0;
	
	parsed_inst=parseinst(name, &dummy);
	
	if(parsed_inst>=0 && failchange)
		return FALSE;
	
	if(!extl_table_gets_t(ns->alloc_tab, name, &nrtab)){
		nrtab=extl_create_table();
		extl_table_sets_t(ns->alloc_tab, name, nrtab);
	}
	
	if(instrq>=0){
		ret=try_inst(reg, ns, nrtab, instrq, parsed_inst>=0, name);
	
	if(ret!=0 || failchange)
		extl_unref_table(nrtab);
		return (ret==1);
	}
	
	extl_table_gets_i(nrtab, "lookup_start", &i);
	
	while(1){
		ret=try_inst(reg, ns, nrtab, i, parsed_inst>=0, name);
		if(ret==1)
			extl_table_sets_i(nrtab, "lookup_start", i+1);
		if(ret!=0)
			break;
		i++;
	}

	extl_unref_table(nrtab);
	return (ret==1);
}


static bool use_name(WRegion *reg, Namespace *ns, 
					 const char *name, bool failchange)
{
	return do_use_name(reg, ns, name, failchange ? 0 : -1, failchange);
}


static bool use_name_parseinst(WRegion *reg, Namespace *ns,
							   const char *name, bool failchange)
{
	int l, inst;
	const char *startinst;
	
	l=strlen(name);
	
	inst=parseinst(name, &startinst);
	if(inst>=0){
		int realnamelen=startinst-name;
		char *realname=ALLOC_N(char, realnamelen);
		if(realname==NULL){
			warn_err();
			/* No return is intended here. */
		}else{
			bool retval;
			memcpy(realname, name, realnamelen);
			realname[realnamelen]='\0';
			retval=do_use_name(reg, ns, realname, inst, failchange);
			free(realname);
			return retval;
		}
	}
	
	return do_use_name(reg, ns, name, failchange ? 0 : -1, failchange);
}



/*}}}*/


/*{{{ Interface */


/*EXTL_DOC
 * Returns the name for \var{reg}.
 */
EXTL_EXPORT
const char *region_name(WRegion *reg)
{
	return reg->ni.name;
}


char *region_make_label(WRegion *reg, int maxw, WFontPtr font)
{
	if(reg->ni.name==NULL)
		return NULL;
	
	return make_label(font, reg->ni.name, maxw);
}


/*EXTL_DOC
 * Set the name of \var{reg} to \var{p}. If the name is already in use,
 * an instance number suffix \code{<n>} will be attempted. If \var{p} has
 * such a suffix, it will be modified, otherwise such a suffix will be
 * added. Setting \var{p} to nil will cause current name to be removed.
 */
EXTL_EXPORT
bool region_set_name(WRegion *reg, const char *p)
{
	if(p==NULL || *p=='\0'){
		region_unuse_name(reg);
		return TRUE;
	}
	
	if(!initialise_ns(&internal_ns))
		return FALSE;
	return use_name_parseinst(reg, &internal_ns, p, FALSE);
}


/*EXTL_DOC
 * Similar to \fnref{region_set_name} except if the name is already in use,
 * other instance numbers will not be attempted. The string \var{p} should
 * not contain a \code{<n>} suffix or this function will fail.
 */
EXTL_EXPORT
bool region_set_name_exact(WRegion *reg, const char *p)
{
	if(p==NULL || *p=='\0'){
		region_unuse_name(reg);
		return TRUE;
	}
	
	if(!initialise_ns(&internal_ns))
		return FALSE;
	return use_name(reg, &internal_ns, p, TRUE);
}


bool clientwin_set_name(WClientWin *cwin, const char *p)
{
	if(p==NULL || *p=='\0'){
		region_unuse_name((WRegion*)cwin);
		return TRUE;
	}
	
	if(!initialise_ns(&clientwin_ns))
		return FALSE;
	return use_name((WRegion*)cwin, &clientwin_ns, p, FALSE);
}


/*}}}*/


/*{{{ Lookup and complete */


static WRegion *do_lookup_region(Namespace *ns, const char *cname,
								 const char *typenam)
{
	WRegion *reg;
	const char *name;
	
	if(cname==NULL || !ns->initialised)
		return NULL;
	
	for(reg=ns->list; reg!=NULL; reg=reg->ni.ns_next){
		name=region_name(reg);
		
		if(name!=NULL){
			if(strcmp(name, cname))
				continue;
		}
		
		if(typenam!=NULL && !wobj_is_str((WObj*)reg, typenam))
			continue;

		return reg;
	}
	
	return NULL;
}


/*EXTL_DOC
 * Attempt to find a non-client window region with name \var{name} and type
 * inheriting \var{typenam}.
 */
EXTL_EXPORT
WRegion *lookup_region(const char *name, const char *typenam)
{
	return do_lookup_region(&internal_ns, name, typenam);
}


/*EXTL_DOC
 * Attempt to find a client window with name \var{name}.
 */
EXTL_EXPORT
WClientWin *lookup_clientwin(const char *name)
{
	return (WClientWin*)do_lookup_region(&clientwin_ns, name, "WClientWin");
}


ExtlTab do_complete_region(Namespace *ns, const char *nam, 
						   const char *typenam)
{
	WRegion *reg;
	const char *name;
	int lnum=0, l=0;
	int n=0;
	ExtlTab tab=extl_create_table();
	
	if(nam==NULL)
		nam="";
	
	l=strlen(nam);
	
again:
	
	for(reg=ns->list; reg!=NULL; reg=reg->ni.ns_next){
		name=region_name(reg);
		
		if(name==NULL)
			continue;
		
		if(l!=0){
		   if(lnum==0 && strncmp(name, nam, l))
				continue;
		   if(lnum==1 && strstr(name, nam)==NULL)
				continue;
		}

		if(typenam!=NULL && !wobj_is_str((WObj*)reg, typenam))
			continue;
			
		if(extl_table_seti_s(tab, n+1, name))
			n++;
	}
	
	if(n==0 && lnum==0 && l>=1){
		lnum=1;
		goto again;
	}
	
	return tab;
}


/*EXTL_DOC
 * Find all non-client window regions inheriting \var{typename} and whose
 * name begins with \var{nam} or, if none are found all whose name contains
 * \var{name}.
 */
EXTL_EXPORT
ExtlTab complete_region(const char *nam, const char *typenam)
{
	return do_complete_region(&internal_ns, nam, typenam);
}


/*EXTL_DOC
 * Find all client windows whose name begins with \var{nam} or, if none are
 * found, all whose name contains \var{name}.
 */
EXTL_EXPORT
ExtlTab complete_clientwin(const char *nam)
{
	return do_complete_region(&clientwin_ns, nam, NULL);
}


/*}}}*/

