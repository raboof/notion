/*
 * libextl/private.h
 *
 * Copyright (c) Tuomo Valkonen 2004-2005.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#ifndef LIBEXTL_PRIVATE_H
#define LIBEXTL_PRIVATE_H

#include <libtu/types.h>
#include <libtu/misc.h>
#include <libtu/locale.h>
#include <libtu/output.h>
#include <libtu/debug.h>
#include <libtu/objp.h>

#include "types.h"

/*
 * String routines
 */

#define extl_scopy(S) scopy(S)
#define extl_scopyn(S, N) scopyn(S, N)
#define extl_scat3(S1, S2, S3) scat3(S1, S2, S3)
#define extl_asprintf libtu_asprintf

/*
 * Error display
 */

#if 1
#include <libtu/errorlog.h>
#define EXTL_LOG_ERRORS
/* This part has not been abstracted away from libtu dependence. */
#endif

#define extl_warn warn
#define extl_warn_err_obj(NM) warn_err_obj(NM)

/*#define extl_warn(FMT, ...)
    ({fprintf(stderr, FMT, __VA_ARGS__); fputc('\n', stderr);})*/
/*#define extl_warn_obj_err(O) perror(O) */

/*
 * Level2 type checking
 */

/* Always returns FALSE. */
extern bool extl_obj_error(int ndx, const char *got, const char *wanted);

#define EXTL_CHKO1(IN, NDX, TYPE)                          \
    (OBJ_IS(IN[NDX].o, TYPE)                               \
     ? TRUE                                                \
     : extl_obj_error(NDX, OBJ_TYPESTR(IN[NDX].o), #TYPE))

#define EXTL_CHKO(IN, NDX, TYPE)                           \
    (IN[NDX].o==NULL || OBJ_IS(IN[NDX].o, TYPE)            \
     ? TRUE                                                \
     : extl_obj_error(NDX, OBJ_TYPESTR(IN[NDX].o), #TYPE))

#define EXTL_DEFCLASS(C) INTRCLASS(C)


/*
 * Objects.
 */

typedef Watch ExtlProxy;

#define EXTL_OBJ_CACHED(OBJ) ((OBJ)->flags&OBJ_EXTL_CACHED)
#define EXTL_OBJ_OWNED(OBJ) ((OBJ)->flags&OBJ_EXTL_OWNED)
#define EXTL_OBJ_TYPENAME(OBJ) OBJ_TYPESTR(OBJ)
#define EXTL_OBJ_IS(OBJ, NAME) obj_is_str(OBJ, NAME)

#define EXTL_PROXY_OBJ(PROXY) ((PROXY)->obj)

#define EXTL_BEGIN_PROXY_OBJ(PROXY, OBJ)       \
    watch_init(PROXY);                         \
    watch_setup(PROXY, OBJ, NULL);             \
    (OBJ)->flags|=OBJ_EXTL_CACHED;

#define EXTL_END_PROXY_OBJ(PROXY, OBJ) \
    assert((PROXY)->obj==OBJ);         \
    watch_reset(PROXY);                \
    (OBJ)->flags&=~OBJ_EXTL_CACHED;

#define EXTL_DESTROY_OWNED_OBJ(OBJ) destroy_obj(OBJ)

extern void extl_uncache(Obj *obj);

/*
static void obj_dest_handler(Watch *watch, Obj *obj)
{
    extl_uncache(obj);
    obj->flags&=~OBJ_EXTL_CACHED;
}
*/

/*
 * Miscellaneous.
 */

/* Translate string X to locale. */
/*#define TR(X) X */

/* Debugging. */
/*#define D(X) */


#endif /* LIBEXTL_PRIVATE_H */
