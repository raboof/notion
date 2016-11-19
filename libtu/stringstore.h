/*
 * libtu/stringstore.h
 *
 * Copyright (c) Tuomo Valkonen 2004-2007. 
 *
 * You may distribute and modify this library under the terms of either
 * the Clarified Artistic License or the GNU LGPL, version 2.1 or later.
 */

#ifndef LIBTU_STRINGSTORE_H
#define LIBTU_STRINGSTORE_H

typedef void* StringId;

#define STRINGID_NONE ((StringId)0)

extern const char *stringstore_get(StringId id);
extern StringId stringstore_find(const char *str);
extern StringId stringstore_alloc(const char *str);
extern StringId stringstore_find_n(const char *str, uint l);
extern StringId stringstore_alloc_n(const char *str, uint l);
extern void stringstore_free(StringId id);
extern void stringstore_ref(StringId id);
extern void stringstore_deinit(void);

#endif /* LIBTU_STRINGSTORE_H */
