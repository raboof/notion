/*
 * ion/pwm/pwm.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
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

#include <ioncore/common.h>
#include <ioncore/global.h>
#include <ioncore/ioncore.h>
#include <ioncore/readconfig.h>
#include <ioncore/exec.h>
#include <ioncore/errorlog.h>
#include <ioncore/eventh.h>
#include "../version.h"


/* Options. Getopt is not used because getopt_long is quite gnu-specific
 * and they don't know of '-display foo' -style args anyway.
 * Instead, I've reinvented the wheel in libtu :(.
 */
static OptParserOpt pwm_opts[]={
	{OPT_ID('d'), 	"display", 	OPT_ARG, "host:dpy.scr", "X display to use"},
	{'c', 			"conffile", OPT_ARG, "config_file", "Configuration file"},
	{OPT_ID('o'), 	"oneroot",  0, NULL, "Manage default root window/non-Xinerama screen only"},
	{OPT_ID('c'), 	"confdir", 	OPT_ARG, "dir", "Search directory for configuration files"},
	{OPT_ID('l'), 	"moduledir", OPT_ARG, "dir", "Search directory for modules"},
#ifndef CF_NOXINERAMA	
	{OPT_ID('x'), 	"xinerama", OPT_ARG, "1|0", "Use Xinerama screen information (default: 0/no)"},
#else
	{OPT_ID('x'), 	"xinerama", OPT_ARG, "?", "Ignored: not compiled with Xinerama support"},
#endif
	{OPT_ID('s'),   "sessionname", OPT_ARG, "session_name", "Name of session (affects savefiles)"},
	{OPT_ID('i'),   "i18n", 0, NULL, "Enable use of multibyte string routines, actual "
									 "encoding depending on the locale."},
	END_OPTPARSEROPTS
};


static const char pwm_usage_tmpl[]=
	"Usage: $p [options]\n\n$o\n";


static OptParserCommonInfo pwm_cinfo={
	ION_VERSION,
	pwm_usage_tmpl,
	NULL,
};


int main(int argc, char*argv[])
{
	const char *cfgfile=NULL;
	const char *display=NULL;
	const char *msg=NULL;
	char *cmd=NULL;
	int stflags=IONCORE_STARTUP_NOXINERAMA;
	int opt;
	ErrorLog el;
	FILE *ef=NULL;
	char *efnam=NULL;
	bool may_continue=FALSE;
	bool i18n=FALSE;
	
	libtu_init(argv[0]);

	if(!ioncore_init(argc, argv))
		return EXIT_FAILURE;
	
	ioncore_add_scriptdir(EXTRABINDIR); /* ion-completefile */
	ioncore_add_scriptdir(ETCDIR);
	ioncore_add_scriptdir(PWM_ETCDIR);
	ioncore_add_scriptdir(SHAREDIR);
	ioncore_add_moduledir(MODULEDIR);
	ioncore_set_userdirs("pwm");

	optparser_init(argc, argv, OPTP_MIDLONG, pwm_opts, &pwm_cinfo);
	
	while((opt=optparser_get_opt())){
		switch(opt){
		case OPT_ID('d'):
			display=optparser_get_arg();
			break;
		case 'c':
			cfgfile=optparser_get_arg();
			break;
		case OPT_ID('c'):
			ioncore_add_scriptdir(optparser_get_arg());
			break;
		case OPT_ID('l'):
			ioncore_add_moduledir(optparser_get_arg());
			break;
		case OPT_ID('o'):
			stflags|=IONCORE_STARTUP_ONEROOT;
			break;
		case OPT_ID('x'):
			{
				const char *p=optparser_get_arg();
				if(strcmp(p, "1")==0)
					stflags&=~IONCORE_STARTUP_NOXINERAMA;
				else if(strcmp(p, "0")==0)
					stflags|=IONCORE_STARTUP_NOXINERAMA;
				else
					warn("Invalid parameter to -xinerama.");
			}
			break;
		case OPT_ID('s'):
			ioncore_set_sessiondir(optparser_get_arg());
			break;
		case OPT_ID('i'):
			i18n=TRUE;
			break;
		default:
			optparser_print_error();
			return EXIT_FAILURE;
		}
	}

	/* We may have to pass the file to xmessage so just using tmpfile()
	 * isn't sufficient.
	 */
	libtu_asprintf(&efnam, "%s/ion-%d-startup-errorlog", P_tmpdir,
				   getpid());
	if(efnam==NULL){
		warn_err("Failed to create error log file");
	}else{
		ef=fopen(efnam, "wt");
		if(ef==NULL){
			warn_err_obj(efnam);
			free(efnam);
			efnam=NULL;
		}
		fprintf(ef, "Ion startup error log:\n");
		begin_errorlog_file(&el, ef);
	}

	/* Set up locale and detect encoding.
	 */
	if(i18n){
		if(!ioncore_init_i18n())
			warn("Please fix your locale settings.");
	}
	
	if(ioncore_startup(display, cfgfile, stflags))
		may_continue=TRUE;

fail:
	if(!may_continue)
		warn("Refusing to start due to encountered errors.");
	
	if(ef!=NULL){
		pid_t pid=-1;
		if(end_errorlog(&el)){
			fclose(ef);
			pid=fork();
			if(pid==0){
				setup_environ(DefaultScreen(wglobal.dpy));
				if(!may_continue)
					XCloseDisplay(wglobal.dpy);
				else
					close(wglobal.conn);
				libtu_asprintf(&cmd, CF_XMESSAGE " %s", efnam);
				if(system(cmd)==-1)
					warn_err_obj(cmd);
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
