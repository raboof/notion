/*
 * libtu/map.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002.
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_MAP_H
#define LIBTU_MAP_H

typedef struct _StringIntMap{
    const char *string;
    int value;
} StringIntMap;

#define END_STRINGINTMAP {NULL, 0}

/* Return the index of str in map or -1 if not found. */
extern int stringintmap_ndx(const StringIntMap *map, const char *str);
extern int stringintmap_value(const StringIntMap *map, const char *str,
                              int dflt);
extern const char *stringintmap_key(const StringIntMap *map,
                                    int value, const char *dflt);



typedef void (*FunPtr)(void);

#define DECLFUNPTRMAP(NAME) \
typedef struct _String##NAME##Map{\
    const char *string;\
    NAME value;\
} String##NAME##Map

DECLFUNPTRMAP(FunPtr);

#define END_STRINGPTRMAP {NULL, NULL}

/* Return the index of str in map or -1 if not found. */
extern int stringfunptrmap_ndx(const StringFunPtrMap *map, const char *str);
extern FunPtr stringfunptrmap_value(const StringFunPtrMap *map, const char *str,
                              FunPtr dflt);
extern const char *stringfunptrmap_key(const StringFunPtrMap *map,
                                    FunPtr value, const char *dflt);


#define STRINGFUNPTRMAP_NDX(MAP,STR) stringfunptrmap_ndx((StringFunPtrMap *)MAP, STR)
#define STRINGFUNPTRMAP_VALUE(TYPE,MAP,STR,DFLT) (TYPE)stringfunptrmap_value((StringFunPtrMap *)MAP, STR, (FunPtr)DFLT)
#define STRINGFUNPTRMAP_KEY(MAP,VALUE,DFLT) stringfunptrmap_key((StringFunPtrMap *)MAP, (FunPtr)VALUE, DFLT)


#endif /* LIBTU_MAP_H */
