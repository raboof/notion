/*
 * ion/ioncore/names.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2006. 
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
#include <libextl/extl.h>

#include "common.h"
#include "global.h"
#include "region.h"
#include "clientwin.h"
#include "names.h"
#include "strings.h"
#include "gr.h"


/*{{{ Implementation */


WNamespace ioncore_internal_ns={NULL, FALSE};
WNamespace ioncore_clientwin_ns={NULL, FALSE};


static bool initialise_ns(WNamespace *ns)
{
    if(ns->initialised)
        return TRUE;
    
    if(ns->rb==NULL)
        ns->rb=make_rb();
    
    ns->initialised=(ns->rb!=NULL);
    
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


static int parseinst_simple(const char *inststr)
{
    const char *end=NULL;
    int inst;
    
    if(*inststr=='\0')
        return 0;
    
    if(*inststr=='<'){
        inst=strtoul(inststr+1, (char**)&end, 10);
    
        if(inst>=0 && end!=NULL && *end=='>')
            return inst;
    }

    warn(TR("Corrupt instance number %s."), inststr);
    return -1;
}


#define COMPARE_FN ((Rb_compfn*)compare_nameinfos)

static int compare_nameinfos(const WRegionNameInfo *ni1, 
                             const WRegionNameInfo *ni2)
{
    int l1=0, l2=0;
    int i1=0, i2=0;
    int mc=0;
    
    /* Handle unnamed regions. */

    if(ni1->name==NULL){
        if(ni2->name!=NULL)
            return 1;
        return (ni1==ni2 ? 0 : (ni1<ni2 ? -1 : 1));
    }else if(ni2->name==NULL){
        return -1;
    }

    /* Special case: inst_off<0 means that -inst_off-1 is the actual
     * instance number and the name does not contain it.
     */

    if(ni1->inst_off>=0)
        l1=ni1->inst_off;
    else
        l1=strlen(ni1->name);

    if(ni2->inst_off>=0)
        l2=ni2->inst_off;
    else
        l2=strlen(ni2->name);
    
    /* Check name part first */
    
    mc=strncmp(ni1->name, ni2->name, minof(l1, l2));
    
    if(mc!=0)
        return mc;
    
    if(l1!=l2)
        return (l1<l2 ? -1 : 1);

    /* Same name, different instance */
    
    if(ni1->inst_off>=0)
        i1=parseinst_simple(ni1->name+ni1->inst_off);
    else
        i1=-ni1->inst_off-1; /*???*/

    if(ni2->inst_off>=0)
        i2=parseinst_simple(ni2->name+ni2->inst_off);
    else
        i2=-ni2->inst_off-1; /*???*/

    if(i1!=i2)
        return (i1<i2 ? -1 : 1);
    
    /* Same name and instance */
    
    return 0;
}

static bool insert_reg(Rb_node tree, WRegion *reg)
{
    assert(reg->ni.node==NULL);
    reg->ni.node=(void*)rb_insertg(tree, &(reg->ni), reg, COMPARE_FN);
    return (reg->ni.node!=NULL);
}

static bool separated(const WRegionNameInfo *ni1, 
                      const WRegionNameInfo *ni2, int *i1ret)
{
    int l1, l2;
    int i1, i2;
    int mc;
    
    assert(ni1->name!=NULL);
    
    if(ni2->name==NULL){
        /* Shouldn't happen the way this function is called below; unnamed
         * regions are before others in the traversal order of the tree.
         */
        *i1ret=parseinst_simple(ni1->name+ni1->inst_off);
        return TRUE;
    }
    
    assert(ni1->inst_off>=0 && ni2->inst_off>=0);

    l1=ni1->inst_off;
    l2=ni2->inst_off;

    i1=parseinst_simple(ni1->name+ni1->inst_off);
    *i1ret=i1;
    
    if(l1!=l2)
        return TRUE;
        
    if(memcmp(ni1->name, ni2->name, l1)!=0)
        return TRUE;
    
    i2=parseinst_simple(ni2->name+ni2->inst_off);
    
    return (i2>(i1+1));
}



void region_unregister(WRegion *reg)
{
    if(reg->ni.node!=NULL){
        rb_delete_node((Rb_node)reg->ni.node);
        reg->ni.node=NULL;
    }
    
    if(reg->ni.name!=NULL){
        free(reg->ni.name);
        reg->ni.name=NULL;
        reg->ni.inst_off=0;
    }
}


static bool make_full_name(WRegionNameInfo *ni, const char *name, int inst, 
                           bool append_always)
{
    assert(ni->name==NULL);
    
    if(inst==0 && !append_always)
        ni->name=scopy(name);
    else
        libtu_asprintf(&(ni->name), "%s<%d>", name, inst);
        
    if(ni->name==NULL)
        return FALSE;
    
    ni->inst_off=strlen(name);
    
    return TRUE;
}


static bool do_use_name(WRegion *reg, WNamespace *ns, const char *name,
                        int instrq, bool failchange)
{
    int parsed_inst=-1;
    WRegionNameInfo ni={NULL, 0, NULL};
    const char *dummy=NULL;
    Rb_node node;
    int inst=-1;
    int found=0;

    parsed_inst=parseinst(name, &dummy);
    
    if(!ns->initialised)
        return FALSE;

    /* If there's something that looks like an instance at the end of
     * name, we will append the instance number.
     */
    if(parsed_inst>=0 && inst==0 && failchange)
        return FALSE;
    
    if(instrq>=0){
        WRegionNameInfo tmpni;
        tmpni.name=(char*)name;
        tmpni.inst_off=-instrq-1;
        node=rb_find_gkey_n(ns->rb, &tmpni, COMPARE_FN, &found);
        if(found){
            if(rb_val(node)==(void*)reg){
                /* The region already has the requested name */
                return TRUE;
            }
            if(failchange)
                return FALSE;
        }else{
            inst=instrq;
        }
    }
    
    if(inst<0){
        WRegionNameInfo tmpni;
        
        found=0;
        inst=0;

        tmpni.name=(char*)name;
        tmpni.inst_off=-1;
        node=rb_find_gkey_n(ns->rb, &tmpni, COMPARE_FN, &found);
        
        if(found){
            while(1){
                Rb_node next=rb_next(node);
                
                if(rb_val(node)==(void*)reg){
                    /* The region already has a name of requested form */
                    return TRUE;
                }
                
                if(next==rb_nil(ns->rb) ||
                   separated((const WRegionNameInfo*)node->k.key, 
                             (const WRegionNameInfo*)next->k.key, &inst)){
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
            }
        }
    }

    if(!make_full_name(&ni, name, inst, parsed_inst>=0))
        return FALSE;

    /*
    rb_find_gkey_n(ns->rb, &ni, COMPARE_FN, &found);
    
    assert(!found);
     */

    region_unregister(reg);
    
    reg->ni.name=ni.name;
    reg->ni.inst_off=ni.inst_off;
    
    if(!insert_reg(ns->rb, reg)){
        free(reg->ni.name);
        reg->ni.name=NULL;
        reg->ni.inst_off=0;
        return FALSE;
    }

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
        bool retval=FALSE;
        int realnamelen=startinst-name;
        char *realname=ALLOC_N(char, realnamelen+1);
        if(realname!=NULL){
            memcpy(realname, name, realnamelen);
            realname[realnamelen]='\0';
            retval=do_use_name(reg, ns, realname, inst, FALSE);
            free(realname);
        }
        return retval;
    }
    
    return do_use_name(reg, ns, name, 0, FALSE);
}



/*}}}*/


/*{{{ Interface */


/*EXTL_DOC
 * Returns the name for \var{reg}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
const char *region_name(WRegion *reg)
{
    return reg->ni.name;
}


static bool do_set_name(bool (*fn)(WRegion *reg, WNamespace *ns, const char *p),
                        WRegion *reg, WNamespace *ns, const char *p)
{
    bool ret=TRUE;
    char *nm=NULL;

    if(!initialise_ns(ns))
        return FALSE;
    
    if(p!=NULL){
        nm=scopy(p);
        if(nm==NULL)
            return FALSE;
        str_stripws(nm);
    }

    if(nm==NULL || *nm=='\0'){
        region_unregister(reg);
        ret=insert_reg(ns->rb, reg);
    }else{
        ret=fn(reg, ns, nm);
    }
    
    if(nm!=NULL)
        free(nm);

    region_notify_change(reg, ioncore_g.notifies.name);
    
    return ret;
}


bool region_register(WRegion *reg)
{
    assert(reg->ni.name==NULL);
    
    if(!initialise_ns(&ioncore_internal_ns))
        return FALSE;
    
    return use_name_anyinst(reg, &ioncore_internal_ns, OBJ_TYPESTR(reg));
}


bool clientwin_register(WClientWin *cwin)
{
    WRegion *reg=(WRegion*)cwin;
    
    assert(reg->ni.name==NULL);
    
    if(!initialise_ns(&ioncore_clientwin_ns))
        return FALSE;
    
    return insert_reg(ioncore_clientwin_ns.rb, (WRegion*)cwin);
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
/*    return do_set_name(use_name_parseany, reg, &ioncore_internal_ns, p);*/
    return do_set_name(use_name_parseany, reg, 
            OBJ_IS(reg, WClientWin) ? &ioncore_clientwin_ns : &ioncore_internal_ns,
            p);
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
    WRegionNameInfo ni;
    Rb_node node;
    int found=0;
    const char *instptr=NULL;

    if(cname==NULL || !ns->initialised)
        return NULL;
    
    parseinst(cname, &instptr);
    assert(instptr!=NULL);
    
    ni.name=(char*)cname;
    ni.inst_off=instptr-cname;
    
    node=rb_find_gkey_n(ns->rb, &ni, COMPARE_FN, &found);
    
    if(!found)
        return NULL;
    
    return (WRegion*)node->v.val;
}


/*EXTL_DOC
 * Attempt to find a non-client window region with name \var{name} and type
 * inheriting \var{typenam}.
 */
EXTL_SAFE
EXTL_EXPORT
WRegion *ioncore_lookup_region(const char *name, const char *typenam)
{
    return do_lookup_region(&ioncore_internal_ns, name, typenam);
}


/*EXTL_DOC
 * Attempt to find a client window with name \var{name}.
 */
EXTL_SAFE
EXTL_EXPORT
WClientWin *ioncore_lookup_clientwin(const char *name)
{
    return (WClientWin*)do_lookup_region(&ioncore_clientwin_ns, name, 
                                         "WClientWin");
}


static bool do_list(ExtlFn fn, WNamespace *ns, const char *typenam)
{
    Rb_node node;
    int n=0;
    
    if(!ns->initialised)
        return FALSE;
    
    rb_traverse(node, ns->rb){
        WRegion *reg=(WRegion*)node->v.val;
        
        assert(reg!=NULL);

        if(typenam!=NULL && !obj_is_str((Obj*)reg, typenam))
            continue;

        if(!extl_iter_obj(fn, (Obj*)reg))
            return FALSE;
    }
    
    return TRUE;
}


/*EXTL_DOC
 * Iterate over all non-client window regions with (inherited) class
 * \var{typenam} until \var{iterfn} returns \code{false}.
 * The function itself returns \code{true} if it reaches the end of list
 * without this happening.
 */
EXTL_SAFE
EXTL_EXPORT
bool ioncore_region_i(ExtlFn fn, const char *typenam)
{
    return do_list(fn, &ioncore_internal_ns, typenam);
}


/*EXTL_DOC
 * Iterate over client windows until \var{iterfn} returns \code{false}.
 * The function itself returns \code{true} if it reaches the end of list
 * without this happening.
 */
EXTL_SAFE
EXTL_EXPORT
bool ioncore_clientwin_i(ExtlFn fn)
{
    return do_list(fn, &ioncore_clientwin_ns, NULL);
}


/*}}}*/


/*{{{ Displayname */


const char *region_displayname(WRegion *reg)
{
    const char *ret=NULL;
    CALL_DYN_RET(ret, const char *, region_displayname, reg, (reg));
    return ret;
}


char *region_make_label(WRegion *reg, int maxw, GrBrush *brush)
{
    const char *name=region_displayname(reg);

    if(name==NULL)
        return NULL;
    
    return grbrush_make_label(brush, name, maxw);
}


/*}}}*/
