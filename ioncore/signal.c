/*
 * ion/ioncore/signal.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2004. 
 *
 * Ion is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "property.h"
#include "signal.h"
#include "exec.h"
#include "global.h"
#include "ioncore.h"


static int kill_sig=0;
#if 1
static int wait_sig=0;
#endif
static bool had_tmr=FALSE;
static WTimer queue=INIT_TIMER(NULL);

#define TIMEVAL_LATER(a, b) \
	((a.tv_sec > b.tv_sec) || \
	((a.tv_sec == b.tv_sec) && \
	 (a.tv_usec > b.tv_usec)))


static void do_set_timer()
{
	struct itimerval val={{0, 0}, {0, 0}};
	
	if(queue.next==NULL){
		setitimer(ITIMER_REAL, &val, NULL);
		return;
	}

	/* Subtract queue time from current time, don't go below zero */
	gettimeofday(&(val.it_value), NULL);
	if(TIMEVAL_LATER((queue.next)->when, val.it_value)){
		if(queue.next->when.tv_usec<val.it_value.tv_usec){
			queue.next->when.tv_usec+=1000000;
			queue.next->when.tv_sec--;
		}
		val.it_value.tv_usec=queue.next->when.tv_usec-val.it_value.tv_usec;
		val.it_value.tv_sec=queue.next->when.tv_sec-val.it_value.tv_sec;
		if(val.it_value.tv_usec<0)
			val.it_value.tv_usec=0;
		if(val.it_value.tv_sec<0)
			val.it_value.tv_sec=0;
	}else{
		had_tmr=TRUE;
		return;
	}

	val.it_interval.tv_usec=val.it_value.tv_usec;
	val.it_interval.tv_sec=val.it_value.tv_sec;
	
	if((setitimer(ITIMER_REAL, &val, NULL))){
		had_tmr=TRUE;
	}
}


void check_signals()
{
	char *tmp=NULL;
	struct timeval current_time;
	WTimer *q;

#if 1	
	if(wait_sig!=0){
		pid_t pid;
		wait_sig=0;
		while((pid=waitpid(-1, NULL, WNOHANG|WUNTRACED))>0){
			/* nothing */
		}
	}
#endif
	
	if(kill_sig!=0){
		if(kill_sig==SIGUSR1){
			restart_other_wm(tmp);
			assert(0);
		} 
		if(kill_sig==SIGTERM)
			exit_wm();

		ioncore_deinit();
		kill(getpid(), kill_sig);
	}

	/* Check for timer events in the queue */
	while(had_tmr && queue.next!=NULL){
		had_tmr=FALSE;
		gettimeofday(&current_time, NULL);
		while(queue.next!=NULL){
			if((TIMEVAL_LATER(current_time, queue.next->when))){
				WObj *obj;
				q=queue.next;
				queue.next=q->next;
				q->next=NULL;
				obj=q->paramwatch.obj;
				reset_watch(&(q->paramwatch));
				q->handler(q, obj);
			}else{
				break;
			}
		}
		do_set_timer();
	}
}


static void add_to_current_time(struct timeval *when, uint msecs)
{
	long tmp_usec;

	gettimeofday(when, NULL);
	tmp_usec=when->tv_usec + (msecs * 1000);
	when->tv_usec=tmp_usec % 1000000;
	when->tv_sec+=tmp_usec / 1000000;
}


void kill_timer(WWatch *watch, WObj *obj)
{
	WTimer *tmr;
	warn("Timer handler parameter destroyed.");
	for(tmr=queue.next; tmr!=NULL; tmr=tmr->next){
		if(&(tmr->paramwatch)==watch){
			reset_timer(tmr);
		}
	}
}


bool timer_is_set(WTimer *timer)
{
	WTimer *tmr;
	for(tmr=queue.next; tmr!=NULL; tmr=tmr->next){
		if(tmr==timer)
			return TRUE;
	}
	return FALSE;
}


void set_timer_param(WTimer *timer, uint msecs, WObj *obj)
{
	WTimer *q;

	/* Check for an existing timer event in the queue */
	q=&queue;
	while(q->next!=NULL){
		if(q->next==timer){
			q->next=timer->next;
			break;
		}
		q=q->next;
	}

	/* Initialize the new queue timer event */
	add_to_current_time(&(timer->when), msecs);
	timer->next=NULL;

	/* Add timerevent in place to queue */
	q=&queue;
	for(;;){
		if(q->next==NULL){
			q->next=timer;
			break;
		}
		if(TIMEVAL_LATER(timer->when, q->next->when)){
			q=q->next;
			continue;
		}else{
			timer->next=q->next;
			q->next=timer;
			break;
		}
	}

	do_set_timer();
	
	if(obj!=NULL)
		setup_watch(&(timer->paramwatch), obj, kill_timer);
}


void set_timer(WTimer *timer, uint msecs)
{
	set_timer_param(timer, msecs, NULL);
}


void reset_timer(WTimer *timer)
{
	WTimer *q;
	WTimer *tmpq;

	reset_watch(&(timer->paramwatch));
	
	q=&queue;
	while(q->next!=NULL){
		tmpq=q->next;
		if(q->next==timer){
			q->next=timer->next;
			timer->next=NULL;
			/*free(tmpq);*/
			do_set_timer();
			return;
		}
		q=q->next;
	}
}


/* */


static void fatal_signal_handler(int signal_num)
{
	set_warn_handler(NULL);
	warn("Caught fatal signal %d. Dying without deinit.", signal_num); 
	signal(signal_num, SIG_DFL);
	kill(getpid(), signal_num);
}

		   
static void deadly_signal_handler(int signal_num)
{
	set_warn_handler(NULL);
	warn("Caught signal %d. Dying.", signal_num);
	signal(signal_num, SIG_DFL);
	if(wglobal.opmode==OPMODE_INIT)
		kill(getpid(), signal_num);
	else
		kill_sig=signal_num;
}


static void chld_handler(int signal_num)
{
#if 0
	pid_t pid;

	while((pid=waitpid(-1, NULL, WNOHANG|WUNTRACED))>0){
		/* nothing */
	}
#else
	wait_sig=1;
#endif
}


static void exit_handler(int signal_num)
{
	kill_sig=signal_num;
}


static void timer_handler(int signal_num)
{
	had_tmr=TRUE;
}


static void ignore_handler(int signal_num)
{
	
}


#ifndef SA_RESTART
 /* glibc is broken (?) and does not define SA_RESTART with
  * '-ansi -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED', so just try to live
  * without it.
  */
#define SA_RESTART 0
#endif


void trap_signals()
{
	struct sigaction sa;
	sigset_t set, oldset;

	sigemptyset(&set);
	sigemptyset(&oldset);
	sigprocmask(SIG_SETMASK, &set, &oldset);
	
#define DEADLY(X) signal(X, deadly_signal_handler);
#define FATAL(X) signal(X, fatal_signal_handler);
#define IGNORE(X) signal(X, SIG_IGN)
	
	DEADLY(SIGHUP);
	DEADLY(SIGQUIT);
	DEADLY(SIGINT);
	DEADLY(SIGABRT);
	
	FATAL(SIGILL);
	FATAL(SIGSEGV);
	FATAL(SIGFPE);
	FATAL(SIGBUS);
	
	IGNORE(SIGTRAP);
	/*IGNORE(SIGWINCH);*/

	sigemptyset(&(sa.sa_mask));
	sa.sa_handler=chld_handler;
	sa.sa_flags=SA_NOCLDSTOP|SA_RESTART;
	sigaction(SIGCHLD, &sa, NULL);

	sa.sa_handler=exit_handler;
	sa.sa_flags=SA_RESTART;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	
	sa.sa_handler=timer_handler;
	sigaction(SIGALRM, &sa, NULL);

	/* SIG_IGN is preserved over execve and since the the default action
	 * for SIGPIPE is not to ignore it, some programs may get upset if
	 * the behaviour is not the default.
	 */
	sa.sa_handler=ignore_handler;
	sigaction(SIGPIPE, &sa, NULL);

#undef IGNORE
#undef FATAL
#undef DEADLY
}

