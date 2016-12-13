/*
 * notion/ioncore/log.h
 *
 * Copyright (c) the Notion team 2013.
 *
 * See the included file LICENSE for details.
 */

#ifndef NOTION_IONCORE_LOG_H

typedef enum{
    DEBUG = 0, /** Not usually shown, but can be useful for debugging */
    INFO,  /** Usually shown, does not necessarily indicate an error */
    WARN,  /** Usually shown, indicates a likely but non-fatal misconfiguration/error */
    ERROR  /** Shown, indicates an error */
} LogLevel;


typedef enum{
    GENERAL,
    FONT,
    RANDR,
    FOCUS,
    VALGRIND /* Useful while debugging valgrind warnings */
} LogCategory;


/** When logging from C code, don't use this function directly, use the LOG macro instead */
extern void log_message(LogLevel level, LogCategory category, const char *file, const int line, const char* function, const char* message, ...);

#if __STDC_VERSION__ >= 199901L
#define LOG(level, category, ...) log_message(level, category, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
extern void LOG(LogLevel level, LogCategory category, const char* message, ...);
#endif

#endif /*NOTION_IONCORE_LOG_H*/
