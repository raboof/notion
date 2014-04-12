/*
 * notion/ioncore/log.c
 *
 * Copyright (c) the Notion team 2013
 *
 * See the included file LICENSE for details.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include "log.h"

const char* loglevel_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
};

/** For this category, show log messages at this loglevel and above */
LogLevel minimumLevel(LogCategory category)
{
    switch(category){
        case FONT:
            /** For https://sourceforge.net/p/notion/bugs/63/ */
            return DEBUG;
        default:
            return INFO;
    }
}

#define TIME_BUFFER_MAXSIZE 100

void printtime(FILE *stream, const char *format, time_t time)
{
    char buf[TIME_BUFFER_MAXSIZE];
    strftime(buf, TIME_BUFFER_MAXSIZE, format, localtime(&time));
    fprintf(stream, "%s", buf);
}

void vlog_message(LogLevel level, LogCategory category, const char *file, int line, const char *function, const char *message, va_list argp)
{
    if(level >= minimumLevel(category)){
        printtime(stderr, "%F %T ", time(NULL));
        fprintf(stderr, "%-6s", loglevel_names[level]);
        if(file==NULL)
            fprintf(stderr, "Notion: ");
        else
            fprintf(stderr, "/notion/../%s:%d: %s: ", file, line, function);

        vfprintf(stderr, message, argp);
        fprintf(stderr, "\n");
    }
}

void log_message(LogLevel level, LogCategory category, const char *file, int line, const char* function, const char* message, ...)
{
    va_list argp;
    va_start(argp, message);
    vlog_message(level, category, file, line, function, message, argp);
    va_end(argp);
}

#if __STDC_VERSION__ < 199901L
extern void LOG(LogLevel level, LogCategory category, const char* message, ...)
{
    va_list argp;
    va_start(argp, message);
    vlog_message(level, category, NULL, -1, NULL, message, argp);
    va_end(argp);
}
#endif

