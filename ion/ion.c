/*
 * ion/ion/ion.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libtu/util.h>
#include <libtu/optparser.h>
#include <libtu/errorlog.h>
#include <libextl/readconfig.h>
#include <libmainloop/exec.h>

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/ioncore.h>
#include <ioncore/exec.h>
#include <ioncore/event.h>
#include "../version.h"


/* Options. Getopt is not used because getopt_long is quite gnu-specific
 * and they don't know of '-display foo' -style args anyway.
 * Instead, I've reinvented the wheel in libtu :(.
 */
static OptParserOpt ion_opts[]={
    {OPT_ID('d'), "display",  OPT_ARG, "host:dpy.scr", 
     DUMMY_TR("X display to use")},
    
    {'c', "conffile", OPT_ARG, "config_file", 
     DUMMY_TR("Configuration file")},
    
    {'s', "searchdir", OPT_ARG, "dir", 
     DUMMY_TR("Add directory to search path")},

    {OPT_ID('o'), "oneroot",  0, NULL,
     DUMMY_TR("Manage default screen only")},

    {OPT_ID('s'), "session",  OPT_ARG, "session_name", 
     DUMMY_TR("Name of session (affects savefiles)")},
    
    {OPT_ID('S'), "smclientid", OPT_ARG, "client_id", 
     DUMMY_TR("Session manager client ID")},

    {OPT_ID('N'), "noerrorlog", 0, NULL, 
     DUMMY_TR("Do not create startup error log and display it "
              "with xmessage.")},
    
    {'h', "help", 0, NULL, 
     DUMMY_TR("Show this help")},
    
    {'V', "version", 0, NULL,
     DUMMY_TR("Show program version")},
    
    {OPT_ID('a'), "about", 0, NULL,
     DUMMY_TR("Show about text")},
    
    END_OPTPARSEROPTS
};


void check_new_user_help()
{
    const char *userdir=extl_userdir();
    char *oldbeard=NULL;
    char *tmp=NULL, *cmd=NULL;
    pid_t pid;
    bool ret;

    if(userdir==NULL){
        warn(TR("Could not get user configuration file directory."));
        return;
    }
    
    libtu_asprintf(&oldbeard, "%s/.welcome_msg_displayed", userdir);
    
    if(oldbeard==NULL)
        return;
    
    if(access(oldbeard, F_OK)==0){
        free(oldbeard);
        return;
    }

    libtu_asprintf(&tmp, TR("%s/welcome.txt"), SHAREDIR);
    
    if(tmp!=NULL){
        if(access(tmp, F_OK)==0)
            libtu_asprintf(&cmd, "%s %s", CF_XMESSAGE, tmp);
        else
            libtu_asprintf(&cmd, "%s %s/welcome.txt", CF_XMESSAGE, SHAREDIR);
    
        free(tmp);
        
        if(cmd!=NULL){
            ret=ioncore_exec(cmd);
    
            free(cmd);
    
            if(ret){
                /* This should actually be done when less or xmessage returns,
                 * but that would mean yet another script...
                 */
                mkdir(userdir, 0700);
                if(open(oldbeard, O_CREAT|O_RDWR, 0600)<0)
                    warn_err_obj(oldbeard);
            }
        }
    }

    free(oldbeard);
}


static void help()
{
    int i;
    printf(TR("Usage: %s [options]\n\n"), prog_execname());
    for(i=0; ion_opts[i].descr!=NULL; i++)
        ion_opts[i].descr=TR(ion_opts[i].descr);
    optparser_printhelp(OPTP_MIDLONG, ion_opts);
    printf("\n");
}


int main(int argc, char*argv[])
{
    const char *cfgfile="cfg_ion";
    const char *display=NULL;
    char *cmd=NULL;
    int stflags=0;
    int opt;
    ErrorLog el;
    FILE *ef=NULL;
    char *efnam=NULL;
    bool may_continue=FALSE;
    bool noerrorlog=FALSE;

    libtu_init(argv[0]);

    if(!ioncore_init("ion3", argc, argv, LOCALEDIR))
        return EXIT_FAILURE;

    extl_add_searchdir(EXTRABINDIR); /* ion-completefile */
    extl_add_searchdir(MODULEDIR);
    extl_add_searchdir(ETCDIR);
    extl_add_searchdir(SHAREDIR);
    extl_add_searchdir(LCDIR);
    extl_set_userdirs("ion3");

    optparser_init(argc, argv, OPTP_MIDLONG, ion_opts);
    
    while((opt=optparser_get_opt())){
        switch(opt){
        case OPT_ID('d'):
            display=optparser_get_arg();
            break;
        case 'c':
            cfgfile=optparser_get_arg();
            break;
        case 's':
            extl_add_searchdir(optparser_get_arg());
            break;
        case OPT_ID('S'):
            ioncore_g.sm_client_id=optparser_get_arg();
            break;
        case OPT_ID('o'):
            stflags|=IONCORE_STARTUP_ONEROOT;
            break;
        case OPT_ID('s'):
            extl_set_sessiondir(optparser_get_arg());
            break;
        case OPT_ID('N'):
            noerrorlog=TRUE;
            break;
        case 'h':
            help();
            return EXIT_SUCCESS;
        case 'V':
            printf("%s\n", ION_VERSION);
            return EXIT_SUCCESS;
        case OPT_ID('a'):
            printf("%s\n", ioncore_aboutmsg());
            return EXIT_SUCCESS;
        default:
            warn(TR("Invalid command line."));
            help();
            return EXIT_FAILURE;
        }
    }

    if(!noerrorlog){
        /* We may have to pass the file to xmessage so just using tmpfile()
         * isn't sufficient.
         */
        libtu_asprintf(&efnam, "%s/ion-%d-startup-errorlog", P_tmpdir,
                       getpid());
        if(efnam==NULL){
            warn_err();
        }else{
            ef=fopen(efnam, "wt");
            if(ef==NULL){
                warn_err_obj(efnam);
                free(efnam);
                efnam=NULL;
            }else{
                cloexec_braindamage_fix(fileno(ef));
                fprintf(ef, TR("Ion startup error log:\n"));
                errorlog_begin_file(&el, ef);
            }
        }
    }

    if(ioncore_startup(display, cfgfile, stflags))
        may_continue=TRUE;

fail:
    if(!may_continue)
        warn(TR("Refusing to start due to encountered errors."));
    else
        check_new_user_help();
    
    if(ef!=NULL){
        pid_t pid=-1;
        if(errorlog_end(&el) && ioncore_g.dpy!=NULL){
            fclose(ef);
            pid=fork();
            if(pid==0){
                ioncore_setup_display(DefaultScreen(ioncore_g.dpy));
                if(!may_continue)
                    XCloseDisplay(ioncore_g.dpy);
                else
                    close(ioncore_g.conn);
                libtu_asprintf(&cmd, CF_XMESSAGE " %s", efnam);
                if(cmd==NULL){
                    warn_err();
                }else if(system(cmd)==-1){
                    warn_err_obj(cmd);
                }
                unlink(efnam);
                exit(EXIT_SUCCESS);
            }
            if(!may_continue && pid>0)
                waitpid(pid, NULL, 0);
        }else{
            fclose(ef);
        }
        if(pid<0)
            unlink(efnam);
        free(efnam);
    }

    if(!may_continue)
        return EXIT_FAILURE;
    
    ioncore_mainloop();
    
    /* The code should never return here */
    return EXIT_SUCCESS;
}
