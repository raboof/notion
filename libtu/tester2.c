/*
 * libtu/tester2.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <stdio.h>

#include "misc.h"
#include "tokenizer.h"
#include "parser.h"
#include "util.h"


static bool test_fn(Tokenizer *tokz, int n, Token *toks)
{
    printf("test_fn() %d %s\n", n, TOK_IDENT_VAL(toks));

    return TRUE;
}



static bool sect_fn(Tokenizer *tokz, int n, Token *toks)
{
    printf("sect_fn() %d %s\n", n, TOK_IDENT_VAL(toks+1));

    return TRUE;
}


static bool test2_fn(Tokenizer *tokz, int n, Token *toks)
{
    printf("test2_fn() %d %s %f\n", n, TOK_BOOL_VAL(toks+1) ? "TRUE" : "FALSE", TOK_DOUBLE_VAL(toks+2));

    return TRUE;
}

static bool test3_fn(Tokenizer *tokz, int n, Token *toks)
{
    if(n<=2)
        printf("test3_fn() %d \"%s\"\n", n, TOK_STRING_VAL(toks+1));
    else
        printf("test3_fn() %d \"%s\" %ld\n", n, TOK_STRING_VAL(toks+1), TOK_LONG_VAL(toks+2));

    return TRUE;
}


static ConfOpt opts[]={
    {"test", NULL, test_fn, NULL},
    {"t2", "bd", test2_fn, NULL},
    {"foo", "s?l", test3_fn, NULL},
    {"sect", "s", sect_fn, opts},
    {NULL, NULL, NULL, NULL}
};


int main(int argc, char *argv[])
{
    libtu_init(argv[0]);
    parse_config_file(stdin, opts, TOKZ_ERROR_TOLERANT);

    return EXIT_SUCCESS;
}

