/*
 * ion/ioncore/hooks.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <libtu/rb.h>
#include <libtu/objp.h>
#include "common.h"
#include <libextl/extl.h>
#include "hooks.h"


EXTL_EXPORT
IMPLCLASS(WHook, Obj, hook_deinit, NULL);

static Rb_node named_hooks=NULL;


/*{{{ Named hooks */


/* If hk==NULL to register, new is attempted to be created. */
WHook *ioncore_register_hook(const char *name, WHook *hk)
{
    bool created=FALSE;
    char *nnm;
    
    if(hk==NULL)
        return NULL;
    
    if(named_hooks==NULL){
        named_hooks=make_rb();
        if(named_hooks==NULL)
            return NULL;
    }
    
    nnm=scopy(name);
    
    if(nnm==NULL)
        return NULL;
    
    if(!rb_insert(named_hooks, nnm, hk)){
        free(nnm);
        destroy_obj((Obj*)hk);
    }
    
    return hk;
}


WHook *ioncore_unregister_hook(const char *name, WHook *hk)
{
    bool found=FALSE;
    Rb_node node;

    if(named_hooks==NULL)
        return NULL;
    
    if(hk==NULL){
        assert(name!=NULL);
        node=rb_find_key_n(named_hooks, name, &found);
    }else{
        rb_traverse(node, named_hooks){
            if((WHook*)rb_val(node)==hk){
                found=TRUE;
                break;
            }
        }
    }

    if(found){
        hk=(WHook*)rb_val(node);
        free((char*)node->k.key);
        rb_delete_node(node);
    }
    
    return hk;
}


/*EXTL_DOC
 * Find named hook \var{name}.
 */
EXTL_SAFE
EXTL_EXPORT
WHook *ioncore_get_hook(const char *name)
{
    if(named_hooks!=NULL){
        bool found=FALSE;
        Rb_node node=rb_find_key_n(named_hooks, name, &found);
        if(found)
            return (WHook*)rb_val(node);
    }
    
    return NULL;
}


/*}}}*/


/*{{{ Init/deinit */


static void destroy_item(WHook *hk, WHookItem *item)
{
    if(item->fn==NULL)
        extl_unref_fn(item->efn);
    UNLINK_ITEM(hk->items, item, next, prev);
    free(item);
}


static WHookItem *create_item(WHook *hk)
{
    WHookItem *item=ALLOC(WHookItem);
    if(item!=NULL){
        LINK_ITEM_FIRST(hk->items, item, next, prev);
        item->fn=NULL;
        item->efn=extl_fn_none();
    }
    
    return item;
}


bool hook_init(WHook *hk)
{
    hk->items=NULL;
    return TRUE;
}


WHook *create_hook()
{
    CREATEOBJ_IMPL(WHook, hook, (p));
}


void hook_deinit(WHook *hk)
{
    ioncore_unregister_hook(NULL, hk);
    while(hk->items!=NULL)
        destroy_item(hk, hk->items);
}


/*}}}*/


/*{{{ Find/add/remove */


WHookItem *hook_find(WHook *hk, WHookDummy *fn)
{
    WHookItem *hi;
    
    for(hi=hk->items; hi!=NULL; hi=hi->next){
        if(hi->fn==fn)
            return hi;
    }
    
    return NULL;
}


WHookItem *hook_find_extl(WHook *hk, ExtlFn efn)
{
    WHookItem *hi;
    
    for(hi=hk->items; hi!=NULL; hi=hi->next){
        if(extl_fn_eq(hi->efn, efn))
            return hi;
    }
    
    return NULL;
}


/*EXTL_DOC
 * Is \var{fn} hooked to hook \var{hk}?
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
bool hook_listed(WHook *hk, ExtlFn efn)
{
    return hook_find_extl(hk, efn)!=NULL;
}


bool hook_add(WHook *hk, WHookDummy *fn)
{
    WHookItem *item;
    
    if(hook_find(hk, fn))
        return FALSE;
    
    item=create_item(hk);
    if(item==NULL)
        return FALSE;
    item->fn=fn;
    return TRUE;
}


/*EXTL_DOC
 * Add \var{efn} to the list of functions to be called when the
 * hook \var{hk} is triggered.
 */
EXTL_EXPORT_AS(WHook, add)
bool hook_add_extl(WHook *hk, ExtlFn efn)
{
    WHookItem *item;
    
    if(efn==extl_fn_none()){
        warn(TR("No function given."));
        return FALSE;
    }

    if(hook_find_extl(hk, efn))
        return FALSE;
    
    item=create_item(hk);
    
    if(item==NULL)
        return FALSE;
    
    item->efn=extl_ref_fn(efn);
    
    return TRUE;
}


bool hook_remove(WHook *hk, WHookDummy *fn)
{
    WHookItem *item=hook_find(hk, fn);
    if(item!=NULL)
        destroy_item(hk, item);
    return (item!=NULL);
}


/*EXTL_DOC
 * Remove \var{efn} from the list of functions to be called when the 
 * hook \var{hk} is triggered.
 */
EXTL_EXPORT_AS(WHook, remove)
bool hook_remove_extl(WHook *hk, ExtlFn efn)
{
    WHookItem *item=hook_find_extl(hk, efn);
    if(item!=NULL)
        destroy_item(hk, item);
    return (item!=NULL);
}


/*}}}*/


/*{{{ Basic marshallers */


static bool marshall_v(WHookDummy *fn, void *param)
{
    fn();
    return TRUE;
}
    

static bool marshall_extl_v(ExtlFn fn, void *param)
{
    extl_call(fn, NULL, NULL);
    return TRUE;
}


static bool marshall_o(WHookDummy *fn, void *param)
{
    fn((Obj*)param);
    return TRUE;
}
    

static bool marshall_extl_o(ExtlFn fn, void *param)
{
    return extl_call(fn, "o", NULL, (Obj*)param);
}


static bool marshall_p(WHookDummy *fn, void *param)
{
    fn(param);
    return TRUE;
}


static bool marshall_alt_v(bool (*fn)(), void *param)
{
    return fn();
}
    

static bool marshall_extl_alt_v(ExtlFn fn, void *param)
{
    bool ret=FALSE;
    extl_call(fn, NULL, "b", &ret);
    return ret;
}


static bool marshall_alt_o(bool (*fn)(), void *param)
{
    return fn((Obj*)param);
}
    

static bool marshall_extl_alt_o(ExtlFn fn, void *param)
{
    bool ret=FALSE;
    extl_call(fn, "o", "b", (Obj*)param, &ret);
    return ret;
}


static bool marshall_alt_p(bool (*fn)(), void *param)
{
    return fn(param);
}


/*}}}*/


/*{{{ Call */


void hook_call(const WHook *hk, void *p,
               WHookMarshall *m, WHookMarshallExtl *em)
{
    WHookItem *hi, *next;
    
    for(hi=hk->items; hi!=NULL; hi=next){
        next=hi->next;
        if(hi->fn!=NULL)
            m(hi->fn, p);
        else if(em!=NULL)
            em(hi->efn, p);
    }
}


bool hook_call_alt(const WHook *hk, void *p,
                   WHookMarshall *m, WHookMarshallExtl *em)
{
    WHookItem *hi, *next;
    bool ret=FALSE;
    
    for(hi=hk->items; hi!=NULL; hi=next){
        next=hi->next;
        if(hi->fn!=NULL)
            ret=m(hi->fn, p);
        else if(em!=NULL)
            ret=em(hi->efn, p);
        if(ret)
            break;
    }
    
    return ret;
}


void hook_call_v(const WHook *hk)
{
    hook_call(hk, NULL, marshall_v, marshall_extl_v);
}


void hook_call_o(const WHook *hk, Obj *o)
{
    hook_call(hk, o, marshall_o, marshall_extl_o);
}


void hook_call_p(const WHook *hk, void *p, WHookMarshallExtl *em)
{
    hook_call(hk, p, marshall_p, em);
}


bool hook_call_alt_v(const WHook *hk)
{
    return hook_call_alt(hk, NULL, (WHookMarshall*)marshall_alt_v, 
                         (WHookMarshallExtl*)marshall_extl_alt_v);
}


bool hook_call_alt_o(const WHook *hk, Obj *o)
{
    return hook_call_alt(hk, o, (WHookMarshall*)marshall_alt_o, 
                         (WHookMarshallExtl*)marshall_extl_alt_o);
}


bool hook_call_alt_p(const WHook *hk, void *p, WHookMarshallExtl *em)
{
    return hook_call_alt(hk, p, (WHookMarshall*)marshall_alt_p, em);
}


/*}}}*/



