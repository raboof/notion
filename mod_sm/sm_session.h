
#ifndef ION_MOD_SM_SESSION_H
#define ION_MOD_SM_SESSION_H

extern bool sm_init_session();
extern void sm_set_ion_id(char *client_id);
extern char *sm_get_ion_id();
extern void sm_resign(char hint);
#ifndef XSM
extern void sm_request_shutdown();
#endif
extern void sm_close();

#endif /* ION_MOD_SM_BINDING_H */
