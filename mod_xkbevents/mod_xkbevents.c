/*
 * Ion xkb events
 *
 * Copyright (c) Sergey Redin 2006.
 * Copyright (c) Etan Reisner 2011.
 *
 * Released under the MIT License.
 */

#include <X11/XKBlib.h>
#include <X11/extensions/XKB.h>

#include "ioncore/event.h"
#include "ioncore/global.h"
#include "ioncore/xwindow.h"

#include "exports.h"

static int xkb_event_code, xkb_error_code;
WHook *xkb_group_event=NULL, *xkb_bell_event=NULL;

INTRSTRUCT(WAnyParams);
DECLSTRUCT(WAnyParams){
    bool send_event; /* True => synthetically generated */
    Time time; /* server time when event generated */
    unsigned int device; /* Xkb device ID, will not be XkbUseCoreKbd */
};

INTRSTRUCT(WGroupParams);
DECLSTRUCT(WGroupParams){
    WAnyParams any;

    int group;
    int base_group;
    int latched_group;
    int locked_group;
};

INTRSTRUCT(WBellParams);
DECLSTRUCT(WBellParams){
    WAnyParams any;

    int percent;
    int pitch;
    int duration;
    unsigned int bell_class;
    unsigned int bell_id;
    char *name;
    WClientWin *window;
    bool event_only;
};

/*{{{ Module information */

#include "version.h"

char mod_xkbevents_ion_api_version[]=ION_API_VERSION;

/*}}} Module information */

static char *get_atom_name(Atom atom)
{
    char *xatomname, *atomname;

    xatomname = XGetAtomName(ioncore_g.dpy, atom);
    atomname = scopy(xatomname);
    XFree(xatomname);

    return atomname;
}

static bool docall(ExtlFn fn, ExtlTab t)
{
    bool ret;

    extl_protect(NULL);
    ret=extl_call(fn, "t", NULL, t);
    extl_unprotect(NULL);

    extl_unref_table(t);

    return ret;
}

#define MRSH_ANY(PRM,TAB)                                      \
    extl_table_sets_b(TAB, "send_event", PRM->any.send_event); \
    extl_table_sets_i(TAB, "time", PRM->any.time);             \
    extl_table_sets_i(TAB, "device", PRM->any.device)

static bool mrsh_group_extl(ExtlFn fn, WGroupParams *param)
{
    ExtlTab t=extl_create_table();

    MRSH_ANY(param,t);
    if(param->group!=-1)
        extl_table_sets_i(t, "group", param->group + 1);
    if(param->base_group!=-1)
        extl_table_sets_i(t, "base", param->base_group + 1);
    if(param->latched_group!=-1)
        extl_table_sets_i(t, "latched", param->latched_group + 1);
    if(param->locked_group!=-1)
        extl_table_sets_i(t, "locked", param->locked_group + 1);

    return docall(fn, t);
}

static bool mrsh_bell_extl(ExtlFn fn, WBellParams *param)
{
    ExtlTab t=extl_create_table();

    MRSH_ANY(param,t);
    extl_table_sets_i(t, "percent", param->percent);
    extl_table_sets_i(t, "pitch", param->pitch);
    extl_table_sets_i(t, "duration", param->duration);

    extl_table_sets_i(t, "bell_class", param->bell_class);
    extl_table_sets_i(t, "bell_id", param->bell_id);

    if(param->name){
        extl_table_sets_s(t, "name", param->name);
        free(param->name);
    }

    if(param->window)
        extl_table_sets_o(t, "window", (Obj*)param->window);

    extl_table_sets_b(t, "event_only", param->event_only);

    return docall(fn, t);
}

#define PARAM_ANY(PRM,NM)                  \
    PRM.any.send_event=kev->NM.send_event; \
    PRM.any.time=kev->NM.time;             \
    PRM.any.device=kev->NM.device

#define CHANGED(NM,FLD) (kev->state.changed&XkbGroup##NM##Mask)?kev->state.FLD:-1

bool handle_xkb_event(XEvent *ev)
{
    void *p=NULL;
    WHook *hook=NULL;
    XkbEvent *kev=NULL;
    WHookMarshallExtl *fn=NULL;

    if(ev->type!=xkb_event_code)
        return FALSE;

    kev=(XkbEvent*)ev;

    switch(kev->any.xkb_type){
    case XkbStateNotify:
        {
            WGroupParams p2;
            p=&p2;

            hook=xkb_group_event;
            fn=(WHookMarshallExtl*)mrsh_group_extl;

            PARAM_ANY(p2,state);

            p2.group=CHANGED(State,group);
            p2.base_group=CHANGED(Base,base_group);
            p2.latched_group=CHANGED(Latch,latched_group);
            p2.locked_group=CHANGED(Lock,locked_group);
        }
        break;
    case XkbBellNotify:
        {
            WBellParams p2;
            p=&p2;

            hook=xkb_bell_event;
            fn=(WHookMarshallExtl*)mrsh_bell_extl;

            PARAM_ANY(p2,bell);

            p2.percent=kev->bell.percent;
            p2.pitch=kev->bell.pitch;
            p2.duration=kev->bell.duration;

            p2.bell_class=kev->bell.bell_class;
            p2.bell_id=kev->bell.bell_id;

            p2.name=NULL;
            if(kev->bell.name!=None)
                p2.name=get_atom_name(kev->bell.name);

            p2.window=NULL;
            if(kev->bell.window!=None)
                p2.window=XWINDOW_REGION_OF_T(kev->bell.window, WClientWin);

            p2.event_only=kev->bell.event_only;
        }
        break;
    }

    if(hook && p && fn)
        hook_call_p(hook, p, fn);

    return FALSE;
}

#undef CHANGED

#undef PARAM_ANY


/*EXTL_DOC
 * Set the current XKB group. See \code{XkbLockGroup}(3) manual page
 * for details. See xkbion.lua for example use.
 */
EXTL_EXPORT
int mod_xkbevents_lock_group(int state)
{
    return XkbLockGroup(ioncore_g.dpy, XkbUseCoreKbd, state);
}

/*EXTL_DOC
 * Latch modifiers. See \code{XkbLatchModifiers}(3) manual page
 * for details.
 */
EXTL_EXPORT
int mod_xkbevents_lock_modifiers(int affect, int values)
{
    return XkbLockModifiers(ioncore_g.dpy, XkbUseCoreKbd, affect, values);
}

/*{{{ Init & deinit */

/* ion never does this though it looks to me like that leaks (though I suppose
 * that doesn't matter if modules can't ever be unloaded at runtime.
void deinit_hooks()
{
}
*/

#define INIT_HOOK_(NM)                             \
    NM=mainloop_register_hook(#NM, create_hook()); \
    if(NM==NULL) return FALSE;

static bool init_hooks()
{
    INIT_HOOK_(xkb_group_event);
    INIT_HOOK_(xkb_bell_event);

    return TRUE;
}

#undef INIT_HOOK_

void mod_xkbevents_deinit()
{
    mod_xkbevents_unregister_exports();
}

bool mod_xkbevents_init()
{
    int opcode;
    int major=XkbMajorVersion;
    int minor=XkbMinorVersion;

    if(!XkbLibraryVersion(&major,&minor)){
        warn(TR("X library built with XKB version %d.%02d but mod_xkbevents was built with XKB version %d.%02d. Going to try to work anyway."), major, minor, XkbMajorVersion, XkbMinorVersion);
    }

    if(!XkbQueryExtension(ioncore_g.dpy,&opcode,&xkb_event_code,&xkb_error_code,&major,&minor)>0){
        if ((major!=0)||(minor!=0))
            warn(TR("Server supports incompatible XKB version %d.%02d. Going to try to work anyway."), major, minor);
        else
            warn(TR("XkbQueryExtension failed. Going to try to work anyway."));
    }

    if(!init_hooks())
        return FALSE;

    if(!mod_xkbevents_register_exports())
        return FALSE;

    if(!hook_add(ioncore_handle_event_alt, (void (*)())handle_xkb_event))
        return FALSE;

    /* Select for the specific XkbState events we care about. */
    XkbSelectEventDetails(ioncore_g.dpy, XkbUseCoreKbd, XkbStateNotify,
                          XkbGroupStateMask|XkbGroupBaseMask|XkbGroupLockMask,
                          XkbGroupStateMask|XkbGroupBaseMask|XkbGroupLockMask);

    /* Select for all XkbBell events (we can't select for less). */
    XkbSelectEvents(ioncore_g.dpy, XkbUseCoreKbd, XkbBellNotifyMask, XkbBellNotifyMask);

    return TRUE;
}

/*}}} Init & deinit */
