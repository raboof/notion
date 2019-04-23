#include <libextl/extl.h>
#include "profiling.h"

#ifdef PROFILING_ENABLED
#include <stdio.h>
#include <time.h>
#include <malloc.h>

#include <libextl/extl.h>

static FILE *fp_trace;
struct timespec *current_time;

void
__cyg_profile_func_enter (void *func,  void *caller)
{
    if(fp_trace != NULL) {
        clock_gettime(CLOCK_REALTIME, current_time);
        fprintf(fp_trace, "e\t%p\t%p\t%ld.%09lu\n", func, caller, current_time->tv_sec, current_time->tv_nsec);
        fflush(fp_trace);
    }
}

void
__cyg_profile_func_exit (void *func, void *caller)
{
    if(fp_trace != NULL) {
        clock_gettime(CLOCK_REALTIME, current_time);
        fprintf(fp_trace, "x\t%p\t%p\t%ld.%09lu\n", func, caller, current_time->tv_sec, current_time->tv_nsec);
        fflush(fp_trace);
    }
}

void profileLuaCall(const enum ExtlHookEvent event, const char *name, const char *source, int currentline)
{
    if(fp_trace != NULL) {
        clock_gettime(CLOCK_REALTIME, current_time);
        if(event == EXTL_HOOK_ENTER)
            fprintf(fp_trace, "e\t%s %s:%d\t\t%ld.%09lu\n", name, source, currentline, current_time->tv_sec, current_time->tv_nsec);
        else if (event == EXTL_HOOK_EXIT)
            fprintf(fp_trace, "x\t%s %s:%d\t\t%ld.%09lu\n", name, source, currentline, current_time->tv_sec, current_time->tv_nsec);
        fflush(fp_trace);
    }
}

#endif

/*EXTL_DOC
 * Start profiling (if enabled at compile-time)
 */
EXTL_SAFE
EXTL_EXPORT
void ioncore_profiling_start(char* filename)
{
#ifdef PROFILING_ENABLED
    current_time = malloc(sizeof(struct timespec));
    fp_trace = fopen(filename, "w");
    extl_sethook(profileLuaCall);
#else
    (void)filename;
#endif
}

/*EXTL_DOC
 * Stop profiling (if enabled at compile-time)
 */
EXTL_SAFE
EXTL_EXPORT
void ioncore_profiling_stop()
{
#ifdef PROFILING_ENABLED
    FILE *fp_trace_to_close = fp_trace;
    fp_trace = NULL;
    free(current_time);
    if(fp_trace_to_close != NULL) {
        fflush(fp_trace_to_close);
        fclose(fp_trace_to_close);
    }

    extl_resethook();
#endif
}
