/*
 * libtu/np-conv.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#include <math.h>

#ifdef NP_SIMPLE_IMPL

#define FN_NUM_TO_SIGNED(T, UMAX, MAX, MIN)                          \
 static int num_to_##T(T *ret, const NPNum *num, bool allow_uns_big) \
 {                                                                   \
    if(num->type!=NPNUM_INT)                                         \
        return E_TOKZ_NOTINT;                                        \
                                                                     \
    if(!num->negative){                                              \
        *ret=num->ival;                                              \
        if(allow_uns_big?num->ival>UMAX:num->ival>MAX)               \
        return E_TOKZ_RANGE;                                         \
    }else{                                                           \
        *ret=-num->ival;                                             \
        if(num->ival>-(ulong)MIN)                                    \
        return E_TOKZ_RANGE;                                         \
    }                                                                \
    return 0;                                                        \
 }

#define FN_NUM_TO_UNSIGNED(T, UMAX, MIN)                         \
 static int num_to_##T(T *ret, const NPNum *num, bool allow_neg) \
 {                                                               \
    if(num->type!=NPNUM_INT)                                     \
        return E_TOKZ_NOTINT;                                    \
                                                                 \
    if(!num->negative){                                          \
        *ret=num->ival;                                          \
        if(num->ival>UMAX)                                       \
        return E_TOKZ_RANGE;                                     \
    }else{                                                       \
        *ret=-num->ival;                                         \
        if(!allow_neg || num->ival>(ulong)-MIN)                  \
        return E_TOKZ_RANGE;                                     \
    }                                                            \
    return 0;                                                    \
 }

#define FN_NUM_TO_FLOAT(T, POW)                  \
 static int num_to_##T(T *ret, const NPNum *num) \
 {                                               \
    *ret=(num->negative?-num->fval:num->fval);   \
    return 0;                                    \
 }

#else /* NP_SIMPLE_IMPL */

#define FN_NUM_TO_SIGNED(T, UMAX, MAX, MIN)                          \
 static int num_to_##T(T *ret, const NPNum *num, bool allow_uns_big) \
 {                                                                   \
    if(num->exponent)                                                \
        return E_TOKZ_NOTINT;                                        \
    if(num->nmantissa>0)                                             \
        return E_TOKZ_RANGE;                                         \
                                                                     \
    if(!num->negative){                                              \
        *ret=num->mantissa[0];                                       \
        if(allow_uns_big?num->mantissa[0]>UMAX:num->mantissa[0]>MAX) \
            return E_TOKZ_RANGE;                                     \
    }else{                                                           \
        *ret=-num->mantissa[0];                                      \
        if(num->mantissa[0]>-(ulong)MIN)                             \
            return E_TOKZ_RANGE;                                     \
    }                                                                \
    return 0;                                                        \
}

#define FN_NUM_TO_UNSIGNED(T, UMAX, MIN)                         \
 static int num_to_##T(T *ret, const NPNum *num, bool allow_neg) \
 {                                                               \
    if(num->exponent)                                            \
        return E_TOKZ_NOTINT;                                    \
    if(num->nmantissa>0)                                         \
        return E_TOKZ_RANGE;                                     \
                                                                 \
    if(!num->negative){                                          \
        *ret=num->mantissa[0];                                   \
        if(num->mantissa[0]>UMAX)                                \
            return E_TOKZ_RANGE;                                 \
    }else{                                                       \
        *ret=-num->mantissa[0];                                  \
        if(!allow_neg || num->mantissa[0]>(ulong)-MIN)           \
            return E_TOKZ_RANGE;                                 \
    }                                                            \
    return 0;                                                    \
}


#define FN_NUM_TO_FLOAT(T, POW)                  \
 static int num_to_##T(T *ret, const NPNum *num) \
 {                                               \
    T d=0;                                       \
    int i;                                       \
                                                 \
    for(i=num->nmantissa;i>=0;i--)               \
        d=d*(T)(ULONG_MAX+1.0)+num->mantissa[i]; \
                                                 \
    d*=POW(num->base, num->exponent);            \
    *ret=d;                                      \
                                                 \
    return 0;                                    \
 }

#endif /* NP_SIMPLE_IMPL */

FN_NUM_TO_SIGNED(long, ULONG_MAX, LONG_MAX, LONG_MIN)
FN_NUM_TO_SIGNED(char, UCHAR_MAX, CHAR_MAX, CHAR_MIN)
FN_NUM_TO_FLOAT(double, pow)

#undef NEG
