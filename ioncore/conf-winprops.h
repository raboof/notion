/*
 * ion/ioncore/winprops.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_IONCORE_WINPROPS_H
#define ION_IONCORE_WINPROPS_H

#include <libtu/parser.h>
#include "common.h"
#include "winprops.h"

extern bool ioncore_begin_winprop(Tokenizer *tokz, int n, Token *toks);
extern ConfOpt ioncore_winprop_opts[];

#endif /* ION_IONCORE_WINPROPS_H */
