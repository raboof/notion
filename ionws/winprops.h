/*
 * ion/winprops.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef ION_WINPROPS_H
#define ION_WINPROPS_H

#include <libtu/parser.h>
#include <wmcore/common.h>
#include <wmcore/winprops.h>

extern bool ion_begin_winprop(Tokenizer *tokz, int n, Token *toks);
extern ConfOpt ion_winprop_opts[];

#endif /* ION_WINPROPS_H */
