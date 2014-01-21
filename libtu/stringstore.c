/*
 * libtu/stringstore.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2007. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <stdlib.h>
#include <string.h>

#include "misc.h"
#include "output.h"
#include "rb.h"
#include "stringstore.h"


static Rb_node stringstore=NULL;


const char *stringstore_get(StringId id)
{
    return (id==STRINGID_NONE
            ? NULL
            : (const char*)(((Rb_node)id)->k.key));
}


typedef struct{
    const char *key;
    uint len;
} D;

static int cmp(const void *d_, const char *nodekey)
{
    D *d=(D*)d_;
    
    int res=strncmp(d->key, nodekey, d->len);
    
    return (res!=0 
            ? res 
            : (nodekey[d->len]=='\0' ? 0 : -1));
}


StringId stringstore_find_n(const char *str, uint l)
{
    Rb_node node;
    int found=0;
    D d;
    
    if(stringstore==NULL)
        return STRINGID_NONE;
    
    d.key=str;
    d.len=l;
    
    node=rb_find_gkey_n(stringstore, &d, (Rb_compfn*)cmp, &found);
    
    if(!found)
        return STRINGID_NONE;
    
    return (StringId)node;
}


StringId stringstore_find(const char *str)
{
    return stringstore_find_n(str, strlen(str));
}


StringId stringstore_alloc_n(const char *str, uint l)
{
    Rb_node node=(Rb_node)stringstore_find_n(str, l);
    char *s;
    
    if(node!=NULL){
        node->v.ival++;
        return node;
    }
    
    if(stringstore==NULL){
        stringstore=make_rb();
        if(stringstore==NULL)
            return STRINGID_NONE;
    }
    
    s=scopyn(str, l);
    
    if(s==NULL)
        return STRINGID_NONE;
    
    node=rb_insert(stringstore, s, NULL);
    
    if(node==NULL)
        return STRINGID_NONE;
    
    node->v.ival=1;
        
    return (StringId)node;
}


StringId stringstore_alloc(const char *str)
{
    if(str==NULL)
        return STRINGID_NONE;
    
    return stringstore_alloc_n(str, strlen(str));
}


void stringstore_free(StringId id)
{
    Rb_node node=(Rb_node)id;
    
    if(node==NULL)
        return;
    
    if(node->v.ival<=0){
        warn("Stringstore reference count corrupted.");
        return;
    }
    
    node->v.ival--;
    
    if(node->v.ival==0){
        char *s=(char*)(node->k.key);
        rb_delete_node(node);
        free(s);
    }
}


void stringstore_ref(StringId id)
{
    Rb_node node=(Rb_node)id;
    
    if(node!=NULL)
        node->v.ival++;
}

