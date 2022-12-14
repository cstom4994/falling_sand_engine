// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_LOGGING_H_
#define _METADOT_LOGGING_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        va_list ap;
        const char *fmt;
        const char *file;
        struct tm *time;
        void *udata;
        int line;
        int level;
    } LogEvent;

    typedef void (*LogFn)(LogEvent *ev);
    typedef void (*LogLockFn)(bool lock, void *udata);

    enum {
        LOG_TRACE,
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR,
        LOG_FATAL
    };

#define log_trace(...) metadot_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) metadot_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...) metadot_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) metadot_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) metadot_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) metadot_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

    const char *metadot_log_levelstring(int level);
    void metadot_log_setlock(LogLockFn fn, void *udata);
    void metadot_log_setlevel(int level);
    void metadot_log_setquiet(bool enable);
    int metadot_log_addcallback(LogFn fn, void *udata, int level);
    int metadot_log_addfp(FILE *fp, int level);

    void metadot_log(int level, const char *file, int line, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
