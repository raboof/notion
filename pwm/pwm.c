/*
 * ion/pwm/pwm.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2007. 
 *
 * See the included file LICENSE for details.
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
#include <libtu/prefix.h>
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
static OptParserOpt pwm_opts[]={
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


static void help()
{
    int i;
    printf(TR("Usage: %s [options]\n\n"), libtu_progname());
    for(i=0; pwm_opts[i].descr!=NULL; i++)
        pwm_opts[i].descr=TR(pwm_opts[i].descr);
    optparser_printhelp(OPTP_MIDLONG, pwm_opts);
    printf("\n");
}


int main(int argc, char*argv[])
{
    const char *cfgfile="cfg_pwm";
    const char *display=NULL;
    char *cmd=NULL;
    int stflags=0;
    int opt;
    ErrorLog el;
    FILE *ef=NULL;
    char *efnam=NULL;
    bool may_continue=FALSE;
    bool noerrorlog=FALSE;
    char *localedir;
    
    libtu_init(argv[0]);

#ifdef CF_RELOCATABLE    
    prefix_set(argv[0], PWM3_LOCATION);
#endif

    localedir=prefix_add(LOCALEDIR);
    
    if(!ioncore_init("pwm3", argc, argv, localedir))
        return EXIT_FAILURE;

    if(localedir!=NULL)
        free(localedir);
    
    prefix_wrap_simple(extl_add_searchdir, EXTRABINDIR); /* ion-completefile */
    prefix_wrap_simple(extl_add_searchdir, MODULEDIR);
    prefix_wrap_simple(extl_add_searchdir, ETCDIR);
#ifdef PWM_ETCDIR    
    prefix_wrap_simple(extl_add_searchdir, PWM_ETCDIR);
#endif
    prefix_wrap_simple(extl_add_searchdir, SHAREDIR);
    prefix_wrap_simple(extl_add_searchdir, LCDIR);
    extl_set_userdirs("pwm3");

    optparser_init(argc, argv, OPTP_MIDLONG, pwm_opts);
    
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
        libtu_asprintf(&efnam, "%s/pwm-%d-startup-errorlog", P_tmpdir,
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
                fprintf(ef, TR("PWM startup error log:\n"));
                errorlog_begin_file(&el, ef);
            }
        }
    }

    if(ioncore_startup(display, cfgfile, stflags))
        may_continue=TRUE;

fail:
    if(!may_continue)
        warn(TR("Refusing to start due to encountered errors."));
    
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
