/*
 * ion/ioncore/names.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */


#include <string.h>
#include <limits.h>

#include <libtu/rb.h>
#include <libtu/minmax.h>
#include <libtu/objp.h>

#include "common.h"
#include "region.h"
#include "clientwin.h"
#include "names.h"
#include "strings.h"
#include "gr.h"
#include "extl.h"


/*{{{ Implementation */


WNamespace ioncore_internal_ns={0, NULL, NULL, FALSE};
WNamespace ioncore_clientwin_ns={0, NULL, NULL, FALSE};


static bool initialise_ns(WNamespace *ns)
{
    if(ns->initialised)
        return TRUE;
    
    if(ns->rb==NULL)
        ns->rb=make_rb();
    if(ns->rb_unnamed==NULL)
        ns->rb_unnamed=make_rb();
    
    ns->initialised=(ns->rb!=NULL && ns->rb_unnamed!=NULL);
    return ns->initialised;
}


static int parseinst(const char *name, const char **startinst)
{
    const char *p2, *p3=NULL;
    int inst;
    int l=strlen(name);
    
    *startinst=name+l;

    if(name[l-1]!='>')
        return -1;
    
    p2=strrchr(name, '<');
    if(p2==NULL)
        return -1;
    
    inst=strtoul(p2+1, (char**)&p3, 10);
    
    if(inst<0 || p3!=name+l-1)
        return -1;
    
    *startinst=p2;
    return inst;
}


void region_do_unuse_name(WRegion *reg, bool insert_unnamed)
{
    WNamespace *ns=reg->ni.namespaceinfo;
    Rb_node node;
    int found=0;
    
    if(ns==NULL || !ns->initialised)
        return;
    
    if(reg->ni.name==NULL)
        node=rb_find_pkey_n(ns->rb_unnamed, reg, &found);
    else
        node=rb_find_key_n(ns->rb, reg->ni.name, &found);

    if(reg->ni.name!=NULL){
        free(reg->ni.name);
        reg->ni.name=NULL;
    }
    
    if(found)
        rb_delete_node(node);
    
    if(insert_unnamed){
        if(rb_insertp(ns->rb_unnamed, reg, reg))
            return;
        warn_err();
    }

    reg->ni.namespaceinfo=NULL;
}


void region_unuse_name(WRegion *reg)
{
    region_do_unuse_name(reg, TRUE);
    region_notify_change(reg);
}


static char *make_full_name(const char *name, int inst, 
                            bool append_always)
{
    char *newname=NULL;
    
    if(inst==0 && !append_always)
        newname=scopy(name);
    else
        libtu_asprintf(&newname, "%s<%d>", name, inst);
    
    return newname;
}


static bool separated(const char *n1, const char *n2, int *i1ret)
{
    const char *n1e, *n2e;
    int i1, i2;

    i1=maxof(0, parseinst(n1, &n1e));
    i2=maxof(0, parseinst(n2, &n2e));

    *i1ret=i1;
    
    /* Instance if n2 is is greater enough */
    if(i2>i1+1)
        return TRUE;
    
    /* Different name */
    if((n1e-n1)!=(n2e-n2))
        return TRUE;
    
    return (memcmp(n1, n2, n1e-n1)!=0);
}


static bool do_use_name(WRegion *reg, WNamespace *ns, const char *name,
                        int instrq, bool failchange)
{
    int parsed_inst=-1;
    char *fullname=NULL;
    const char *dummy=NULL;
    Rb_node node;
    int inst=0;
    int found=0;
    
    parsed_inst=parseinst(name, &dummy);
    
    if(ns->rb==NULL)
        return FALSE;

    /* If there's something that looks like an instance at the end of
     * name, we will append the instance number.
     */
    if(parsed_inst>=0 && inst==0 && failchange)
        return FALSE;
    
    if(instrq>=0){
        fullname=make_full_name(name, instrq, parsed_inst>=0);
        node=rb_find_key_n(ns->rb, fullname, &found);
        if(found){
            free(fullname);
            fullname=NULL;
            if(rb_val(node)==(void*)reg){
                /* The region already has the requested name */
                return TRUE;
            }
            if(failchange)
                return FALSE;
        }
    }
    
    if(fullname==NULL){
        found=0;
        inst=0;
        node=rb_find_key_n(ns->rb, name, &found);
        
        if(found){
            Rb_node next=node->c.list.flink;
            
            while(1){
                if(rb_val(node)==(void*)reg){
                    /* The region already has a name of requested form */
                    return TRUE;
                }
                
                if(next==NULL ||
                   separated((const char*)node->k.key, 
                             (const char*)next->k.key, &inst)){
                    /* 'inst' should be next free instance after increment
                     * as separation was greater then one.
                     */
                    inst++;
                    break;
                }
                /* 'inst' should be instance of next after increment 
                 * as separation was one.
                 */
                inst++;
                node=next;
                next=node->c.list.flink;
            }
        }
        
        fullname=make_full_name(name, inst, parsed_inst>=0);
    
        if(fullname==NULL)
            return FALSE;
    }
    
    if(!rb_insert(ns->rb, fullname, reg))
        return FALSE;
    /* Unuse old name or remove from unnamed tree now that we have 
     * managed to allocate new one
     */
    region_do_unuse_name(reg, FALSE);
    reg->ni.name=fullname;
    reg->ni.namespaceinfo=ns;
    return TRUE;
}


static bool use_name_anyinst(WRegion *reg, WNamespace *ns, const char *name)
{
    return do_use_name(reg, ns, name, -1, FALSE);
}


static bool use_name_exact(WRegion *reg, WNamespace *ns, const char *name)
{
    return do_use_name(reg, ns, name, 0, TRUE);
}


static bool use_name_parseany(WRegion *reg, WNamespace *ns, const char *name)
{
    int l, inst;
    const char *startinst;
    
    l=strlen(name);
    
    inst=parseinst(name, &startinst);
    if(inst>=0){
        int realnamelen=startinst-name;
        char *realname=ALLOC_N(char, realnamelen+1);
        if(realname==NULL){
            warn_err();
            /* No return is intended here. */
        }else{
            bool retval;
            memcpy(realname, name, realnamelen);
            realname[realnamelen]='\0';
            retval=do_use_name(reg, ns, realname, inst, FALSE);
            free(realname);
            return retval;
        }
    }
    
    return do_use_name(reg, ns, name, 0, FALSE);
}



/*}}}*/


/*{{{ Interface */


/*EXTL_DOC
 * Returns the name for \var{reg}.
 */
EXTL_EXPORT_MEMBER
const char *region_name(WRegion *reg)
{
    return reg->ni.name;
}


char *region_make_label(WRegion *reg, int maxw, GrBrush *brush)
{
    if(reg->ni.name==NULL)
        return NULL;
    
    return grbrush_make_label(brush, reg->ni.name, maxw);
}


static bool do_set_name(bool (*fn)(WRegion *reg, WNamespace *ns, const char *p),
                        WRegion *reg, WNamespace *ns, const char *p)
{
    bool ret=TRUE;
    char *nm=NULL;
    
    if(p!=NULL){
        nm=scopy(p);
        if(nm==NULL)
            return FALSE;
        str_stripws(nm);
    }
    
    if(nm==NULL || *nm=='\0'){
        region_do_unuse_name(reg, TRUE);
    }else{
        if(!initialise_ns(ns))
            return FALSE;
        ret=fn(reg, ns, nm);
    }
    
    if(nm!=NULL)
        free(nm);

    region_notify_change(reg);
    
    return ret;
}


bool region_init_name(WRegion *reg)
{
    if(!initialise_ns(&ioncore_internal_ns))
        return FALSE;
    
    if(OBJ_IS(reg, WClientWin)){
        region_do_unuse_name(reg, TRUE);
        return TRUE;
    }
    
    return use_name_anyinst(reg, &ioncore_internal_ns, OBJ_TYPESTR(reg));
}


/*EXTL_DOC
 * Set the name of \var{reg} to \var{p}. If the name is already in use,
 * an instance number suffix \code{<n>} will be attempted. If \var{p} has
 * such a suffix, it will be modified, otherwise such a suffix will be
 * added. Setting \var{p} to nil will cause current name to be removed.
 */
EXTL_EXPORT_MEMBER
bool region_set_name(WRegion *reg, const char *p)
{
    return do_set_name(use_name_parseany, reg, &ioncore_internal_ns, p);
}


/*EXTL_DOC
 * Similar to \fnref{WRegion.set_name} except if the name is already in use,
 * other instance numbers will not be attempted. The string \var{p} should
 * not contain a \code{<n>} suffix or this function will fail.
 */
EXTL_EXPORT_MEMBER
bool region_set_name_exact(WRegion *reg, const char *p)
{
    return do_set_name(use_name_exact, reg, &ioncore_internal_ns, p);
}


bool clientwin_set_name(WClientWin *cwin, const char *p)
{
    return do_set_name(use_name_anyinst, (WRegion*)cwin,
                       &ioncore_clientwin_ns, p);
}


/*}}}*/


/*{{{ Lookup and list */


static WRegion *do_lookup_region(WNamespace *ns, const char *cname,
                                 const char *typenam)
{
    Rb_node node;
    int found=0;
    
    if(cname==NULL || !ns->initialised)
        return NULL;
    
    node=rb_find_key_n(ns->rb, cname, &found);
    
    if(!found)
        return NULL;
    
    return (WRegion*)node->v.val;
}


/*EXTL_DOC
 * Attempt to find a non-client window region with name \var{name} and type
 * inheriting \var{typenam}.
 */
EXTL_EXPORT
WRegion *ioncore_lookup_region(const char *name, const char *typenam)
{
    return do_lookup_region(&ioncore_internal_ns, name, typenam);
}


/*EXTL_DOC
 * Attempt to find a client window with name \var{name}.
 */
EXTL_EXPORT
WClientWin *ioncore_lookup_clientwin(const char *name)
{
    return (WClientWin*)do_lookup_region(&ioncore_clientwin_ns, name, 
                                         "WClientWin");
}


static int do_list_tree(ExtlTab tab, int n, Rb_node rb, const char *typenam)
{
    Rb_node node;
    
    rb_traverse(node, rb){
        WRegion *reg=(WRegion*)node->v.val;
        
        assert(reg!=NULL);
        
        if(typenam!=NULL && !obj_is_str((Obj*)reg, typenam))
            continue;

        if(extl_table_seti_o(tab, n+1, (Obj*)reg))
            n++;
    }
    
    return n;
}


static ExtlTab do_list(WNamespace *ns, const char *typenam)
{
    ExtlTab tab;
    int n=0;
    
    if(!ns->initialised)
        return extl_table_none();
    
    tab=extl_create_table();
    
    n=do_list_tree(tab, n, ns->rb, typenam);
    n=do_list_tree(tab, n, ns->rb_unnamed, typenam);
    
    return tab;
}


/*EXTL_DOC
 * Find all non-client window regions inheriting \var{typenam}.
 */
EXTL_EXPORT
ExtlTab ioncore_region_list(const char *typenam)
{
    return do_list(&ioncore_internal_ns, typenam);
}


/*EXTL_DOC
 * Return a list of all client windows.
 */
EXTL_EXPORT
ExtlTab ioncore_clientwin_list()
{
    return do_list(&ioncore_clientwin_ns, NULL);
}


/*}}}*/

