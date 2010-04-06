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

#endif /* LIBTU_MAP_H */
