/*
 * ion/lua/luaextl.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <libtu/util.h>
#include <ioncore/common.h>
#include <ioncore/obj.h>
#include <ioncore/objp.h>
#include <ioncore/errorlog.h>

#include "luaextl.h"

/* Maximum number of parameters and return values for calls from Lua
 * and (if va_copy is not available) return value from Lua functions.
 */
#define MAX_PARAMS 16

static lua_State *l_st=NULL;

static bool extl_stack_get(lua_State *st, int pos, char type, bool copystring,
						   void *valret);


/*{{{ WObj userdata handling -- unsafe */


static int wobj_metaref=LUA_NOREF;
static int obj_cache_ref=LUA_NOREF;


static void extl_get_wobj_metatable(lua_State *st)
{
	lua_rawgeti(st, LUA_REGISTRYINDEX, wobj_metaref);
}


static WObj *extl_get_wobj(lua_State *st, int pos)
{
	WWatch *watch;
	bool eq;
	
	if(!lua_isuserdata(st, pos))
		return NULL;
	
	if(!lua_getmetatable(st, pos))
		return NULL;
	
	extl_get_wobj_metatable(st);
	
	eq=lua_equal(st, -1, -2);
	
	lua_pop(st, 2);
	
	if(eq){
		watch=(WWatch*)lua_touserdata(st, pos);
		if(watch==NULL || watch->obj==NULL){
			/* Replace dead object with nil */
			lua_pushnil(st);
			lua_replace(st, pos);
		}else{
			return watch->obj;
		}
	}

	return NULL;
}


static void extl_obj_dest_handler(WWatch *watch, WObj *obj)
{
	D(warn("%s destroyed while Lua code is still referencing it.",
		   WOBJ_TYPESTR(obj)));
}


/* how to call this? */
static void extl_push_obj(lua_State *st, WObj *obj)
{
	WWatch *watch;
	
	if(obj==NULL){
		lua_pushnil(st);
		return;
	}

	if(obj->flags&WOBJ_EXTL_CACHED){
		lua_rawgeti(st, LUA_REGISTRYINDEX, obj_cache_ref);
		lua_pushlightuserdata(st, obj);
		lua_gettable(st, -2);
		if(lua_isuserdata(st, -1)){
			D(fprintf(stderr, "found %p cached\n", obj));
			lua_remove(st, -2);
			return;
		}
		lua_pop(st, 2);
	}

	D(fprintf(stderr, "Creating %p\n", obj));
	
	watch=(WWatch*)lua_newuserdata(st, sizeof(WWatch));
	
	/* Lua shouldn't return if the allocation fails */
	
	init_watch(watch);
	setup_watch(watch, obj, extl_obj_dest_handler);
	
	extl_get_wobj_metatable(st);
	lua_setmetatable(st, -2);

	lua_rawgeti(st, LUA_REGISTRYINDEX, obj_cache_ref);
	lua_pushlightuserdata(st, obj);
	lua_pushvalue(st, -3);
	lua_settable(st, -3);
	lua_pop(st, 1);
	obj->flags|=WOBJ_EXTL_CACHED;
}
	
	
/*{{{ Functions available to Lua ucode */


static int extl_obj_gc_handler(lua_State *st)
{
	WWatch *watch;
	
	if(extl_get_wobj(st, 1)==NULL)
		return 0;

	watch=(WWatch*)lua_touserdata(st, 1);
	
	if(watch!=NULL){
		D(fprintf(stderr, "Collecting %p\n", watch->obj));
		if(watch->obj!=NULL){
			D(fprintf(stderr, "was cached\n"));
			watch->obj->flags&=~WOBJ_EXTL_CACHED;
		}
		reset_watch(watch);
	}
	
	return 0;
}


static int extl_obj_typename(lua_State *st)
{
	WObj *obj;

	if(!extl_stack_get(st, 1, 'o', FALSE, &obj))
		return 0;
	
	lua_pushstring(st, WOBJ_TYPESTR(obj));
	return 1;
}


static int extl_obj_is(lua_State *st)
{
	WObj *obj;
	const char *tn;
	
	if(!extl_stack_get(st, 1, 'o', FALSE, &obj)){
		lua_pushboolean(st, 0);
	}else{
		tn=lua_tostring(st, 2);
		lua_pushboolean(st, wobj_is_str(obj, tn));
	}
	
	return 1;
}


/*}}}*/


static bool extl_init_obj_info(lua_State *st)
{
	lua_newtable(st);
	lua_newtable(st);
	lua_pushstring(st, "__mode");
	lua_pushstring(st, "v");
	lua_settable(st, -3);
	lua_setmetatable(st, -2);
	obj_cache_ref=lua_ref(st, -1);
	
	lua_newtable(st);
	lua_pushstring(st, "__gc");
	lua_pushcfunction(st, extl_obj_gc_handler);
	lua_settable(st, -3); /* set metatable.__gc=extl_obj_gc_handler */
	wobj_metaref=lua_ref(st, -1);

	lua_pushcfunction(st, extl_obj_typename);
	lua_setglobal(st, "obj_typename");
	lua_pushcfunction(st, extl_obj_is);
	lua_setglobal(st, "obj_is");

	return TRUE;
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
	
	lua_cpcall(st, extl_docpcall, &param);
	
	lua_settop(st, oldtop);
	
	return param.retval;
}


/*}}}*/


/*{{{ Error handling and reporting -- unsafe */


static int extl_stack_trace(lua_State *st)
{
	lua_Debug ar;
	int lvl=1;
	
	lua_pushstring(st, "Stack trace:");

	for( ; lua_getstack(st, lvl, &ar); lvl++){
		if(lua_getinfo(st, "Sln", &ar)==0){
			lua_pushfstring(st, "\n    (Unable to get debug info for level %d)",
							lvl);
		}else{
			lua_pushfstring(st, "\n    %s: line %d, function: %s",
							ar.source==NULL ? "(unknown)" : ar.source,
							ar.currentline,
							ar.name==NULL ? "(unknown)" : ar.name);
		}
		lua_concat(st, 2);
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
	
	begin_errorlog(&el);
	
	err=lua_pcall(st, n+1, 1, 0);
	
	end_errorlog(&el);
	deinit_errorlog(&el);
	
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
		warn("Failed to initialize WObj metatable\n");
		goto fail;
	}

	lua_pushcfunction(l_st, extl_collect_errors);
	lua_setglobal(l_st, "collect_errors");

	return TRUE;
fail:
	lua_close(l_st);
	return FALSE;
}


const char *extl_extension()
{
	return "lua";
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
		WObj *obj=extl_get_wobj(st, pos);
		if(obj==NULL)
			return FALSE;
		if(valret){
			*((WObj**)valret)=obj;
			D(fprintf(stderr, "Got obj %p, ", obj);
			  fprintf(stderr, "%s\n", WOBJ_TYPESTR(obj)));
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
		extl_push_obj(st, *(WObj**)ptr);
	}else if(spec=='s' || spec=='S'){
		lua_pushstring(st, *(char**)ptr);
	}else if(spec=='t' || spec=='f'){
		lua_rawgeti(st, LUA_REGISTRYINDEX, *(int*)ptr);
	}else{
		lua_pushnil(st);
	}
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
	int ref;
	const char *sentry;
	int ientry;
	char type;
	void *val;
	bool insertlast;
} TableParams;


static bool extl_table_dodo_gets(lua_State *st, TableParams *params)
{
	lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
	lua_pushstring(st, params->sentry);
	lua_gettable(st, -2);
	if(lua_isnil(st, -1))
		return FALSE;
	
	return extl_stack_get(st, -1, params->type, TRUE, params->val);
}


static bool extl_table_do_gets(ExtlTab ref, const char *entry,
							   char type, void *valret)
{
	TableParams params;
	
	params.ref=ref;
	params.sentry=entry;
	params.type=type;
	params.val=valret;
	
	return extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_dodo_gets, &params);
}


bool extl_table_gets_o(ExtlTab ref, const char *entry, WObj **ret)
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


typedef struct{
	int ref;
	int n;
} GetNParams;


static bool extl_table_do_get_n(lua_State *st, GetNParams *params)
{
	lua_getglobal(st, "table");
	lua_pushstring(st, "getn");
	lua_gettable(st, -2);
	lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
	lua_call(st, 1, 1);
	params->n=lua_tonumber(st, -1);
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



static bool extl_table_dodo_geti(lua_State *st, TableParams *params)
{
	lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
	lua_rawgeti(st, -1, params->ientry);
	if(!lua_isnil(st, -1) &&
	   extl_stack_get(st, -1, params->type, TRUE, params->val))
		return TRUE;
	
	return FALSE;
}


static bool extl_table_do_geti(ExtlTab ref, int entry, char type, void *valret)
{
	TableParams params;
	
	params.ref=ref;
	params.ientry=entry;
	params.type=type;
	params.val=valret;
	
	return extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_dodo_geti, &params);
}


bool extl_table_geti_o(ExtlTab ref, int entry, WObj **ret)
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


/*}}}*/


/*{{{ Table/set */


static bool extl_table_dodo_sets(lua_State *st, TableParams *params)
{
	lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
	lua_pushstring(st, params->sentry);
	extl_stack_push(st, params->type, params->val);
	lua_settable(st, -3);
	return TRUE;
}


static bool extl_table_do_sets(ExtlTab ref, const char *entry,
							   char type, void *val)
{
	TableParams params;
		
	params.ref=ref;
	params.sentry=entry;
	params.type=type;
	params.val=val;
	
	return extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_dodo_sets, &params);
}



bool extl_table_sets_o(ExtlTab ref, const char *entry, WObj *val)
{
	return extl_table_do_sets(ref, entry, 'o', (void*)&val);
}

bool extl_table_sets_i(ExtlTab ref, const char *entry, int val)
{
	return extl_table_do_sets(ref, entry, 'i', (void*)&val);
}

bool extl_table_sets_d(ExtlTab ref, const char *entry, double val)
{
	return extl_table_do_sets(ref, entry, 'd', (void*)&val);
}

bool extl_table_sets_b(ExtlTab ref, const char *entry, bool val)
{
	return extl_table_do_sets(ref, entry, 'b', (void*)&val);
}

bool extl_table_sets_s(ExtlTab ref, const char *entry, const char *val)
{
	return extl_table_do_sets(ref, entry, 'S', (void*)&val);
}

bool extl_table_sets_f(ExtlTab ref, const char *entry, ExtlFn val)
{
	return extl_table_do_sets(ref, entry, 'f', (void*)&val);
}

bool extl_table_sets_t(ExtlTab ref, const char *entry, ExtlTab val)
{
	return extl_table_do_sets(ref, entry, 't', (void*)&val);
}


static bool extl_table_do_clears(lua_State *st, TableParams *params)
{
	lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
	lua_pushstring(st, params->sentry);
	lua_pushnil(st);
	lua_settable(st, -3);
	return TRUE;
}


bool extl_table_clears(ExtlTab ref, const char *entry)
{
	TableParams params;

	params.ref=ref;
	params.sentry=entry;

	return extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_do_clears, &params);
}


static bool extl_table_dodo_seti(lua_State *st, TableParams *params)
{
	lua_getglobal(st, "table");
	lua_pushstring(st, "insert");
	lua_gettable(st, -2);
	lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
	if(!params->insertlast)
		lua_pushnumber(st, params->ientry);
	extl_stack_push(st, params->type, params->val);
	lua_call(st, 2+!params->insertlast, 0);
	
	return TRUE;
}


static bool extl_table_do_seti(ExtlTab ref, bool insertlast, int entry,
							   char type, void *val)
{
	TableParams params;
	
	params.ref=ref;
	params.insertlast=insertlast;
	params.ientry=entry;
	params.type=type;
	params.val=val;

	return extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_dodo_seti, &params);
}


bool extl_table_seti_o(ExtlTab ref, int entry, WObj *val)
{
	return extl_table_do_seti(ref, FALSE, entry, 'o', (void*)&val);
}

bool extl_table_seti_i(ExtlTab ref, int entry, int val)
{
	return extl_table_do_seti(ref, FALSE, entry, 'i', (void*)&val);
}

bool extl_table_seti_d(ExtlTab ref, int entry, double val)
{
	return extl_table_do_seti(ref, FALSE, entry, 'd', (void*)&val);
}

bool extl_table_seti_b(ExtlTab ref, int entry, bool val)
{
	return extl_table_do_seti(ref, FALSE, entry, 'b', (void*)&val);
}

bool extl_table_seti_s(ExtlTab ref, int entry, const char *val)
{
	return extl_table_do_seti(ref, FALSE, entry, 'S', (void*)&val);
}

bool extl_table_seti_f(ExtlTab ref, int entry, ExtlFn val)
{
	return extl_table_do_seti(ref, FALSE, entry, 'f', (void*)&val);
}

bool extl_table_seti_t(ExtlTab ref, int entry, ExtlTab val)
{
	return extl_table_do_seti(ref, FALSE, entry, 't', (void*)&val);
}


static bool extl_table_do_cleari(lua_State *st, TableParams *params)
{
	lua_rawgeti(st, LUA_REGISTRYINDEX, params->ref);
	lua_pushnil(st);
	lua_rawseti(st, -2, params->ientry);
	return TRUE;
}


bool extl_table_cleari(ExtlTab ref, int entry)
{
	TableParams params;

	params.ref=ref;
	params.ientry=entry;

	return extl_cpcall(l_st, (ExtlCPCallFn*)extl_table_do_cleari, &params);
}


/*}}}*/


/*{{{ Function calls to Lua */


static bool extl_push_args(lua_State *st, bool intab, const char *spec,
						   va_list *argsp)
{
	int i=1;
	
	while(*spec!='\0'){
		switch(*spec){
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
			extl_push_obj(st, va_arg(*argsp, WObj*));
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
		if(intab)
			lua_rawseti(st, -2, i);
		i++;
		spec++;
	}
	
	return TRUE;
}


typedef struct{
	const char *spec;
	const char *rspec;
	va_list args;
	void *misc;
	bool intab;
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
	va_copy(args, param->args);
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
		ptr=va_arg(param->args, void*);
		param->ret_ptrs[param->nret]=ptr;
#endif
		if(!extl_stack_get(st, -m, *spec, TRUE, ptr)){
			/* This is the only place where we allow nil-objects */
			if(*spec=='o' && lua_isnil(st, -m)){
				*(WObj**)ptr=NULL;
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
		/* For dostring and dofile arguments are passed in the table 'arg'.
		 * The make 'arg' appear local without messing up with any such
		 * variable actually defined in the function, we need to make a new
		 * environment containg the parameter and __index and __newindex of
		 * the metatable set to point to the global environment. We don't
		 * need to reset this environment as we created the function.
		 */
		if(param->intab){
			lua_newtable(st); /* Create arg */
			lua_newtable(st); /* Create new environment */
			lua_pushstring(st, "arg");
			lua_pushvalue(st, -3);
			lua_settable(st, -3); /* Set arg in the new environment */
			/* Now there's fn, arg, newenv in stack */
			lua_newtable(st); /* Create metatable */
			lua_pushstring(st, "__index");
			lua_getfenv(st, -5); /* Get old environment */
			lua_settable(st, -3); /* Set metatable.__index */
			lua_pushstring(st, "__newindex");
			lua_getfenv(st, -5); /* Get old environment */
			lua_settable(st, -3); /* Set metatable.__newindex */
			/* Now there's fn, arg, newenv, meta in stack */
			lua_setmetatable(st, -2); /* Set metatable for new environment */
			lua_setfenv(st, -3);
			/* Now there should be just fn, arg in stack */
		}

		if(!extl_push_args(st, param->intab, param->spec, &(param->args)))
			return FALSE;

		if(param->intab){
			lua_pop(st, 1); /* Pop "arg"; it now only resides in the env. */
			n=0;
		}
	}

	if(param->rspec!=NULL)
		m=strlen(param->rspec);
	
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
		ptr=va_arg(param->args, void*);
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
					  const char *rspec, va_list args)
{
	ExtlDoCallParam param;
	
	if(fnref==LUA_NOREF || fnref==LUA_REFNIL)
		return FALSE;

	param.spec=spec;
	param.rspec=rspec;
	param.args=args;
	param.misc=(void*)&fnref;
	param.intab=FALSE;

	return extl_cpcall_call(l_st, (ExtlCPCallFn*)extl_do_call_vararg, &param);
}


bool extl_call(ExtlFn fnref, const char *spec, const char *rspec, ...)
{
	bool retval;
	va_list args;
	
	va_start(args, rspec);
	retval=extl_call_vararg(fnref, spec, rspec, args);
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
							const char *rspec, va_list args)
{
	ExtlDoCallParam param;
	param.spec=spec;
	param.rspec=rspec;
	param.args=args;
	param.misc=(void*)name;
	param.intab=FALSE;

	return extl_cpcall_call(l_st, (ExtlCPCallFn*)extl_do_call_named_vararg,
							&param);
}


bool extl_call_named(const char *name, const char *spec, const char *rspec, ...)
{
	bool retval;
	va_list args;
	
	va_start(args, rspec);
	retval=extl_call_named_vararg(name, spec, rspec, args);
	va_end(args);
	
	return retval;
}


/*}}}*/


/*{{{ extl_dofile */


static bool extl_do_dofile_vararg(lua_State *st, ExtlDoCallParam *param)
{
	bool ret=FALSE;
	const char *file=(const char*)param->misc;
	
	printf("lua_dofile(%s)\n", file);
	
	 /* Save CURRENT_FILE.  */
	lua_getglobal(st, "CURRENT_FILE");
	
	if(luaL_loadfile(st, file)!=0){
		warn("%s", lua_tostring(st, -1));
		return FALSE;
	}

	/* Set new CURRENT_FILE. If Lua fails in extl_dodo_call_vararg, the
	 * value will remain incorrect until next return from this function.
	 * (or within a succeeding include call). But this certainly is not
	 * critical. If Lua fails, we're probably out of memory anyway and
	 * keeping Ion from crashing is more important.
	 */
	lua_pushstring(st, file);
	lua_setglobal(st, "CURRENT_FILE");

	ret=extl_dodo_call_vararg(st, param);
	
	/* Restore CURRENT_FILE */
	lua_setglobal(st, "CURRENT_FILE");

	return ret;
}


bool extl_dofile_vararg(const char *file, const char *spec,
						const char *rspec, va_list args)
{
	ExtlDoCallParam param;
	param.spec=spec;
	param.rspec=rspec;
	param.args=args;
	param.misc=(void*)file;
	param.intab=TRUE;
		
	return extl_cpcall_call(l_st, (ExtlCPCallFn*)extl_do_dofile_vararg, &param);
}


bool extl_dofile(const char *file, const char *spec, const char *rspec, ...)
{
	bool retval;
	va_list args;
	
	va_start(args, rspec);
	retval=extl_dofile_vararg(file, spec, rspec, args);
	va_end(args);
	
	return retval;
}


/*}}}*/


/*{{{ extl_dofile */


static bool extl_do_dostring_vararg(lua_State *st, ExtlDoCallParam *param)
{
	const char *string=(const char*)param->misc;
	
	if(luaL_loadbuffer(st, string, strlen(string), string)!=0){
		warn("%s", lua_tostring(st, -1));
		return FALSE;
	}
	
	return extl_dodo_call_vararg(st, param);
}


bool extl_dostring_vararg(const char *string, const char *spec,
						  const char *rspec, va_list args)
{
	ExtlDoCallParam param;
	param.spec=spec;
	param.rspec=rspec;
	param.args=args;
	param.misc=(void*)string;
	param.intab=TRUE;
	
	return extl_cpcall_call(l_st, (ExtlCPCallFn*)extl_do_dostring_vararg,
							&param);
}


bool extl_dostring(const char *string, const char *spec, const char *rspec, ...)
{
	bool retval;
	va_list args;
	
	va_start(args, rspec);
	retval=extl_dostring_vararg(string, spec, rspec, args);
	va_end(args);
	
	return retval;
}


/*}}}*/


/*}}}*/


/*{{{ L1 call handler */


/* List of safe functions */

static const char **extl_safelist=NULL;

const char **extl_set_safelist(const char **newlist)
{
	const char **oldlist=extl_safelist;
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
			if(strcmp(spec->name, extl_safelist[i])==0)
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
	WarnHandler *old_handler;
	WarnChain *prev;
};


static WarnChain *warnchain;
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


static int extl_l1_call_handler(lua_State *st)
{
	WarnChain ch;
	L1Param param={{NULL, }, {NULL, }, NULL, 0, 0, 0};
	L1Param *old_param;
	const char *p;
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
		param.no=0;
		p=lua_tostring(st, -1);
		notrace=TRUE;
		warn("%s", p);
		notrace=FALSE;
	}

	if(ch.need_trace || ret!=0){
		extl_stack_trace(st);
		p=lua_tostring(st, -1);
		notrace=TRUE;
		warn(p);
		notrace=FALSE;
		lua_pop(st, 1);
	}
	
	return param.no;
}


/*}}}*/
	

/*{{{ Function registration */


static bool extl_do_register_function(lua_State *st, ExtlExportedFnSpec *spec)
{
	ExtlExportedFnSpec *spec2;
	
	if((spec->ispec!=NULL && strlen(spec->ispec)>MAX_PARAMS) ||
	   (spec->ospec!=NULL && strlen(spec->ospec)>MAX_PARAMS)){
		warn("Function '%s' has more parameters than the level 1 "
			 "call handler can handle\n", spec->name);
		return FALSE;
	}

	spec2=lua_newuserdata(st, sizeof(ExtlExportedFnSpec));
	if(spec2==NULL){
		warn("Unable to create upvalue for '%s'\n", spec->name);
		return FALSE;
	}
	memcpy(spec2, spec, sizeof(ExtlExportedFnSpec));
	
	lua_getregistry(st);
	lua_pushvalue(st, -2); /* Get spec2 */
	lua_pushfstring(st, "ioncore_luaextl_%s_upvalue", spec->name);
	lua_settable(st, -3); /* Set registry.ioncore_luaextl_fn_upvalue=spec2 */
	lua_pop(st, 1); /* Pop registry */
	lua_pushcclosure(st, extl_l1_call_handler, 1);
	lua_setglobal(st, spec->name);
	
	return TRUE;
}


bool extl_register_function(ExtlExportedFnSpec *spec)
{
	return extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_register_function, spec);
}


static bool extl_do_unregister_function(lua_State *st, ExtlExportedFnSpec *spec)
{
	ExtlExportedFnSpec *spec2;

	lua_getregistry(st);
	lua_pushfstring(st, "ioncore_luaextl_%s_upvalue", spec->name);
	lua_pushvalue(st, -1);
	lua_gettable(st, -3); /* Get registry.ioncore_luaextl_fn_upvalue */
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
	lua_settable(st, -3); /* Clear registry.ioncore_luaextl_fn_upvalue */
	lua_pushnil(st); /* Clear _G.fn */
	lua_setglobal(st, spec->name);
	
	return TRUE;
}


void extl_unregister_function(ExtlExportedFnSpec *spec)
{
	extl_cpcall(l_st, (ExtlCPCallFn*)extl_do_unregister_function, spec);
}


/*}}}*/

