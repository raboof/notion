/*
 * ion/lua/luaextl.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <time.h>

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <libtu/util.h>
#include <libtu/obj.h>
#include <libtu/objp.h>
#include <libtu/errorlog.h>
#include <ioncore/common.h>
#include <ioncore/readconfig.h>

#include "luaextl.h"

#define MAGIC 0xf00ba7

/* Maximum number of parameters and return values for calls from Lua
 * and (if va_copy is not available) return value from Lua functions.
 */
#define MAX_PARAMS 16

static lua_State *l_st=NULL;

static bool extl_stack_get(lua_State *st, int pos, char type, bool copystring,
                           void *valret);

static void flushtrace();


/*{{{ Safer rawget/set/getn */


#define CHECK_TABLE(ST, INDEX) luaL_checktype(ST, INDEX, LUA_TTABLE)

static int luaL_getn_check(lua_State *st, int index)
{
    CHECK_TABLE(st, index);
    return luaL_getn(st, index);
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
        warn("Lua stack full");
        return 0;
    }

    param->retval=param->fn(st, param->udata);
    return 0;
}

                            
static bool extl_cpcall(lua_State *st, ExtlCPCallFn *fn, void *ptr)
{
    ExtlCPCallParam param;
    int oldtop=lua_gettop(st);
    
    param.fn=fn;
    param.udata=ptr;
    param.retval=FALSE;
    
    
    if(lua_cpcall(st, extl_docpcall, &param)!=0){
        D(fprintf(stderr, "-->%s\n", lua_tostring(st, -1)));
    }
    
    lua_settop(st, oldtop);
    
    return param.retval;
}


/*}}}*/


/*{{{ Obj userdata handling -- unsafe */


static int obj_cache_ref=LUA_NOREF;


static Obj *extl_get_wobj(lua_State *st, int pos)
{
    Watch *watch;
    int val;
    
    if(!lua_isuserdata(st, pos))
        return NULL;
    
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
        watch=(Watch*)lua_touserdata(st, pos);
        if(watch!=NULL && watch->obj!=NULL)
            return watch->obj;
    }

    return NULL;
}


static void extl_uncache(lua_State *st, Obj *obj)
{
    lua_pushlightuserdata(st, obj);
    lua_pushnil(st);
    lua_rawset_check(st, LUA_REGISTRYINDEX);
    obj->flags&=~OBJ_EXTL_CACHED;
}


static void extl_obj_dest_handler(Watch *watch, Obj *obj)
{
    if(obj->flags&OBJ_EXTL_CACHED)
        extl_cpcall(l_st, (ExtlCPCallFn*)extl_uncache, obj);
}


static void extl_push_obj(lua_State *st, Obj *obj)
{
    Watch *watch;
    
    if(obj==NULL){
        lua_pushnil(st);
        return;
    }

    if(obj->flags&OBJ_EXTL_CACHED){
        lua_pushlightuserdata(st, obj);
        lua_rawget(st, LUA_REGISTRYINDEX);
        if(lua_isuserdata(st, -1)){
            D(fprintf(stderr, "found %p cached\n", obj));
            return;
        }
    }

    D(fprintf(stderr, "Creating %p\n", obj));

    watch=(Watch*)lua_newuserdata(st, sizeof(Watch));
    
    /* Lua shouldn't return if the allocation fails */
    
    watch_init(watch);
    watch_setup(watch, obj, extl_obj_dest_handler);
    
    lua_pushfstring(st, "luaextl_%s_metatable", OBJ_TYPESTR(obj));
    lua_gettable(st, LUA_REGISTRYINDEX);
    if(lua_isnil(st, -1)){
        lua_pop(st, 2);
        lua_pushnil(st);
    }else{
        lua_setmetatable(st, -2);
        
        /* Store in cache */
        lua_pushlightuserdata(st, obj);
        lua_pushvalue(st, -2);
        lua_rawset_check(st, LUA_REGISTRYINDEX);
        obj->flags|=OBJ_EXTL_CACHED;
    }
}
    
    
/*{{{ Functions available to Lua code */


static int extl_obj_gc_handler(lua_State *st)
{
    Watch *watch;
    
    if(extl_get_wobj(st, 1)==NULL)
        return 0;

    /* This should not happen, actually. Our object cache should
     * hold references to all objects seen on the Lua side until
     * they are destroyed.
     */
    
    watch=(Watch*)lua_touserdata(st, 1);
    
    if(watch!=NULL){
        if(watch->obj!=NULL)
            watch->obj->flags&=~OBJ_EXTL_CACHED;
        watch_reset(watch);
    }
    
    return 0;
}


static int extl_obj_typename(lua_State *st)
{
    Obj *obj;

    if(!extl_stack_get(st, 1, 'o', FALSE, &obj))
        return 0;
    
    lua_pushstring(st, OBJ_TYPESTR(obj));
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
    Obj *obj;
    lua_pushboolean(st, extl_stack_get(st, 1, 'o', FALSE, &obj));
    return 1;
}

/* Dummy code for documentation generation. */

/*EXTL_DOC
 * Does \var{obj} still exist on the C side of Ion?
 */
EXTL_EXPORT_AS(global, obj_exists)
bool __obj_exists(Obj *obj);


static int extl_obj_is(lua_State *st)
{
    Obj *obj;
    const char *tn;
    
    if(!extl_stack_get(st, 1, 'o', FALSE, &obj)){
        lua_pushboolean(st, 0);
    }else{
        tn=lua_tostring(st, 2);
        lua_pushboolean(st, obj_is_str(obj, tn));
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
    int r;
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
    warn("Unable to get caller file from stack.");
    return 0;
}


static int extl_include(lua_State *st)
{
    bool res;
    const char *toincl, *cfdir;
    
    toincl=luaL_checkstring(st, 1);
    
    if(extl_current_file_or_dir(st, TRUE)!=1){
        res=ioncore_read_config(toincl, NULL, TRUE);
    }else{
        cfdir=lua_tostring(st, -1);
        res=ioncore_read_config(toincl, cfdir, TRUE);
        lua_pop(st, 1);
    }
    lua_pushboolean(st, res);
    return 1;
}

/* Dummy code for documentation generation. */

/*EXTL_DOC
 * Execute another file with Lua code.
 */
EXTL_EXPORT_AS(global, include)
bool include(const char *what);


/*}}}*/


static bool extl_init_obj_info(lua_State *st)
{
    static ExtlExportedFnSpec dummy[]={
        {NULL, NULL, NULL, NULL, NULL}
    };
    
    extl_register_class("Obj", dummy, NULL);
    
    /* Create cache */
    lua_newtable(st);
    /*lua_newtable(st);
    lua_pushstring(st, "__mode");
    lua_pushstring(st, "v");
    lua_rawset_check(st, -3);
    lua_setmetatable(st, -2);*/
    obj_cache_ref=lua_ref(st, -1);

    lua_pushcfunction(st, extl_obj_typename);
    lua_setglobal(st, "obj_typename");
    lua_pushcfunction(st, extl_obj_is);
    lua_setglobal(st, "obj_is");
    lua_pushcfunction(st, extl_obj_exists);
    lua_setglobal(st, "obj_exists");
    lua_pushcfunction(st, extl_include);
    lua_setglobal(st, "include");

    return TRUE;
}


/*}}}*/


/*{{{ Error handling and reporting -- unsafe */


static int extl_stack_trace(lua_State *st)
{
    lua_Debug ar;
    int lvl=0;
    int n_skip=0;
    
    lua_pushstring(st, "Stack trace:");

    for( ; lua_getstack(st, lvl, &ar); lvl++){
        bool is_c=FALSE;
        
        if(lua_getinfo(st, "Sln", &ar)==0){
            lua_pushfstring(st, "\n(Unable to get debug info for level %d)",
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
                lua_pushstring(st, "\n  [Skipping unnamed C functions.]");
                /*lua_pushstring(st, "\n...skipping...");*/
                lua_concat(st, 2);
            }
            n_skip++;
        }
    }
    return 1;
}


static int extl_do_collect_errors(lua_State *st)
{
    int n, err;
    ErrorLog *el=(ErrorLog*)lua_touserdata(st, -1);

    lua_pop(st, 1);
    
    n=lua_gettop(st)-1;
    err=lua_pcall(st, n, 0, 0);
    
    if(err!=0)
        warn("%s", lua_tostring(st, -1));
    
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
        warn("collect_errors internal error");
    
    return 1;
}


/*}}}*/


/*{{{ Init -- unsafe, but it doesn't matter at this point */


bool extl_init()
{
    l_st=lua_open();
    
    if(l_st==NULL){
        warn("Unable to initialize Lua.\n");
        return FALSE;
    }

    luaopen_base(l_st);
    luaopen_table(l_st);
    luaopen_io(l_st);
    luaopen_string(l_st);
    luaopen_math(l_st);
    luaopen_loadlib(l_st);
    
    if(!extl_init_obj_info(l_st)){
        warn("Failed to initialize Obj metatable\n");
        goto fail;
    }

    lua_pushcfunction(l_st, extl_collect_errors);
    lua_setglobal(l_st, "collect_errors");

    return TRUE;
fail:
    lua_close(l_st);
    return FALSE;
}


void extl_deinit()
{
    lua_close(l_st);
    l_st=NULL;
}


/*}}}*/


/*{{{ Stack get/push -- all unsafe */


static bool extl_stack_get(lua_State *st, int pos, char type, bool copystring,
                           void *valret)
{
    double d=0;
    const char *str;
          
    if(type=='i' || type=='d'){
        if(lua_type(st, pos)!=LUA_TNUMBER)
            return FALSE;
        
        d=lua_tonumber(st, pos);
        
        if(type=='i'){
            if(d-floor(d)!=0)
                return FALSE;
            if(valret)
                *((int*)valret)=d;
        }else{
            if(valret)
                *((double*)valret)=d;
        }
        return TRUE;
    }
    
    if(type=='b'){
        if(valret)
            *((bool*)valret)=lua_toboolean(st, pos);
        return TRUE;
    }

    if(lua_type(st, pos)==LUA_TNIL || lua_type(st, pos)==LUA_TNONE){
        if(type=='t' || type=='f'){
            if(valret)
                *((int*)valret)=LUA_NOREF;
        }else if(type=='s' || type=='S'){
            if(valret)
                *((char**)valret)=NULL;
        }else{
            return FALSE;
        }
        return TRUE;
    }
    
    if(type=='s' || type=='S'){
        if(lua_type(st, pos)!=LUA_TSTRING)
            return FALSE;
        if(valret){
            str=lua_tostring(st, pos);
            if(str!=NULL && copystring){
                str=scopy(str);
                if(str==NULL){
                    warn_err();
                    return FALSE;
                }
            }
            *((const char**)valret)=str;
        }
        return TRUE;
    }
    
    if(type=='f'){
        if(!lua_isfunction(st, pos))
            return FALSE;
        if(valret){
            lua_pushvalue(st, pos);
            *((int*)valret)=lua_ref(st, 1);
        }
        return TRUE;
    }

    if(type=='t'){
        if(!lua_istable(st, pos))
            return FALSE;
        if(valret){
            lua_pushvalue(st, pos);
            *((int*)valret)=lua_ref(st, 1);
        }
        return TRUE;
    }

    if(type=='o'){
        Obj *obj=extl_get_wobj(st, pos);
        if(obj==NULL)
            return FALSE;
        if(valret){
            *((Obj**)valret)=obj;
            D(fprintf(stderr, "Got obj %p, ", obj);
              fprintf(stderr, "%s\n", OBJ_TYPESTR(obj)));
        }
        return TRUE;
    }
    
    return FALSE;
}


static void extl_stack_push(lua_State *st, char spec, void *ptr)
{
    if(spec=='i'){
        lua_pushnumber(st, *(int*)ptr);
    }else if(spec=='d'){
        lua_pushnumber(st, *(double*)ptr);
    }else if(spec=='b'){
        lua_pushboolean(st, *(bool*)ptr);
    }else if(spec=='o'){
        extl_push_obj(st, *(Obj**)ptr);
    }else if(spec=='s' || spec=='S'){
        lua_pushstring(st, *(char**)ptr);
    }else if(spec=='t' || spec=='f'){
        lua_rawgeti(st, LUA_REGISTRYINDEX, *(int*)ptr);
    }else{
        lua_pushnil(st);
    }
}


static bool extl_stack_push_vararg(lua_State *st, char spec, va_list *argsp)
{
    switch(spec){
    case 'i':
        lua_pushnumber(st, (double)va_arg(*argsp, int));
        break;
    case 'd':
        lua_pushnumber(st, va_arg(*argsp, double));
        break;
    case 'b':
        lua_pushboolean(st, va_arg(*argsp, bool));
        break;
    case 'o':
        extl_push_obj(st, va_arg(*argsp, Obj*));
        break;
    case 'S':
    case 's':
        lua_pushstring(st, va_arg(*argsp, char*));
        break;
    case 'f':
    case 't':
        lua_rawgeti(st, LUA_REGISTRYINDEX, va_arg(*argsp, int));
        break;
    default:
        return FALSE;
    }
    
    return TRUE;
}


/*}}}*/


/*{{{ Free */


enum{STRINGS_NONE, STRINGS_NONCONST, STRINGS_ALL};


static void extl_free(void *ptr, char spec, int strings)
{
    if(((spec=='s' && strings!=STRINGS_NONE) ||
        (spec=='S' && strings==STRINGS_ALL)) && *(char**)ptr!=NULL){
        if(*(char**)ptr!=NULL)
            free(*(char**)ptr);
        *(char**)ptr=NULL;
    }else if(spec=='t'){
        extl_unref_table(*(ExtlTab*)ptr);
    }else if(spec=='f'){
        extl_unref_fn(*(ExtlFn*)ptr);
    }
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
    lua_unref(st, *refp);
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
    *refp=lua_ref(st, 1);
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
    *refp=lua_ref(st, 1);
    return TRUE;
}


ExtlTab extl_create_table()
{
    ExtlTab ref;
    if(extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_create_table, &ref))
        return ref;
    return LUA_NOREF;
}


/* globals */

static bool extl_do_globals(lua_State *st, int *refp)
{
    lua_pushvalue(st, LUA_GLOBALSINDEX);
    *refp=lua_ref(st, 1);
    return TRUE;
}


ExtlTab extl_globals()
{
    ExtlTab ref;
    if(extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_globals, &ref))
        return ref;
    return LUA_NOREF;
}


/*}}}*/


/*{{{ Table/get */


typedef struct{
    ExtlTab ref;
    char type;
    char itype;
    va_list *argsp;
} TableParams2;


static bool extl_table_dodo_get2(lua_State *st, TableParams2 *params)
{
    lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
    extl_stack_push_vararg(st, params->itype, params->argsp);
    lua_gettable(st, -2);
    if(lua_isnil(st, -1))
        return FALSE;
    
    return extl_stack_get(st, -1, params->type, TRUE, 
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
    params->n=luaL_getn_check(st, -1);
    return TRUE;
}


int extl_table_get_n(ExtlTab ref)
{
    GetNParams params;
    int oldtop;
    
    params.ref=ref;
    params.n=0;
    
    extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_do_get_n, &params);
    
    return params.n;
}


/*}}}*/


/*{{{ Table/set */


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


/*{{{ Table/clear entry */


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
        warn("Too many return values. Use a C compiler that has va_copy "
             "to support more");
        return FALSE;
    }
#endif
    
    while(m>0){
#ifdef CF_HAS_VA_COPY
        ptr=va_arg(args, void*);
#else
        ptr=va_arg(*(param->args), void*);
        param->ret_ptrs[param->nret]=ptr;
#endif
        if(!extl_stack_get(st, -m, *spec, TRUE, ptr)){
            /* This is the only place where we allow nil-objects */
            if(*spec=='o' && lua_isnil(st, -m)){
                *(Obj**)ptr=NULL;
            }else{
                warn("Invalid return value (expected '%c', got lua type \"%s\").",
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
    bool ret=TRUE;
    int n=0, m=0;
    
    if(lua_isnil(st, -1))
        return FALSE;

    if(param->spec!=NULL)
        n=strlen(param->spec);

    if(!lua_checkstack(st, n+8)){
        warn("Stack full");
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
        warn("%s", lua_tostring(st, -1));
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


/*{{{ extl_call */


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


/*{{{ extl_call_named */


static bool extl_do_call_named_vararg(lua_State *st, ExtlDoCallParam *param)
{
    lua_getglobal(st, (const char*)(param->misc));
    return extl_dodo_call_vararg(st, param);
}


bool extl_call_named_vararg(const char *name, const char *spec,
                            const char *rspec, va_list *args)
{
    ExtlDoCallParam param;
    param.spec=spec;
    param.rspec=rspec;
    param.args=args;
    param.misc=(void*)name;

    return extl_cpcall_call(l_st, (ExtlCPCallFn*)extl_do_call_named_vararg,
                            &param);
}


bool extl_call_named(const char *name, const char *spec, const char *rspec, ...)
{
    bool retval;
    va_list args;
    
    va_start(args, rspec);
    retval=extl_call_named_vararg(name, spec, rspec, &args);
    va_end(args);
    
    return retval;
}


/*}}}*/


/*}}}*/


/*{{{ extl_loadfile/string */


static int call_loaded(lua_State *st)
{
    int i, nargs=lua_gettop(st);

    /* Get the loaded file/string as function */
    lua_pushvalue(st, lua_upvalueindex(1));
    
    /* Fill 'arg' */
    lua_getfenv(st, -1);
    lua_pushstring(st, "arg");
    
    if(nargs>0){
        lua_newtable(st);
        for(i=1; i<=nargs; i++){
            lua_pushvalue(st, i);
            lua_rawseti_check(st, -2, i);
        }
    }else{
        lua_pushnil(st);
    }
    
    lua_rawset_check(st, -3);
    lua_pop(st, 1);
    lua_call(st, 0, LUA_MULTRET);
    return (lua_gettop(st)-nargs);
}


typedef struct{
    const char *src;
    /*bool isfile;*/
    ExtlFn *resptr;
} ExtlLoadParam;


static bool extl_do_load(lua_State *st, ExtlLoadParam *param)
{
    int res=0;
    
    /*if(param->isfile){*/
        res=luaL_loadfile(st, param->src);
    /*}else{
        res=luaL_loadbuffer(st, param->src, strlen(param->src), param->src);
    }*/
    
    if(res!=0){
        warn("%s", lua_tostring(st, -1));
        return FALSE;
    }
    
    lua_newtable(st); /* Create new environment */
    /* Now there's fn, newenv in stack */
    lua_newtable(st); /* Create metatable */
    lua_pushstring(st, "__index");
    lua_getfenv(st, -4); /* Get old environment */
    lua_rawset_check(st, -3); /* Set metatable.__index */
    lua_pushstring(st, "__newindex");
    lua_getfenv(st, -4); /* Get old environment */
    lua_rawset_check(st, -3); /* Set metatable.__newindex */
    /* Now there's fn, newenv, meta in stack */
    lua_setmetatable(st, -2); /* Set metatable for new environment */
    lua_setfenv(st, -2);
    /* Now there should be just fn in stack */

    /* Callloaded will put any parameters it gets in the table 'arg' in
     * the newly created environment.
     */
    lua_pushcclosure(st, call_loaded, 1);
    *(param->resptr)=lua_ref(st, -1);
    
    return TRUE;
}


bool extl_loadfile(const char *file, ExtlFn *ret)
{
    ExtlLoadParam param;
    param.src=file;
    /*param.isfile=TRUE;*/
    param.resptr=ret;

    return extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_load, &param);
}


#if 0
bool extl_loadstring(const char *str, ExtlFn *ret)
{
    ExtlLoadParam param;
    param.src=str;
    param.isfile=FALSE;
    param.resptr=ret;

    return extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_load, &param);
}
#endif

/*}}}*/


/*{{{ L1 call handler */


/* List of safe functions */

static const ExtlSafelist *extl_safelist=NULL;

const ExtlSafelist *extl_set_safelist(const ExtlSafelist *newlist)
{
    const ExtlSafelist *oldlist=extl_safelist;
    extl_safelist=newlist;
    return oldlist;
}


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

static L1Param *current_param;
    
static int extl_l1_call_handler2(lua_State *st)
{
    L1Param *param=current_param;
    ExtlExportedFnSpec *spec=param->spec;
    int i;

    if(spec==NULL){
        warn("L1 call handler upvalues corrupt.");
        return 0;
    }
    
    if(spec->fn==NULL){
        warn("Called function has been unregistered");
        return 0;
    }
    
    D(fprintf(stderr, "%s called\n", spec->name));
    
    if(extl_safelist!=NULL){
        for(i=0; extl_safelist[i]!=NULL; i++){
            if(extl_safelist[i]!=spec->fn)
                break;
        }
        if(extl_safelist[i]==NULL){
            warn("Attempt to call an unsafe function \"%s\" in restricted mode.",
                 spec->name);
            return 0;
        }
    }
    
    if(!lua_checkstack(st, MAX_PARAMS+1)){
        warn("Stack full");
        return 0;
    }
    
    param->ni=(spec->ispec==NULL ? 0 : strlen(spec->ispec));
    
    for(i=0; i<param->ni; i++){
        if(!extl_stack_get(st, i+1, spec->ispec[i], FALSE,
                           (void*)&(param->ip[i]))){
            warn("Argument %d to %s is of invalid type. "
                 "(Argument template is '%s', got lua type %s).",
                 i+1, spec->name, spec->ispec,
                 lua_typename(st, lua_type(st, i+1)));
            return 0;
        }
        param->ii=i+1;
    }
    
    if(!spec->l2handler(spec->fn, param->ip, param->op))
        return 0;
    
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


INTRSTRUCT(WarnChain);
DECLSTRUCT(WarnChain){
    bool need_trace;
    lua_State *st;
    WarnHandler *old_handler;
    WarnChain *prev;
};


static WarnChain *warnchain=NULL;
static bool notrace=FALSE;


static void l1_warn_handler(const char *message)
{
    WarnChain *ch=warnchain;
    static int called=0;
    
    assert(warnchain!=NULL);
    
    if(called==0 && !notrace)
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

    if(notrace)
        return;
    
    extl_stack_trace(ch->st);
    p=lua_tostring(ch->st, -1);
    notrace=TRUE;
    warn(p);
    notrace=FALSE;
    ch->need_trace=FALSE;
    lua_pop(ch->st, 1);
}


static void flushtrace()
{
    if(warnchain && warnchain->need_trace)
        do_trace(warnchain);
}


static int extl_l1_call_handler(lua_State *st)
{
    WarnChain ch;
    L1Param param={{NULL, }, {NULL, }, NULL, 0, 0, 0};
    L1Param *old_param;
    int ret;
    int n=lua_gettop(st);
    
    
    /* Get the info we need on the function and then set up a safe
     * environment for extl_l1_call_handler2. 
     */
    param.spec=(ExtlExportedFnSpec*)lua_touserdata(st, lua_upvalueindex(1));
    lua_pushcfunction(st, extl_l1_call_handler2);
    lua_insert(st, 1);
    
    old_param=current_param;
    current_param=&param;
    
    ch.old_handler=set_warn_handler(l1_warn_handler);
    ch.need_trace=FALSE;
    ch.st=st;
    ch.prev=warnchain;
    warnchain=&ch;
    
    /* Ok, Lua may now freely fail in extl_l1_call_handler2, we can handle
     * that.
     */
    ret=lua_pcall(st, n, LUA_MULTRET, 0);
    
    /* Now that the actual call handler has returned, we need to free
     * any of our data before calling Lua again.
     */
    current_param=old_param;
    extl_l1_finalize(&param);
    
    warnchain=ch.prev;
    set_warn_handler(ch.old_handler);

    /* Ok, we can now safely use Lua functions again without fear of
     * leaking.
     */
    if(ret!=0){
        const char *p;
        param.no=0;
        p=lua_tostring(st, -1);
        notrace=TRUE;
        warn("%s", p);
        notrace=FALSE;
    }

    if(ret!=0 || ch.need_trace)
        do_trace(&ch);
    
    return param.no;
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
    ExtlExportedFnSpec *spec=data->spec, *spec2;
    int ind=LUA_GLOBALSINDEX;
    
    if((spec->ispec!=NULL && strlen(spec->ispec)>MAX_PARAMS) ||
       (spec->ospec!=NULL && strlen(spec->ospec)>MAX_PARAMS)){
        warn("Function '%s' has more parameters than the level 1 "
             "call handler can handle\n", spec->name);
        return FALSE;
    }

    if(data->table!=LUA_NOREF){
        lua_rawgeti(st, LUA_REGISTRYINDEX, data->table);
        ind=-3;
    }

    lua_pushstring(st, spec->name);

    spec2=lua_newuserdata(st, sizeof(ExtlExportedFnSpec));
    
    memcpy(spec2, spec, sizeof(ExtlExportedFnSpec));

    lua_getregistry(st);
    lua_pushvalue(st, -2); /* Get spec2 */
    lua_pushfstring(st, "luaextl_%s_%s_upvalue", 
                    data->cls, spec->name);
    lua_rawset_check(st, -3); /* Set registry.luaextl_fn_upvalue=spec2 */
    lua_pop(st, 1); /* Pop registry */
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
    ExtlExportedFnSpec *spec=data->spec, *spec2;
    int ind=LUA_GLOBALSINDEX;

    lua_getregistry(st);
    lua_pushfstring(st, "luaextl_%s_%s_upvalue", 
                    data->cls, spec->name);
    lua_pushvalue(st, -1);
    lua_gettable(st, -3); /* Get registry.luaextl_fn_upvalue */
    spec2=lua_touserdata(st, -1);

    if(spec2==NULL)
        return FALSE;
    
    spec2->ispec=NULL;
    spec2->ospec=NULL;
    spec2->fn=NULL;
    spec2->name=NULL;
    spec2->l2handler=NULL;

    lua_pop(st, 1); /* Pop the upvalue */
    lua_pushnil(st);
    lua_rawset_check(st, -3); /* Clear registry.luaextl_fn_upvalue */
    
    if(data->table!=LUA_NOREF){
        lua_rawgeti(st, LUA_REGISTRYINDEX, data->table);
        ind=-3;
    }
    
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
            warn("Could not find metatable for parent class %s of %s.\n",
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
    data->refret=lua_ref(st, 1); /* TODO: free on failure */
    if(!data->hide){
        lua_pushstring(st, data->cls);
        lua_pushvalue(st, -2);
        lua_rawset(st, LUA_GLOBALSINDEX);
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
           
    if(!extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_register_class, &clsdata)){
        warn("Unable to register class %s.\n", cls);
        return FALSE;
    }

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
    data->refret=lua_ref(st, -1);
    lua_pop(st, 1);
    /* Set the entry from registry to nil. */
    lua_pushnil(st);
    lua_rawset(st, LUA_REGISTRYINDEX);
    
    /* Reset the global reference to the class to nil. */
    lua_pushstring(st, data->cls);
    lua_pushnil(st);
    lua_rawset(st, LUA_GLOBALSINDEX);
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
    
    clsdata->refret=lua_ref(st, -1);
    
    return TRUE;
}


bool extl_register_module(const char *mdl, ExtlExportedFnSpec *fns)
{
    ClassData clsdata;
    
    clsdata.cls=mdl;
    clsdata.parent=NULL;
    clsdata.refret=LUA_NOREF;
    clsdata.hide=FALSE; /* unused, but initialise */
    
    if(!extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_register_module, &clsdata)){
        warn("Unable to register module %s.\n", mdl);
        return FALSE;
    }

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
    clsdata->refret=lua_ref(st, -1);
    
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
            warn("Maximize serialisation depth reached.");
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
        warn("Unable to serialise type %s.", 
             lua_typename(st, lua_type(st, -1)));
    }
    lua_pop(st, 1);
    return TRUE;
}


static bool extl_do_serialise(lua_State *st, SerData *d)
{
    if(!extl_getref(st, d->tab))
        return FALSE;
    
    return ser(st, d->f, 0);
}


/* Tab must not contain recursive references! */
extern bool extl_serialise(const char *file, ExtlTab tab)
{
    SerData d;
    bool ret;

    d.tab=tab;
    d.f=fopen(file, "w");
    
    if(d.f==NULL){
        warn_err_obj(file);
        return FALSE;
    }
    
    fprintf(d.f, "-- This file has been generated by Ion. Do no edit.\n");
    fprintf(d.f, "return ");
    
    ret=extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_serialise, &d);
    
    fprintf(d.f, "\n\n");
    
    fclose(d.f);
    
    return ret;
}


/*}}}*/

