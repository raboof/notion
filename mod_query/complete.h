/*
 * ion/mod_query/complete.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007.
 *
 * See the included file LICENSE for details.
 */

#ifndef ION_MOD_QUERY_COMPLETE_H
#define ION_MOD_QUERY_COMPLETE_H

#include <libtu/obj.h>
#include <libextl/extl.h>
#include <ioncore/common.h>
#include "edln.h"
#include "wedln.h"

INTRCLASS(WComplProxy);

DECLCLASS(WComplProxy){
    Obj o;
    Watch wedln_watch;
    int id;
    int cycle;
};


extern WComplProxy *create_complproxy(WEdln *wedln, int id, int cycle);

extern bool complproxy_set_completions(WComplProxy *proxy, ExtlTab compls);


extern int edln_do_completions(Edln *edln, char **completions, int ncomp,
                               const char *beg, const char *end,
                               bool setcommon, bool nosort);
extern void edln_set_completion(Edln *edln, const char *comp,
                                const char *beg, const char *end);

#endif /* ION_MOD_QUERY_COMPLETE_H */
