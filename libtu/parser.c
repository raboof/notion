/*
 * libtu/parser.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <string.h>
#include <errno.h>

#include "parser.h"
#include "misc.h"
#include "output.h"

#define MAX_TOKENS     256
#define MAX_NEST    256


enum{
    P_NONE=1,
    P_EOF,
    P_STMT,
    P_STMT_NS,
    P_STMT_SECT,
    P_BEG_SECT,
    P_END_SECT
};


/* */


static bool opt_include(Tokenizer *tokz, int n, Token *toks);


static ConfOpt common_opts[]={
    {"include", "s", opt_include, NULL},
    {NULL, NULL, NULL, NULL}
};


/* */


static int read_statement(Tokenizer *tokz, Token *tokens, int *ntok_ret)
{
    int ntokens=0;
    Token *tok=NULL;
    int had_comma=0; /* 0 - no, 1 - yes, 2 - not had, not expected */
    int retval=0;
    int e=0;

    while(1){
        tok=&tokens[ntokens];

        if(!tokz_get_token(tokz, tok)){
            e=1;
            continue;
        }

        if(ntokens==MAX_TOKENS-1){
            e=E_TOKZ_TOKEN_LIMIT;
            tokz_warn_error(tokz, tok->line, e);
            if(!(tokz->flags&TOKZ_ERROR_TOLERANT))
                break;
        }else{
            ntokens++;
        }

        if(!TOK_IS_OP(tok)){
            if(ntokens==1 && !had_comma){
                /* first token */
                had_comma=2;
            }else{
                if(had_comma==0)
                    goto syntax;

                had_comma=0;
            }
            continue;
        }

        /* It is an operator */
        ntokens--;

        switch(TOK_OP_VAL(tok)){
        case OP_SCOLON:
            retval=(ntokens==0 ? P_NONE : P_STMT_NS);
            break;

        case OP_NEXTLINE:
            retval=(ntokens==0 ? P_NONE : P_STMT);
            break;

        case OP_L_BRC:
            retval=(ntokens==0 ? P_BEG_SECT : P_STMT_SECT);
            break;

        case OP_R_BRC:
            if(ntokens==0){
                retval=P_END_SECT;
            }else{
                tokz_unget_token(tokz, tok);
                retval=P_STMT_NS;
            }
            break;

        case OP_EOF:
            retval=(ntokens==0 ? P_EOF : P_STMT_NS);

            if(had_comma==1){
                e=E_TOKZ_UNEXPECTED_EOF;
                goto handle_error;
            }

            goto end;

        case OP_COMMA:
            if(had_comma!=0)
                goto syntax;

            had_comma=1;
            continue;

        default:
            goto syntax;
        }

        if(had_comma!=1)
            break;

    syntax:
        e=E_TOKZ_SYNTAX;
    handle_error:
        tokz_warn_error(tokz, tok->line, e);

        if(!(tokz->flags&TOKZ_ERROR_TOLERANT) || retval!=0)
            break;
    }

end:
    if(e!=0)
        retval=-retval;

    *ntok_ret=ntokens;

    return retval;
}


static bool find_beg_sect(Tokenizer *tokz)
{
    Token tok=TOK_INIT;

    while(tokz_get_token(tokz, &tok)){
        if(TOK_IS_OP(&tok)){
            if(TOK_OP_VAL(&tok)==OP_NEXTLINE)
                continue;

            if(TOK_OP_VAL(&tok)==OP_SCOLON)
                return FALSE;

            if(TOK_OP_VAL(&tok)==OP_L_BRC)
                return TRUE;
        }

        tokz_unget_token(tokz, &tok);
        break;
    }
    return FALSE;
}


/* */


static const ConfOpt* lookup_option(const ConfOpt *opts, const char *name)
{
    while(opts->optname!=NULL){
        if(strcmp(opts->optname, name)==0)
            return opts;
        opts++;
    }
    return NULL;
}


static bool call_end_sect(Tokenizer *tokz, const ConfOpt *opts)
{
    opts=lookup_option(opts, "#end");
    if(opts!=NULL)
        return opts->fn(tokz, 0, NULL);

    return TRUE;
}


static bool call_cancel_sect(Tokenizer *tokz, const ConfOpt *opts)
{
    opts=lookup_option(opts, "#cancel");
    if(opts!=NULL)
        return opts->fn(tokz, 0, NULL);

    return TRUE;
}


/* */


bool parse_config_tokz(Tokenizer *tokz, const ConfOpt *options)
{
    Token tokens[MAX_TOKENS];
    bool alloced_optstack=FALSE;
    int i, t, ntokens=0;
    int init_nest_lvl;
    bool had_error;
    int errornest=0;
    bool is_default=FALSE;

    /* Allocate tokz->optstack if it does not yet exist (if it does,
     * we have been called from an option handler)
     */
    if(!tokz->optstack){
        tokz->optstack=ALLOC_N(const ConfOpt*, MAX_NEST);
        if(!tokz->optstack){
            warn_err();
            return FALSE;
        }

        memset(tokz->optstack, 0, sizeof(ConfOpt*)*MAX_NEST);
        init_nest_lvl=tokz->nest_lvl=0;
        alloced_optstack=TRUE;
    }else{
        init_nest_lvl=tokz->nest_lvl;
    }

    tokz->optstack[init_nest_lvl]=options;

    for(i=0; i<MAX_TOKENS; i++)
        tok_init(&tokens[i]);


    while(1){
        had_error=FALSE;

        /* free the tokens */
        while(ntokens--)
            tok_free(&tokens[ntokens]);

        /* read the tokens */
        t=read_statement(tokz, tokens, &ntokens);

        if((had_error=t<0))
            t=-t;

        switch(t){
        case P_STMT:
        case P_STMT_NS:
        case P_STMT_SECT:

            if(errornest)
                had_error=TRUE;
            else if(tokz->flags&TOKZ_PARSER_INDENT_MODE)
                verbose_indent(tokz->nest_lvl);

            if(!TOK_IS_IDENT(tokens+0)){
                had_error=TRUE;
                tokz_warn_error(tokz, tokens->line,
                                E_TOKZ_IDENTIFIER_EXPECTED);
            }

            if(t==P_STMT){
                if(find_beg_sect(tokz))
                    t=P_STMT_SECT;
            }

            if(had_error)
                break;

            /* Got the statement and its type */

            options=lookup_option(tokz->optstack[tokz->nest_lvl],
                                  TOK_IDENT_VAL(tokens+0));
            if(options==NULL)
                options=lookup_option(common_opts, TOK_IDENT_VAL(tokens+0));
            if(options==NULL && (tokz->flags&TOKZ_DEFAULT_OPTION)){
                options=lookup_option(tokz->optstack[tokz->nest_lvl], "#default");
                is_default=(options!=NULL);
            }

            if(options==NULL){
                had_error=TRUE;
                tokz_warn_error(tokz, tokens->line, E_TOKZ_UNKNOWN_OPTION);
            }else if(!is_default) {
                had_error=!check_args(tokz, tokens, ntokens, options->argfmt);
            }

            if(had_error)
                break;

            /* Found the option and arguments are ok */

            if(options->opts!=NULL){
                if(t!=P_STMT_SECT){
                    had_error=TRUE;
                    tokz_warn_error(tokz, tokz->line, E_TOKZ_LBRACE_EXPECTED);
                }else if(tokz->nest_lvl==MAX_NEST-1){
                    tokz_warn_error(tokz, tokz->line, E_TOKZ_MAX_NEST);
                    had_error=TRUE;
                }else{
                    tokz->nest_lvl++;
                    tokz->optstack[tokz->nest_lvl]=options->opts;
                }
            }else if(t==P_STMT_SECT){
                had_error=TRUE;
                tokz_warn_error(tokz, tokz->line, E_TOKZ_SYNTAX);
            }

            if(!had_error && options->fn!=NULL){
                had_error=!options->fn(tokz, ntokens, tokens);
                if(t==P_STMT_SECT && had_error)
                    tokz->nest_lvl--;
            }
            break;

        case P_EOF:
            if(tokz_popf(tokz)){
                break;
            }else if(tokz->nest_lvl>0 || errornest>0){
                tokz_warn_error(tokz, 0, E_TOKZ_UNEXPECTED_EOF);
                had_error=TRUE;
            }
            goto eof;

        case P_BEG_SECT:
            had_error=TRUE;
            errornest++;
            tokz_warn_error(tokz, tokz->line, E_TOKZ_SYNTAX);
            break;

        case P_END_SECT:
            if(tokz->nest_lvl+errornest==0){
                tokz_warn_error(tokz, tokz->line, E_TOKZ_SYNTAX);
                had_error=TRUE;
            }

            if(had_error)
                break;

            if(errornest!=0){
                errornest--;
            }else{
                had_error=!call_end_sect(tokz, tokz->optstack[tokz->nest_lvl]);
                tokz->nest_lvl--;
            }

            if(tokz->nest_lvl<init_nest_lvl)
                goto eof;
        }

        if(!had_error)
            continue;

        if(t==P_STMT_SECT)
            errornest++;

        if(!(tokz->flags&TOKZ_ERROR_TOLERANT))
            break;
    }

eof:
    /* free the tokens */
    while(ntokens--)
        tok_free(&tokens[ntokens]);

    while(tokz->nest_lvl>=init_nest_lvl){
        if(tokz->flags&TOKZ_ERROR_TOLERANT || !had_error)
            call_end_sect(tokz, tokz->optstack[tokz->nest_lvl]);
        else
            call_cancel_sect(tokz, tokz->optstack[tokz->nest_lvl]);
        tokz->nest_lvl--;
    }

    /* Free optstack if it was alloced by this call */
    if(alloced_optstack){
        free(tokz->optstack);
        tokz->optstack=NULL;
        tokz->nest_lvl=0;
    }

    if(tokz->flags&TOKZ_PARSER_INDENT_MODE)
        verbose_indent(init_nest_lvl);

    return !had_error;
}


/* */


bool parse_config(const char *fname, const ConfOpt *options, int flags)
{
    Tokenizer *tokz;
    bool ret;

    tokz=tokz_open(fname);

    if(tokz==NULL)
        return FALSE;

    tokz->flags|=flags&~TOKZ_READ_COMMENTS;

    ret=parse_config_tokz(tokz, options);

    tokz_close(tokz);

    return ret;
}


bool parse_config_file(FILE *file, const ConfOpt *options, int flags)
{
    Tokenizer *tokz;
    bool ret;

    tokz=tokz_open_file(file, NULL);

    if(tokz==NULL)
        return FALSE;

    tokz->flags|=flags&~TOKZ_READ_COMMENTS;

    ret=parse_config_tokz(tokz, options);

    tokz_close(tokz);

    return ret;
}


/*
 * Argument validity checking stuff
 */


static int arg_match(Token *tok, char c, bool si)
{
    char c2=tok->type;

    if(c=='.' || c=='*')
        return 0;

    if(c2==c)
        return 0;

    if(si){
        if(c2=='i' && c=='s'){
            TOK_SET_IDENT(tok, TOK_STRING_VAL(tok));
            return 0;
        }

        if(c2=='s' && c=='i'){
            TOK_SET_STRING(tok, TOK_IDENT_VAL(tok));
            return 0;
        }
    }

    if(c2=='c' && c=='l'){
        TOK_SET_LONG(tok, TOK_CHAR_VAL(tok));
        return 0;
    }

    if(c2=='l' && c=='c'){
        TOK_SET_CHAR(tok, TOK_LONG_VAL(tok));
        return 0;
    }

    if(c2=='l' && c=='d'){
        TOK_SET_DOUBLE(tok, TOK_LONG_VAL(tok));
        return 0;
    }

    if(c=='b'){
        if(c2=='l'){
            TOK_SET_BOOL(tok, TOK_LONG_VAL(tok));
            return 0;
        }else if(c2=='i'){
            if(strcmp(TOK_IDENT_VAL(tok), "TRUE")==0){
                tok_free(tok);
                TOK_SET_BOOL(tok, TRUE);
                return 0;
            }else if(strcmp(TOK_IDENT_VAL(tok), "FALSE")==0){
                tok_free(tok);
                TOK_SET_BOOL(tok, FALSE);
                return 0;
            }
        }
    }

    return E_TOKZ_INVALID_ARGUMENT;
}


static int check_argument(const char **pret, Token *tok, const char *p,
                          bool si)
{
    int mode;
    int e=E_TOKZ_TOO_MANY_ARGS;

again:
    mode=0;

    if(*p=='*'){
        *pret=p;
        return 0;
    }else if(*p=='?'){
        mode=1;
        p++;
    }else if(*p==':'){
        mode=2;
        p++;
    }else if(*p=='+'){
        *pret=p;
        return arg_match(tok, *(p-1), si);
    }

    while(*p!='\0'){
        e=arg_match(tok, *p, si);
        if(e==0){
            p++;
            while(mode==2 && *p==':'){
                if(*++p=='\0')
                    break; /* Invalid argument format string, though... */
                p++;
            }
            *pret=p;
            return 0;
        }

        if(mode==0)
            break;

        p++;

        if(mode==1)
            goto again;

        /* mode==2 */

        if(*p!=':')
            break;
        p++;
        e=E_TOKZ_TOO_MANY_ARGS;
    }

    *pret=p;
    return e;
}


static bool args_at_end(const char *p)
{
    if(p==NULL)
        return TRUE;

    while(*p!='\0'){
        if(*p=='*' || *p=='+')
            p++;
        else if(*p=='?')
            p+=2;
        else
            return FALSE;
    }

    return TRUE;
}


bool do_check_args(const Tokenizer *tokz, Token *tokens, int ntokens,
                   const char *fmt, bool si)
{
    int i;
    int e;

    if(fmt==NULL){
        if(ntokens!=1)
            tokz_warn_error(tokz, tokens[0].line, E_TOKZ_TOO_MANY_ARGS);
        return ntokens==1;
    }

    for(i=1; i<ntokens; i++){
        e=check_argument(&fmt, &tokens[i], fmt, si);
        if(e!=0){
            tokz_warn_error(tokz, tokens[i].line, e);
            return FALSE;
        }
    }

    if(!args_at_end(fmt)){
        tokz_warn_error(tokz, tokens[i].line, E_TOKZ_TOO_FEW_ARGS);
        return FALSE;
    }

    return TRUE;
}


bool check_args(const Tokenizer *tokz, Token *tokens, int ntokens,
                const char *fmt)
{
    return do_check_args(tokz, tokens, ntokens, fmt, FALSE);
}


bool check_args_loose(const Tokenizer *tokz, Token *tokens, int ntokens,
                const char *fmt)
{
    return do_check_args(tokz, tokens, ntokens, fmt, TRUE);
}


/* */


static bool try_include(Tokenizer *tokz, const char *fname)
{
    FILE *f;

    f=fopen(fname, "r");

    if(f==NULL)
        return FALSE;

    if(!tokz_pushf_file(tokz, f, fname)){
        fclose(f);
        return FALSE;
    }

    return TRUE;
}


static bool try_include_dir(Tokenizer *tokz, const char *dir, int dlen,
                        const char *file)
{
    char *tmpname;
    bool retval;

    tmpname=scatn(dir, dlen, file, -1);

    if(tmpname==NULL){
        warn_err();
        return FALSE;
    }

    retval=try_include(tokz, tmpname);

    free(tmpname);

    return retval;
}


static bool opt_include(Tokenizer *tokz, int UNUSED(n), Token *toks)
{
    const char *fname=TOK_STRING_VAL(toks+1);
    const char *lastndx=NULL;
    bool retval, e;
    int i=0;

    if(fname[0]!='/' && tokz->name!=NULL)
        lastndx=strrchr(tokz->name, '/');

    if(lastndx==NULL)
        retval=try_include(tokz, fname);
    else
        retval=try_include_dir(tokz, tokz->name, lastndx-tokz->name+1, fname);

    if(retval==TRUE)
        return TRUE;

    e=errno;

    if(tokz->includepaths!=NULL){
        while(tokz->includepaths[i]!=NULL){
            if(try_include_dir(tokz, tokz->includepaths[i], -1, fname))
                return TRUE;
            i++;
        }
    }

    warn_obj(fname, "%s", strerror(e));

    return FALSE;
}


extern void tokz_set_includepaths(Tokenizer *tokz, char **paths)
{
    tokz->includepaths=paths;
}



ConfOpt libtu_dummy_confopts[]={
    END_CONFOPTS
};



bool parse_config_tokz_skip_section(Tokenizer *tokz)
{
    return parse_config_tokz(tokz, libtu_dummy_confopts);
}
