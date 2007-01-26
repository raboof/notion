/*
 * ion/de/private.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_DE_PRIVATE_H
#define ION_DE_PRIVATE_H

#define DE_SUB_IND " ->"
#define DE_SUB_IND_LEN 3

#define MATCHES(S, A) (gr_stylespec_score(&(S), A)>0)
#define MATCHES2(S, A1, A2) (gr_stylespec_score2(&(S), A1, A2)>0)

#define ENSURE_INITSPEC(S, NM) \
    if((S).attrs==NULL) gr_stylespec_load(&(S), NM);
    
#endif /* ION_DE_PRIVATE_H */
