/*
 * ion/lua/luaextl.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_LUA_LUAEXTL_H
#define ION_LUA_LUAEXTL_H

#include <stdarg.h>

#include <libtu/obj.h>

#define EXTL_EXTENSION    "lua"
#define EXTL_COMPILED_EXTENSION    "lc"

/* o: userdata/Obj
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
#define EXTL_EXPORT_AS(X)
*/

typedef int ExtlTab;
typedef int ExtlFn;

typedef union{
    Obj *o;
    int i;
    double d;
    bool b;
    const char *s;
    ExtlFn f;
    ExtlTab t;
} ExtlL2Param;

typedef bool ExtlL2CallHandler(void (*fn)(), ExtlL2Param *in,
                               ExtlL2Param *out);

typedef void ExtlExportedFn(void);

typedef struct{
    const char *name;
    ExtlExportedFn *fn;
    const char *ispec;
    const char *ospec;
    ExtlL2CallHandler *l2handler;
} ExtlExportedFnSpec;

typedef ExtlExportedFn *ExtlSafelist;

extern ExtlFn extl_unref_fn(ExtlFn ref);
extern ExtlTab extl_unref_table(ExtlTab ref);
extern ExtlFn extl_fn_none();
extern ExtlTab extl_table_none();
/* These should be called to store function and table references gotten
 * as arguments to functions.
 */
extern ExtlFn extl_ref_fn(ExtlFn ref);
extern ExtlTab extl_ref_table(ExtlTab ref);

extern ExtlTab extl_create_table();
extern ExtlTab extl_globals();

/* Table/get */
extern bool extl_table_get_vararg(ExtlTab ref, char itype, char type, 
                                  va_list *args);
extern bool extl_table_get(ExtlTab ref, char itype, char type, ...);

extern bool extl_table_gets_o(ExtlTab ref, const char *entry, Obj **ret);
extern bool extl_table_gets_i(ExtlTab ref, const char *entry, int *ret);
extern bool extl_table_gets_d(ExtlTab ref, const char *entry, double *ret);
extern bool extl_table_gets_b(ExtlTab ref, const char *entry, bool *ret);
extern bool extl_table_gets_s(ExtlTab ref, const char *entry, char **ret);
extern bool extl_table_gets_f(ExtlTab ref, const char *entry, ExtlFn *ret);
extern bool extl_table_gets_t(ExtlTab ref, const char *entry, ExtlTab *ret);

extern int extl_table_get_n(ExtlTab ref);
extern bool extl_table_geti_o(ExtlTab ref, int entry, Obj **ret);
extern bool extl_table_geti_i(ExtlTab ref, int entry, int *ret);
extern bool extl_table_geti_d(ExtlTab ref, int entry, double *ret);
extern bool extl_table_geti_b(ExtlTab ref, int entry, bool *ret);
extern bool extl_table_geti_s(ExtlTab ref, int entry, char **ret);
extern bool extl_table_geti_f(ExtlTab ref, int entry, ExtlFn *ret);
extern bool extl_table_geti_t(ExtlTab ref, int entry, ExtlTab *ret);

/* Table/set */
extern bool extl_table_set_vararg(ExtlTab ref, char itype, char type, 
                                  va_list *args);
extern bool extl_table_set(ExtlTab ref, char itype, char type, ...);

extern bool extl_table_sets_o(ExtlTab ref, const char *entry, Obj *val);
extern bool extl_table_sets_i(ExtlTab ref, const char *entry, int val);
extern bool extl_table_sets_d(ExtlTab ref, const char *entry, double val);
extern bool extl_table_sets_b(ExtlTab ref, const char *entry, bool val);
extern bool extl_table_sets_s(ExtlTab ref, const char *entry, const char *val);
extern bool extl_table_sets_f(ExtlTab ref, const char *entry, ExtlFn val);
extern bool extl_table_sets_t(ExtlTab ref, const char *entry, ExtlTab val);

extern bool extl_table_seti_o(ExtlTab ref, int entry, Obj *val);
extern bool extl_table_seti_i(ExtlTab ref, int entry, int val);
extern bool extl_table_seti_d(ExtlTab ref, int entry, double val);
extern bool extl_table_seti_b(ExtlTab ref, int entry, bool val);
extern bool extl_table_seti_s(ExtlTab ref, int entry, const char *val);
extern bool extl_table_seti_f(ExtlTab ref, int entry, ExtlFn val);
extern bool extl_table_seti_t(ExtlTab ref, int entry, ExtlTab val);

/* Table/clear */

extern bool extl_table_clear_vararg(ExtlTab ref, char itype, va_list *args);
extern bool extl_table_clear(ExtlTab ref, char itype, ...);

extern bool extl_table_clears(ExtlTab ref, const char *entry);
extern bool extl_table_cleari(ExtlTab ref, int entry);

/* Call */

extern const ExtlSafelist *extl_set_safelist(const ExtlSafelist *sl);

extern bool extl_call_vararg(ExtlFn fnref, const char *spec,
                             const char *rspec, va_list *args);
extern bool extl_call(ExtlFn fnref, const char *spec,
                      const char *rspec, ...);

extern bool extl_call_named_vararg(const char *name, const char *spec,
                                   const char *rspec, va_list *args);
extern bool extl_call_named(const char *name, const char *spec,
                            const char *rspec, ...);

/* Load file/string */

extern bool extl_loadfile(const char *file, ExtlFn *ret);
extern bool extl_loadstring(const char *file, ExtlFn *ret);

/* Register */

extern bool extl_register_function(ExtlExportedFnSpec *spec);
extern void extl_unregister_function(ExtlExportedFnSpec *spec);
extern bool extl_register_functions(ExtlExportedFnSpec *spec);
extern void extl_unregister_functions(ExtlExportedFnSpec *spec);

bool extl_register_class(const char *cls, ExtlExportedFnSpec *fns,
                         const char *parent);
void extl_unregister_class(const char *cls, ExtlExportedFnSpec *fns);

bool extl_register_module(const char *cls, ExtlExportedFnSpec *fns);
void extl_unregister_module(const char *cls, ExtlExportedFnSpec *fns);

/* Misc. */

extern bool extl_init();
extern void extl_deinit();

#endif /* ION_LUA_LUAEXTL_H */
