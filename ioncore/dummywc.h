/*
 * ion/ioncore/dummywc.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

/* This file contains dummy implementations of multibyte/widechar routines
 * used by Ion for retarded platforms.
 */

#ifndef ION_IONCORE_DUMMYWC_H
#define ION_IONCORE_DUMMYWC_H

#include <string.h>
#include <ctype.h>

#define wchar_t int
#define mbstate_t int

#define iswalnum isalnum
#define iswprint isprint
#define iswspace isspace

#define mbrlen dummywc_mbrlen
#define mbtowc dummywc_mbtowc
#define mbrtowc dummywc_mbrtowc

static size_t dummywc_mbrlen(const char *s, size_t n, mbstate_t *ps)
{
    if(*s=='\0')
        return 0;
    return 1;
}

static int dummywc_mbtowc(wchar_t *pwc, const char *s, size_t n)
{
    if(n>0 && *s!='\0'){
        *pwc=*s;
        return 1;
    }
    return 0;
}

static size_t dummywc_mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
    return mbtowc(pwc, s, n);
}

#endif /* ION_IONCORE_DUMMYWC_H */

