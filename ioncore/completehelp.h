/*
 * wmcore/completehelp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef WMCORE_COMPLETEHELP_H
#define WMCORE_COMPLETEHELP_H

bool add_to_complist(char ***cp_ret, int *n, char *name);
bool add_to_complist_copy(char ***cp_ret, int *n, const char *nam);

#endif /* WMCORE_COMPLETEHELP_H */
