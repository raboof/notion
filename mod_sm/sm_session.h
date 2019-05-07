/*
 * ion/mod_sm/sm_session.h
 *
 * Copyright (c) Tuomo Valkonen 2004-2007.
 *
 * Based on the code of the 'sm' module for Ion1 by an unknown contributor.
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef ION_MOD_SM_SESSION_H
#define ION_MOD_SM_SESSION_H

extern bool mod_sm_init_session();
extern void mod_sm_set_ion_id(const char *client_id);
extern char *mod_sm_get_ion_id();
extern void mod_sm_close();
extern void mod_sm_smhook(int what);

#endif /* ION_MOD_SM_BINDING_H */
