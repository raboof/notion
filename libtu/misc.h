/*
 * libtu/misc.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2005.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_MISC_H
#define LIBTU_MISC_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "types.h"

#define ALLOC(X) (X*)malloczero(sizeof(X))
#define ALLOC_N(X, N) (X*)malloczero(sizeof(X)*(N))
#define REALLOC_N(PTR, X, S, N) (X*)remalloczero(PTR, sizeof(X)*(S), sizeof(X)*(N))

#define FREE(X) do{if(X!=NULL)free(X);}while(0)

#define XOR(X, Y) (((X)==0) != ((Y)==0))

/* UNUSED macro, for function argument */
#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

extern void* malloczero(size_t size);
extern void* remalloczero(void *ptr, size_t oldsize, size_t newsize);

extern char* scopy(const char *p);
extern char* scopyn(const char *p, size_t n);
extern char* scat(const char *p1, const char *p2);
extern char* scatn(const char *p1, ssize_t n1, const char *p2, ssize_t n2);
extern char* scat3(const char *p1, const char *p2, const char *p3);
extern void stripws(char *p);
extern const char *libtu_strcasestr(const char *str, const char *ptn);

extern const char* simple_basename(const char *name);

/* I dislike fread and fwrite... */
extern bool readf(FILE *fd, void *buf, size_t n);
extern bool writef(FILE *fd, const void *buf, size_t n);

#endif /* LIBTU_MISC_H */
