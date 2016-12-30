/*
 * libtu/parser.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_PARSER_H
#define LIBTU_PARSER_H

#include "tokenizer.h"

/*
 * format:
 *     l = long
 *    d = double
 *     i = identifier
 *     s = string
 *     c = char
 *  . = 1 times any     ("l.d")
 *  * = 0 or more times any (must be the last, "sd*")
 *     ? = optional        ("?c")
 *     : = conditional        (":c:s")
 *  + = 1 or more times last (most be the last, "l+")
 * special entries:
 *
 * "#end"         call this handler at the end of section.
 * "#cancel"     call this handler when recovering from error
 */

#define END_CONFOPTS {NULL, NULL, NULL, NULL}

typedef struct _ConfOpt{
    const char *optname;
    const char *argfmt;
    bool (*fn)(Tokenizer *tokz, int n, Token *toks);
    struct _ConfOpt *opts;
} ConfOpt;

#define CONFOPTS_NOT_SET libtu_dummy_confopts
extern ConfOpt libtu_dummy_confopts[];

extern bool parse_config_tokz(Tokenizer *tokz, const ConfOpt *options);
extern bool parse_config_tokz_skip_section(Tokenizer *tokz);
extern bool parse_config(const char *fname, const ConfOpt *options, int flags);
extern bool parse_config_file(FILE *file, const ConfOpt *options, int flags);
extern bool check_args(const Tokenizer *tokz, Token *tokens, int ntokens,
                       const char *fmt);
extern bool check_args_loose(const Tokenizer *tokz, Token *tokens, int ntokens,
                             const char *fmt);

#endif /* LIBTU_PARSER_H */
