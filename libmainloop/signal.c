/*
 * ion/libmainloop/signal.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2005. 
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

#include <libtu/objp.h>
#include <libtu/types.h>
#include <libtu/misc.h>
#include <libtu/locale.h>
#include <libtu/output.h>

#include "signal.h"


static int kill_sig=0;
#if 1
static int wait_sig=0;
#endif
static bool had_tmr=FALSE;

/*{{{ Timers */


static WTimer *queue=NULL;


#define TIMEVAL_LATER(a, b) \
    ((a.tv_sec > b.tv_sec) || \
    ((a.tv_sec == b.tv_sec) && \
     (a.tv_usec > b.tv_usec)))


static void do_timer_set()
{
    struct itimerval val={{0, 0}, {0, 0}};
    
    if(queue==NULL){
        setitimer(ITIMER_REAL, &val, NULL);
        return;
    }

    /* Subtract queue time from current time, don't go below zero */
    gettimeofday(&(val.it_value), NULL);
    if(TIMEVAL_LATER((queue)->when, val.it_value)){
        if(queue->when.tv_usec<val.it_value.tv_usec){
            queue->when.tv_usec+=1000000;
            queue->when.tv_sec--;
        }
        val.it_value.tv_usec=queue->when.tv_usec-val.it_value.tv_usec;
        val.it_value.tv_sec=queue->when.tv_sec-val.it_value.tv_sec;
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


bool mainloop_check_signals()
{
    struct timeval current_time;
    WTimer *q;
    int ret=0;

#if 1    
    if(wait_sig!=0){
        pid_t pid;
        wait_sig=0;
        while((pid=waitpid(-1, NULL, WNOHANG|WUNTRACED))>0){
            /* nothing */
        }
    }
#endif
    
    if(kill_sig!=0)
        return kill_sig;

    /* Check for timer events in the queue */
    while(had_tmr && queue!=NULL){
        had_tmr=FALSE;
        gettimeofday(&current_time, NULL);
        while(queue!=NULL){
            if(TIMEVAL_LATER(current_time, queue->when)){
                q=queue;
                queue=q->next;
                q->next=NULL;
                if(q->handler!=NULL){
                    WTimerHandler *handler=q->handler;
                    q->handler=NULL;
                    handler(q);
                }else if(q->extl_handler!=extl_fn_none()){
                    ExtlFn fn=q->extl_handler;
                    q->extl_handler=extl_fn_none();
                    extl_call(fn, "o", NULL, q);
                    extl_unref_fn(fn);
                }
            }else{
                break;
            }
        }
        do_timer_set();
    }
    
    return ret;
}


static void add_to_current_time(struct timeval *when, uint msecs)
{
    long tmp_usec;

    gettimeofday(when, NULL);
    tmp_usec=when->tv_usec + (msecs * 1000);
    when->tv_usec=tmp_usec % 1000000;
    when->tv_sec+=tmp_usec / 1000000;
}


bool timer_is_set(WTimer *timer)
{
    WTimer *tmr;
    for(tmr=queue; tmr!=NULL; tmr=tmr->next){
        if(tmr==timer)
            return TRUE;
    }
    return FALSE;
}


void timer_do_set(WTimer *timer, uint msecs, WTimerHandler *handler,
                  ExtlFn fn)
{
    WTimer *q, **qptr;
    bool set=FALSE;
    
    timer_reset(timer);

    /* Initialize the new queue timer event */
    add_to_current_time(&(timer->when), msecs);
    timer->next=NULL;
    timer->handler=handler;
    timer->extl_handler=fn;

    /* Add timerevent in place to queue */
    q=queue;
    qptr=&queue;
    
    while(q!=NULL){
        if(TIMEVAL_LATER(q->when, timer->when))
            break;
        qptr=&(q->next);
        q=q->next;
    }
    
    timer->next=q;
    *qptr=timer;

    do_timer_set();
}


void timer_set(WTimer *timer, uint msecs, WTimerHandler *handler)
{
    timer_do_set(timer, msecs, handler, extl_fn_none());
}


void timer_set_extl(WTimer *timer, uint msecs, ExtlFn fn)
{
    timer_do_set(timer, msecs, NULL, extl_ref_fn(fn));
}

    
void timer_reset(WTimer *timer)
{
    WTimer *q=queue, **qptr=&queue;
    
    while(q!=NULL){
        if(q==timer){
            *qptr=timer->next;
            do_timer_set();
            break;
        }
        qptr=&(q->next);
        q=q->next;
        
    }
    
    timer->handler=NULL;
    extl_unref_fn(timer->extl_handler);
    timer->extl_handler=extl_fn_none();
}


bool timer_init(WTimer *timer)
{
    timer->when.tv_sec=0;
    timer->when.tv_usec=0;
    timer->next=NULL;
    timer->handler=NULL;
    timer->extl_handler=extl_fn_none();
    return TRUE;
}

void timer_deinit(WTimer *timer)
{
    timer_reset(timer);
}


WTimer *create_timer()
{
    CREATEOBJ_IMPL(WTimer, timer, (p));
}


WTimer *create_timer_extl_owned()
{
    WTimer *timer=create_timer();
    if(timer!=NULL)
        ((Obj*)timer)->flags|=OBJ_EXTL_OWNED;
    return timer;
}


IMPLCLASS(WTimer, Obj, timer_deinit, NULL);


/*}}}*/


/*{{{ Signal handling */


static void fatal_signal_handler(int signal_num)
{
    set_warn_handler(NULL);
    warn(TR("Caught fatal signal %d. Dying without deinit."), signal_num); 
    signal(signal_num, SIG_DFL);
    kill(getpid(), signal_num);
}

           
static void deadly_signal_handler(int signal_num)
{
    set_warn_handler(NULL);
    warn(TR("Caught signal %d. Dying."), signal_num);
    signal(signal_num, SIG_DFL);
    /*if(ioncore_g.opmode==IONCORE_OPMODE_INIT)
        kill(getpid(), signal_num);
    else*/
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
    if(kill_sig>0){
        warn(TR("Got signal %d while %d is still to be handled."),
             signal_num, kill_sig);
    }
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


void mainloop_trap_timer()
{
    struct sigaction sa;
    
    sigemptyset(&(sa.sa_mask));
    sa.sa_handler=timer_handler;
    sa.sa_flags=SA_RESTART;
    sigaction(SIGALRM, &sa, NULL);
}


void mainloop_trap_signals()
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


/*}}}*/

