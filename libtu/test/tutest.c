/*
 * libtu/tester.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <stdio.h>

#include "../misc.h"
#include "../tokenizer.h"
#include "../util.h"

int test_get_token() {
    Tokenizer*tokz;
    Token tok=TOK_INIT;

    if(!(tokz=tokz_open("data_gettoken")))
        return 1;

    // 1
    if(tokz_get_token(tokz, &tok) == FALSE)
        return 11;
    if (tok.type != TOK_LONG)
        return 12;
    if (TOK_LONG_VAL(&tok) != 1)
        return 13;
    
    // \n
    if(tokz_get_token(tokz, &tok) == FALSE)
        return 21;
    if (tok.type != TOK_OP)
        return 22;

    // 2.3
    if(tokz_get_token(tokz, &tok) == FALSE)
        return 31;
    if (tok.type != TOK_DOUBLE)
        return 32;
    if (TOK_DOUBLE_VAL(&tok) != 2.3){
        fprintf(stderr, "Expected 2.3, got %f\n", TOK_DOUBLE_VAL(&tok));
        return 33;
    }

    // ignore comment and go to next line
    if(tokz_get_token(tokz, &tok) == FALSE)
        return 41;
    if (tok.type != TOK_OP)
        return 42;
    if (TOK_OP_VAL(&tok) != OP_NEXTLINE)
        return 43;

    // ignore line
    if(tokz_get_token(tokz, &tok) == FALSE)
        return 51;
    if (tok.type != TOK_OP)
        return 52;
    if (TOK_OP_VAL(&tok) != OP_EOF)
        return 53;

    tokz_close(tokz);
    /*
    while(tokz_get_token(tokz, &tok)){
        switch(tok.type){
        case TOK_LONG:
            printf("long - %ld\n", TOK_LONG_VAL(&tok));
            break;
        case TOK_DOUBLE:
            printf("double - %g\n", TOK_DOUBLE_VAL(&tok));
            break;
        case TOK_CHAR:
            printf("char - '%c'\n", TOK_CHAR_VAL(&tok));
            break;
        case TOK_STRING:
            printf("string - \"%s\"\n", TOK_STRING_VAL(&tok));
            break;
        case TOK_IDENT:
            printf("ident - %s\n", TOK_IDENT_VAL(&tok));
            break;
        case TOK_COMMENT:
            printf("comment - %s\n", TOK_COMMENT_VAL(&tok));
            break;
        case TOK_OP:
            printf("operator - %03x\n", TOK_OP_VAL(&tok));
            break;
        }
    }
    */
    return 0;
}

int main(int argc, char *argv[])
{
    fprintf(stdout, "[TESTING] libtu ====\n");
    libtu_init(argv[0]);

    int result = 0;
    int err = 0;

    fprintf(stdout, "[TEST] test_get_token: ");
    result = test_get_token();
    if (result != 0) {
        fprintf(stdout, "[ERROR]: %d\n", result);
        err += 1;
    } else {
        fprintf(stdout, "[OK]\n");
    }

    return err;
}

