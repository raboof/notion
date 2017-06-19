/*
 * libtu/numparser2.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#define MAX_MANTISSA 10  /* should be enough for our needs */
#define ULONG_SIZE (sizeof(ulong)*8)

enum{
    NPNUM_INT,
    NPNUM_FLOAT
};

#ifdef NP_SIMPLE_IMPL

typedef struct _NPNum{
    int type;
    int base;
     bool negative;
    double fval;
    ulong ival;
} NPNum;

#define NUM_INIT {0, 0, 0, 0.0, 0}

static int npnum_mulbase_add(NPNum *num, long base, long v)
{
    double iold=num->ival;

    num->fval=num->fval*base+(double)v;

    num->ival*=base;

    if(num->ival<iold)
        num->type=NPNUM_FLOAT;

    num->ival+=v;

    return 0;
}

#else /* NP_SIMPLE_IMPL */

typedef struct _NPNum{
    unsigned char nmantissa;
    int type;
    int base;
     bool negative;
    ulong mantissa[MAX_MANTISSA];
    long exponent;
} NPNum;

#define NUM_INIT {0, 0, 0, 0, {0,}, 0}

#define ADD_EXP(NUM, X) (NUM)->exponent+=(X);

#if defined(__GNUG__) && defined(i386) && !defined(NP_NO_I386_ASM)
 #define NP_I386_ASM
#endif

static int npnum_mulbase_add(NPNum *num, long base, long v)
{
    long i, j;
    ulong overflow;
#ifndef NP_I386_ASM
    ulong val;
#endif

    for(i=num->nmantissa;i>=0;i--){
#ifdef NP_I386_ASM
        __asm__("mul %4\n"
                : "=a" (num->mantissa[i]), "=d" (overflow)
                : "0" (num->mantissa[i]), "1" (0), "q" (base)
                : "eax", "edx");
#else
        overflow=0;
        val=num->mantissa[i];

        if(val<ULONG_MAX/base){
            val*=base;
        }else if(val){
            ulong tmp=val;
            ulong old=val;
            for(j=0; j<base; j++){
                val+=tmp;
                if(val<=old)
                    overflow++;
                old=val;
            }
        }
        num->mantissa[i]=val;
#endif
        if(overflow){
            if(i==num->nmantissa){
                if(num->nmantissa==MAX_MANTISSA)
                    return E_TOKZ_TOOBIG;
                num->nmantissa++;
            }
            num->mantissa[i+1]+=overflow;
        }
    }
    num->mantissa[0]+=v;

    return 0;
}

#undef NP_I386_ASM

#endif /* NP_SIMPLE_IMPL */


/* */


static bool is_end(int c)
{
    /* oops... EOF was missing */
    return (c==EOF || (c!='.' && ispunct(c)) || isspace(c) || iscntrl(c));
}


/* */


static int parse_exponent(NPNum *num, Tokenizer *tokz, int c)
{
    long exp=0;
    bool neg=FALSE;
    int err=0;

    c=GETCH();

    if(c=='-' || c=='+'){
        if(c=='-')
            neg=TRUE;
        c=GETCH();
    }

    for(; 1; c=GETCH()){
        if(isdigit(c)){
            exp*=10;
            exp+=c-'0';
        }else if(is_end(c)){
            UNGETCH(c);
            break;
        }else{
            err=E_TOKZ_NUMFMT;
        }
    }

    if(neg)
        exp*=-1;

#ifndef NP_SIMPLE_IMPL
    ADD_EXP(num, exp);
#else
    num->fval*=pow(num->base, exp);
#endif
    return err;
}


static int parse_number(NPNum *num, Tokenizer *tokz, int c)
{
    int base=10;
    int dm=1;
    int err=0;
    int tmp;
#ifdef NP_SIMPLE_IMPL
    double divisor=base;
#endif

    if(c=='-' || c=='+'){
        if(c=='-')
            num->negative=TRUE;
        c=GETCH();
        if(!isdigit(c))
            err=E_TOKZ_NUMFMT;
    }

    if(c=='0'){
        dm=0;
        c=GETCH();
        if(c=='x' || c=='X'){
            base=16;
            c=GETCH();
        }else if(c=='b' || c=='B'){
            base=2;
            c=GETCH();
        }else if('0'<=c && c<='7'){
            base=8;
        }else{
            dm=2;
        }
    }

    num->base=base;

    for(; 1; c=GETCH()){
        if((c=='e' || c=='E') && dm!=0){
            if(dm<2){
                err=E_TOKZ_NUMFMT;
                continue;
            }
            tmp=parse_exponent(num, tokz, c);
            if(err==0)
                err=tmp;
            break;
        }

        if(isxdigit(c)){
            if('0'<=c && c<='9')
                c-='0';
            else if(isupper(c))
                c-='A'-10;
            else
                c-='a'-10;

            if(c>=base)
                err=E_TOKZ_NUMFMT;

#ifdef NP_SIMPLE_IMPL
            if(dm==3){
                num->fval+=(double)c/divisor;
                divisor*=base;
            }else
#endif
            {
                tmp=npnum_mulbase_add(num, base, c);
                if(err==0)
                    err=tmp;
            }

            if(dm==1)
                dm=2;
#ifndef NP_SIMPLE_IMPL
            else if(dm==3)
                ADD_EXP(num, -1);
#endif
            continue;
        }

        if(c=='.'){
            if(dm!=2){
                err=E_TOKZ_NUMFMT;
            }
            dm=3;
#ifdef NP_SIMPLE_IMPL
            num->type=NPNUM_FLOAT;
            divisor=base;
#endif
            continue;
        }

        if(is_end(c)){
            UNGETCH(c);
            break;
        }

        err=E_TOKZ_NUMFMT;
    }

#ifndef NP_SIMPLE_IMPL
    num->type=(num->exponent==0 ? NPNUM_INT : NPNUM_FLOAT);
#endif

    return err;
}
