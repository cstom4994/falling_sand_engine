// Copyright(c) 2022-2023, KaoruXun All rights reserved.

// Some code refer to tinycthread,
// link to https://github.com/tinycthread/tinycthread

#ifndef _METADOT_THREADPOOL_H_
#define _METADOT_THREADPOOL_H_

#include "core/macros.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma region PortableThread

#if defined(METADOT_PLATFORM_POSIX)
#undef _FEATURES_H
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#if !defined(_POSIX_C_SOURCE) || ((_POSIX_C_SOURCE - 0) < 199309L)
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#if !defined(_XOPEN_SOURCE) || ((_XOPEN_SOURCE - 0) < 500)
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#define _XPG6
#endif

#include <time.h>

#if defined(METADOT_PLATFORM_POSIX)
#include <pthread.h>
#elif defined(METADOT_PLATFORM_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define __UNDEF_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef __UNDEF_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef __UNDEF_LEAN_AND_MEAN
#endif
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define TTHREAD_NORETURN _Noreturn
#elif defined(__GNUC__)
#define TTHREAD_NORETURN __attribute__((__noreturn__))
#else
#define TTHREAD_NORETURN
#endif

#ifndef TIME_UTC
#define TIME_UTC 1
#define _TTHREAD_EMULATE_TIMESPEC_GET_

#if defined(METADOT_PLATFORM_WIN32)
struct _tthread_timespec {
    time_t tv_sec;
    long tv_nsec;
};
#define timespec _tthread_timespec
#endif

int _tthread_timespec_get(struct timespec *ts, int base);
#define timespec_get _tthread_timespec_get
#endif

#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L)) && !defined(_Thread_local)
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
#define _Thread_local __thread
#else
#define _Thread_local __declspec(thread)
#endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && (((__GNUC__ << 8) | __GNUC_MINOR__) < ((4 << 8) | 9))
#define _Thread_local __thread
#endif

#if defined(METADOT_PLATFORM_WIN32)
#define TSS_DTOR_ITERATIONS (4)
#else
#define TSS_DTOR_ITERATIONS PTHREAD_DESTRUCTOR_ITERATIONS
#endif

#define thrd_error 0
#define thrd_success 1
#define thrd_timedout 2
#define thrd_busy 3
#define thrd_nomem 4

#define mtx_plain 0
#define mtx_timed 1
#define mtx_recursive 2

#if defined(METADOT_PLATFORM_WIN32)
typedef struct {
    union {
        CRITICAL_SECTION cs;
        HANDLE mut;
    } mHandle;
    int mAlreadyLocked;
    int mRecursive;
    int mTimed;
} mtx_t;
#else
typedef pthread_mutex_t mtx_t;
#endif

int mtx_init(mtx_t *mtx, int type);
void mtx_destroy(mtx_t *mtx);
int mtx_lock(mtx_t *mtx);
int mtx_timedlock(mtx_t *mtx, const struct timespec *ts);
int mtx_trylock(mtx_t *mtx);
int mtx_unlock(mtx_t *mtx);

#if defined(METADOT_PLATFORM_WIN32)
typedef struct {
    HANDLE mEvents[2];
    unsigned int mWaitersCount;
    CRITICAL_SECTION mWaitersCountLock;
} cnd_t;
#else
typedef pthread_cond_t cnd_t;
#endif

int cnd_init(cnd_t *cond);
void cnd_destroy(cnd_t *cond);
int cnd_signal(cnd_t *cond);
int cnd_broadcast(cnd_t *cond);
int cnd_wait(cnd_t *cond, mtx_t *mtx);
int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts);

#if defined(METADOT_PLATFORM_WIN32)
typedef HANDLE thrd_t;
#else
typedef pthread_t thrd_t;
#endif

typedef int (*thrd_start_t)(void *arg);

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
thrd_t thrd_current(void);
int thrd_detach(thrd_t thr);
int thrd_equal(thrd_t thr0, thrd_t thr1);
TTHREAD_NORETURN void thrd_exit(int res);
int thrd_join(thrd_t thr, int *res);
int thrd_sleep(const struct timespec *duration, struct timespec *remaining);
void thrd_yield(void);

#if defined(METADOT_PLATFORM_WIN32)
typedef DWORD tss_t;
#else
typedef pthread_key_t tss_t;
#endif

typedef void (*tss_dtor_t)(void *val);

int tss_create(tss_t *key, tss_dtor_t dtor);
void tss_delete(tss_t key);
void *tss_get(tss_t key);
int tss_set(tss_t key, void *val);

#if defined(METADOT_PLATFORM_WIN32)
typedef struct {
    LONG volatile status;
    CRITICAL_SECTION lock;
} once_flag;
#define ONCE_FLAG_INIT \
    { 0, }
#else
#define once_flag pthread_once_t
#define ONCE_FLAG_INIT PTHREAD_ONCE_INIT
#endif

#if defined(METADOT_PLATFORM_WIN32)
void call_once(once_flag *flag, void (*func)(void));
#else
#define call_once(flag, func) pthread_once(flag, func)
#endif

#pragma endregion PortableThread

typedef struct metadot_thpool_ *ThreadPoolC;

ThreadPoolC metadot_thpool_init(int num_threads);
int metadot_thpool_addwork(ThreadPoolC, void (*function_p)(void *), void *arg_p);
void metadot_thpool_wait(ThreadPoolC);
void metadot_thpool_pause(ThreadPoolC);
void metadot_thpool_resume(ThreadPoolC);
void metadot_thpool_destroy(ThreadPoolC);
int metadot_thpool_workingcounts(ThreadPoolC);

#ifdef __cplusplus
}
#endif

#endif
