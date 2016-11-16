/*
 * ion/mod_sm/sm_session.c
 *
 * Copyright (c) Tuomo Valkonen 2004-2009.
 *
 * Based on the code of the 'sm' module for Ion1 by an unknown contributor.
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <libtu/misc.h>

#include <X11/Xlib.h>
#include <X11/SM/SMlib.h>

#include <libextl/readconfig.h>
#include <libmainloop/select.h>
#include <libmainloop/exec.h>
#include <ioncore/exec.h>
#include <ioncore/global.h>
#include <ioncore/ioncore.h>
#include "sm_session.h"


static IceConn ice_sm_conn=NULL;
static SmcConn sm_conn=NULL;
static int sm_fd=-1;

static char *sm_client_id=NULL;
static char restart_hint=SmRestartImmediately;

static Bool sent_save_done=FALSE;

/* Function to be called when sm tells client save is complete */
static void (*save_complete_fn)();

void mod_sm_set_ion_id(const char *client_id)
{
    if(sm_client_id)
        free(sm_client_id);

    if(client_id==NULL)
        sm_client_id=NULL;
    else
        sm_client_id=scopy(client_id);
}

char *mod_sm_get_ion_id()
{
    return sm_client_id;
}

/* Called when there's data to be read.
 IcePcocessMessages determines message protocol,
 unpacks the message and sends it to the client via
 registered callbacks. */

static void sm_process_messages(int UNUSED(fd), void *UNUSED(data))
{
    Bool ret;

    if(IceProcessMessages(ice_sm_conn, NULL, &ret)==IceProcessMessagesIOError){
        mod_sm_close();
    }
}

/* Callback triggered when an Ice connection is
 opened or closed. */

static void sm_ice_watch_fd(IceConn conn,
                            IcePointer UNUSED(client_data),
                            Bool opening,
                            IcePointer *UNUSED(watch_data))
{
    if(opening){
        if(sm_fd!=-1){ /* shouldn't happen */
            warn(TR("Too many ICE connections."));
        }
        else{
            sm_fd=IceConnectionNumber(conn);
            cloexec_braindamage_fix(sm_fd);
            mainloop_register_input_fd(sm_fd, NULL, &sm_process_messages);
        }
    }
    else{
        if (IceConnectionNumber(conn)==sm_fd){
            mainloop_unregister_input_fd(sm_fd);
            sm_fd=-1;
        }
    }
}

/* Store restart information and stuff in the session manager */

static void sm_set_some_properties()
{
    SmPropValue program_val, userid_val;
    SmProp program_prop, userid_prop, clone_prop;
    SmProp *props[3];

    props[0]=&program_prop;
    props[1]=&userid_prop;
    props[2]=&clone_prop;

    program_val.value=ioncore_g.argv[0];
    program_val.length=strlen(program_val.value);
    program_prop.name=SmProgram;
    program_prop.type=SmARRAY8;
    program_prop.num_vals=1;
    program_prop.vals=&program_val;

    userid_val.value=getenv("USER");
    userid_val.length=strlen(userid_val.value);
    userid_prop.name=SmUserID;
    userid_prop.type=SmARRAY8;
    userid_prop.num_vals=1;
    userid_prop.vals=&userid_val;

    clone_prop.name=SmCloneCommand;
    clone_prop.type=SmLISTofARRAY8;
    clone_prop.num_vals=1;
    clone_prop.vals=&program_val;

    SmcSetProperties(sm_conn,
                     sizeof(props)/sizeof(props[0]),
                     (SmProp **)&props);
}

static void sm_set_other_properties()
{
    char *restore="-session";
    char *clientid="-smclientid";
    char *rmprog="/bin/rm";
    char *rmarg="-rf";
    int nvals=0, i;
    const char *sdir=NULL, *cid=NULL;

    SmPropValue restart_hint_val, *restart_val=NULL;
    SmProp restart_hint_prop={ SmRestartStyleHint, SmCARD8, 1, NULL};
    SmProp restart_prop={ SmRestartCommand, SmLISTofARRAY8, 0, NULL};

    SmProp *props[2];

    restart_hint_prop.vals=&restart_hint_val;

    props[0]=&restart_prop;
    props[1]=&restart_hint_prop;

    sdir=extl_sessiondir();
    cid=mod_sm_get_ion_id();

    if(sdir==NULL || cid==NULL)
        return;

    restart_hint_val.value=&restart_hint;
    restart_hint_val.length=1;

    restart_val=(SmPropValue *)malloc((ioncore_g.argc+4)*sizeof(SmPropValue));
    for(i=0; i<ioncore_g.argc; i++){
        if(strcmp(ioncore_g.argv[i], restore)==0 ||
           strcmp(ioncore_g.argv[i], clientid)==0){
            i++;
        }else{
            restart_val[nvals].value=ioncore_g.argv[i];
            restart_val[nvals++].length=strlen(ioncore_g.argv[i]);
        }
    }
    restart_val[nvals].value=restore;
    restart_val[nvals++].length=strlen(restore);
    restart_val[nvals].value=(char*)sdir;
    restart_val[nvals++].length=strlen(sdir);
    restart_val[nvals].value=clientid;
    restart_val[nvals++].length=strlen(clientid);
    restart_val[nvals].value=(char*)cid;
    restart_val[nvals++].length=strlen(cid);
    restart_prop.num_vals=nvals;
    restart_prop.vals=restart_val;
    /*
    discard_val[0].length=strlen(rmprog);
    discard_val[0].value=rmprog;
    discard_val[1].length=strlen(rmarg);
    discard_val[1].value=rmarg;
    discard_val[2].length=strlen(sdir);
    discard_val[2].value=(char*)sdir;
    */

    SmcSetProperties(sm_conn,
                     sizeof(props)/sizeof(props[0]),
                     (SmProp **)&props);

    free(restart_val);
}

static void sm_set_properties()
{
    static bool init=TRUE;

    if(init){
        sm_set_some_properties();
        init=FALSE;
    }

    sm_set_other_properties();
}


/* Callback for the save yourself phase 2 message.
 This message is sent by the sm when other clients in the session are finished
 saving state. This is requested in the save yourself callback by clients
 like this one that manages other clients. */

static void sm_save_yourself_phase2(SmcConn conn, SmPointer UNUSED(client_data))
{
    Bool success;

    if(!(success=ioncore_do_snapshot(TRUE)))
        warn(TR("Failed to save session state"));
    else
        sm_set_properties();

    SmcSaveYourselfDone(conn, success);
    sent_save_done=TRUE;
}

/* Callback. Called when the client recieves a save yourself
 message from the sm. */

static void sm_save_yourself(SmcConn UNUSED(conn),
                             SmPointer UNUSED(client_data),
                             int UNUSED(save_type),
                             Bool UNUSED(shutdown),
                             int UNUSED(interact_style),
                             Bool UNUSED(fast))
{
    if(!SmcRequestSaveYourselfPhase2(sm_conn, sm_save_yourself_phase2, NULL)){
        warn(TR("Failed to request save-yourself-phase2 from "
                "session manager."));
        SmcSaveYourselfDone(sm_conn, False);
        sent_save_done=TRUE;
    }else{
        sent_save_done=FALSE;
    }
}

/* Response to the shutdown cancelled message */

static void sm_shutdown_cancelled(SmcConn conn, SmPointer UNUSED(client_data))
{
    save_complete_fn=NULL;
    if(!sent_save_done){
        SmcSaveYourselfDone(conn, False);
        sent_save_done=True;
    }
}

/* Callback */

static void sm_save_complete(SmcConn UNUSED(conn), SmPointer UNUSED(client_data))
{
    if(save_complete_fn){
        save_complete_fn();
        save_complete_fn=NULL;
    }
}

/* Callback */

static void sm_die(SmcConn conn, SmPointer UNUSED(client_data))
{
    assert(conn==sm_conn);
    ioncore_do_exit();
}


/* Connects to the sm and registers
 callbacks for different messages */

bool mod_sm_init_session()
{
    char error_str[256];
    char *new_client_id=NULL;
    SmcCallbacks smcall;

    if(getenv("SESSION_MANAGER")==0){
        warn(TR("SESSION_MANAGER environment variable not set."));
        return FALSE;
    }

    if(IceAddConnectionWatch(&sm_ice_watch_fd, NULL) == 0){
        warn(TR("Session Manager: IceAddConnectionWatch failed."));
        return FALSE;
    }

    memset(&smcall, 0, sizeof(smcall));
    smcall.save_yourself.callback=&sm_save_yourself;
    smcall.save_yourself.client_data=NULL;
    smcall.die.callback=&sm_die;
    smcall.die.client_data=NULL;
    smcall.save_complete.callback=&sm_save_complete;
    smcall.save_complete.client_data=NULL;
    smcall.shutdown_cancelled.callback=&sm_shutdown_cancelled;
    smcall.shutdown_cancelled.client_data=NULL;

    if((sm_conn=SmcOpenConnection(NULL, /* network ids */
                                  NULL, /* context */
                                  1, 0, /* protocol major, minor */
                                  SmcSaveYourselfProcMask |
                                  SmcSaveCompleteProcMask |
                                  SmcShutdownCancelledProcMask |
                                  SmcDieProcMask,
                                  &smcall,
                                  sm_client_id, &new_client_id,
                                  sizeof(error_str), error_str)) == NULL)
    {
        warn(TR("Unable to connect to the session manager."));
        return FALSE;
    }

    mod_sm_set_ion_id(new_client_id);
    free(new_client_id);

    ice_sm_conn=SmcGetIceConnection(sm_conn);

    return TRUE;
}


void mod_sm_close()
{
    if(sm_conn!=NULL){
        SmcCloseConnection(sm_conn, 0, NULL);
        sm_conn=NULL;
    }

    ice_sm_conn=NULL;

    if(sm_fd>=0){
        mainloop_unregister_input_fd(sm_fd);
        close(sm_fd);
        sm_fd=-1;
    }

    if(sm_client_id!=NULL){
        free(sm_client_id);
        sm_client_id=NULL;
    }
}


static void sm_exit()
{
    sm_die(sm_conn, NULL);
}


static void sm_restart()
{
    ioncore_do_restart();
}


void mod_sm_smhook(int what)
{
    save_complete_fn=NULL;

    /* pending check? */

    switch(what){
    case IONCORE_SM_RESIGN:
        restart_hint=SmRestartIfRunning;
        sm_set_properties();
        /*SmcRequestSaveYourself(sm_conn, SmSaveBoth, False,
                               SmInteractStyleAny, False, False);
        save_complete_fn=&sm_exit;*/
        ioncore_do_exit();
        break;
    case IONCORE_SM_SHUTDOWN:
        restart_hint=SmRestartIfRunning;
        SmcRequestSaveYourself(sm_conn, SmSaveBoth, True,
                               SmInteractStyleAny, False, True);
        break;
    case IONCORE_SM_RESTART:
        restart_hint=SmRestartImmediately;
        SmcRequestSaveYourself(sm_conn, SmSaveBoth, False,
                               SmInteractStyleAny, False, False);
        save_complete_fn=&sm_exit;
        break;
    case IONCORE_SM_RESTART_OTHER:
        restart_hint=SmRestartIfRunning;
        SmcRequestSaveYourself(sm_conn, SmSaveBoth, False,
                               SmInteractStyleAny, False, False);
        save_complete_fn=&sm_restart;
        break;
    case IONCORE_SM_SNAPSHOT:
        restart_hint=SmRestartImmediately;
        SmcRequestSaveYourself(sm_conn, SmSaveBoth, False,
                               SmInteractStyleAny, False, True);
        break;
    }
}


