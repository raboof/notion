/*
 * ion/mainloop/hooks.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_LIBMAINLOOP_HOOKS_H
#define ION_LIBMAINLOOP_HOOKS_H

#include <libtu/types.h>
#include <libextl/extl.h>

INTRSTRUCT(WHookItem);
INTRCLASS(WHook);

typedef void WHookDummy();
typedef bool WHookMarshall(WHookDummy *fn, void *param);
typedef bool WHookMarshallExtl(ExtlFn fn, void *param);

DECLSTRUCT(WHookItem){
    WHookDummy *fn;
    ExtlFn efn;
    WHookItem *next, *prev;
};

DECLCLASS(WHook){
    Obj obj;
    WHookItem *items;
};


/* If hk==NULL to register, new is attempted to be created. */
extern WHook *mainloop_register_hook(const char *name, WHook *hk);
extern WHook *mainloop_unregister_hook(const char *name, WHook *hk);
extern WHook *mainloop_get_hook(const char *name);

extern WHook *create_hook();
extern bool hook_init(WHook *hk);
extern void hook_deinit(WHook *hk);

extern bool hook_add(WHook *hk, WHookDummy *fn);
extern bool hook_remove(WHook *hk, WHookDummy *fn);
extern WHookItem *hook_find(WHook *hk, WHookDummy *fn);

extern bool hook_add_extl(WHook *hk, ExtlFn fn);
extern bool hook_remove_extl(WHook *hk, ExtlFn fn);
extern WHookItem *hook_find_extl(WHook *hk, ExtlFn efn);

extern void hook_call(const WHook *hk, void *p,
                      WHookMarshall *m, WHookMarshallExtl *em);
extern void hook_call_v(const WHook *hk);
extern void hook_call_o(const WHook *hk, Obj *o);
extern void hook_call_p(const WHook *hk, void *p, WHookMarshallExtl *em);

extern bool hook_call_alt(const WHook *hk, void *p,
                          WHookMarshall *m, WHookMarshallExtl *em);
extern bool hook_call_alt_v(const WHook *hk);
extern bool hook_call_alt_o(const WHook *hk, Obj *o);
extern bool hook_call_alt_p(const WHook *hk, void *p, WHookMarshallExtl *em);


#endif /* ION_LIBMAINLOOP_HOOKS_H */
