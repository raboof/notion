/*
 * ion/lua/luaextl.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <libtu/util.h>
#include <ioncore/common.h>
#include <ioncore/obj.h>
#include <ioncore/objp.h>
#include <ioncore/completehelp.h>

#include "luaextl.h"


static lua_State *l_st=NULL;

static bool extl_stack_get(lua_State *st, int pos, char type, bool copystring,
						   void *valret);

/*{{{ WObj userdata handling */


static void extl_get_wobj_metatable(lua_State *st)
{
	lua_getregistry(st);
	lua_pushstring(st, "ioncore_luaextl_WObj_metatable");
	lua_gettable(st, -2);
	lua_remove(st, -2);
}


static bool extl_verify_wobj(lua_State *st, int pos)
{
	bool ret;
	
	if(!lua_isuserdata(st, pos))
		return FALSE;
	
	if(!lua_getmetatable(st, pos))
		return FALSE;
	
	extl_get_wobj_metatable(st);
	
	ret=lua_equal(st, -1, -2);
	
	lua_pop(st, 2);
	
	return ret;
}


static WObj *extl_do_get_obj(lua_State *st, int pos)
{
	WWatch *watch=(WWatch*)lua_touserdata(st, pos);
	return watch->obj;
}


static void extl_obj_dest_handler(WWatch *watch, WObj *obj)
{
	D(fprintf(stderr, "Object destroyed while Lua code is still referencing it."));
}


static int extl_obj_gc_handler(lua_State *st)
{
	WWatch *watch;
	
	if(!extl_verify_wobj(st, 1))
		return 0;
	
	watch=(WWatch*)lua_touserdata(st, 1);
	
	if(watch!=NULL)
		reset_watch(watch);
	
	return 0;
}


static bool extl_push_obj(lua_State *st, WObj *obj)
{
	WWatch *watch;
	
	if(obj==NULL){
		lua_pushnil(st);
		return TRUE;
	}

	watch=(WWatch*)lua_newuserdata(l_st, sizeof(WWatch));
	
	if(watch==NULL)
		return FALSE;
	
	init_watch(watch);
	setup_watch(watch, obj, extl_obj_dest_handler);
	
	extl_get_wobj_metatable(st);
	lua_setmetatable(st, -2);
	
	return TRUE;
}
	
	
static int extl_obj_typename(lua_State *st)
{
	WObj *obj;

	if(!extl_stack_get(st, -1, 'o', FALSE, &obj)){
		lua_pushnil(st);
	}else{
		lua_pushstring(st, WOBJ_TYPESTR(obj));
	}
	
	return 1;
}


static int extl_obj_is(lua_State *st)
{
	WObj *obj;
	const char *tn;
	
	if(!extl_stack_get(st, -2, 'o', FALSE, &obj)){
		lua_pushboolean(st, 0);
	}else{
		tn=lua_tostring(st, -1);
		lua_pushboolean(st, wobj_is_str(obj, tn));
	}
	
	return 1;
}


static bool extl_init_obj_info(lua_State *st)
{
	lua_getregistry(st);
	lua_pushstring(st, "ioncore_luaextl_WObj_metatable");
	lua_newtable(st);
	lua_pushstring(st, "__gc");
	lua_pushcfunction(st, extl_obj_gc_handler);
	lua_settable(st, -3); /* set metatable.__gc=extl_obj_gc_handler */
	lua_settable(st, -3); /* set registry.WObj_metatable=metatable */
	lua_pop(st, 1); /* pop registry */

	lua_pushcfunction(st, extl_obj_typename);
	lua_setglobal(st, "obj_typename");
	lua_pushcfunction(st, extl_obj_is);
	lua_setglobal(st, "obj_is");

	return TRUE;
}


/*}}}*/


/*{{{ Init */


bool extl_init()
{
	l_st=lua_open();
	
	if(l_st==NULL){
		warn("Unable to initialize Lua.\n");
		return FALSE;
	}

	lua_baselibopen(l_st);
	lua_tablibopen(l_st);
	lua_iolibopen(l_st);
	lua_strlibopen(l_st);
	lua_mathlibopen(l_st);
	
	if(!extl_init_obj_info(l_st)){
		warn("Failed to initialize WObj metatable\n");
		goto fail;
	}
	
	return TRUE;
fail:
	lua_close(l_st);
	return FALSE;
}


const char *extl_extension()
{
	return "lua";
}


/*}}}*/


/*{{{ Stack get/push */


static bool extl_stack_get(lua_State *st, int pos, char type, bool copystring,
						   void *valret)
{
	double d=0;
	const char *str;
			  
	if(type=='0'){
		if(!lua_isnil(st, pos))
			return FALSE;
		if(valret)
			*((int*)valret)=lua_ref(st, pos);
		return TRUE;
	}
		
	if(type=='i' || type=='d'){
		if(lua_type(st, pos)!=LUA_TNUMBER)
			return FALSE;
		
		if(type=='i'){
			d=lua_tonumber(st, pos);
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
		if(lua_isboolean(st, pos)){
			if(valret)
				*((bool*)valret)=lua_toboolean(st, pos);
			return TRUE;
		}
		if(lua_isnumber(st, pos)){
			if(valret)
				*((bool*)valret)=(lua_tonumber(st, pos)==0 ? FALSE : TRUE);
			return TRUE;
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
		if(valret)
			*((int*)valret)=lua_ref(st, pos);
		return TRUE;
	}

	if(type=='t'){
		if(!lua_istable(st, pos))
			return FALSE;
		if(valret)
			*((int*)valret)=lua_ref(st, pos);
		return TRUE;
	}

	if(type=='o'){
		if(!extl_verify_wobj(st, pos))
			return FALSE;
		if(valret)
			*((WObj**)valret)=extl_do_get_obj(st, pos);
		return TRUE;
	}
	
	return FALSE;
}


static bool extl_stack_push(lua_State *st, char spec, void *ptr)
{
	if(spec=='i'){
		lua_pushnumber(st, *(int*)ptr);
	}else if(spec=='d'){
		lua_pushnumber(st, *(double*)ptr);
	}else if(spec=='b'){
		lua_pushboolean(st, *(bool*)ptr);
	}else if(spec=='o'){
		if(!extl_push_obj(st, *(WObj**)ptr)){
			lua_pushnil(st);
			return FALSE;
		}
	}else if(spec=='s' || spec=='S'){
		lua_pushstring(st, *(char**)ptr);
		if(spec=='s')
			free(*(char**)ptr);
	}else if(spec=='t' || spec=='f'){
		lua_rawgeti(l_st, LUA_REGISTRYINDEX, *(int*)ptr);
		return !lua_isnil(st, -1);
	}else{
		lua_pushnil(st);
		return FALSE;
	}
	
	return TRUE;
}


/*}}}*/


/*{{{ References misc. */


/* lua_getref macros in 5.0 don't behave as before */
static bool extl_getref(lua_State *st, int ref)
{
	lua_rawgeti(l_st, LUA_REGISTRYINDEX, ref);
	if(lua_isnil(st, -1)){
		lua_pop(st, 1);
		return FALSE;
	}
	return TRUE;
}
	
	
ExtlFn extl_unref_fn(ExtlFn ref)
{
	lua_unref(l_st, ref);
	return LUA_NOREF;
}


ExtlFn extl_unref_table(ExtlTab ref)
{
	lua_unref(l_st, ref);
	return LUA_NOREF;
}

					   
ExtlFn extl_fn_none()
{
	return LUA_NOREF;
}


ExtlTab extl_table_none()
{
	return LUA_NOREF;
}


ExtlTab extl_ref_table(ExtlTab ref)
{
	if(extl_getref(l_st, ref)==0)
		return LUA_NOREF;
	return lua_ref(l_st, ref);
}


ExtlFn extl_ref_fn(ExtlFn ref)
{
	if(extl_getref(l_st, ref)==0)
		return LUA_NOREF;
	return lua_ref(l_st, ref);
}


/*}}}*/


/*{{{ Tables */


static bool extl_table_do_gets(ExtlTab ref, const char *entry, char type,
							   void *valret)
{
	bool ret=FALSE;
	int oldtop=lua_gettop(l_st);
	
	if(extl_getref(l_st, ref)==0)
		return FALSE;
	
	lua_pushstring(l_st, entry);
	lua_gettable(l_st, -2);
	ret=extl_stack_get(l_st, -1, type, TRUE, valret);
	
	lua_settop(l_st, oldtop);
	
	return ret;
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


int extl_table_get_n(ExtlTab ref)
{
	int oldtop=lua_gettop(l_st);
	int n=0;
	
	if(!lua_checkstack(l_st, 8 /*something that should be enough*/))
		return 0;
	
	if(extl_getref(l_st, ref)==0){
		fprintf(stderr, "nilref\n");
		return 0;
	}
	
	lua_getglobal(l_st, "table");
	lua_pushstring(l_st, "getn");
	lua_gettable(l_st, -2);
	lua_pushvalue(l_st, -3);
	if(lua_pcall(l_st, 1, 1, 0)==0){
		n=lua_tonumber(l_st, -1);
	}else{
		warn("%s", lua_tostring(l_st, -1));
	}
	
	lua_settop(l_st, oldtop);
	
	return n;
}


static bool extl_table_do_geti(ExtlTab ref, int entry, char type, void *valret)
{
	bool ret=FALSE;
	int oldtop=lua_gettop(l_st);
	
	if(extl_getref(l_st, ref)==0)
		return FALSE;
	
	lua_rawgeti(l_st, -1, entry);
	ret=extl_stack_get(l_st, -1, type, TRUE, valret);
	
	lua_settop(l_st, oldtop);
	
	return ret;
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


/*{{{ Function calls to Lua */


static bool extl_push_args(lua_State *st, bool intab, const char *spec,
						   va_list args)
{
	int i=1;
	
	while(*spec!='\0'){
		if(intab)
			lua_pushnumber(l_st, i);
		switch(*spec){
		case 'i':
			lua_pushnumber(l_st, (double)va_arg(args, int));
			break;
		case 'd':
			lua_pushnumber(l_st, va_arg(args, double));
			break;
		case 'b':
			lua_pushboolean(l_st, va_arg(args, bool));
			break;
		case 'o':
			if(!extl_push_obj(l_st, va_arg(args, WObj*)))
				return FALSE;
			break;
		case 'S':
		case 's':
			lua_pushstring(l_st, va_arg(args, char*));
			break;
		case 'f':
		case 't':
			if(!extl_getref(l_st, va_arg(args, int)))
				return FALSE;
			break;
		default:
			return FALSE;
		}
		if(intab)
			lua_rawset(l_st, -3);
		i++;
		spec++;
	}
	
	return TRUE;
}


static void extl_free_param(void *ptr, char spec, bool strings)
{
	if((spec=='s' || spec=='S') && strings && *(char**)ptr!=NULL){
		free(*(char**)ptr);
		*(char**)ptr=NULL;
	}else if(spec=='t'){
		extl_unref_table(*(ExtlTab*)ptr);
	}else if(spec=='f'){
		extl_unref_fn(*(ExtlFn*)ptr);
	}
}


static bool extl_get_retvals(lua_State *st, const char *spec, int m,
							 va_list args)
{
	int om=m;
	void *ptr;
	char *s;
	va_list oargs;
	
	va_copy(oargs,args);
	
	while(m>0){
		ptr=va_arg(args, void*);
		if(!extl_stack_get(st, -m, *spec, TRUE, ptr)){
			warn("Invalid return value.\n");
			goto fail;
		}
		spec++;
		m--;
	}
	
	va_end(oargs);
	
	return TRUE;
	
fail:
	/* There was a failure getting some return value. We have to free what
	 * was already gotten.
	 */
	spec-=(om-m);
	while(om>m){
		ptr=va_arg(oargs, void*);
		extl_free_param(ptr, *spec, TRUE);
		spec++;
		om--;
	}

	va_end(oargs);
	
	return FALSE;
}


static bool extl_do_call_vararg(lua_State *st, int oldtop, bool intab,
								const char *spec, const char *rspec,
								va_list args)
{
	bool ret=TRUE;
	int n=0, m=0;

	/* For dostring and dofile arguments are passed in the global table 'arg'.
	 */
	if(intab)
		lua_newtable(l_st);

	if(spec!=NULL){
		n=strlen(spec);
	
		/* +1 for extl_push_obj */
		if(!lua_checkstack(l_st, n+1)){
			warn("Stack full");
			return FALSE;
		}
		
		ret=extl_push_args(st, intab, spec, args);
	}

	if(ret){
		if(intab){
			lua_setglobal(l_st, "arg");
			n=0;
		}
		
		if(rspec!=NULL)
			m=strlen(rspec);
		
		if(lua_pcall(st, n, m, 0)!=0){
			warn_obj("extl_do_call_vararg", "%s", lua_tostring(st, -1));
			ret=FALSE;
		}else{
			if(m>0)
				ret=extl_get_retvals(st, rspec, m, args);
		}
		
		if(intab){
			lua_pushnil(l_st);
			lua_setglobal(l_st, "arg");
		}
	}
	
	lua_settop(l_st, oldtop);
	
	return ret;
}


/*{{{ extl_call */


bool extl_call_vararg(ExtlFn fnref, const char *spec,
					  const char *rspec, va_list args)
{
	int oldtop=lua_gettop(l_st);
	
	if(!extl_getref(l_st, fnref))
		return FALSE;
	
	return extl_do_call_vararg(l_st, oldtop, FALSE, spec, rspec, args);
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


bool extl_call_named_vararg(const char *name, const char *spec,
							const char *rspec, va_list args)
{
	int oldtop=lua_gettop(l_st);
	
	lua_getglobal(l_st, name);
	
	return extl_do_call_vararg(l_st, oldtop, FALSE, spec, rspec, args);
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


bool extl_dofile_vararg(const char *file, const char *spec,
						const char *rspec, va_list args)
{
	bool ret=FALSE;
	int oldtop;
	
	fprintf(stderr, "lua_dofile(%s)\n", file);
	
	oldtop=lua_gettop(l_st);
	lua_pushstring(l_st, file);
	lua_setglobal(l_st, "CURRENT_FILE");
	if(luaL_loadfile(l_st, file)!=0){
		warn("%s", lua_tostring(l_st, -1));
	}else{
		ret=extl_do_call_vararg(l_st, oldtop, TRUE, spec, rspec, args);
	}

restore:
	lua_pushnil(l_st);
	lua_setglobal(l_st, "CURRENT_FILE");
	
	lua_settop(l_st, oldtop);

	return ret;
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


bool extl_dostring_vararg(const char *string, const char *spec,
						  const char *rspec, va_list args)
{
	bool ret=FALSE;
	int oldtop;
	
	oldtop=lua_gettop(l_st);
	if(luaL_loadbuffer(l_st, string, strlen(string), string)!=0){
		warn("%s", lua_tostring(l_st, -1));
	}else{
		ret=extl_do_call_vararg(l_st, oldtop, TRUE, spec, rspec, args);
	}

	lua_settop(l_st, oldtop);

	return ret;
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


/*{{{ Function calls from lua */


static const char **extl_safelist=NULL;


const char **extl_set_safelist(const char **newlist)
{
	const char **oldlist=extl_safelist;
	extl_safelist=newlist;
	return oldlist;
}
	
	
#define MAX_PARAMS 16


static int extl_l1_call_handler2(lua_State *st)
{
	ExtlL2Param ip[MAX_PARAMS]={NULL, };
	ExtlL2Param op[MAX_PARAMS]={NULL, };
	ExtlExportedFnSpec *spec;
	int i, ni, no, top;
	bool ret;

	top=lua_gettop(st);
	
	spec=(ExtlExportedFnSpec*)lua_touserdata(st, lua_upvalueindex(1));
	if(spec==NULL){
		warn("L1 call handler upvalues corrupt.");
		return 0;
	}
	
	if(spec->fn==NULL){
		warn("Called function has been unregistered");
		return 0;
	}
	
	/*fprintf(stderr, "%s called\n", spec->name);*/
	
	/* Check safelist */
	if(extl_safelist!=NULL){
		for(i=0; extl_safelist[i]!=NULL; i++){
			if(strcmp(spec->name, extl_safelist[i])==0)
				break;
		}
		if(extl_safelist[i]==NULL){
			warn("Attempt to call an unsafe function in restricted mode.");
			return 0;
		}
	}
	
	ni=(spec->ispec==NULL ? 0 : strlen(spec->ispec));
	no=(spec->ospec==NULL ? 0 : strlen(spec->ospec));
	
	/* +1 for extl_push_obj */
	if(!lua_checkstack(st, no+1)){
		warn("Stack full");
		return 0;
	}
	
	for(i=0; i<ni; i++){
		if(!extl_stack_get(st, i+1, spec->ispec[i], FALSE, (void*)&(ip[i]))){
			warn("Invalid arguments. Template is '%s'.", spec->ispec);
			while(--i>=0)
				extl_free_param((void*)&(ip[i]), spec->ispec[i], FALSE);
			return 0;
		}
	}
	
	ret=spec->l2handler(spec->fn, ip, op);
	
	for(i=0; i<ni; i++)
		extl_free_param((void*)&(ip[i]), spec->ispec[i], FALSE);

	if(!ret)
		return 0;
	
	for(i=0; i<no; i++)
		extl_stack_push(st, spec->ospec[i], (void*)&(op[i]));

	return no;
}




static bool need_trace=FALSE;
static WarnHandler *old_warn_handler=NULL;
static bool wh_has_been_set=FALSE;

static void l1_warn_handler(const char *message)
{
	static int called=0;
	
	if(old_warn_handler!=NULL && called==0){
		called++;
		old_warn_handler(message);
		called--;
	}else{
		fprintf(stderr, "%s: %s\n", prog_execname(), message);
	}
	
	need_trace=TRUE;
}


static void extl_stack_trace(lua_State *st)
{
	lua_Debug ar;
	int lvl=1;
	
	warn("Stack trace: ");
				 
	for( ; lua_getstack(st, lvl, &ar); lvl++){
		if(lua_getinfo(st, "Sln", &ar)==0){
			warn("  (Unable to get debug info for level %d)", lvl);
		}else{
			warn("  %s\n   current line: %d, function: %s",
				 ar.source, ar.currentline, ar.name);
		}
	}
}


static void clean_warn_handler(const char *msg)
{
	fwrite(msg, sizeof(char), strlen(msg), stderr);
	fputc('\n', stderr);
}


static int extl_l1_call_handler(lua_State *st)
{
	bool old_need_trace=need_trace;
	WarnHandler *old_old_warn_handler=old_warn_handler;
	int ret;
	
	old_warn_handler=set_warn_handler(l1_warn_handler);
	
	ret=extl_l1_call_handler2(st);

	if(need_trace){
		if(old_warn_handler==NULL)
			set_warn_handler(clean_warn_handler);
		else
			set_warn_handler(old_warn_handler);
		extl_stack_trace(st);
		set_warn_handler(l1_warn_handler);
	}
	
	set_warn_handler(old_warn_handler);
	old_warn_handler=old_old_warn_handler;
	
	need_trace=old_need_trace;
	
	return ret;
}


bool extl_register_function(ExtlExportedFnSpec *spec)
{
	ExtlExportedFnSpec *spec2;
	int oldtop;
	
	if((spec->ispec!=NULL && strlen(spec->ispec)>MAX_PARAMS) ||
	   (spec->ospec!=NULL && strlen(spec->ospec)>MAX_PARAMS)){
		warn("Function '%s' has more parameters than the level 1 "
			 "call handler can handle\n", spec->name);
		return FALSE;
	}

	oldtop=lua_gettop(l_st);

	spec2=lua_newuserdata(l_st, sizeof(ExtlExportedFnSpec));
	if(spec2==NULL){
		warn("Unable to create upvalue for '%s'\n", spec->name);
		return FALSE;
	}
	memcpy(spec2, spec, sizeof(ExtlExportedFnSpec));

	/*lua_pushlightuserdata(l_st, spec);*/
	lua_getregistry(l_st);
	lua_pushvalue(l_st, -2); /* Get spec2 */
	lua_pushfstring(l_st, "ioncore_luaextl_%s_upvalue", spec->name);
	lua_settable(l_st, -3); /* Set registry.ioncore_luaextl_fn_upvalue=spec2 */
	lua_pop(l_st, 1); /* Pop registry */
	lua_pushcclosure(l_st, extl_l1_call_handler, 1);
	lua_setglobal(l_st, spec->name);
	
	lua_settop(l_st, oldtop);
	
	return TRUE;
}


void extl_unregister_function(ExtlExportedFnSpec *spec)
{
	ExtlExportedFnSpec *spec2;
	
	lua_getregistry(l_st);
	lua_pushfstring(l_st, "ioncore_luaextl_%s_upvalue", spec->name);
	lua_gettable(l_st, -2); /* Get registry.ioncore_luaextl_fn_upvalue */
	spec2=lua_touserdata(l_st, -1);
	if(spec2!=NULL){
		spec2->ispec=NULL;
		spec2->ospec=NULL;
		spec2->fn=NULL;
		spec2->name=NULL;
		spec2->l2handler=NULL;
	}
	
	lua_pushnil(l_st);
	lua_setglobal(l_st, spec->name);
}


/*}}}*/



/*{{{ Misc */


int extl_complete_fn(char *nam, char ***cp_ret, char **beg, void *unused)
{
	int oldtop=lua_gettop(l_st);
	int n=0;
	int l=strlen(nam);
	const char *fnam;
	
	lua_pushnil(l_st);
	
	for(; lua_next(l_st, LUA_GLOBALSINDEX)!=0; lua_pop(l_st, 1)){
		if(!lua_isfunction(l_st, -1))
			continue;
		
		fnam=lua_tostring(l_st, -2);
		
		if(fnam==NULL)
			continue;
			
		if(strncmp(nam, fnam, l)==0)
			add_to_complist_copy(cp_ret, &n, fnam);
	}
	
	lua_settop(l_st, oldtop);
	
	return n;
}
	
	
/*}}}*/


