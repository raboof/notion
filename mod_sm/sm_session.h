
#ifndef ION_MOD_SM_SESSION_H
#define ION_MOD_SM_SESSION_H

extern bool mod_sm_init_session();
extern void mod_sm_set_ion_id(const char *client_id);
extern char *mod_sm_get_ion_id();
extern void mod_sm_close();

#endif /* ION_MOD_SM_BINDING_H */
