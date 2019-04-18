/*
 * libextl/luaextl.c
 *
 * Copyright (c) The Notion Team 2011
 * Copyright (c) Tuomo Valkonen 1999-2005.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <unistd.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <libtu/obj.h>
#include <libtu/objp.h>
#include <libtu/dlist.h>
#include <libtu/util.h>

#include "readconfig.h"
#include "luaextl.h"
#include "private.h"

#define MAGIC 0xf00ba7

/* Maximum number of parameters and return values for calls from Lua
 * and (if va_copy is not available) return value from Lua functions.
 */
#define MAX_PARAMS 16

static lua_State *l_st=NULL;
ExtlHook current_hook=NULL;

static bool extl_stack_get(lua_State *st, int pos, char type,
                           bool copystring, bool *wasdeadobject,
                           void *valret);

static int extl_protected(lua_State *st);

#ifdef EXTL_LOG_ERRORS
static void flushtrace();
#else
#define flushtrace()
#endif

/*{{{ Safer rawget/set/getn */


#define CHECK_TABLE(ST, INDEX) luaL_checktype(ST, INDEX, LUA_TTABLE)

static int lua_objlen_check(lua_State *st, int index)
{
    CHECK_TABLE(st, index);
#if LUA_VERSION_NUM>=502
    return lua_rawlen(st, index);
#else
    return lua_objlen(st, index);
#endif
}


static void lua_rawset_check(lua_State *st, int index)
{
    CHECK_TABLE(st, index);
    lua_rawset(st, index);
}


static void lua_rawseti_check(lua_State *st, int index, int n)
{
    CHECK_TABLE(st, index);
    lua_rawseti(st, index, n);
}


static void lua_rawget_check(lua_State *st, int index)
{
    CHECK_TABLE(st, index);
    lua_rawget(st, index);
}


static void lua_rawgeti_check(lua_State *st, int index, int n)
{
    CHECK_TABLE(st, index);
    lua_rawgeti(st, index, n);
}


/*}}}*/


/*{{{ A cpcall wrapper */


typedef bool ExtlCPCallFn(lua_State *st, void *ptr);


typedef struct{
    ExtlCPCallFn *fn;
    void *udata;
    bool retval;
} ExtlCPCallParam;


static int extl_docpcall(lua_State *st)
{
    ExtlCPCallParam *param=(ExtlCPCallParam*)lua_touserdata(st, -1);

    /* Should be enough for most things */
    if(!lua_checkstack(st, 8)){
        extl_warn(TR("Lua stack full."));
        return 0;
    }

    param->retval=param->fn(st, param->udata);
    return 0;
}


static bool extl_cpcall(lua_State *st, ExtlCPCallFn *fn, void *ptr)
{
    ExtlCPCallParam param;
    int oldtop=lua_gettop(st);
    int err;

    param.fn=fn;
    param.udata=ptr;
    param.retval=FALSE;


#if LUA_VERSION_NUM>=502
    /* TODO: Call appropriate lua_checkstack!?
    lua_checkstack(st, 2); */
    lua_pushcfunction(st, extl_docpcall);
    lua_pushlightuserdata(st, &param);
    err=lua_pcall(st, 1, 0, 0);
#else
    err=lua_cpcall(st, extl_docpcall, &param);
#endif
    if(err==LUA_ERRRUN){
        extl_warn("%s", lua_tostring(st, -1));
    }else if(err==LUA_ERRMEM){
        extl_warn("%s", strerror(ENOMEM));
    }else if(err!=0){
        extl_warn(TR("Unknown Lua error."));
    }

    lua_settop(st, oldtop);

    return param.retval;
}


/*}}}*/


/*{{{ Obj userdata handling -- unsafe */


static int owned_cache_ref=LUA_NOREF;


static Obj *extl_get_obj(lua_State *st, int pos,
                         bool *invalid, bool *dead)
{
    int val;

    *dead=FALSE;
    *invalid=TRUE;

    if(!lua_isuserdata(st, pos)){
        *invalid=!lua_isnil(st, pos);
        return NULL;
    }

    if(!lua_getmetatable(st, pos))
        return NULL;

    /* If the userdata object is a proper Obj, metatable[MAGIC] must
     * have been set to MAGIC.
     */
    lua_pushnumber(st, MAGIC);
    lua_gettable(st, -2);
    val=lua_tonumber(st, -1);
    lua_pop(st, 2);

    if(val==MAGIC){
        ExtlProxy *proxy=(ExtlProxy*)lua_touserdata(st, pos);

        *invalid=FALSE;

        if(proxy!=NULL){
            Obj *obj=EXTL_PROXY_OBJ(proxy);
            if(obj==NULL){
                *dead=TRUE;
                *invalid=TRUE;
            }
            return obj;
        }
    }

    return NULL;
}


static void extl_uncache_(lua_State *st, Obj *obj)
{
    if(EXTL_OBJ_OWNED(obj)){
        lua_rawgeti(st, LUA_REGISTRYINDEX, owned_cache_ref);
        lua_pushlightuserdata(st, obj);
        lua_pushnil(st);
        lua_rawset(st, -3);
    }else{
        lua_pushlightuserdata(st, obj);
        lua_pushnil(st);
        lua_rawset(st, LUA_REGISTRYINDEX);
    }
}


void extl_uncache(Obj *obj)
{
    extl_cpcall(l_st, (ExtlCPCallFn*)extl_uncache_, obj);
}


static void extl_push_obj(lua_State *st, Obj *obj)
{
    ExtlProxy *proxy;

    if(obj==NULL){
        lua_pushnil(st);
        return;
    }

    if(EXTL_OBJ_CACHED(obj)){
        if(EXTL_OBJ_OWNED(obj)){
            lua_rawgeti(st, LUA_REGISTRYINDEX, owned_cache_ref);
            lua_pushlightuserdata(st, obj);
            lua_rawget(st, -2);
            lua_remove(st, -2); /* owned_cache */
        }else{
            lua_pushlightuserdata(st, obj);
            lua_rawget(st, LUA_REGISTRYINDEX);
        }
        if(lua_isuserdata(st, -1)){
            D(fprintf(stderr, "found %p cached\n", obj));
            return;
        }
        lua_pop(st, 1);
    }

    D(fprintf(stderr, "Creating %p\n", obj));

    proxy=(ExtlProxy*)lua_newuserdata(st, sizeof(ExtlProxy));

    /* Lua shouldn't return if the allocation fails */

    lua_pushfstring(st, "luaextl_%s_metatable", OBJ_TYPESTR(obj));
    lua_gettable(st, LUA_REGISTRYINDEX);
    if(lua_isnil(st, -1)){
        lua_pop(st, 2);
        lua_pushnil(st);
    }else{
        lua_setmetatable(st, -2);

        /* Store in cache */
        if(EXTL_OBJ_OWNED(obj)){
            lua_rawgeti(st, LUA_REGISTRYINDEX, owned_cache_ref);
            lua_pushlightuserdata(st, obj);
            lua_pushvalue(st, -3);  /* the WWatch */
            lua_rawset_check(st, -3);
            lua_pop(st, 1); /* owned_cache */
        }else{
            lua_pushlightuserdata(st, obj);
            lua_pushvalue(st, -2); /* the WWatch */
            lua_rawset_check(st, LUA_REGISTRYINDEX);
        }
        EXTL_BEGIN_PROXY_OBJ(proxy, obj);
    }
}


/*{{{ Functions available to Lua code */


static int extl_obj_gc_handler(lua_State *st)
{
    ExtlProxy *proxy;
    bool dead=FALSE, invalid=FALSE;
    Obj *obj;

    obj=extl_get_obj(st, 1, &invalid, &dead);

    if(obj==NULL){
        /* This should not happen, actually. Our object cache should
         * hold references to all objects seen on the Lua side until
         * they are destroyed.
         */
        return 0;
    }

    proxy=(ExtlProxy*)lua_touserdata(st, 1);

    if(proxy!=NULL)
        EXTL_END_PROXY_OBJ(proxy, obj);

    if(EXTL_OBJ_OWNED(obj))
        EXTL_DESTROY_OWNED_OBJ(obj);

    return 0;
}


static int extl_obj_typename(lua_State *st)
{
    Obj *obj=NULL;

    if(!extl_stack_get(st, 1, 'o', FALSE, NULL, &obj) || obj==NULL)
        return 0;

    lua_pushstring(st, EXTL_OBJ_TYPENAME(obj));
    return 1;
}

/* Dummy code for documentation generation. */

/*EXTL_DOC
 * Return type name of \var{obj}.
 */
EXTL_EXPORT_AS(global, obj_typename)
const char *__obj_typename(Obj *obj);


static int extl_obj_exists(lua_State *st)
{
    Obj *obj=NULL;

    extl_stack_get(st, 1, 'o', FALSE, NULL, &obj);

    lua_pushboolean(st, obj!=NULL);

    return 1;
}

/* Dummy code for documentation generation. */

/*EXTL_DOC
 * Does \var{obj} still exist on the C side of the application?
 */
EXTL_EXPORT_AS(global, obj_exists)
bool __obj_exists(Obj *obj);


static int extl_obj_is(lua_State *st)
{
    Obj *obj=NULL;
    const char *tn;

    extl_stack_get(st, 1, 'o', FALSE, NULL, &obj);

    if(obj==NULL){
        lua_pushboolean(st, 0);
    }else{
        tn=lua_tostring(st, 2);
        lua_pushboolean(st, EXTL_OBJ_IS(obj, tn));
    }

    return 1;
}

/* Dummy code for documentation generation. */

/*EXTL_DOC
 * Is \var{obj} of type \var{typename}.
 */
EXTL_EXPORT_AS(global, obj_is)
bool __obj_is(Obj *obj, const char *typename);


static int extl_current_file_or_dir(lua_State *st, bool dir)
{
    lua_Debug ar;
    const char *s, *p;

    if(lua_getstack(st, 1, &ar)!=1)
        goto err;
    if(lua_getinfo(st, "S", &ar)==0)
        goto err;

    if(ar.source==NULL || ar.source[0]!='@')
        return 0; /* not a file */

    s=ar.source+1;

    if(!dir){
        lua_pushstring(st, s);
    }else{
        p=strrchr(s, '/');
        if(p==NULL){
            lua_pushstring(st, ".");
        }else{
            lua_pushlstring(st, s, p-s);
        }
    }
    return 1;

err:
    extl_warn("Unable to get caller file from stack.");
    return 0;
}


static int extl_dopath(lua_State *st)
{
    const char *toincl, *cfdir;
    bool res, complain;

    toincl=luaL_checkstring(st, 1);
    complain=!lua_toboolean(st, 2);

    if(extl_current_file_or_dir(st, TRUE)!=1){
        res=extl_read_config(toincl, NULL, complain);
    }else{
        cfdir=lua_tostring(st, -1);
        res=extl_read_config(toincl, cfdir, complain);
        lua_pop(st, 1);
    }
    lua_pushboolean(st, res);
    return 1;
}

/* Dummy code for documentation generation. */

/*EXTL_DOC
 * Look up and execute another file with Lua code.
 */
EXTL_EXPORT_AS(global, dopath)
bool dopath(const char *what);


/*}}}*/


static bool extl_init_obj_info(lua_State *st)
{
    static ExtlExportedFnSpec dummy[]={
        {NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, FALSE}
    };

    extl_register_class("Obj", dummy, NULL);

    /* Create cache for proxies to objects owned by Lua-side.
     * These need to be in a weak table to ever be collected.
     */
    lua_newtable(st);
    lua_newtable(st);
    lua_pushstring(st, "__mode");
    lua_pushstring(st, "v");
    lua_rawset_check(st, -3);
    lua_setmetatable(st, -2);
    owned_cache_ref=luaL_ref(st, LUA_REGISTRYINDEX);

    lua_pushcfunction(st, extl_obj_typename);
    lua_setglobal(st, "obj_typename");
    lua_pushcfunction(st, extl_obj_is);
    lua_setglobal(st, "obj_is");
    lua_pushcfunction(st, extl_obj_exists);
    lua_setglobal(st, "obj_exists");
    lua_pushcfunction(st, extl_dopath);
    lua_setglobal(st, "dopath");
    lua_pushcfunction(st, extl_protected);
    lua_setglobal(st, "protected");

    return TRUE;
}


/*}}}*/


/*{{{ Error handling and reporting -- unsafe */


static int extl_stack_trace(lua_State *st)
{
    lua_Debug ar;
    int lvl=0;
    int n_skip=0;

    lua_pushstring(st, TR("Stack trace:"));

    for( ; lua_getstack(st, lvl, &ar); lvl++){
        bool is_c=FALSE;

        if(lua_getinfo(st, "Sln", &ar)==0){
            lua_pushfstring(st,
                            TR("\n(Unable to get debug info for level %d)"),
                            lvl);
            lua_concat(st, 2);
            continue;
        }

        is_c=(ar.what!=NULL && strcmp(ar.what, "C")==0);

        if(!is_c || ar.name!=NULL){
            lua_pushfstring(st, "\n%d %s", lvl, ar.short_src);
            if(ar.currentline!=-1)
                lua_pushfstring(st, ":%d", ar.currentline);
            if(ar.name!=NULL)
                lua_pushfstring(st, ": in '%s'", ar.name);
            lua_concat(st, 2+(ar.currentline!=-1)+(ar.name!=NULL));
            n_skip=0;
        }else{
            if(n_skip==0){
                lua_pushstring(st, TR("\n  [Skipping unnamed C functions.]"));
                /*lua_pushstring(st, "\n...skipping...");*/
                lua_concat(st, 2);
            }
            n_skip++;
        }
    }
    return 1;
}


#ifdef EXTL_LOG_ERRORS

static int extl_do_collect_errors(lua_State *st)
{
    int n, err;
    ErrorLog *el=(ErrorLog*)lua_touserdata(st, -1);

    lua_pop(st, 1);

    n=lua_gettop(st)-1;
    err=lua_pcall(st, n, 0, 0);

    if(err!=0)
        extl_warn("%s", lua_tostring(st, -1));

    if(el->msgs_len==0)
        return 0;
    lua_pushstring(st, el->msgs);
    return 1;
}


int extl_collect_errors(lua_State *st)
{
    ErrorLog el;
    int n=lua_gettop(st);
    int err;

    lua_pushcfunction(st, extl_do_collect_errors);
    lua_insert(st, 1);
    lua_pushlightuserdata(st, &el);

    errorlog_begin(&el);

    err=lua_pcall(st, n+1, 1, 0);

    errorlog_end(&el);
    errorlog_deinit(&el);

    if(err!=0)
        extl_warn(TR("Internal error."));

    return 1;
}

#endif


/*}}}*/


/*{{{ Init -- unsafe, but it doesn't matter at this point */


bool extl_init()
{
    l_st=luaL_newstate();

    if(l_st==NULL){
        extl_warn(TR("Unable to initialize Lua."));
        return FALSE;
    }

    luaL_openlibs(l_st);

    if(!extl_init_obj_info(l_st)){
        lua_close(l_st);
        return FALSE;
    }

#ifdef EXTL_LOG_ERRORS
    lua_pushcfunction(l_st, extl_collect_errors);
    lua_setglobal(l_st, "collect_errors");
#endif

    return TRUE;
}


void extl_deinit()
{
    lua_close(l_st);
    l_st=NULL;
}


/*}}}*/


/*{{{ Stack get/push -- all unsafe */


static bool extl_stack_get(lua_State *st, int pos, char type,
                           bool copystring, bool *wasdeadobject,
                           void *valret)
{
    double d=0;
    bool b=FALSE;
    const char *str;

    if(wasdeadobject!=NULL)
        *wasdeadobject=FALSE;

    if(type=='b'){
        if(valret)
            *((bool*)valret)=lua_toboolean(st, pos);
        return TRUE;
    }

    switch(lua_type(st, pos)){
    case LUA_TNUMBER:
        if(type!='i' && type!='d' && type!='a')
            return FALSE;

        d=lua_tonumber(st, pos);

        if(type=='i'){
            if(d-floor(d)!=0)
                return FALSE;
            if(valret)
                *((int*)valret)=d;
        }else if(type=='a'){
            if(valret){
                ((ExtlAny*)valret)->type='d';
                ((ExtlAny*)valret)->value.d=d;
            }
        }else{
            if(valret)
                *((double*)valret)=d;
        }
        return TRUE;
    case LUA_TBOOLEAN:
        // type=='b' case is handled above before the switch
        if(type!='a')
            return FALSE;
        
        if(type=='a'){
            b=lua_toboolean(st, pos);
            if(valret){
                ((ExtlAny*)valret)->type='b';
                ((ExtlAny*)valret)->value.b=b;
            }
        }
        return TRUE;
    case LUA_TNIL:
    case LUA_TNONE:
        if(type=='a'){
            if(valret)
                ((ExtlAny*)valret)->type='v';
        }else if(type=='t' || type=='f'){
            if(valret)
                *((int*)valret)=LUA_NOREF;
        }else if(type=='s' || type=='S'){
            if(valret)
                *((char**)valret)=NULL;
        }else if(type=='o'){
            if(valret)
                *((Obj**)valret)=NULL;
        }else{
            return FALSE;
        }
        return TRUE;

    case LUA_TSTRING:
        if(type!='s' && type!='S' && type!='a')
            return FALSE;
        if(valret){
            str=lua_tostring(st, pos);
            if(str!=NULL && copystring){
                str=extl_scopy(str);
                if(str==NULL)
                    return FALSE;
            }
            if(type=='a'){
                ((ExtlAny*)valret)->type=(copystring ? 's' : 'S');
                ((ExtlAny*)valret)->value.s=str;
            }else{
                *((const char**)valret)=str;
            }
        }
        return TRUE;

    case LUA_TFUNCTION:
        if(type!='f' && type!='a')
            return FALSE;
        if(valret){
            lua_pushvalue(st, pos);
            if(type=='a'){
                ((ExtlAny*)valret)->type='f';
                ((ExtlAny*)valret)->value.f=luaL_ref(st, LUA_REGISTRYINDEX);
            }else{
                *((int*)valret)=luaL_ref(st, LUA_REGISTRYINDEX);
            }
        }
        return TRUE;

    case LUA_TTABLE:
        if(type!='t' && type!='a')
            return FALSE;
        if(valret){
            lua_pushvalue(st, pos);
            if(type=='a'){
                ((ExtlAny*)valret)->type='t';
                ((ExtlAny*)valret)->value.f=luaL_ref(st, LUA_REGISTRYINDEX);
            }else{
                *((int*)valret)=luaL_ref(st, LUA_REGISTRYINDEX);
            }
        }
        return TRUE;

    case LUA_TUSERDATA:
        if(type=='o'|| type=='a'){
            bool invalid=FALSE, dead=FALSE;
            Obj *obj=extl_get_obj(st, pos, &invalid, &dead);
            if(wasdeadobject!=NULL)
                *wasdeadobject=dead;
            if(valret){
                if(type=='a'){
                    ((ExtlAny*)valret)->type='o';
                    ((ExtlAny*)valret)->value.o=obj;
                }else{
                    *((Obj**)valret)=obj;
                }
            }
            return !invalid;
        }
    }

    return FALSE;
}


static void extl_to_any(ExtlAny *a, char type, void *ptr)
{
    if(type=='a'){
        *a=*(ExtlAny*)ptr;
        return;
    }

    a->type=type;

    switch(type){
    case 'i': a->value.i=*(int*)ptr; break;
    case 'd': a->value.d=*(double*)ptr; break;
    case 'b': a->value.b=*(bool*)ptr; break;
    case 'o': a->value.o=*(Obj**)ptr; break;
    case 's':
    case 'S': a->value.s=*(char**)ptr; break;
    case 't': a->value.t=*(ExtlTab*)ptr; break;
    case 'f': a->value.f=*(ExtlFn*)ptr; break;
    }
}


static void extl_to_any_vararg(ExtlAny *a, char type, va_list *argsp)
{
    if(type=='a'){
        *a=va_arg(*argsp, ExtlAny);
        return;
    }

    a->type=type;

    switch(type){
    case 'i': a->value.i=va_arg(*argsp, int); break;
    case 'd': a->value.d=va_arg(*argsp, double); break;
    case 'b': a->value.b=va_arg(*argsp, bool); break;
    case 'o': a->value.o=va_arg(*argsp, Obj*); break;
    case 's':
    case 'S': a->value.s=va_arg(*argsp, char*); break;
    case 't': a->value.t=va_arg(*argsp, ExtlTab); break;
    case 'f': a->value.f=va_arg(*argsp, ExtlFn); break;
    }
}


static void extl_stack_pusha(lua_State *st, ExtlAny *a)
{
    switch(a->type){
    case 'i': lua_pushnumber(st, a->value.i); break;
    case 'd': lua_pushnumber(st, a->value.d); break;
    case 'b': lua_pushboolean(st, a->value.b); break;
    case 'o': extl_push_obj(st, a->value.o); break;
    case 's':
    case 'S': lua_pushstring(st, a->value.s); break;
    case 't': lua_rawgeti(st, LUA_REGISTRYINDEX, a->value.t); break;
    case 'f': lua_rawgeti(st, LUA_REGISTRYINDEX, a->value.f); break;
    default: lua_pushnil(st);
    }
}


static void extl_stack_push(lua_State *st, char spec, void *ptr)
{
    ExtlAny a;

    extl_to_any(&a, spec, ptr);
    extl_stack_pusha(st, &a);
}


static bool extl_stack_push_vararg(lua_State *st, char spec, va_list *argsp)
{
    ExtlAny a;

    extl_to_any_vararg(&a, spec, argsp);
    extl_stack_pusha(st, &a);

    return TRUE;
}


/*}}}*/


/*{{{ Free */


enum{STRINGS_NONE, STRINGS_NONCONST, STRINGS_ALL};


static void extl_any_free(ExtlAny *a, int strings)
{
    if((a->type=='s' && strings!=STRINGS_NONE) ||
       (a->type=='S' && strings==STRINGS_ALL)){
        if(a->value.s!=NULL)
            free((char*)a->value.s);
    }else if(a->type=='t'){
        extl_unref_table(a->value.t);
    }else if(a->type=='f'){
        extl_unref_fn(a->value.f);
    }
}


static void extl_free(void *ptr, char spec, int strings)
{
    ExtlAny a;

    extl_to_any(&a, spec, ptr);
    extl_any_free(&a, strings);
}


/*}}}*/


/*{{{ Table and function references. */


static bool extl_getref(lua_State *st, int ref)
{
    lua_rawgeti(st, LUA_REGISTRYINDEX, ref);
    if(lua_isnil(st, -1)){
        lua_pop(st, 1);
        return FALSE;
    }
    return TRUE;
}

/* Unref */

static bool extl_do_unref(lua_State *st, int *refp)
{
    luaL_unref(st, LUA_REGISTRYINDEX, *refp);
    return TRUE;
}


ExtlFn extl_unref_fn(ExtlFn ref)
{
    extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_unref, &ref);
    return LUA_NOREF;
}


ExtlFn extl_unref_table(ExtlTab ref)
{
    extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_unref, &ref);
    return LUA_NOREF;
}


/* noref */

ExtlFn extl_fn_none()
{
    return LUA_NOREF;
}


ExtlTab extl_table_none()
{
    return LUA_NOREF;
}


/* ref */

static bool extl_do_ref(lua_State *st, int *refp)
{
    if(!extl_getref(st, *refp))
        return FALSE;
    *refp=luaL_ref(st, LUA_REGISTRYINDEX);
    return TRUE;
}


ExtlTab extl_ref_table(ExtlTab ref)
{
    if(extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_ref, &ref))
        return ref;
    return LUA_NOREF;
}


ExtlFn extl_ref_fn(ExtlFn ref)
{
    if(extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_ref, &ref))
        return ref;
    return LUA_NOREF;
}


/* create_table */

static bool extl_do_create_table(lua_State *st, int *refp)
{
    lua_newtable(st);
    *refp=luaL_ref(st, LUA_REGISTRYINDEX);
    return TRUE;
}


ExtlTab extl_create_table()
{
    ExtlTab ref;
    if(extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_create_table, &ref))
        return ref;
    return LUA_NOREF;
}


/* eq */

typedef struct{
    int o1, o2;
    bool ret;
} EqParams;


static bool extl_do_eq(lua_State *st, EqParams *ep)
{
    if(!extl_getref(st, ep->o1))
        return FALSE;
    if(!extl_getref(st, ep->o2))
        return FALSE;
#if LUA_VERSION_NUM>=502
    ep->ret=lua_compare(st, -1, -2,LUA_OPEQ);
#else
    ep->ret=lua_equal(st, -1, -2);
#endif
    return TRUE;
}


bool extl_fn_eq(ExtlFn fn1, ExtlFn fn2)
{
    EqParams ep;
    ep.o1=fn1;
    ep.o2=fn2;
    ep.ret=FALSE;
    extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_eq, &ep);
    return ep.ret;
}


bool extl_table_eq(ExtlTab t1, ExtlTab t2)
{
    EqParams ep;
    ep.o1=t1;
    ep.o2=t2;
    ep.ret=FALSE;
    extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_eq, &ep);
    return ep.ret;
}


/*}}}*/


/*{{{ Table/get */


typedef struct{
    ExtlTab ref;
    char type;
    char itype;
    va_list *argsp;
} TableParams2;


static bool extl_table_dodo_get2(lua_State *st, TableParams2 *params)
{
    if(params->ref<0)
        return FALSE;

    lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
    extl_stack_push_vararg(st, params->itype, params->argsp);
    lua_gettable(st, -2);
    if(lua_isnil(st, -1))
        return FALSE;

    return extl_stack_get(st, -1, params->type, TRUE, NULL,
                          va_arg(*(params->argsp), void*));
}


bool extl_table_get_vararg(ExtlTab ref, char itype, char type, va_list *args)
{
    TableParams2 params;

    params.ref=ref;
    params.itype=itype;
    params.type=type;
    params.argsp=args;

    return extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_dodo_get2, &params);
}


bool extl_table_get(ExtlTab ref, char itype, char type, ...)
{
    va_list args;
    bool retval;

    va_start(args, type);
    retval=extl_table_get_vararg(ref, itype, type, &args);
    va_end(args);

    return retval;
}


static bool extl_table_do_gets(ExtlTab ref, const char *entry,
                               char type, void *valret)
{
    return extl_table_get(ref, 's', type, entry, valret);
}

bool extl_table_gets_a(ExtlTab ref, const char *entry, ExtlAny *ret)
{
    return extl_table_do_gets(ref, entry, 'a', (void*)ret);
}

bool extl_table_gets_o(ExtlTab ref, const char *entry, Obj **ret)
{
    return extl_table_do_gets(ref, entry, 'o', (void*)ret);
}

bool extl_table_gets_i(ExtlTab ref, const char *entry, int *ret)
{
    return extl_table_do_gets(ref, entry, 'i', (void*)ret);
}

bool extl_table_gets_d(ExtlTab ref, const char *entry, double *ret)
{
    return extl_table_do_gets(ref, entry, 'd', (void*)ret);
}

bool extl_table_gets_b(ExtlTab ref, const char *entry, bool *ret)
{
    return extl_table_do_gets(ref, entry, 'b', (void*)ret);
}

bool extl_table_gets_s(ExtlTab ref, const char *entry, char **ret)
{
    return extl_table_do_gets(ref, entry, 's', (void*)ret);
}

bool extl_table_gets_f(ExtlTab ref, const char *entry, ExtlFn *ret)
{
    return extl_table_do_gets(ref, entry, 'f', (void*)ret);
}

bool extl_table_gets_t(ExtlTab ref, const char *entry, ExtlTab *ret)
{
    return extl_table_do_gets(ref, entry, 't', (void*)ret);
}


static bool extl_table_do_geti(ExtlTab ref, int entry, char type, void *valret)
{
    return extl_table_get(ref, 'i', type, entry, valret);
}

bool extl_table_geti_a(ExtlTab ref, int entry, ExtlAny *ret)
{
    return extl_table_do_geti(ref, entry, 'a', (void*)ret);
}

bool extl_table_geti_o(ExtlTab ref, int entry, Obj **ret)
{
    return extl_table_do_geti(ref, entry, 'o', (void*)ret);
}

bool extl_table_geti_i(ExtlTab ref, int entry, int *ret)
{
    return extl_table_do_geti(ref, entry, 'i', (void*)ret);
}

bool extl_table_geti_d(ExtlTab ref, int entry, double *ret)
{
    return extl_table_do_geti(ref, entry, 'd', (void*)ret);
}

bool extl_table_geti_b(ExtlTab ref, int entry, bool *ret)
{
    return extl_table_do_geti(ref, entry, 'b', (void*)ret);
}

bool extl_table_geti_s(ExtlTab ref, int entry, char **ret)
{
    return extl_table_do_geti(ref, entry, 's', (void*)ret);
}

bool extl_table_geti_f(ExtlTab ref, int entry, ExtlFn *ret)
{
    return extl_table_do_geti(ref, entry, 'f', (void*)ret);
}

bool extl_table_geti_t(ExtlTab ref, int entry, ExtlTab *ret)
{
    return extl_table_do_geti(ref, entry, 't', (void*)ret);
}


typedef struct{
    int ref;
    int n;
} GetNParams;


static bool extl_table_do_get_n(lua_State *st, GetNParams *params)
{
    lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
    params->n=lua_objlen_check(st, -1);
    return TRUE;
}


int extl_table_get_n(ExtlTab ref)
{
    GetNParams params;

    params.ref=ref;
    params.n=0;

    extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_do_get_n, &params);

    return params.n;
}


/*}}}*/


/*{{{ Table/set */


static bool extl_table_dodo_set2(lua_State *st, TableParams2 *params)
{
    lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
    extl_stack_push_vararg(st, params->itype, params->argsp);
    extl_stack_push_vararg(st, params->type, params->argsp);
    lua_rawset_check(st, -3);
    return TRUE;
}


bool extl_table_set_vararg(ExtlTab ref, char itype, char type, va_list *args)
{
    TableParams2 params;

    params.ref=ref;
    params.itype=itype;
    params.type=type;
    params.argsp=args;

    return extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_dodo_set2, &params);
}


bool extl_table_set(ExtlTab ref, char itype, char type, ...)
{
    va_list args;
    bool retval;

    va_start(args, type);
    retval=extl_table_set_vararg(ref, itype, type, &args);
    va_end(args);

    return retval;
}

bool extl_table_sets_a(ExtlTab ref, const char *entry, const ExtlAny *val)
{
    return extl_table_set(ref, 's', 'a', entry, val);
}

bool extl_table_sets_o(ExtlTab ref, const char *entry, Obj *val)
{
    return extl_table_set(ref, 's', 'o', entry, val);
}

bool extl_table_sets_i(ExtlTab ref, const char *entry, int val)
{
    return extl_table_set(ref, 's', 'i', entry, val);
}

bool extl_table_sets_d(ExtlTab ref, const char *entry, double val)
{
    return extl_table_set(ref, 's', 'd', entry, val);
}

bool extl_table_sets_b(ExtlTab ref, const char *entry, bool val)
{
    return extl_table_set(ref, 's', 'b', entry, val);
}

bool extl_table_sets_s(ExtlTab ref, const char *entry, const char *val)
{
    return extl_table_set(ref, 's', 'S', entry, val);
}

bool extl_table_sets_f(ExtlTab ref, const char *entry, ExtlFn val)
{
    return extl_table_set(ref, 's', 'f', entry, val);
}

bool extl_table_sets_t(ExtlTab ref, const char *entry, ExtlTab val)
{
    return extl_table_set(ref, 's', 't', entry, val);
}


bool extl_table_seti_a(ExtlTab ref, int entry, const ExtlAny *val)
{
    return extl_table_set(ref, 'i', 'a', entry, val);
}

bool extl_table_seti_o(ExtlTab ref, int entry, Obj *val)
{
    return extl_table_set(ref, 'i', 'o', entry, val);
}

bool extl_table_seti_i(ExtlTab ref, int entry, int val)
{
    return extl_table_set(ref, 'i', 'i', entry, val);
}

bool extl_table_seti_d(ExtlTab ref, int entry, double val)
{
    return extl_table_set(ref, 'i', 'd', entry, val);
}

bool extl_table_seti_b(ExtlTab ref, int entry, bool val)
{
    return extl_table_set(ref, 'i', 'b', entry, val);
}

bool extl_table_seti_s(ExtlTab ref, int entry, const char *val)
{
    return extl_table_set(ref, 'i', 'S', entry, val);
}

bool extl_table_seti_f(ExtlTab ref, int entry, ExtlFn val)
{
    return extl_table_set(ref, 'i', 'f', entry, val);
}

bool extl_table_seti_t(ExtlTab ref, int entry, ExtlTab val)
{
    return extl_table_set(ref, 'i', 't', entry, val);
}


/*}}}*/


/*{{{ Table/clear entry */


static bool extl_table_dodo_clear2(lua_State *st, TableParams2 *params)
{
    lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
    extl_stack_push_vararg(st, params->itype, params->argsp);
    lua_pushnil(st);
    lua_rawset_check(st, -3);
    return TRUE;
}

bool extl_table_clear_vararg(ExtlTab ref, char itype, va_list *args)
{
    TableParams2 params;

    params.ref=ref;
    params.itype=itype;
    /*params.type='?';*/
    params.argsp=args;

    return extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_dodo_clear2, &params);
}

bool extl_table_clear(ExtlTab ref, char itype, ...)
{
    va_list args;
    bool retval;

    va_start(args, itype);
    retval=extl_table_clear_vararg(ref, itype, &args);
    va_end(args);

    return retval;
}


bool extl_table_clears(ExtlTab ref, const char *entry)
{
    return extl_table_clear(ref, 's', entry);
}

bool extl_table_cleari(ExtlTab ref, int entry)
{
    return extl_table_clear(ref, 'i', entry);
}



/*}}}*/


/*{{{ Table iteration */


typedef struct{
    ExtlTab ref;
    ExtlIterFn *fn;
    void *d;
} IterP;


int extl_table_iter_do(lua_State *st, IterP *par)
{
    lua_rawgeti(st, LUA_REGISTRYINDEX, par->ref);

    lua_pushnil(st);

    while(lua_next(st, -2)!=0){
        ExtlAny k, v;

        if(extl_stack_get(st, -2, 'a', FALSE, NULL, &k)){
            bool ret=TRUE;
            if(extl_stack_get(st, -1, 'a', FALSE, NULL, &v)){
                ret=par->fn(k, v, par->d);
                extl_any_free(&v, STRINGS_NONE);
            }
            extl_any_free(&k, STRINGS_NONE);
            if(!ret)
                return 0;
        }

        lua_pop(st, 1);
    }

    return 0;
}


void extl_table_iter(ExtlTab ref, ExtlIterFn *fn, void *d)
{
    IterP par;

    par.ref=ref;
    par.fn=fn;
    par.d=d;

    extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_iter_do, &par);
}


/*}}}*/


/*{{{ Function calls to Lua */


static bool extl_push_args(lua_State *st, const char *spec, va_list *argsp)
{
    int i=1;

    while(*spec!='\0'){
        if(!extl_stack_push_vararg(st, *spec, argsp))
            return FALSE;
        i++;
        spec++;
    }

    return TRUE;
}


typedef struct{
    const char *spec;
    const char *rspec;
    va_list *args;
    void *misc;
    int nret;
#ifndef CF_HAS_VA_COPY
    void *ret_ptrs[MAX_PARAMS];
#endif
} ExtlDoCallParam;


static bool extl_get_retvals(lua_State *st, int m, ExtlDoCallParam *param)
{
    void *ptr;
    const char *spec=param->rspec;

#ifdef CF_HAS_VA_COPY
    va_list args;
    va_copy(args, *(param->args));
#else
    if(m>MAX_PARAMS){
        extl_warn(TR("Too many return values. Use a C compiler that has "
                     "va_copy to support more."));
        return FALSE;
    }
#endif

    while(m>0){
        bool dead=FALSE;
#ifdef CF_HAS_VA_COPY
        ptr=va_arg(args, void*);
#else
        ptr=va_arg(*(param->args), void*);
        param->ret_ptrs[param->nret]=ptr;
#endif
        if(!extl_stack_get(st, -m, *spec, TRUE, &dead, ptr)){
            /* This is the only place where we allow nil-objects */
            /*if(*spec=='o' && lua_isnil(st, -m)){
                *(Obj**)ptr=NULL;
            }else*/
            if(dead){
                extl_warn(TR("Returned dead object."));
                return FALSE;
            }else{
                extl_warn(TR("Invalid return value (expected '%c', "
                             "got lua type \"%s\")."),
                     *spec, lua_typename(st, lua_type(st, -m)));
                return FALSE;
            }
        }

        (param->nret)++;
        spec++;
        m--;
    }

#ifdef CF_HAS_VA_COPY
    va_end(args);
#endif

    return TRUE;
}


/* The function to be called is expected on the top of stack st.
 * This function should be cpcalled through extl_cpcall_call (below), which
 * will take care that we don't leak anything in case of error.
 */
static bool extl_dodo_call_vararg(lua_State *st, ExtlDoCallParam *param)
{
    int n=0, m=0;

    if(lua_isnil(st, -1))
        return FALSE;

    if(param->spec!=NULL)
        n=strlen(param->spec);

    if(!lua_checkstack(st, n+8)){
        extl_warn(TR("Stack full."));
        return FALSE;
    }

    if(n>0){
        if(!extl_push_args(st, param->spec, param->args))
            return FALSE;
    }

    if(param->rspec!=NULL)
        m=strlen(param->rspec);

    flushtrace();

    if(lua_pcall(st, n, m, 0)!=0){
        extl_warn("%s", lua_tostring(st, -1));
        return FALSE;
    }

    if(m>0)
        return extl_get_retvals(st, m, param);

    return TRUE;
}


static bool extl_cpcall_call(lua_State *st, ExtlCPCallFn *fn,
                             ExtlDoCallParam *param)
{
    void *ptr;
    int i;

    param->nret=0;

    if(extl_cpcall(st, fn, param))
        return TRUE;

    /* If param.nret>0, there was an error getting some return value and
     * we must free what we got.
     */

    for(i=0; i<param->nret; i++){
#ifdef CF_HAS_VA_COPY
        ptr=va_arg(*(param->args), void*);
#else
        ptr=param->ret_ptrs[i];
#endif
        extl_free(ptr, *(param->rspec+i), STRINGS_ALL);
    }

    return FALSE;
}


static bool extl_do_call_vararg(lua_State *st, ExtlDoCallParam *param)
{
    if(!extl_getref(st, *(ExtlFn*)(param->misc)))
        return FALSE;
    return extl_dodo_call_vararg(st, param);
}


bool extl_call_vararg(ExtlFn fnref, const char *spec,
                      const char *rspec, va_list *args)
{
    ExtlDoCallParam param;

    if(fnref==LUA_NOREF || fnref==LUA_REFNIL)
        return FALSE;

    param.spec=spec;
    param.rspec=rspec;
    param.args=args;
    param.misc=(void*)&fnref;

    return extl_cpcall_call(l_st, (ExtlCPCallFn*)extl_do_call_vararg, &param);
}


bool extl_call(ExtlFn fnref, const char *spec, const char *rspec, ...)
{
    bool retval;
    va_list args;

    va_start(args, rspec);
    retval=extl_call_vararg(fnref, spec, rspec, &args);
    va_end(args);

    return retval;
}


/*}}}*/


/*{{{ extl_loadfile/string */


/**
 * Expects
 * - a stack with only the parameters to be passed to the function
 * - the function to call as an upvalue
 * Performs
 * - execute the function
 * Returns
 * - the number of return values
 */
static int call_loaded(lua_State *st)
{
    int nargs=lua_gettop(st);

    /* Get the loaded file/string as function */
    lua_pushvalue(st, lua_upvalueindex(1));
    lua_insert(st, 1);

    lua_call(st, nargs, LUA_MULTRET);
    return lua_gettop(st);
}


typedef struct{
    const char *src;
    bool isfile;
    ExtlFn *resptr;
} ExtlLoadParam;


/**
 * Stores a c closure in param->resptr containing a call to call_loaded with
 * as (fixed) parameter the function loaded from the file/buffer in param->src
 */
static bool extl_do_load(lua_State *st, ExtlLoadParam *param)
{
    int res=0;

    if(param->isfile){
        res=luaL_loadfile(st, param->src);
    }else{
        res=luaL_loadbuffer(st, param->src, strlen(param->src), param->src);
    }

    if(res!=0){
        extl_warn("%s", lua_tostring(st, -1));
        return FALSE;
    }

    lua_pushcclosure(st, call_loaded, 1);
    *(param->resptr)=luaL_ref(st, LUA_REGISTRYINDEX);

    return TRUE;
}


bool extl_loadfile(const char *file, ExtlFn *ret)
{
    ExtlLoadParam param;
    param.src=file;
    param.isfile=TRUE;
    param.resptr=ret;

    return extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_load, &param);
}


bool extl_loadstring(const char *str, ExtlFn *ret)
{
    ExtlLoadParam param;
    param.src=str;
    param.isfile=FALSE;
    param.resptr=ret;

    return extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_load, &param);
}


/*}}}*/


/*{{{ L1 CH error logging */

#ifdef EXTL_LOG_ERRORS

INTRSTRUCT(WarnChain);
DECLSTRUCT(WarnChain){
    bool need_trace;
    lua_State *st;
    WarnHandler *old_handler;
    WarnChain *prev;
};


static WarnChain *warnchain=NULL;
static int notrace=0;


static void l1_warn_handler(const char *message)
{
    WarnChain *ch=warnchain;
    static int called=0;

    assert(warnchain!=NULL);

    if(called==0 && notrace==0)
        ch->need_trace=TRUE;

    called++;
    warnchain=ch->prev;
    ch->old_handler(message);
    warnchain=ch;
    called--;
}


static void do_trace(WarnChain *ch)
{
    const char *p;

    if(notrace!=0)
        return;

    extl_stack_trace(ch->st);
    p=lua_tostring(ch->st, -1);
    notrace++;
    extl_warn(p);
    notrace--;
    ch->need_trace=FALSE;
    lua_pop(ch->st, 1);
}

static void flushtrace()
{
    if(warnchain && warnchain->need_trace)
        do_trace(warnchain);
}

#endif

/*}}}*/


/*{{{ L1-CH safe functions */


static int protect_count=0;
static ExtlSafelist *safelists=NULL;


void extl_protect(ExtlSafelist *l)
{
    protect_count++;
    if(l!=NULL){
        if(l->count==0){
            LINK_ITEM(safelists, l, next, prev);
        }
        l->count++;
    }
}


void extl_unprotect(ExtlSafelist *l)
{
    assert(protect_count>0);
    protect_count--;

    if(l!=NULL){
        assert(l->count>0);
        l->count--;
        if(l->count==0){
            UNLINK_ITEM(safelists, l, next, prev);
        }
    }
}


static bool extl_check_protected(ExtlExportedFnSpec *spec)
{
    ExtlSafelist *l;
    bool ok=FALSE;
    int j;

    if(protect_count>0 && !spec->safe){
        for(l=safelists; l!=NULL; l=l->next){
            ok=TRUE;
            for(j=0; l->list[j]!=NULL; j++){
                if(l->list[j]==spec->fn)
                    break;
            }
            if(l->list[j]==NULL){
                ok=FALSE;
                break;
            }
        }
    }else{
        ok=TRUE;
    }

    return ok;
}


/*}}}*/


/*{{{ L1 call handler */

/* To get around potential memory leaks and corruption that could be caused
 * by Lua's longjmp-on-error lameness, The L1 call handler is divided into
 * two steps. In the first step we first setup a call to the second step.
 * At this point it is still fine if Lua raises an error. Then we set up
 * our warning handlers and stuff--at which point Lua's raising an error
 * would corrupt our data--and finally call the second step with lua_pcall.
 * Now the second step can safely call Lua's functions and do what is needed.
 * When the second step returns, we deallocate our data in the L1Param
 * structure that was passed to the second step and reset warning handlers.
 * After that it is again safe to call Lua's functions.
 */

typedef struct{
    ExtlL2Param ip[MAX_PARAMS];
    ExtlL2Param op[MAX_PARAMS];
    ExtlExportedFnSpec *spec;
    int ii, ni,  no;
} L1Param;

static L1Param *current_param=NULL;


static int extl_l1_call_handler2(lua_State *st)
{
    L1Param *param=current_param;
    ExtlExportedFnSpec *spec=param->spec;
    int i;

    D(fprintf(stderr, "%s called\n", spec->name));

    if(!lua_checkstack(st, MAX_PARAMS+1)){
        extl_warn(TR("Stack full."));
        return 0;
    }

    param->ni=(spec->ispec==NULL ? 0 : strlen(spec->ispec));

    for(i=0; i<param->ni; i++){
        bool dead=FALSE;
        if(!extl_stack_get(st, i+1, spec->ispec[i], FALSE, &dead,
                           (void*)&(param->ip[i]))){
            if(dead){
                extl_warn(TR("Argument %d to %s is a dead object."),
                          i+1, spec->name);
            }else{
                extl_warn(TR("Argument %d to %s is of invalid type. "
                             "(Argument template is '%s', got lua type %s)."),
                          i+1, spec->name, spec->ispec,
                          lua_typename(st, lua_type(st, i+1)));
            }
            return 0;
        }

        param->ii=i+1;
    }

    if(spec->untraced)
        notrace++;

    if(!spec->l2handler(spec->fn, param->ip, param->op))
        return 0;

    if(spec->untraced)
        notrace--;

    param->no=(spec->ospec==NULL ? 0 : strlen(spec->ospec));

    for(i=0; i<param->no; i++)
        extl_stack_push(st, spec->ospec[i], (void*)&(param->op[i]));

    return param->no;
}


static void extl_l1_finalize(L1Param *param)
{
    ExtlExportedFnSpec *spec=param->spec;
    int i;

    for(i=0; i<param->ii; i++)
        extl_free((void*)&(param->ip[i]), spec->ispec[i], STRINGS_NONE);

    for(i=0; i<param->no; i++)
        extl_free((void*)&(param->op[i]), spec->ospec[i], STRINGS_NONCONST);
}



static bool extl_l1_just_check_protected=FALSE;


static int extl_l1_call_handler(lua_State *st)
{
#ifdef EXTL_LOG_ERRORS
    WarnChain ch;
#endif
    L1Param param={{NULL, }, {NULL, }, NULL, 0, 0, 0};
    L1Param *old_param;
    int ret;
    int n=lua_gettop(st);


    /* Get the info we need on the function, check it's ok, and then set
     * up a safe environment for extl_l1_call_handler2.
     */
    param.spec=(ExtlExportedFnSpec*)lua_touserdata(st, lua_upvalueindex(1));

    if(param.spec==NULL){
        extl_warn(TR("L1 call handler upvalues corrupt."));
        return 0;
    }

    if(!param.spec->registered){
        extl_warn(TR("Called function has been unregistered."));
        return 0;
    }

    if(extl_l1_just_check_protected){
        /* Just checking whether the function may be called. */
        lua_pushboolean(st, !extl_check_protected(param.spec));
        return 1;
    }

    if(!extl_check_protected(param.spec)){
        extl_warn(TR("Ignoring call to unsafe function \"%s\" in "
                     "restricted mode."), param.spec->name);
        return 0;
    }


    lua_pushcfunction(st, extl_l1_call_handler2);
    lua_insert(st, 1);

    old_param=current_param;
    current_param=&param;

#ifdef EXTL_LOG_ERRORS
    ch.old_handler=set_warn_handler(l1_warn_handler);
    ch.need_trace=FALSE;
    ch.st=st;
    ch.prev=warnchain;
    warnchain=&ch;
#endif

    /* Ok, Lua may now freely fail in extl_l1_call_handler2, we can handle
     * that.
     */
    ret=lua_pcall(st, n, LUA_MULTRET, 0);

    /* Now that the actual call handler has returned, we need to free
     * any of our data before calling Lua again.
     */
    current_param=old_param;
    extl_l1_finalize(&param);

#ifdef EXTL_LOG_ERRORS
    warnchain=ch.prev;
    set_warn_handler(ch.old_handler);

    /* Ok, we can now safely use Lua functions again without fear of
     * leaking.
     */
    if(ret!=0){
        const char *p;
        param.no=0;
        p=lua_tostring(st, -1);
        notrace++;
        extl_warn("%s", p);
        notrace--;
    }

    if(ret!=0 || ch.need_trace)
        do_trace(&ch);
#else
    if(ret!=0)
        lua_error(st);
#endif

    return param.no;
}


/*EXTL_DOC
 * Is calling the function \var{fn} not allowed now? If \var{fn} is nil,
 * tells if some functions are not allowed to be called now due to
 * protected mode.
 */
EXTL_EXPORT_AS(global, protected)
bool __protected(ExtlFn fn);

static int extl_protected(lua_State *st)
{
    int ret;

    if(lua_isnil(st, 1)){
        lua_pushboolean(st, protect_count>0);
        return 1;
    }

    if(!lua_isfunction(st, 1)){
        lua_pushboolean(st, TRUE);
        return 1;
    }

    if(lua_tocfunction(st, 1)!=(lua_CFunction)extl_l1_call_handler){
        lua_pushboolean(st, FALSE);
        return 1;
    }

    extl_l1_just_check_protected=TRUE;
    ret=lua_pcall(st, 0, 1, 0);
    extl_l1_just_check_protected=FALSE;
    if(ret!=0)
        lua_pushboolean(st, TRUE);
    return 1;
}

/*}}}*/


/*{{{ Function registration */


typedef struct{
    ExtlExportedFnSpec *spec;
    const char *cls;
    ExtlTab table;
} RegData;


static bool extl_do_register_function(lua_State *st, RegData *data)
{
    ExtlExportedFnSpec *spec=data->spec;
#if LUA_VERSION_NUM>=502
    int ind;
#else
    int ind=LUA_GLOBALSINDEX;
#endif

    if((spec->ispec!=NULL && strlen(spec->ispec)>MAX_PARAMS) ||
       (spec->ospec!=NULL && strlen(spec->ospec)>MAX_PARAMS)){
        extl_warn(TR("Function '%s' has more parameters than the level 1 "
                     "call handler can handle"), spec->name);
        return FALSE;
    }

    if(data->table!=LUA_NOREF){
        lua_rawgeti(st, LUA_REGISTRYINDEX, data->table);
        ind=-3;
    }
#if LUA_VERSION_NUM>=502
    else{
        lua_pushglobaltable(st);
        ind=-3;
    }
#endif

    lua_pushstring(st, spec->name);

    lua_pushlightuserdata(st, spec);
    lua_pushcclosure(st, extl_l1_call_handler, 1);

    lua_rawset_check(st, ind);

    return TRUE;
}


static bool extl_do_register_functions(ExtlExportedFnSpec *spec, int max,
                                       const char *cls, int table)
{
    int i;

    RegData regdata;
    regdata.spec=spec;
    regdata.cls=cls;
    regdata.table=table;

    for(i=0; spec[i].name && i<max; i++){
        regdata.spec=&(spec[i]);
        if(!extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_register_function,
                        &regdata)){
            return FALSE;
        }
        spec[i].registered=TRUE;
    }

    return TRUE;
}


bool extl_register_function(ExtlExportedFnSpec *spec)
{
    return extl_do_register_functions(spec, 1, "", LUA_NOREF);
}


bool extl_register_functions(ExtlExportedFnSpec *spec)
{
    return extl_do_register_functions(spec, INT_MAX, "", LUA_NOREF);
}


static bool extl_do_unregister_function(lua_State *st, RegData *data)
{
    ExtlExportedFnSpec *spec=data->spec;
#if LUA_VERSION_NUM>=502
    int ind;
#else
    int ind=LUA_GLOBALSINDEX;
#endif

    if(data->table!=LUA_NOREF){
        lua_rawgeti(st, LUA_REGISTRYINDEX, data->table);
        ind=-3;
    }
#if LUA_VERSION_NUM>=502
    else{
        lua_pushglobaltable(st);
        ind=-3;
    }
#endif

    /* Clear table.fn */
    lua_pushstring(st, spec->name);
    lua_pushnil(st);
    lua_rawset_check(st, ind);

    return TRUE;
}


static void extl_do_unregister_functions(ExtlExportedFnSpec *spec, int max,
                                         const char *cls, int table)
{
    int i;

    RegData regdata;
    regdata.spec=spec;
    regdata.cls=cls;
    regdata.table=table;

    for(i=0; spec[i].name && i<max; i++){
        regdata.spec=&(spec[i]);
        extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_unregister_function,
                    &regdata);
        spec[i].registered=FALSE;
    }
}

void extl_unregister_function(ExtlExportedFnSpec *spec)
{
    extl_do_unregister_functions(spec, 1, "", LUA_NOREF);
}


void extl_unregister_functions(ExtlExportedFnSpec *spec)
{
    extl_do_unregister_functions(spec, INT_MAX, "", LUA_NOREF);
}


/*}}}*/


/*{{{ Class registration */


typedef struct{
    const char *cls, *parent;
    int refret;
    bool hide;
} ClassData;


static bool extl_do_register_class(lua_State *st, ClassData *data)
{
    /* Create the globally visible WFoobar table in which the function
     * references reside.
     */
    lua_newtable(st);

    /* Set type information.
     */
    lua_pushstring(st, "__typename");
    lua_pushstring(st, data->cls);
    lua_settable(st, -3);

    /* If we have a parent class (i.e. class!=Obj), we want also the parent's
     * functions visible in this table so set up a metatable to do so.
     */
    if(data->parent!=NULL){
        /* Get luaextl_ParentClass_metatable */
        lua_pushfstring(st, "luaextl_%s_metatable", data->parent);
        lua_gettable(st, LUA_REGISTRYINDEX);
        if(!lua_istable(st, -1)){
            extl_warn("Could not find metatable for parent class %s of %s.\n",
                      data->parent, data->cls);
            return FALSE;
        }
        /* Create our metatable */
        lua_newtable(st);
        /* Get parent_metatable.__index */
        lua_pushstring(st, "__index");
        lua_pushvalue(st, -1);
        /* Stack: cls, parent_meta, meta, "__index", "__index" */
        lua_gettable(st, -4);
        /* Stack: cls, parent_meta, meta, "__index", parent_meta.__index */
        lua_pushvalue(st, -1);
        lua_insert(st, -3);
        /* Stack: cls, parent_meta, meta, parent_meta.__index, "__index", parent_meta.__index */
        lua_rawset_check(st, -4);
        /* Stack: cls, parent_meta, meta, parent_meta.__index */
        lua_pushstring(st, "__parentclass");
        lua_insert(st, -2);
        /* Stack: cls, parent_meta, meta, "__parentclass", parent_meta.__index */
        lua_settable(st, -5);
        /* Stack: cls, parent_meta, meta, */
        lua_setmetatable(st, -3);
        lua_pop(st, 1);
        /* Stack: cls */
    }

    /* Set the global WFoobar */
    lua_pushvalue(st, -1);
    data->refret=luaL_ref(st, LUA_REGISTRYINDEX); /* TODO: free on failure */
    if(!data->hide){
#if LUA_VERSION_NUM>=502
        lua_pushglobaltable(st);
        lua_pushstring(st, data->cls);
        lua_pushvalue(st, -3);
        lua_rawset(st, -3);
        lua_remove(st, -1);
#else
        lua_pushstring(st, data->cls);
        lua_pushvalue(st, -2);
        lua_rawset(st, LUA_GLOBALSINDEX);
#endif
    }

    /* New we create a metatable for the actual objects with __gc metamethod
     * and __index pointing to the table created above. The MAGIC entry is
     * used to check that userdatas passed to us really are Watches with a
     * high likelihood.
     */
    lua_newtable(st);

    lua_pushnumber(st, MAGIC);
    lua_pushnumber(st, MAGIC);
    lua_rawset_check(st, -3);

    lua_pushstring(st, "__index");
    lua_pushvalue(st, -3);
    lua_rawset_check(st, -3); /* set metatable.__index=WFoobar created above */
    lua_pushstring(st, "__gc");
    lua_pushcfunction(st, extl_obj_gc_handler);
    lua_rawset_check(st, -3); /* set metatable.__gc=extl_obj_gc_handler */
    lua_pushfstring(st, "luaextl_%s_metatable", data->cls);
    lua_insert(st, -2);
    lua_rawset(st, LUA_REGISTRYINDEX);

    return TRUE;
}


bool extl_register_class(const char *cls, ExtlExportedFnSpec *fns,
                         const char *parent)
{
    ClassData clsdata;
    clsdata.cls=cls;
    clsdata.parent=parent;
    clsdata.refret=LUA_NOREF;
    clsdata.hide=(strcmp(cls, "Obj")==0);/*(fns==NULL);*/

    D(assert(strcmp(cls, "Obj")==0 || parent!=NULL));

    if(!extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_register_class, &clsdata))
        return FALSE;

    if(fns==NULL)
        return TRUE;

    return extl_do_register_functions(fns, INT_MAX, cls, clsdata.refret);
}


static void extl_do_unregister_class(lua_State *st, ClassData *data)
{
    /* Get reference from registry to the metatable. */
    lua_pushfstring(st, "luaextl_%s_metatable", data->cls);
    lua_pushvalue(st, -1);
    lua_gettable(st, LUA_REGISTRYINDEX);
    /* Get __index and return it for resetting the functions. */
    lua_pushstring(st, "__index");
    lua_gettable(st, -2);
    data->refret=luaL_ref(st, LUA_REGISTRYINDEX);
    lua_pop(st, 1);
    /* Set the entry from registry to nil. */
    lua_pushnil(st);
    lua_rawset(st, LUA_REGISTRYINDEX);

    /* Reset the global reference to the class to nil. */
#if LUA_VERSION_NUM>=502
    lua_pushglobaltable(st);
    lua_pushstring(st, data->cls);
    lua_pushnil(st);
    lua_rawset(st, -3);
    lua_remove(st, -1);
#else
    lua_pushstring(st, data->cls);
    lua_pushnil(st);
    lua_rawset(st, LUA_GLOBALSINDEX);
#endif
}


void extl_unregister_class(const char *cls, ExtlExportedFnSpec *fns)
{
    ClassData clsdata;
    clsdata.cls=cls;
    clsdata.parent=NULL;
    clsdata.refret=LUA_NOREF;
    clsdata.hide=FALSE; /* unused, but initialise */

    if(!extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_unregister_class,
                    &clsdata))
        return;

    /* We still need to reset function upvalues. */
    if(fns!=NULL)
        extl_do_unregister_functions(fns, INT_MAX, cls, clsdata.refret);
}


/*}}}*/


/*{{{ Module registration */


static bool extl_do_register_module(lua_State *st, ClassData *clsdata)
{
    lua_getglobal(st, clsdata->cls);

    if(!lua_istable(st, -1)){
        lua_newtable(st);
        lua_pushvalue(st, -1);
        lua_setglobal(st, clsdata->cls);
    }
    lua_pushfstring(st, "luaextl_module_%s", clsdata->cls);
    lua_pushvalue(st, -2);
    lua_rawset(st, LUA_REGISTRYINDEX);

    clsdata->refret=luaL_ref(st, LUA_REGISTRYINDEX);

    return TRUE;
}


bool extl_register_module(const char *mdl, ExtlExportedFnSpec *fns)
{
    ClassData clsdata;

    clsdata.cls=mdl;
    clsdata.parent=NULL;
    clsdata.refret=LUA_NOREF;
    clsdata.hide=FALSE; /* unused, but initialise */

    if(!extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_register_module, &clsdata))
        return FALSE;

    if(fns==NULL)
        return TRUE;

    return extl_do_register_functions(fns, INT_MAX, mdl, clsdata.refret);
}


static bool extl_do_unregister_module(lua_State *st, ClassData *clsdata)
{
    lua_pushfstring(st, "luaextl_module_%s", clsdata->cls);
    lua_pushvalue(st, -1);
    lua_pushnil(st);
    lua_rawset(st, LUA_REGISTRYINDEX);
    clsdata->refret=luaL_ref(st, LUA_REGISTRYINDEX);

    return TRUE;
}


void extl_unregister_module(const char *mdl, ExtlExportedFnSpec *fns)
{
    ClassData clsdata;

    clsdata.cls=mdl;
    clsdata.parent=NULL;
    clsdata.refret=LUA_NOREF;
    clsdata.hide=FALSE; /* unused, but initialise */

    if(!extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_unregister_module, &clsdata))
        return;

    if(fns!=NULL)
        extl_do_unregister_functions(fns, INT_MAX, mdl, clsdata.refret);
}


/*}}}*/


/*{{{ Serialise */

typedef struct{
    FILE *f;
    ExtlTab tab;
} SerData;


static void write_escaped_string(FILE *f, const char *str)
{
    fputc('"', f);

    while(str && *str){
        if(((*str)&0x7f)<32 || *str=='"' || *str=='\\'){
            /* Lua uses decimal in escapes */
            fprintf(f, "\\%03d", (int)(uchar)(*str));
        }else{
            fputc(*str, f);
        }
        str++;
    }

    fputc('"', f);
}


static void indent(FILE *f, int lvl)
{
    int i;
    for(i=0; i<lvl; i++)
        fprintf(f, "    ");
}


static bool ser(lua_State *st, FILE *f, int lvl)
{

    lua_checkstack(st, 5);

    switch(lua_type(st, -1)){
    case LUA_TBOOLEAN:
        fprintf(f, "%s", lua_toboolean(st, -1) ? "true" : "false");
        break;
    case LUA_TNUMBER:
        fprintf(f, "%s", lua_tostring(st, -1));
        break;
    case LUA_TNIL:
        fprintf(f, "nil");
        break;
    case LUA_TSTRING:
        write_escaped_string(f, lua_tostring(st, -1));
        break;
    case LUA_TTABLE:
        if(lvl+1>=EXTL_MAX_SERIALISE_DEPTH){
            extl_warn(TR("Maximal serialisation depth reached."));
            fprintf(f, "nil");
            lua_pop(st, 1);
            return FALSE;
        }

        fprintf(f, "{\n");
        lua_pushnil(st);
        while(lua_next(st, -2)!=0){
            lua_pushvalue(st, -2);
            indent(f, lvl+1);
            fprintf(f, "[");
            ser(st, f, lvl+1);
            fprintf(f, "] = ");
            ser(st, f, lvl+1);
            fprintf(f, ",\n");
        }
        indent(f, lvl);
        fprintf(f, "}");
        break;
    default:
        extl_warn(TR("Unable to serialize type %s."),
                  lua_typename(st, lua_type(st, -1)));
    }
    lua_pop(st, 1);
    return TRUE;
}


static bool extl_do_serialize(lua_State *st, SerData *d)
{
    if(!extl_getref(st, d->tab))
        return FALSE;

    return ser(st, d->f, 0);
}


/* Tab must not contain recursive references! */
extern bool extl_serialize(const char *file, ExtlTab tab)
{
    SerData d;
    bool ret;
    int fd;
    char tmp_file[strlen(file) + 8];

    tmp_file[0] = '\0';
    strcat(tmp_file, file);
    strcat(tmp_file, ".XXXXXX");
    fd = mkstemp(tmp_file);
    if(fd == -1) {
        extl_warn_err_obj(tmp_file);
        return FALSE;
    }

    d.tab=tab;
    d.f=fdopen(fd, "w");

    if(d.f==NULL){
        extl_warn_err_obj(tmp_file);
        unlink(tmp_file);
        return FALSE;
    }

    fprintf(d.f, TR("-- This file has been generated by %s. Do not edit.\n"), libtu_progname());
    fprintf(d.f, "return ");

    ret=extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_serialize, &d);

    fprintf(d.f, "\n\n");

    fclose(d.f);

    if(ret && rename(tmp_file, file) != 0) {
        extl_warn_err_obj(file);
        ret = FALSE;
    }

    if(!ret) {
        unlink(tmp_file);
    }

    return ret;
}

void extl_dohook(lua_State *L, lua_Debug *ar)
{
    enum ExtlHookEvent event;

    lua_getinfo(L, "Sn", ar);
    if (ar->event == LUA_HOOKCALL)
        event = EXTL_HOOK_ENTER;
    else if (ar->event == LUA_HOOKRET)
        event = EXTL_HOOK_EXIT;
    else
        event = EXTL_HOOK_UNKNOWN;

    if (ar->source[0] == '@' && strcmp(ar->what, "Lua") == 0)
        (*current_hook) (event, ar->name, ar->source + 1, ar->linedefined);
}

void extl_sethook(ExtlHook hook)
{
    current_hook = hook;
    lua_sethook(l_st, extl_dohook, LUA_MASKCALL | LUA_MASKRET, -1);
}

void extl_resethook()
{
    lua_sethook(l_st, NULL, 0, -1);
}

/*}}}*/

