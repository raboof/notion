/*
 * libtu/tokenizer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "tokenizer.h"
#include "misc.h"
#include "output.h"
#include "private.h"


static const char *errors[]={
    DUMMY_TR("(no error)"),
    DUMMY_TR("Unexpected end of file"),                /* E_TOKZ_UNEXPECTED_EOF */
    DUMMY_TR("Unexpected end of line"),                /* E_TOKZ_UNEXPECTED_EOL */
    DUMMY_TR("End of line expected"),                /* E_TOKZ_EOL_EXPECTED */
    DUMMY_TR("Invalid character"),                    /* E_TOKZ_INVALID_CHAR*/
    DUMMY_TR("Numeric constant too big"),            /* E_TOKZ_TOOBIG */
    DUMMY_TR("Invalid numberic format"),            /* E_TOKZ_NUMFMT */
    DUMMY_TR("Junk after numeric constant"),        /* E_TOKZ_NUM_JUNK */
    DUMMY_TR("Not an integer"),                        /* E_TOKZ_NOTINT */
    DUMMY_TR("Numeric constant out of range"),        /* E_TOKZ_RANGE */
    DUMMY_TR("Multi-character character constant"),    /* E_TOKZ_MULTICHAR */
    DUMMY_TR("Token/statement limit reached"),        /* E_TOKZ_TOKEN_LIMIT */
    DUMMY_TR("Unknown option"),                        /* E_TOKZ_UNKONWN_OPTION */
    DUMMY_TR("Syntax error"),                        /* E_TOKZ_SYNTAX */
    DUMMY_TR("Invalid argument"),                    /* E_TOKZ_INVALID_ARGUMENT */
    DUMMY_TR("End of statement expected"),            /* E_TOKZ_EOS_EXPECTED */
    DUMMY_TR("Too few arguments"),                    /* E_TOKZ_TOO_FEW_ARGS */
    DUMMY_TR("Too many arguments"),                    /* E_TOKZ_TOO_MANY_ARGS */
    DUMMY_TR("Maximum section nestin level exceeded"), /* E_TOK_Z_MAX_NEST */
    DUMMY_TR("Identifier expected"),                /* E_TOKZ_IDENTIFIER_EXPECTED */
    DUMMY_TR("Starting brace ('{') expected"),        /* E_TOKZ_LBRACE_EXPECTED */
};


/* */

#define STRBLEN 32

#define STRING_DECL(X) int err=0; char* X=NULL; char X##_tmp[STRBLEN]; int X##_tmpl=0
#define STRING_DECL_P(X, P) int err=0; char* X=NULL; char X##_tmp[STRBLEN]=P; int X##_tmpl=sizeof(P)-1
#define STRING_APPEND(X, C) {if(!_string_append(&X, X##_tmp, &X##_tmpl, c)) err=-ENOMEM;}
#define STRING_FREE(X) if(X!=NULL) free(X)
#define STRING_FINISH(X) {if(err!=0) return err; if(!_string_finish(&X, X##_tmp, X##_tmpl)) err=-ENOMEM;}


static bool _string_append(char **p, char *tmp, int *tmplen, char c)
{
    char *tmp2;
    
    if(*tmplen==STRBLEN-1){
        tmp[STRBLEN-1]='\0';
        if(*p!=NULL){
            tmp2=scat(*p, tmp);
            free(*p);
            *p=tmp2;
        }else{
            *p=scopy(tmp);
        }
        *tmplen=1;
        tmp[0]=c;
        return *p!=NULL;
    }else{
        tmp[(*tmplen)++]=c;
        return TRUE;
    }
}


static bool _string_finish(char **p, char *tmp, int tmplen)
{
    char *tmp2;
    
    if(tmplen==0){
        if(*p==NULL)
            *p=scopy("");
    }else{
        tmp[tmplen]='\0';
        if(*p!=NULL){
            tmp2=scat(*p, tmp);
            free(*p);
            *p=tmp2;
        }else{
            *p=scopy(tmp);
        }
    }
    return *p!=NULL;
}


/* */


#define INC_LINE() tokz->line++
#define GETCH() _getch(tokz)
#define UNGETCH(C) _ungetch(tokz, C)

static int _getch(Tokenizer *tokz)
{
    int c;
    
    if(tokz->ungetc!=-1){
        c=tokz->ungetc;
        tokz->ungetc=-1;
    }else if (tokz->flags&TOKZ_READ_FROM_BUFFER) {
        assert(tokz->buffer.data!=NULL);
        if (tokz->buffer.pos==tokz->buffer.len)
            c=EOF;
        else
            c=tokz->buffer.data[tokz->buffer.pos++];
    }else{
        c=getc(tokz->file);
    }

    return c;
}


static void _ungetch(Tokenizer *tokz, int c)
{
    tokz->ungetc=c;
}


/* */


static int scan_line_comment(Token *tok, Tokenizer *tokz)
{
    STRING_DECL_P(s, "#");
    int c;

    c=GETCH();
                
    while(c!='\n' && c!=EOF){
        STRING_APPEND(s, c);
        c=GETCH();
    }

    UNGETCH(c);

    STRING_FINISH(s);
    
    TOK_SET_COMMENT(tok, s);
    
    return 0;
}


static int skip_line_comment(Tokenizer *tokz)
{
    int c;
    
    do{
        c=GETCH();
    }while(c!='\n' && c!=EOF);

    UNGETCH(c);
        
    return 0;
}


/* */


static int scan_c_comment(Token *tok, Tokenizer *tokz)
{
    STRING_DECL_P(s, "/*");
    int c;
    int st=0;
    
    while(1){
        c=GETCH();
        
        if(c==EOF){
            STRING_FREE(s);
            return E_TOKZ_UNEXPECTED_EOF;
        }
        
        STRING_APPEND(s, c);
        
        if(c=='\n'){
            INC_LINE();
        }else if(st==0 && c=='*'){
            st=1;
        }else if(st==1){
            if(c=='/')
                break;
            st=0;
        }
    }

    STRING_FINISH(s);

    TOK_SET_COMMENT(tok, s);

    return 0;
}


static int skip_c_comment(Tokenizer *tokz)
{
    int c;
    int st=0;
    
    while(1){
        c=GETCH();
        
        if(c==EOF)
            return E_TOKZ_UNEXPECTED_EOF;
        
        if(c=='\n')
            INC_LINE();
        else if(st==0 && c=='*')
            st=1;
        else if(st==1){
            if(c=='/')
                break;
            st=0;
        }
    }
    
    return 0;
}


/* */


static int scan_char_escape(Tokenizer *tokz)
{
    static char* special_chars="nrtbae";
    static char* specials="\n\r\t\b\a\033";
    int base, max;
    int i ,c;
    
    c=GETCH();
    
    for(i=0;special_chars[i];i++){
        if(special_chars[i]==c)
            return specials[c];
    }
    
    if(c=='x' || c=='X'){
        base=16;max=2;i=0;
    }else if(c=='d' || c=='D'){
        base=10;max=3;i=0;
    }else if(c=='8' || c=='9'){
        base=10;max=2;i=c-'0';
    }else if('0'<=c && c<='7'){
        base=8;max=2;i=c-'0';
    }else if(c=='\n'){
        UNGETCH(c);
        return -2;
    }else{
        return c;
    }
    
        
    while(--max>=0){
        c=GETCH();
        
        if(c==EOF)
            return EOF;
        
        if(c=='\n'){
            UNGETCH(c);
            return -2;
        }
        
        if(base==16){
            if(!isxdigit(c))
                break;
            
            i<<=4;
            
            if(isdigit(c))
                i+=c-'0';
            else if(i>='a')
                i+=0xa+c-'a';
            else
                i+=0xa+c-'a';
            
        }else if(base==10){
            if(!isdigit(c))
                break;
            i*=10;
            i+=c-'0';
        }else{
            if(c<'0' || c>'7')
                break;
            i<<=3;
            i+=c-'0';
        }
    }
    
    if(max>=0)
        UNGETCH(c);

    return i;
}


/* */


static int scan_string(Token *tok, Tokenizer *tokz, bool escapes)
{
    STRING_DECL(s);
    int c;

    while(1){    
        c=GETCH();
        
        if(c=='"')
            break;
        
        if(c=='\n'){
            UNGETCH(c);
            STRING_FREE(s);
            return E_TOKZ_UNEXPECTED_EOL;
        }
        
        if(c=='\\' && escapes){
            c=scan_char_escape(tokz);
            if(c==-2){
                STRING_FREE(s);
                return E_TOKZ_UNEXPECTED_EOL;
            }
        }
        
        if(c==EOF){
            STRING_FREE(s);
            return E_TOKZ_UNEXPECTED_EOF;
        }
        
        STRING_APPEND(s, c);
    }
    
    STRING_FINISH(s);
    
    TOK_SET_STRING(tok, s);

    return 0;
}


/* */


static int scan_char(Token *tok, Tokenizer *tokz)
{
    int c, c2;
    
    c=GETCH();
    
    if(c==EOF)
        return E_TOKZ_UNEXPECTED_EOF;
    
    if(c=='\n')
        return E_TOKZ_UNEXPECTED_EOL;

    if(c=='\\'){
        c=scan_char_escape(tokz);    
        
        if(c==EOF)
            return E_TOKZ_UNEXPECTED_EOF;
        
        if(c==-2)
            return E_TOKZ_UNEXPECTED_EOL;
    }
    
    c2=GETCH();
    
    if(c2!='\'')
        return E_TOKZ_MULTICHAR;
    
    TOK_SET_CHAR(tok, c);
    
    return 0;
}


/* */


#define START_IDENT(X) (isalpha(X) || X=='_' || X=='$')


static int scan_identifier(Token *tok, Tokenizer *tokz, int c)
{
    STRING_DECL(s);
    
    do{
        STRING_APPEND(s, c);
        c=GETCH();
    }while(isalnum(c) || c=='_' || c=='$');
    
    UNGETCH(c);
    
    STRING_FINISH(s);
    
    TOK_SET_IDENT(tok, s);

    return 0;
}

#define NP_SIMPLE_IMPL
#include "np/numparser2.h"
#include "np/np-conv.h"


static int scan_number(Token *tok, Tokenizer *tokz, int c)
{
    NPNum num=NUM_INIT;
    int e;
    
    if((e=parse_number(&num, tokz, c)))
        return e;
    
    if(num.type==NPNUM_INT){
        long l;
        if((e=num_to_long(&l, &num, TRUE)))
            return e;
    
        TOK_SET_LONG(tok, l);
    }else if(num.type==NPNUM_FLOAT){
          double d;
          if((e=num_to_double(&d, &num)))
              return e;
            
        TOK_SET_DOUBLE(tok, d);
    }else{
        return E_TOKZ_NUMFMT;
    }

    return 0;
}


/* */


static uchar op_map[]={
    0x00,        /* ________ 0-7     */
    0x00,        /* ________ 8-15    */
    0x00,        /* ________ 16-23   */
    0x00,        /* ________ 24-31   */
    0x62,        /* _!___%&_ 32-39   */
    0xff,        /* ()*+,-./ 40-47   */
    0x00,        /* ________ 48-55   */
    0xfc,        /* __:;<=>? 56-63   */
    0x01,        /* @_______ 64-71   */
    0x00,        /* ________ 72-79   */
    0x00,        /* ________ 80-87   */
    0x78,        /* ___[_]^_ 88-95   */
    0x00,        /* ________ 96-103  */
    0x00,        /* ________ 104-111 */
    0x00,        /* ________ 112-119 */
    0x38        /* ___{|}__ 120-127 */
};


static bool map_isset(uchar *map, uint ch)
{
    if(ch>127)
        return FALSE;

    return map[ch>>3]&(1<<(ch&7));
}


static bool is_opch(uint ch)
{
    return map_isset(op_map, ch);
}


static int scan_op(Token *tok, Tokenizer *tokz,  int c)
{
    int c2;
    int op=-1;
    
    /* Quickly check it is an operator character */
    if(!is_opch(c))
        return E_TOKZ_INVALID_CHAR;

    switch(c){
    case '+':
    case '-':
    case '*':
/*    case '/':     Checked elsewhere */
    case '%':
    case '^':
    case '!':
    case '=':
    case '<':
    case '>':
        c2=GETCH();
        if(c2=='='){
            op=c|(c2<<8);
        }else if(c2==c && (c2!='%' && c2!='!' && c2!='*')){
            if(c=='<' || c=='>'){
                int c3=GETCH();
                if(c3=='='){
                    op=c|(c2<<8)|(c3<<16);
                }else{
                    UNGETCH(c3);
                    op=c|(c2<<8);
                }
            }else{
                op=c|(c2<<8);
            }
        }else{
            UNGETCH(c2);
            op=c;
        }
        break;
        
    /* It is already known that it is a operator so these are not needed
    case ':':
    case '~':
    case '?':
    case '.':
    case ';';
    case '{':
    case '}':
    case '@':        
    case '|':
    case '&':
    */
    default:
        op=c;
    }
    
    TOK_SET_OP(tok, op);

    return 0;
}


/* */


void tokz_warn(const Tokenizer *tokz, int line, const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    
    if(tokz!=NULL)
        warn_obj_line_v(tokz->name, line, fmt, args);
    else
        warn(fmt, args);
    
    va_end(args);
}


void tokz_warn_error(const Tokenizer *tokz, int line, int e)
{
    if(e==E_TOKZ_UNEXPECTED_EOF)
        line=0;
    
    if(e<0)
        tokz_warn(tokz, line, "%s", strerror(-e));
    else
        tokz_warn(tokz, line, "%s", TR(errors[e]));
}


bool tokz_get_token(Tokenizer *tokz, Token *tok)
{
    int c, c2, e;
    
    if (!(tokz->flags&TOKZ_READ_FROM_BUFFER))
    assert(tokz->file!=NULL);
    
    tok_free(tok);
    
    if(!TOK_IS_INVALID(&(tokz->ungettok))){
        *tok=tokz->ungettok;
        tokz->ungettok.type=TOK_INVALID;
        return TRUE;
    }

    while(1){
    
        e=0;
        
        do{
            c=GETCH();
        }while(c!='\n' && c!=EOF && isspace(c));
    
        tok->line=tokz->line;
    
        switch(c){
        case EOF:
            TOK_SET_OP(tok, OP_EOF);
            return TRUE;
            
        case '\n':
            INC_LINE();
            
            if(tokz->flags&TOKZ_IGNORE_NEXTLINE)
                continue;
            
            TOK_SET_OP(tok, OP_NEXTLINE);
            
            return TRUE;
            
        case '\\':
            do{
                c=GETCH();
                if(c==EOF){
                    TOK_SET_OP(tok, OP_EOF);
                    return FALSE;
                }
                if(!isspace(c) && e==0){
                    e=E_TOKZ_EOL_EXPECTED;
                    tokz_warn_error(tokz, tokz->line, e);
                    if(!(tokz->flags&TOKZ_ERROR_TOLERANT))
                        return FALSE;
                }
            }while(c!='\n');
            
            INC_LINE();
            continue;

        case '#':
            if(tokz->flags&TOKZ_READ_COMMENTS){
                e=scan_line_comment(tok, tokz);
                break;
            }else if((e=skip_line_comment(tokz))){
                break;
            }
            
            continue;
            
        case '/':
            c2=GETCH();
            
            if(c2=='='){
                TOK_SET_OP(tok, OP_AS_DIV);
                return TRUE;
            }
            
            if(c2!='*'){
                UNGETCH(c2);
                TOK_SET_OP(tok, OP_DIV);
                return TRUE;
            }
            
            if(tokz->flags&TOKZ_READ_COMMENTS){
                e=scan_c_comment(tok, tokz);
                break;
            }else if((e=skip_c_comment(tokz))){
                break;
            }
            
            continue;
            
        case '\"':
            e=scan_string(tok, tokz, TRUE);
            break;

        case '\'':
            e=scan_char(tok, tokz);
            break;

        default: 
            if(('0'<=c && c<='9') || c=='-' || c=='+'){
                e=scan_number(tok, tokz, c);
                break;
            }

             if(START_IDENT(c))
                e=scan_identifier(tok, tokz, c);
            else
                e=scan_op(tok, tokz, c);
        }
        
        if(!e)
            return TRUE;
        
        tokz_warn_error(tokz, tokz->line, e);
        return FALSE;
    }
}


void tokz_unget_token(Tokenizer *tokz, Token *tok)
{
    tok_free(&(tokz->ungettok));    
    tokz->ungettok=*tok;
    tok->type=TOK_INVALID;
}


/*
 * File open
 */

static bool do_tokz_pushf(Tokenizer *tokz)
{
    Tokenizer_FInfo *finfo;
    
    finfo=REALLOC_N(tokz->filestack, Tokenizer_FInfo,
                    tokz->filestack_n, tokz->filestack_n+1);
    
    if(finfo==NULL)
        return FALSE;

    tokz->filestack=finfo;
    finfo=&(finfo[tokz->filestack_n++]);
    
    finfo->file=tokz->file;
    finfo->name=tokz->name;
    finfo->line=tokz->line;
    finfo->ungetc=tokz->ungetc;
    finfo->ungettok=tokz->ungettok;
    
    return TRUE;
}


bool tokz_pushf_file(Tokenizer *tokz, FILE *file, const char *fname)
{
    char *fname_copy=NULL;
    
    if(file==NULL)
        return FALSE;

    if(fname!=NULL){
        fname_copy=scopy(fname);
        if(fname_copy==NULL){
            warn_err();
            return FALSE;
        }
    }
    
    if(tokz->file!=NULL){
        if(!do_tokz_pushf(tokz)){
            warn_err();
            if(fname_copy!=NULL)
                free(fname_copy);
            return FALSE;
        }
    }
    
    tokz->file=file;
    tokz->name=fname_copy;
    tokz->line=1;
    tokz->ungetc=-1;    
    tokz->ungettok.type=TOK_INVALID;
    
    return TRUE;
}


bool tokz_pushf(Tokenizer *tokz, const char *fname)
{
    FILE *file;
    
    file=fopen(fname, "r");
    
    if(file==NULL){
        warn_err_obj(fname);
        return FALSE;
    }
    
    if(!tokz_pushf_file(tokz, file, fname)){
        fclose(file);
        return FALSE;
    }

    return TRUE;
}


               
static Tokenizer *tokz_create()
{
    Tokenizer*tokz;
    
    tokz=ALLOC(Tokenizer);
    
    if(tokz==NULL){
        warn_err();
        return NULL;
    }
    
    tokz->file=NULL;
    tokz->name=NULL;
    tokz->line=1;
    tokz->ungetc=-1;    
    tokz->ungettok.type=TOK_INVALID;
    tokz->flags=0;
    tokz->optstack=NULL;
    tokz->nest_lvl=0;
    tokz->filestack_n=0;
    tokz->filestack=NULL;
    tokz->buffer.data=0;
    tokz->buffer.len=0;
    tokz->buffer.pos=0;
    
    return tokz;
}

    
Tokenizer *tokz_open(const char *fname)
{ 
    Tokenizer *tokz;
    
    tokz=tokz_create();
    
    if(!tokz_pushf(tokz, fname)){
        free(tokz);
        return NULL;
    }
    
    return tokz;
}


Tokenizer *tokz_open_file(FILE *file, const char *fname)
{
    Tokenizer *tokz;
    
    tokz=tokz_create();
    
    if(!tokz_pushf_file(tokz, file, fname)){
        free(tokz);
        return NULL;
    }
    
    return tokz;
}

Tokenizer *tokz_prepare_buffer(char *buffer, int len)
{
    Tokenizer *tokz;
    char old=0;

    tokz=tokz_create();
    if(len>0){
        old=buffer[len-1];
        buffer[len-1]='\0';
    }

    tokz->flags|=TOKZ_READ_FROM_BUFFER;
    tokz->buffer.data=scopy(buffer);
    tokz->buffer.len=(len>0 ? (uint)len : strlen(tokz->buffer.data));
    tokz->buffer.pos=0;

    if(old>0)
        buffer[len-1]=old;

    return tokz;
}

/*
 * File close
 */

static bool do_tokz_popf(Tokenizer *tokz, bool shrink)
{
    Tokenizer_FInfo *finfo;
    
    if(tokz->filestack_n<=0)
        return FALSE;

    if(tokz->file!=NULL)
        fclose(tokz->file);
    if(tokz->name!=NULL)
        free(tokz->name);
    
    finfo=&(tokz->filestack[--tokz->filestack_n]);
    
    tokz->file=finfo->file;
    tokz->name=finfo->name;
    tokz->line=finfo->line;
    tokz->ungetc=finfo->ungetc;
    tokz->ungettok=finfo->ungettok;
    
    if(tokz->filestack_n==0){
        free(tokz->filestack);
        tokz->filestack=NULL;
    }else if(shrink){
        finfo=REALLOC_N(tokz->filestack, Tokenizer_FInfo,
                        tokz->filestack_n+1, tokz->filestack_n);
        if(finfo==NULL)
            warn_err();
        else
            tokz->filestack=finfo;
    }
    
    return TRUE;
}


bool tokz_popf(Tokenizer *tokz)
{
    return do_tokz_popf(tokz, TRUE);
}
    

void tokz_close(Tokenizer *tokz)
{
    while(tokz->filestack_n>0)
        do_tokz_popf(tokz, FALSE);
          
    if(tokz->file!=NULL)
        fclose(tokz->file);
    if(tokz->name!=NULL)
        free(tokz->name);
    tok_free(&(tokz->ungettok));
    
    free(tokz);
}



/* */


void tok_free(Token *tok)
{
    if(TOK_IS_STRING(tok) || TOK_IS_IDENT(tok) || TOK_IS_COMMENT(tok)){
        if(TOK_STRING_VAL(tok)!=NULL)
            free(TOK_STRING_VAL(tok));
    }
    
    tok->type=TOK_INVALID;
}


void tok_init(Token *tok)
{
    static Token dummy=TOK_INIT;
    
    memcpy(tok, &dummy, sizeof(*tok));
}

