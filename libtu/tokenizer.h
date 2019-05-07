/*
 * libtu/tokenizer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_TOKENIZER_H
#define LIBTU_TOKENIZER_H

#include <stdio.h>
#include "types.h"


#define TOK_SET_BOOL(TOK, VAL)         {(TOK)->type=TOK_BOOL; (TOK)->u.bval=VAL;}
#define TOK_SET_LONG(TOK, VAL)         {(TOK)->type=TOK_LONG; (TOK)->u.lval=VAL;}
#define TOK_SET_DOUBLE(TOK, VAL)     {(TOK)->type=TOK_DOUBLE; (TOK)->u.dval=VAL;}
#define TOK_SET_CHAR(TOK, VAL)         {(TOK)->type=TOK_CHAR; (TOK)->u.cval=VAL;}
#define TOK_SET_STRING(TOK, VAL)     {(TOK)->type=TOK_STRING; (TOK)->u.sval=VAL;}
#define TOK_SET_IDENT(TOK, VAL)     {(TOK)->type=TOK_IDENT; (TOK)->u.sval=VAL;}
#define TOK_SET_COMMENT(TOK, VAL)     {(TOK)->type=TOK_COMMENT; (TOK)->u.sval=VAL;}
#define TOK_SET_OP(TOK, VAL)         {(TOK)->type=TOK_OP; (TOK)->u.opval=VAL;}

#define TOK_TYPE(TOK)                ((TOK)->type)
#define TOK_BOOL_VAL(TOK)            ((TOK)->u.bval)
#define TOK_LONG_VAL(TOK)            ((TOK)->u.lval)
#define TOK_DOUBLE_VAL(TOK)            ((TOK)->u.dval)
#define TOK_CHAR_VAL(TOK)            ((TOK)->u.cval)
#define TOK_STRING_VAL(TOK)            ((TOK)->u.sval)
#define TOK_IDENT_VAL(TOK)            ((TOK)->u.sval)
#define TOK_COMMENT_VAL(TOK)        ((TOK)->u.sval)
#define TOK_OP_VAL(TOK)                ((TOK)->u.opval)

#define TOK_IS_INVALID(TOK)            ((TOK)->type==TOK_INVALID)
#define TOK_IS_BOOL(TOK)            ((TOK)->type==TOK_BOOL)
#define TOK_IS_LONG(TOK)            ((TOK)->type==TOK_LONG)
#define TOK_IS_DOUBLE(TOK)            ((TOK)->type==TOK_DOUBLE)
#define TOK_IS_CHAR(TOK)            ((TOK)->type==TOK_CHAR)
#define TOK_IS_STRING(TOK)            ((TOK)->type==TOK_STRING)
#define TOK_IS_IDENT(TOK)            ((TOK)->type==TOK_IDENT)
#define TOK_IS_COMMENT(TOK)            ((TOK)->type==TOK_COMMENT)
#define TOK_IS_OP(TOK)                ((TOK)->type==TOK_OP)

#define TOK_OP_IS(TOK, OP)            ((TOK)->type==TOK_OP && (TOK)->u.opval==(OP))

#define TOK_TAKE_STRING_VAL(TOK)    ((TOK)->type=TOK_INVALID, (TOK)->u.sval)
#define TOK_TAKE_IDENT_VAL(TOK)        ((TOK)->type=TOK_INVALID, (TOK)->u.sval)
#define TOK_TAKE_COMMENT_VAL(TOK)    ((TOK)->type=TOK_INVALID, (TOK)->u.sval)


enum{
    TOK_INVALID=0,
    TOK_LONG='l',
    TOK_DOUBLE='d',
    TOK_CHAR='c',
    TOK_STRING='s',
    TOK_IDENT='i',
    TOK_BOOL='b',
    TOK_COMMENT='#',
    TOK_OP='+'
};


enum{
#define OP2(X,Y)   ((X)|((Y)<<8))
#define OP3(X,Y,Z) ((X)|((Y)<<8)|((Z)<<16))

    OP_L_PAR=    '(', OP_R_PAR=    ')', OP_L_BRK=    '[', OP_R_BRK=    ']',
    OP_L_BRC=    '{', OP_R_BRC=    '}', OP_COMMA=    ',', OP_SCOLON=    ';',

    OP_PLUS=    '+', OP_MINUS=    '-', OP_MUL=    '*', OP_DIV=    '/',
    OP_MOD=        '%', OP_POW=    '^', OP_OR=     '|', OP_AND=    '&',
    /*OP_NOT=    '~',*/ OP_NOT=    '!', OP_ASGN=    '=', OP_LT=        '<',
    OP_GT=        '>', OP_DOT=    '.', OP_COLON=    ':', OP_QMARK=    '?',
    OP_AT=        '@',
    OP_NEXTLINE='\n',OP_EOF=    -1,

    OP_INC=        OP2('+','+'),         OP_DEC=    OP2('-','-'),
    OP_LSHIFT=    OP2('<','<'),          OP_RSHIFT=    OP2('>','>'),
    OP_AS_INC=    OP2('+','='),          OP_AS_DEC= OP2('-','='),
    OP_AS_MUL=    OP2('*','='),          OP_AS_DIV= OP2('/','='),
    OP_AS_MOD=    OP2('%','='),          OP_AS_POW= OP2('^','='),

/*    AS_OR=        OP2('|','='),         AS_AND=    OP2('&','='), */
    OP_EQ=        OP2('=','='),          OP_NE=        OP2('!','='),
    OP_LE=        OP2('<','='),          OP_GE=        OP2('>','=')

/*    L_AND=        OP2('&','&'), L_OR=        OP2('|','|'),
    L_XOR=        OP2('^','^'), */

/*    AsLShift=    OP3('<','<','='),
    AsRShift=    OP3('>','>','='), */

#undef OP2
#undef OP3
};


typedef struct{
    int type;
    int line;
    union{
        bool bval;
        long lval;
        double dval;
        char cval;
        char *sval;
        int opval;
    } u;
} Token;

#define TOK_INIT {0, 0, {0}}


extern void tok_free(Token*tok);
extern void tok_init(Token*tok);


/* */


enum{
    TOKZ_IGNORE_NEXTLINE=0x1,
    TOKZ_READ_COMMENTS=0x2,
    TOKZ_PARSER_INDENT_MODE=0x04,
    TOKZ_ERROR_TOLERANT=0x8,
    TOKZ_READ_FROM_BUFFER=0x10,
    TOKZ_DEFAULT_OPTION=0x20
};


enum{
    E_TOKZ_UNEXPECTED_EOF=1,
    E_TOKZ_UNEXPECTED_EOL,
    E_TOKZ_EOL_EXPECTED,
    E_TOKZ_INVALID_CHAR,
    E_TOKZ_TOOBIG,
    E_TOKZ_NUMFMT,
    E_TOKZ_NUM_JUNK,
    E_TOKZ_NOTINT,
    E_TOKZ_RANGE,
    E_TOKZ_MULTICHAR,

    E_TOKZ_TOKEN_LIMIT,
    E_TOKZ_UNKNOWN_OPTION,
    E_TOKZ_SYNTAX,
    E_TOKZ_INVALID_ARGUMENT,
    E_TOKZ_EOS_EXPECTED,
    E_TOKZ_TOO_FEW_ARGS,
    E_TOKZ_TOO_MANY_ARGS,
    E_TOKZ_MAX_NEST,
    E_TOKZ_IDENTIFIER_EXPECTED,

    E_TOKZ_LBRACE_EXPECTED
};


struct _ConfOpt;

typedef struct _Tokenizer_FInfo{
    FILE *file;
    char *name;
    int line;
    int ungetc;
    Token ungettok;
} Tokenizer_FInfo;

typedef struct _Tokenizer_Buffer{
        char *data;
        int len;
        int pos;
} Tokenizer_Buffer;

typedef struct _Tokenizer{
    FILE *file;
    char *name;
    int line;
    int ungetc;
    Token ungettok;

    Tokenizer_Buffer buffer;

    int flags;
    const struct _ConfOpt **optstack;
    int nest_lvl;
    void *user_data;

    int filestack_n;
    Tokenizer_FInfo *filestack;

    char **includepaths;
} Tokenizer;


extern Tokenizer *tokz_open(const char *fname);
extern Tokenizer *tokz_open_file(FILE *file, const char *fname);
extern Tokenizer *tokz_prepare_buffer(char *buffer, int len);
extern void tokz_close(Tokenizer *tokz);
extern bool tokz_get_token(Tokenizer *tokz, Token *tok);
extern void tokz_unget_token(Tokenizer *tokz, Token *tok);
extern void tokz_warn_error(const Tokenizer *tokz, int line, int e);
extern void tokz_warn(const Tokenizer *tokz, int line, const char *fmt, ...);

extern bool tokz_pushf(Tokenizer *tokz, const char *fname);
extern bool tokz_pushf_file(Tokenizer *tokz, FILE *file, const char *fname);
extern bool tokz_popf(Tokenizer *tokz);

extern void tokz_set_includepaths(Tokenizer *tokz, char **paths);

#endif /* LIBTU_TOKENIZER_H */
