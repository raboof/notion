/*
 * ion/ioncore/completehelp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_COMPLETEHELP_H
#define ION_IONCORE_COMPLETEHELP_H

bool add_to_complist(char ***cp_ret, int *n, char *name);
bool add_to_complist_copy(char ***cp_ret, int *n, const char *nam);

#endif /* ION_IONCORE_COMPLETEHELP_H */
