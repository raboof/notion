/*
 * ion/ioncore/exec.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#include <sys/types.h>
#include <sys/signal.h>
#include <unistd.h>
#include <fcntl.h>
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
#include "readfds.h"


#define SHELL_PATH "/bin/sh"
#define SHELL_NAME "sh"
#define SHELL_ARG "-c"


/*{{{ Exec */


void do_exec(const char *cmd)
{
	char *argv[4];
	
	wglobal.dpy=NULL;
	
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


/*EXTL_DOC
 * Run \var{cmd} with the environment variable DISPLAY set to
 * point to \var{scr}.
 */
EXTL_EXPORT
bool exec_on_screen(WScreen *scr, const char *cmd)
{
	int pid;
	char *tmp;
	
	if(cmd==NULL)
		return FALSE;

	pid=fork();
	
	if(pid<0){
		warn_err();
		return FALSE;
	}
	
	if(pid!=0)
		return TRUE;
	
	setup_environ(scr->xscr);
	
	close(wglobal.conn);
	
	tmp=ALLOC_N(char, strlen(cmd)+8);
	
	if(tmp==NULL)
		die_err();
	
	sprintf(tmp, "exec %s", cmd);
	
	do_exec(tmp);
	
	/* We should not get here */
	return FALSE;
}


#define BL 1024

static void process_pipe(int fd, void *p)
{
	char buf[BL];
	int n;
	
	while(1){
		n=read(fd, buf, BL-1);
		if(n<0){
			warn_err_obj("pipe read");
			return;
		}
		if(n>0){
			buf[n]='\0';
			if(!extl_call(*(ExtlFn*)p, "s", NULL, &buf))
				break;
		}
		if(n==0){
			/* Call with no argument/NULL string to signify EOF */
			extl_call(*(ExtlFn*)p, NULL, NULL);
			break;
		}
		if(n<BL-1)
			return;
	}
	
	/* We get here on EOL or if the handler failed */
	unregister_input_fd(fd);
	close(fd);
	extl_unref_fn(*(ExtlFn*)p);
	free(p);
}

#undef BL	
	
/*EXTL_DOC
 * Run \var{cmd} with a read pipe connected to its stdout.
 * When data is received through the pipe, \var{handler} is called
 * with that data.
 */
EXTL_EXPORT
bool popen_bgread(const char *cmd, ExtlFn handler)
{
	int pid;
	char *tmp;
	int fds[2];
	
	if(pipe(fds)!=0){
		warn_err_obj("pipe()");
		return FALSE;
	}

	pid=fork();
	
	if(pid<0){
		close(fds[0]);
		close(fds[1]);
		warn_err();
	}
	
	if(pid!=0){
		ExtlFn *p;
		close(fds[1]);
		fcntl(fds[0], O_NONBLOCK);
		p=ALLOC(ExtlFn);
		if(p!=NULL){
			*(ExtlFn*)p=extl_ref_fn(handler);
			if(register_input_fd(fds[0], p, process_pipe))
				return TRUE;
			extl_unref_fn(*(ExtlFn*)p);
			free(p);
		}
		close(fds[0]);
		return FALSE;
	}
	
	/* Redirect stdout */
	close(fds[0]);
	close(1);
	dup(fds[1]);
	
	/*setup_environ(scr->xscr);*/
	
	close(wglobal.conn);
	
	tmp=ALLOC_N(char, strlen(cmd)+8);
	
	if(tmp==NULL)
		die_err();
	
	sprintf(tmp, "exec %s", cmd);
	
	do_exec(tmp);
	
	/* We should not get here */
	return FALSE;
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


/*EXTL_DOC
 * Causes the window manager to exit.
 */
EXTL_EXPORT
void exit_wm()
{
	exitret(EXIT_SUCCESS);
}


/*EXTL_DOC
 * Attempt to restart another window manager \var{cmd}.
 */
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


/*EXTL_DOC
 * Restart Ioncore.
 */
EXTL_EXPORT
void restart_wm()
{
	restart_other_wm(NULL);
}


/*}}}*/

