/*
 * ion/lua/luaextl.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_LUA_LUAEXTL_H
#define ION_LUA_LUAEXTL_H

#include <stdarg.h>

#include <ioncore/common.h>
#include <ioncore/objp.h>

/* o: userdata/WObj
 * i: integer
 * d: double
 * b: boolean
 * s: string
 * t: table
 * f: function (c or lua)
 * v: void
 */

/* In ioncore/common.h
#define EXTL_EXPORT
*/

typedef int ExtlTab;
typedef int ExtlFn;

typedef union{
	WObj *o;
	int i;
	double d;
	bool b;
	char *s;
	ExtlFn f;
	ExtlTab t;
} ExtlL2Param;

typedef bool ExtlL2CallHandler(void (*fn)(), ExtlL2Param *in,
							   ExtlL2Param *out);

typedef struct{
	const char *name;
	void (*fn)(void);
	const char *ispec;
	const char *ospec;
	ExtlL2CallHandler *l2handler;
} ExtlExportedFnSpec;


extern ExtlFn extl_unref_fn(ExtlFn ref);
extern ExtlTab extl_unref_table(ExtlTab ref);
extern ExtlFn extl_fn_none();
extern ExtlTab extl_table_none();
/* These should be called to store function and table references gotten
 * as arguments to functions.
 */
extern ExtlFn extl_ref_fn(ExtlFn ref);
extern ExtlTab extl_ref_table(ExtlTab ref);

extern bool extl_table_gets_o(ExtlTab ref, const char *entry, WObj **ret);
extern bool extl_table_gets_i(ExtlTab ref, const char *entry, int *ret);
extern bool extl_table_gets_d(ExtlTab ref, const char *entry, double *ret);
extern bool extl_table_gets_b(ExtlTab ref, const char *entry, bool *ret);
extern bool extl_table_gets_s(ExtlTab ref, const char *entry, char **ret);
extern bool extl_table_gets_f(ExtlTab ref, const char *entry, ExtlFn *ret);
extern bool extl_table_gets_t(ExtlTab ref, const char *entry, ExtlTab *ret);

extern int extl_table_get_n(ExtlTab ref);
extern bool extl_table_geti_o(ExtlTab ref, int entry, WObj **ret);
extern bool extl_table_geti_i(ExtlTab ref, int entry, int *ret);
extern bool extl_table_geti_d(ExtlTab ref, int entry, double *ret);
extern bool extl_table_geti_b(ExtlTab ref, int entry, bool *ret);
extern bool extl_table_geti_s(ExtlTab ref, int entry, char **ret);
extern bool extl_table_geti_f(ExtlTab ref, int entry, ExtlFn *ret);
extern bool extl_table_geti_t(ExtlTab ref, int entry, ExtlTab *ret);

extern bool extl_call_vararg(ExtlFn fnref, const char **safelist,
							 const char *spec, const char *rspec,
							 va_list args);
extern bool extl_call(ExtlFn fnref, const char *spec,
					  const char *rspec, ...);
extern bool extl_call_restricted(ExtlFn fnref, const char **safelist,
								 const char *spec, const char *rspec, ...);
extern bool extl_call_named_vararg(const char *name, const char *spec,
								   const char *rspec, va_list args);
extern bool extl_call_named(const char *name, const char *spec,
							const char *rspec, ...);

extern bool extl_register_function(ExtlExportedFnSpec *spec);
extern void extl_unregister_function(ExtlExportedFnSpec *spec);

extern bool extl_runfile(const char *file);
extern const char* extl_extension();

extern bool extl_init();

extern int extl_complete_fn(char *nam, char ***cp_ret, char **beg,
							void *unused);

#endif /* ION_LUA_LUAEXTL_H */
