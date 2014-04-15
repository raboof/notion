/*
 * ion/libmainloop/signal.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2009. 
 *
 * See the included file LICENSE for details.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <libtu/objp.h>
#include <libtu/types.h>
#include <libtu/misc.h>
#include <libtu/locale.h>
#include <libtu/output.h>

#include "signal.h"
#include "hooks.h"

static int kill_sig=0;
#if 1
static int wait_sig=0;
#endif

static int usr2_sig=0;
static bool had_tmr=FALSE;

WHook *mainloop_sigchld_hook=NULL;
WHook *mainloop_sigusr2_hook=NULL;

static sigset_t special_sigs;


/*{{{ Timers */


static WTimer *queue=NULL;


int mainloop_gettime(struct timeval *val)
{
#if defined(_POSIX_MONOTONIC_CLOCK) && (_POSIX_MONOTONIC_CLOCK>=0)
    struct timespec spec;
    int ret;
    static int checked=0;
    
    if(checked>=0){
        ret=clock_gettime(CLOCK_MONOTONIC, &spec);
    
        if(ret==-1 && errno==EINVAL && checked==0){
            checked=-1;
        }else{
            checked=1;
            
            val->tv_sec=spec.tv_sec;
            val->tv_usec=spec.tv_nsec/1000;
        
            return ret;
        }
    }
#else
    #warning "Monotonic clock unavailable; please fix your operating system."
#endif
    return gettimeofday(val, NULL);
}


#define TIMEVAL_LATER(a, b) \
    ((a.tv_sec > b.tv_sec) || \
    ((a.tv_sec == b.tv_sec) && \
     (a.tv_usec > b.tv_usec)))

#define USECS_IN_SEC 1000000


bool libmainloop_get_timeout(struct timeval *tv)
{
    if(queue==NULL)
        return FALSE;

    /* Subtract queue time from current time, don't go below zero */
    mainloop_gettime(tv);
    if(TIMEVAL_LATER((queue)->when, (*tv))){
        if(queue->when.tv_usec<tv->tv_usec){
            tv->tv_usec=(queue->when.tv_usec+USECS_IN_SEC)-tv->tv_usec;
            /* TIMEVAL_LATER ensures >= 0 */
            tv->tv_sec=(queue->when.tv_sec-1)-tv->tv_sec;
        }else{
            tv->tv_usec=queue->when.tv_usec-tv->tv_usec;
            tv->tv_sec=queue->when.tv_sec-tv->tv_sec;
        }
        /* POSIX and some kernels have been designed by absolute morons and 
         * contain idiotic artificial restrictions on the value of tv_usec, 
         * that will only cause more code being run and clock cycles being 
         * spent to do the same thing, as the kernel will in any case convert
         * the seconds to some other units.
         */
         tv->tv_sec+=tv->tv_usec/USECS_IN_SEC;
         tv->tv_usec%=USECS_IN_SEC;
    }else{
        had_tmr=TRUE;
        return FALSE;
    }
    
    return TRUE;
}


static void do_timer_set()
{
    struct itimerval val={{0, 0}, {0, 0}};
    
    if(libmainloop_get_timeout(&val.it_value)){
        val.it_interval.tv_usec=0;
        val.it_interval.tv_sec=0;
        
        if((setitimer(ITIMER_REAL, &val, NULL)))
            had_tmr=TRUE;
    }else if(!had_tmr){
        setitimer(ITIMER_REAL, &val, NULL);
    }
}


typedef struct{
    pid_t pid;
    int code;
} ChldParams;


static bool mrsh_chld(void (*fn)(pid_t, int), ChldParams *p)
{
    fn(p->pid, p->code);
    return TRUE;
}


static bool mrsh_chld_extl(ExtlFn fn, ChldParams *p)
{
    ExtlTab t=extl_create_table();
    bool ret;

    extl_table_sets_i(t, "pid", (int)p->pid);
    
    if(WIFEXITED(p->code)){
        extl_table_sets_b(t, "exited", TRUE);
        extl_table_sets_i(t, "exitstatus", WEXITSTATUS(p->code));
    }
    if(WIFSIGNALED(p->code)){
        extl_table_sets_b(t, "signaled", TRUE);
        extl_table_sets_i(t, "termsig", WTERMSIG(p->code));
#ifdef WCOREDUMP 
        extl_table_sets_i(t, "coredump", WCOREDUMP(p->code));
#endif
    }
    if(WIFSTOPPED(p->code)){
        extl_table_sets_b(t, "stopped", TRUE);
        extl_table_sets_i(t, "stopsig", WSTOPSIG(p->code));
    }
    /*if(WIFCONTINUED(p->code)){
        extl_table_sets_b(t, "continued", TRUE);
    }*/
    
    ret=extl_call(fn, "t", NULL, t);
    
    extl_unref_table(t);
    
    return ret;
}

static bool mrsh_usr2(void (*fn)(void), void *p)
{
    fn();
    return TRUE;
}

static bool mrsh_usr2_extl(ExtlFn fn, void *p)
{
    bool ret;
    ExtlTab t=extl_create_table();
    ret=extl_call(fn, "t", NULL, t);
    extl_unref_table(t);
    return ret;
}


bool mainloop_check_signals()
{
    struct timeval current_time;
    WTimer *q;
    int ret=0;

    if(usr2_sig!=0){
        usr2_sig=0;
        if(mainloop_sigusr2_hook!=NULL){
            hook_call(mainloop_sigusr2_hook, NULL,
                      (WHookMarshall*)mrsh_usr2,
                      (WHookMarshallExtl*)mrsh_usr2_extl);
        }
    }

#if 1    
    if(wait_sig!=0){
        ChldParams p;
        wait_sig=0;
        while((p.pid=waitpid(-1, &p.code, WNOHANG|WUNTRACED))>0){
            if(mainloop_sigchld_hook!=NULL &&
               (WIFEXITED(p.code) || WIFSIGNALED(p.code))){
                hook_call(mainloop_sigchld_hook, &p, 
                          (WHookMarshall*)mrsh_chld,
                          (WHookMarshallExtl*)mrsh_chld_extl);
            }
        }
    }
#endif
    
    if(kill_sig!=0)
        return kill_sig;

    /* Check for timer events in the queue */
    while(had_tmr){
        had_tmr=FALSE;
        if(queue==NULL)
            break;
        mainloop_gettime(&current_time);
        while(queue!=NULL){
            if(TIMEVAL_LATER(current_time, queue->when)){
                q=queue;
                queue=q->next;
                q->next=NULL;
                if(q->handler!=NULL){
                    WTimerHandler *handler=q->handler;
                    Obj *obj=q->objwatch.obj;
                    q->handler=NULL;
                    watch_reset(&(q->objwatch));
                    handler(q, obj);
                }else if(q->extl_handler!=extl_fn_none()){
                    ExtlFn fn=q->extl_handler;
                    Obj *obj=q->objwatch.obj;
                    watch_reset(&(q->objwatch));
                    q->extl_handler=extl_fn_none();
                    extl_call(fn, "o", NULL, obj);
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


void mainloop_block_signals(sigset_t *oldmask)
{
    sigprocmask(SIG_BLOCK, &special_sigs, oldmask);
}


bool mainloop_unhandled_signals()
{
    return (usr2_sig || wait_sig || kill_sig || had_tmr);
}


static void add_to_current_time(struct timeval *when, uint msecs)
{
    long tmp_usec;

    mainloop_gettime(when);
    tmp_usec=when->tv_usec + (msecs * 1000);
    when->tv_usec=tmp_usec % 1000000;
    when->tv_sec+=tmp_usec / 1000000;
}


/*EXTL_DOC
 * Is timer set?
 */
EXTL_EXPORT_MEMBER
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
                  Obj *obj, ExtlFn fn)
{
    WTimer *q, **qptr;
    
    timer_reset(timer);

    /* Initialize the new queue timer event */
    add_to_current_time(&(timer->when), msecs);
    timer->next=NULL;
    timer->handler=handler;
    timer->extl_handler=fn;
    if(obj!=NULL)
        watch_setup(&(timer->objwatch), obj, NULL);
    else
        watch_reset(&(timer->objwatch));

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


void timer_set(WTimer *timer, uint msecs, WTimerHandler *handler,
               Obj *obj)
{
    timer_do_set(timer, msecs, handler, obj, extl_fn_none());
}


/*EXTL_DOC
 * Set \var{timer} to call \var{fn} in \var{msecs} milliseconds.
 */
EXTL_EXPORT_AS(WTimer, set)
void timer_set_extl(WTimer *timer, uint msecs, ExtlFn fn)
{
    timer_do_set(timer, msecs, NULL, NULL, extl_ref_fn(fn));
}

    
/*EXTL_DOC
 * Reset timer.
 */
EXTL_EXPORT_MEMBER
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
    watch_reset(&(timer->objwatch));
}


bool timer_init(WTimer *timer)
{
    timer->when.tv_sec=0;
    timer->when.tv_usec=0;
    timer->next=NULL;
    timer->handler=NULL;
    timer->extl_handler=extl_fn_none();
    watch_init(&(timer->objwatch));
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

/*EXTL_DOC
 * Create a new timer.
 */
EXTL_EXPORT_AS(mainloop, create_timer)
WTimer *create_timer_extl_owned()
{
    WTimer *timer=create_timer();
    if(timer!=NULL)
        ((Obj*)timer)->flags|=OBJ_EXTL_OWNED;
    return timer;
}


EXTL_EXPORT
IMPLCLASS(WTimer, Obj, timer_deinit, NULL);


/*}}}*/


/*{{{ Signal handling */

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

static void usr2_handler(int signal_num)
{
    usr2_sig=1;
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

#define IFTRAP(X) if(sigismember(which, X))
#define DEADLY(X) IFTRAP(X) signal(X, deadly_signal_handler);
#define IGNORE(X) IFTRAP(X) signal(X, SIG_IGN)


void mainloop_trap_signals(const sigset_t *which)
{
    struct sigaction sa;
    sigset_t set, oldset;
    sigset_t dummy;

    if(which==NULL){
        sigfillset(&dummy);
        which=&dummy;
    }
    
    sigemptyset(&special_sigs);
    sigemptyset(&set);
    sigemptyset(&oldset);
    sigprocmask(SIG_SETMASK, &set, &oldset);

    /* I do not handle SIG{ILL,SEGV,FPE,BUS} since there's not much I can do in
     * response */

    DEADLY(SIGHUP);
    DEADLY(SIGQUIT);
    DEADLY(SIGINT);
    DEADLY(SIGABRT);
    
    IGNORE(SIGTRAP);
    /*IGNORE(SIGWINCH);*/

    sigemptyset(&(sa.sa_mask));

    IFTRAP(SIGALRM){
        sa.sa_handler=timer_handler;
        sa.sa_flags=SA_RESTART;
        sigaction(SIGALRM, &sa, NULL);
        sigaddset(&special_sigs, SIGALRM);
    }

    IFTRAP(SIGCHLD){
        sa.sa_handler=chld_handler;
        sa.sa_flags=SA_NOCLDSTOP|SA_RESTART;
        sigaction(SIGCHLD, &sa, NULL);
        sigaddset(&special_sigs, SIGCHLD);
    }

    IFTRAP(SIGUSR2){
        sa.sa_handler=usr2_handler;
        sa.sa_flags=SA_RESTART;
        sigaction(SIGUSR2, &sa, NULL);
        sigaddset(&special_sigs, SIGUSR2);
    }

    IFTRAP(SIGTERM){
        sa.sa_handler=exit_handler;
        sa.sa_flags=SA_RESTART;
        sigaction(SIGTERM, &sa, NULL);
        sigaddset(&special_sigs, SIGTERM);
    }
    
    IFTRAP(SIGUSR1){
        sa.sa_handler=exit_handler;
        sa.sa_flags=SA_RESTART;
        sigaction(SIGUSR1, &sa, NULL);
    }
    
    /* SIG_IGN is preserved over execve and since the the default action
     * for SIGPIPE is not to ignore it, some programs may get upset if
     * the behaviour is not the default.
     */
    IFTRAP(SIGPIPE){
        sa.sa_handler=ignore_handler;
        sigaction(SIGPIPE, &sa, NULL);
    }

}

#undef IGNORE
#undef DEADLY


/*}}}*/

