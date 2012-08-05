#include <libextl/extl.h>
#include "profiling.h"

#ifdef PROFILING_ENABLED
#include <google/profiler.h>
#endif

/*EXTL_DOC
 * Start profiling (if enabled at compile-time)
 */
EXTL_SAFE
EXTL_EXPORT
void ioncore_profiling_start(char* filename) {
#ifdef PROFILING_ENABLED
    ProfilerStart(filename);
#endif
}

/*EXTL_DOC
 * Stop profiling (if enabled at compile-time)
 */
EXTL_SAFE
EXTL_EXPORT
void ioncore_profiling_stop() {
#ifdef PROFILING_ENABLED
    ProfilerStop();
#endif
}

