/*
 * ion/ioncore/exec.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "exec.h"
#include "property.h"
#include "signal.h"
#include "global.h"
#include "ioncore.h"


#define SHELL_PATH "/bin/sh"
#define SHELL_NAME "sh"
#define SHELL_ARG "-c"


/*{{{ Exec */


void do_exec(const char *cmd)
{
	char *argv[4];
	
	wglobal.dpy=NULL;
	
	if(cmd==NULL)
		return;
	
#ifndef CF_NO_SETPGID
	setpgid(0, 0);
#endif
	
	argv[0]=SHELL_NAME;
	argv[1]=SHELL_ARG;
	argv[2]=(char*)cmd; /* stupid execve... */
	argv[3]=NULL;
	execvp(SHELL_PATH, argv);

	die_err_obj(cmd);
}


EXTL_EXPORT
void exec_on_screen(WScreen *scr, const char *cmd)
{
	int pid;
	char *tmp;

	pid=fork();
	
	if(pid<0)
		warn_err();
	
	if(pid!=0)
		return;
	
	setup_environ(scr->xscr);
	
	close(wglobal.conn);
	
	tmp=ALLOC_N(char, strlen(cmd)+8);
	
	if(tmp==NULL)
		die_err();
	
	sprintf(tmp, "exec %s", cmd);
	
	do_exec(tmp);
}


void do_open_with(WScreen *scr, const char *cmd, const char *file)
{
	int pid;
	char *tmp;

	pid=fork();
	
	if(pid<0)
		warn_err();
	
	if(pid!=0)
		return;
	
	setup_environ(scr->xscr);
	
	close(wglobal.conn);
	
	tmp=ALLOC_N(char, strlen(cmd)+strlen(file)+10);
	
	if(tmp==NULL)
		die_err();
	
	sprintf(tmp, "exec %s '%s'", cmd, file);
	do_exec(tmp);
}


void setup_environ(int scr)
{
	char *tmp, *ptr;
	char *display;
	
	display=XDisplayName(wglobal.display);
	
	tmp=ALLOC_N(char, strlen(display)+16);
	
	if(tmp==NULL){
		warn_err();
		return;
	}
	
	sprintf(tmp, "DISPLAY=%s", display); 

	ptr=strchr(tmp, ':');
	if(ptr!=NULL){
		ptr=strchr(ptr, '.');
		if(ptr!=NULL)
			*ptr='\0';
	}

	if(scr>=0)
		sprintf(tmp+strlen(tmp), ".%i", scr);

	putenv(tmp);
	
	/*XFree(display);*/
}


/*}}}*/


/*{{{ Exit and restart */


static void exitret(int retval)
{	
	ioncore_deinit();
	exit(retval);
}


EXTL_EXPORT
void exit_wm()
{
	exitret(EXIT_SUCCESS);
}


EXTL_EXPORT
void restart_other_wm(const char *cmd)
{
	ioncore_deinit();
	if(cmd!=NULL){
		do_exec(cmd);
	}
	execvp(wglobal.argv[0], wglobal.argv);
	die_err_obj(wglobal.argv[0]);
}


EXTL_EXPORT
void restart_wm()
{
	restart_other_wm(NULL);
}


/*}}}*/

